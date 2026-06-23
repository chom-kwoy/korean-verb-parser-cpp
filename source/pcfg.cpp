#include <utility>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

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

auto pcfg_from_json(nlohmann::json const& input) -> Pcfg
{
    std::vector<LetterProd> productions{};
    for (auto&& rule : input["rules"]) {
        auto lhs = rule["lhs"].get<std::string>();
        auto prob = rule["prob"].get<float>();
        auto rhs = std::vector<std::variant<Nonterminal, parser::LetterType>>{};
        for (auto&& item : rule["rhs"]) {
            if (item.is_object()) {
                rhs.emplace_back(Nonterminal(item["name"].get<std::string>()));
            } else {
                rhs.emplace_back(item.get<std::string>()[0]);
            }
        }
        productions.push_back({Nonterminal(lhs), rhs, prob});
    }

    const auto start_symbol = input["start_symbol"].get<std::string>();
    return Pcfg(Nonterminal(start_symbol), productions);
}

auto pcfg_from_json(std::istream& istream) -> Pcfg
{
    return pcfg_from_json(nlohmann::json::parse(istream));
}

}  // namespace parser
