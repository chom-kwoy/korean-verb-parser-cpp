#include <map>
#include <set>
#include <utility>
#include <variant>
#include <vector>

#include "pcfg.hpp"

#include "nonterminal.hpp"

namespace parser
{

namespace
{

auto categories_set(std::vector<LetterProd> const& productions) -> std::set<Nonterminal>
{
    std::set<Nonterminal> result;

    for (auto const& prod : productions) {
        result.insert(prod.lhs);
    }

    return result;
}

auto calculate_indexes(std::vector<LetterProd> const& productions) -> Indexes
{
    Indexes result {};

    for (auto const& prod : productions) {
        if (result.lhs_index.count(prod.lhs) == 0) {
            result.lhs_index[prod.lhs] = {};
        }
        result.lhs_index[prod.lhs].push_back(prod);

        if (prod.rhs.empty()) {
            result.empty_index[prod.lhs] = prod;
        } else {
            auto rhs0 = prod.rhs[0];
            if (result.rhs_index.count(rhs0) == 0) {
                result.rhs_index[rhs0] = {};
            }
            result.rhs_index[rhs0].push_back(prod);
        }

        for (auto const& token : prod.rhs) {
            if (std::holds_alternative<LetterType>(token)) {
                result.lexical_index[std::get<LetterType>(token)].insert(prod);
            }
        }
    }

    return result;
}

auto transitive_closure(std::map<Nonterminal, std::set<Nonterminal>> agenda_graph)
    -> std::map<Nonterminal, std::set<Nonterminal>>
{
    std::map<Nonterminal, std::set<Nonterminal>> closure_graph {};

    for (auto const& [key, agenda] : agenda_graph) {
        closure_graph[key] = {{key}};  // reflexive
    }

    for (auto& [i, agenda] : agenda_graph) {
        auto& closure = closure_graph[i];

        while (!agenda.empty()) {
            auto j = *agenda.begin();
            agenda.erase(agenda.begin());

            closure.insert(j);
            closure.insert(closure_graph[j].begin(), closure_graph[j].end());
            agenda.insert(agenda_graph[j].begin(), agenda_graph[j].end());
            for (auto const& item : closure) {
                agenda.erase(item);
            }
        }
    }

    return closure_graph;
}

auto invert_graph(std::map<Nonterminal, std::set<Nonterminal>> const& graph)
    -> std::map<Nonterminal, std::set<Nonterminal>>
{
    std::map<Nonterminal, std::set<Nonterminal>> inverted_graph {};

    for (auto const& [key, values] : graph) {
        for (auto const& value : values) {
            inverted_graph[value].insert(key);
        }
    }

    return inverted_graph;
}

auto calculate_leftcorners(std::set<Nonterminal> const& categories,
                           std::vector<LetterProd> const& productions) -> LeftcornerRelations
{
    // Calculate leftcorner relations, for use in optimized parsing.
    LeftcornerRelations result {};

    for (auto const& cat : categories) {
        result.immediate_leftcorner_categories[cat] = {{cat}};
        result.immediate_leftcorner_words[cat] = {};
    }

    for (auto const& prod : productions) {
        if (!prod.rhs.empty()) {
            auto left = prod.rhs[0];
            if (std::holds_alternative<LetterType>(left)) {
                result.immediate_leftcorner_words[prod.lhs].insert(std::get<LetterType>(left));
            } else {
                result.immediate_leftcorner_categories[prod.lhs].insert(std::get<Nonterminal>(left));
            }
        }
    }

    result.leftcorners = transitive_closure(result.immediate_leftcorner_categories);
    result.leftcorner_parents = invert_graph(result.leftcorners);

    for (auto const& cat : categories) {
        auto& leftcorner = result.leftcorner_words[cat] = {};
        for (auto const& left : result.leftcorners[cat]) {
            leftcorner.insert(result.immediate_leftcorner_words[left].begin(),
                              result.immediate_leftcorner_words[left].end());
        }
    }

    return result;
}

}  // namespace

Pcfg::Pcfg(Nonterminal start, std::vector<LetterProd> productions)
    : m_start {std::move(start)}
    , m_productions {std::move(productions)}
    , m_categories {categories_set(m_productions)}
    , m_indexes {calculate_indexes(m_productions)}
    , m_leftcorner_relations {calculate_leftcorners(m_categories, m_productions)}
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

}  // namespace parser
