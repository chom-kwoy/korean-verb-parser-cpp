#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "pcfg.hpp"
#include "tree.h"

namespace parser
{

// Productions grouped by their first right-hand-side symbol, so that a span only
// has to consider productions that can actually start there, instead of scanning
// the whole grammar. The pointers reference productions owned by the grammar, so
// this must not outlive (nor be copied away from) that grammar.
struct ProdIndex
{
    // NT-first productions that are NOT binary NT-NT (handled separately below):
    // unary NT productions and any longer NT-first productions.
    std::unordered_map<int, std::vector<LetterProd const*>> by_first_nt;
    std::unordered_map<LetterType, std::vector<LetterProd const*>> by_first_term;
    // Unary productions A -> B (single nonterminal rhs), indexed by B. These are
    // the only productions that can fire repeatedly within a single span's
    // fixpoint, since every other production reads strictly smaller, already
    // complete sub-spans.
    std::unordered_map<int, std::vector<LetterProd const*>> unary_by_first_nt;
    // Binary productions A -> B C (both nonterminals), indexed by (B, C). A
    // grammar typically has very few distinct first symbols but thousands of
    // productions per first symbol, so indexing by the pair lets a span fire only
    // the productions whose *both* children are actually present (CYK-style),
    // instead of every production that merely shares a first symbol.
    std::unordered_map<int, std::unordered_map<int, std::vector<LetterProd const*>>> binary_nt;
};

class ViterbiParser
{
    Pcfg m_grammar;
    // Built once from m_grammar; reused across parse() calls. Holds pointers into
    // m_grammar.productions(), so the parser is move-only (a move keeps the
    // grammar's production buffer alive; a copy would dangle these pointers).
    ProdIndex m_index;

  public:
    explicit ViterbiParser(Pcfg const& grammar);
    explicit ViterbiParser(Pcfg&& grammar);

    ViterbiParser(ViterbiParser const&) = delete;
    auto operator=(ViterbiParser const&) -> ViterbiParser& = delete;
    ViterbiParser(ViterbiParser&&) = default;
    auto operator=(ViterbiParser&&) -> ViterbiParser& = default;

    auto parse(std::vector<LetterType> const& tokens, int top_k = 1) const -> std::unordered_set<Tree>;
};

}  // namespace parser
