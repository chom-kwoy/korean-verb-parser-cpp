#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <simdjson.h>

#include "pcfg.hpp"

#include "nonterminal.hpp"

namespace parser
{

Pcfg::Pcfg(Nonterminal start, std::vector<LetterProd> productions)
    : m_start {std::move(start)}
    , m_productions {std::move(productions)}
{
}

auto Pcfg::start() const -> Nonterminal
{
    return m_start;
}

auto Pcfg::productions() const -> std::vector<LetterProd> const&
{
    return m_productions;
}

auto pcfg_from_json(std::string_view json) -> Pcfg
{
    namespace oj = simdjson::ondemand;

    oj::parser parser;
    auto padded = simdjson::padded_string(json);
    oj::document doc = parser.iterate(padded);

    // Read start_symbol before iterating rules so on-demand parsing stays
    // forward-only (the document lists start_symbol first).
    const std::string start_symbol {std::string_view {doc["start_symbol"].get_string()}};

    std::vector<LetterProd> productions {};
    for (auto rule : doc["rules"]) {
        const std::string lhs {std::string_view {rule["lhs"].get_string()}};
        auto rhs = std::vector<std::variant<Nonterminal, LetterType>> {};
        for (auto item : rule["rhs"]) {
            if (item.type() == oj::json_type::object) {
                rhs.emplace_back(Nonterminal(std::string {std::string_view {item["name"].get_string()}}));
            } else {
                const std::string_view term = item.get_string();
                rhs.emplace_back(static_cast<LetterType>(term[0]));
            }
        }
        const auto prob = static_cast<float>(double {rule["prob"].get_double()});
        productions.push_back({Nonterminal(lhs), std::move(rhs), prob});
    }

    return Pcfg(Nonterminal(start_symbol), std::move(productions));
}

auto pcfg_from_json(std::istream& istream) -> Pcfg
{
    std::ostringstream buffer;
    buffer << istream.rdbuf();
    return pcfg_from_json(buffer.str());
}

}  // namespace parser
