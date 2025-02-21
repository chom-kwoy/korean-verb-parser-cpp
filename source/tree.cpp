#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <variant>

#include "tree.h"

#include <nlohmann/json.hpp>

#include "nonterminal.hpp"
#include "pcfg.hpp"

namespace parser
{

Tree::Tree(Nonterminal symbol_, std::vector<TreeNode> children_, float log_prob_)
    : symbol {std::move(symbol_)}
    , children {std::move(children_)}
    , log_prob {log_prob_}
{
}

auto Tree::operator<(Tree const& rhs) const -> bool
{
    if (symbol < rhs.symbol) {
        return true;
    }
    if (rhs.symbol < symbol) {
        return false;
    }
    return std::lexicographical_compare(children.begin(), children.end(), rhs.children.begin(), rhs.children.end());
}

auto Tree::operator==(Tree const& rhs) const -> bool
{
    if (symbol.name() != rhs.symbol.name()) {
        return false;
    }
    if (children.size() != rhs.children.size()) {
        return false;
    }
    return std::equal(children.begin(), children.end(), rhs.children.begin(), rhs.children.end());
}

auto Tree::str(int indent_level) const -> std::string
{
    std::stringstream out;
    for (int i = 0; i < indent_level; ++i) {
        out << "  ";
    }
    out << symbol.name() << "(\n";

    bool first = true;
    for (auto&& child : children) {
        if (!first) {
            out << ",\n";
        }
        first = false;
        if (std::holds_alternative<Tree>(child)) {
            out << std::get<Tree>(child).str(indent_level + 1);
        } else if (std::holds_alternative<LetterType>(child)) {
            for (int i = 0; i < indent_level + 1; ++i) {
                out << "  ";
            }
            out << "\'" << std::get<LetterType>(child) << "\'";
        }
    }
    out << "\n";
    for (int i = 0; i < indent_level; ++i) {
        out << "  ";
    }
    out << ") [" << std::setprecision(3) << log_prob << "]";

    return out.str();
}

namespace
{
// helper type for the visitor #4
template<class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;
}  // namespace

auto Tree::json() const -> nlohmann::json
{
    auto children_json = std::vector<nlohmann::json> {};
    children_json.reserve(children.size());

    for (auto&& child : children) {
        std::visit(
            overloaded {
                [&](Tree const& tree) { children_json.push_back(tree.json()); },
                [&](LetterType letter) { children_json.push_back(std::string {letter}); },
            },
            child);
    }

    return nlohmann::json {
        {"label", symbol.name()},
        {"children", children_json},
        {"log_prob", log_prob},
    };
}

}  // namespace parser

template<typename T, typename... Rest>
void hash_combine(std::size_t& seed, const T& v, const Rest&... rest)
{
    seed ^= std::hash<T> {}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    (hash_combine(seed, rest), ...);
}

auto std::hash<parser::Tree>::operator()(const parser::Tree& tree) const -> std::size_t
{
    std::size_t value = 0;
    hash_combine(value, tree.symbol.name());
    for (auto&& child : tree.children) {
        hash_combine(value, child);
    }

    return value;
}
