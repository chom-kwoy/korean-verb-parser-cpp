#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <limits>
#include <memory>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "viterbiparser.h"

#include "nonterminal.hpp"
#include "pcfg.hpp"
#include "tree.h"

namespace parser
{

ViterbiParser::ViterbiParser(Pcfg const& grammar)
    : m_grammar(grammar)
{
}

ViterbiParser::ViterbiParser(Pcfg&& grammar)
    : m_grammar(std::move(grammar))
{
}

namespace
{

struct Range
{
    int begin, end;
};
using RhsType = LetterProd::RhsType;  // variant<Nonterminal, LetterType>
using NodeSet = std::unordered_set<TreeNode, TreeNodeHash, TreeNodeEq>;

// The chart, addressed directly by span: chart[begin][end] maps each
// nonterminal id to its top-k constituent trees over [begin, end). Terminals are
// not stored; match_rhs matches them against the token sequence directly.
using NtCell = std::unordered_map<int, NodeSet>;
using Chart = std::vector<std::vector<NtCell>>;

// Productions grouped by their first right-hand-side symbol, so that a span only
// has to consider productions that can actually start there, instead of scanning
// the whole grammar. The pointers reference productions owned by the grammar.
struct ProdIndex
{
    std::unordered_map<int, std::vector<LetterProd const*>> by_first_nt;
    std::unordered_map<LetterType, std::vector<LetterProd const*>> by_first_term;
    // Unary productions A -> B (single nonterminal rhs), indexed by B. These are
    // the only productions that can fire repeatedly within a single span's
    // fixpoint, since every other production reads strictly smaller, already
    // complete sub-spans.
    std::unordered_map<int, std::vector<LetterProd const*>> unary_by_first_nt;
};

auto build_prod_index(Pcfg const& grammar) -> ProdIndex
{
    ProdIndex index {};
    for (auto const& prod : grammar.productions()) {
        if (prod.rhs.empty()) {
            // An empty right-hand side can never cover a non-empty span, and
            // spans considered here are always non-empty, so skip it.
            continue;
        }
        auto const& first = prod.rhs[0];
        if (std::holds_alternative<Nonterminal>(first)) {
            const int cat = std::get<Nonterminal>(first).id();
            index.by_first_nt[cat].push_back(&prod);
            if (prod.rhs.size() == 1) {
                index.unary_by_first_nt[cat].push_back(&prod);
            }
        } else {
            index.by_first_term[std::get<LetterType>(first)].push_back(&prod);
        }
    }
    return index;
}

/**
 * @return a set of all the lists of children that cover `range`
 *  and that match `rhs`.
 */
auto match_rhs(std::span<const RhsType> rhs, Range range, Chart const& chart, std::vector<LetterType> const& tokens)
    -> std::vector<std::vector<TreeNode>>
{
    // Base case
    if (range.begin >= range.end and rhs.empty()) {
        return {std::vector<TreeNode> {}};
    }
    if (range.begin >= range.end or rhs.empty()) {
        return {};
    }

    auto childlists = std::vector<std::vector<TreeNode>> {};

    auto prepend = [&](TreeNode const& left, std::vector<std::vector<TreeNode>> const& rights)
    {
        for (auto&& right : rights) {
            auto new_child = std::vector {left};
            new_child.reserve(1 + right.size());
            std::copy(right.begin(), right.end(), std::back_inserter(new_child));
            childlists.push_back(std::move(new_child));
        }
    };

    auto const& first = rhs[0];
    if (std::holds_alternative<LetterType>(first)) {
        // A terminal matches exactly one token, at range.begin.
        const int split = range.begin + 1;
        if (split <= range.end && tokens[static_cast<std::size_t>(range.begin)] == std::get<LetterType>(first)) {
            auto rights = match_rhs(rhs.subspan(1), {split, range.end}, chart, tokens);
            prepend(TreeNode {std::get<LetterType>(first)}, rights);
        }
    } else {
        const int symbol = std::get<Nonterminal>(first).id();
        for (int split = range.begin; split <= range.end; ++split) {
            auto const& cell = chart[static_cast<std::size_t>(range.begin)][static_cast<std::size_t>(split)];
            auto it = cell.find(symbol);
            if (it == cell.end()) {
                continue;
            }
            auto rights = match_rhs(rhs.subspan(1), {split, range.end}, chart, tokens);
            for (auto&& left : it->second) {
                prepend(left, rights);
            }
        }
    }

    return childlists;
}

/**
 * @return a list of the production instantiations that cover a
 * given span of the text.  A "production instantiation" is
 * a tuple containing a production and a list of children,
 * where the production's right hand side matches the list of
 * children; and the children cover `range`.
 *
 * Only productions whose first right-hand-side symbol can actually start at
 * `range.begin` are considered: nonterminals listed in `cats_starting_here`
 * (categories that already have a constituent beginning at `range.begin`), plus
 * the single terminal at that position. This avoids scanning the whole grammar.
 */
auto find_instantiations(Range range,
                         Chart const& chart,
                         ProdIndex const& index,
                         std::unordered_set<int> const& cats_starting_here,
                         std::vector<LetterType> const& tokens,
                         bool unary_only) -> std::vector<std::pair<LetterProd const*, std::vector<TreeNode>>>
{
    auto result = std::vector<std::pair<LetterProd const*, std::vector<TreeNode>>> {};

    auto consider = [&](LetterProd const* production)
    {
        auto childlists =
            match_rhs(std::span {production->rhs.begin(), production->rhs.end()}, range, chart, tokens);
        for (auto&& childlist : childlists) {
            result.emplace_back(production, std::move(childlist));
        }
    };

    // After the first pass only unary productions can produce anything new, so
    // restrict to those and skip the (unchanging) terminal-first productions.
    auto const& nt_index = unary_only ? index.unary_by_first_nt : index.by_first_nt;

    // Productions whose first symbol is a nonterminal present at range.begin.
    for (int cat : cats_starting_here) {
        if (auto it = nt_index.find(cat); it != nt_index.end()) {
            for (auto const* production : it->second) {
                consider(production);
            }
        }
    }

    if (unary_only) {
        return result;
    }

    // Productions whose first symbol is the terminal at range.begin.
    if (range.begin < range.end) {
        const LetterType token = tokens[static_cast<std::size_t>(range.begin)];
        if (auto it = index.by_first_term.find(token); it != index.by_first_term.end()) {
            for (auto const* production : it->second) {
                consider(production);
            }
        }
    }

    return result;
}

/**
 * Find any constituents that might cover `range`, and add them
 * to the most likely constituents table.
 */
auto add_constituents_spanning(Range range,
                               Chart& chart,
                               std::vector<std::unordered_set<int>>& starts_at,
                               int top_k,
                               ProdIndex const& index,
                               std::vector<LetterType> const& tokens)
{
    NtCell& chart_cell = chart[static_cast<std::size_t>(range.begin)][static_cast<std::size_t>(range.end)];

    // Since some of the grammar productions may be unary, we need to
    // repeatedly try productions until none of them add any new constituents.
    // The first pass tries every applicable production; later passes only need
    // unary productions, the only ones that can chain within this span.
    bool changed = true;
    bool first_pass = true;
    while (changed) {
        changed = false;

        // Find all ways instantiations of the grammar productions that
        // cover the span.
        auto instantiations = find_instantiations(
            range, chart, index, starts_at[static_cast<std::size_t>(range.begin)], tokens, /*unary_only=*/!first_pass);
        first_pass = false;

        // For each production instantiation, add a new
        // Tree whose probability is the product of the
        // children's probabilities and the production's
        // probability.
        for (auto&& [production, children] : instantiations) {
            float log_p = std::log(production->prob);
            for (auto&& child : children) {
                if (std::holds_alternative<std::shared_ptr<const Tree>>(child)) {
                    log_p += std::get<std::shared_ptr<const Tree>>(child)->log_prob;
                }
            }

            auto node = production->lhs;
            auto tree = std::make_shared<const Tree>(node, std::move(children), log_p);
            const TreeNode tree_node = tree;

            // If it's a new constituent category for this span, add it.
            auto cell_it = chart_cell.find(node.id());
            if (cell_it == chart_cell.end()) {
                chart_cell.emplace(node.id(), NodeSet {tree_node});
                starts_at[static_cast<std::size_t>(range.begin)].insert(node.id());
                changed = true;
            } else {
                auto& constituent = cell_it->second;
                const bool contains = constituent.contains(tree_node);
                if (not contains) {
                    const int c_size = static_cast<int>(constituent.size());
                    float min_log_prob = std::numeric_limits<float>::max();
                    for (auto&& c_tree : constituent) {
                        min_log_prob =
                            std::min(min_log_prob, std::get<std::shared_ptr<const Tree>>(c_tree)->log_prob);
                    }

                    if (c_size < top_k or min_log_prob < tree->log_prob) {
                        constituent.insert(tree_node);
                        if (c_size > top_k) {
                            constituent.erase(std::min_element(
                                constituent.begin(),
                                constituent.end(),
                                [](auto&& lhs, auto&& rhs) {
                                    return std::get<std::shared_ptr<const Tree>>(lhs)->log_prob
                                        < std::get<std::shared_ptr<const Tree>>(rhs)->log_prob;
                                }));
                        }
                        changed = true;
                    }
                }
            }
        }
    }
}

}  // namespace

auto ViterbiParser::parse(std::vector<LetterType> const& tokens, int top_k) const -> std::unordered_set<Tree>
{
    const int num_tokens = static_cast<int>(tokens.size());

    // The most likely constituent table, addressed by span. chart[begin][end]
    // holds, per nonterminal category, the top-k trees covering [begin, end).
    Chart chart(static_cast<std::size_t>(num_tokens) + 1,
                std::vector<NtCell>(static_cast<std::size_t>(num_tokens) + 1));

    // Index the grammar productions by their first symbol once, so each span can
    // skip productions that cannot start there.
    const ProdIndex prod_index = build_prod_index(m_grammar);

    // starts_at[i] tracks the nonterminal categories that have a constituent
    // beginning at position i, which drives find_instantiations.
    std::vector<std::unordered_set<int>> starts_at(static_cast<std::size_t>(num_tokens) + 1);

    // Consider each span of length 1, 2, ..., n; and add any trees
    // that might cover that span to the constituents dictionary.
    for (int length = 1; length <= num_tokens; ++length) {
        // Find the most likely constituent spanning `length` text elements
        for (int begin = 0; begin <= num_tokens - length; ++begin) {
            add_constituents_spanning({begin, begin + length}, chart, starts_at, top_k, prod_index, tokens);
        }
    }

    // Return the tree that spans the entire text & have the right cat
    auto const& top_cell = chart[0][static_cast<std::size_t>(num_tokens)];
    if (auto it = top_cell.find(m_grammar.start().id()); it != top_cell.end()) {
        std::unordered_set<Tree> result {};
        for (auto&& node : it->second) {
            result.insert(*std::get<std::shared_ptr<const Tree>>(node));
        }
        return result;
    }

    // No solution found
    return {};
}

}  // namespace parser
