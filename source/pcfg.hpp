#pragma once

#include <istream>
#include <map>
#include <set>
#include <vector>

#include <nlohmann/json.hpp>

#include "nonterminal.hpp"
#include "production.hpp"

namespace parser
{

using LetterType = char;
using LetterProd = Production<LetterType>;

struct Indexes
{
    std::map<Nonterminal, std::vector<LetterProd>> lhs_index;
    std::map<LetterProd::RhsType, std::vector<LetterProd>> rhs_index;
    std::map<Nonterminal, LetterProd> empty_index;
    std::map<LetterType, std::set<LetterProd>> lexical_index;
};

struct LeftcornerRelations
{
    std::map<Nonterminal, std::set<Nonterminal>> immediate_leftcorner_categories;
    std::map<Nonterminal, std::set<LetterType>> immediate_leftcorner_words;
    std::map<Nonterminal, std::set<Nonterminal>> leftcorners;  // transitive closure
    std::map<Nonterminal, std::set<Nonterminal>> leftcorner_parents;
    std::map<Nonterminal, std::set<LetterType>> leftcorner_words;
};

class Pcfg
{
    Nonterminal m_start;
    std::vector<LetterProd> m_productions;

    // Indexes
    std::set<Nonterminal> m_categories;
    Indexes m_indexes;
    LeftcornerRelations m_leftcorner_relations;

  public:
    Pcfg(Nonterminal start, std::vector<LetterProd> productions);

    auto start() const -> Nonterminal;
    auto productions() const -> std::vector<LetterProd> const&;
};

// Build a Pcfg from a JSON document of the form
//   {"start_symbol": "...", "rules": [{"lhs": "...", "rhs": [...], "prob": ...}, ...]}
// where each rhs entry is either {"name": "<Nonterminal>"} or a single-character
// string terminal. The istream overload parses the stream first.
auto pcfg_from_json(nlohmann::json const& input) -> Pcfg;
auto pcfg_from_json(std::istream& istream) -> Pcfg;

}  // namespace parser
