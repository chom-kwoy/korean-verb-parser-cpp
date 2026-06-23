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

namespace
{

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
            // Binary NT-NT productions go to the (B, C) pair index; everything
            // else NT-first goes through the general first-symbol path.
            if (prod.rhs.size() == 2 && std::holds_alternative<Nonterminal>(prod.rhs[1])) {
                const int second = std::get<Nonterminal>(prod.rhs[1]).id();
                index.binary_nt[cat][second].push_back(&prod);
            } else {
                index.by_first_nt[cat].push_back(&prod);
                if (prod.rhs.size() == 1) {
                    index.unary_by_first_nt[cat].push_back(&prod);
                }
            }
        } else {
            index.by_first_term[std::get<LetterType>(first)].push_back(&prod);
        }
    }
    return index;
}

}  // namespace

ViterbiParser::ViterbiParser(Pcfg const& grammar)
    : m_grammar(grammar)
    , m_index(build_prod_index(m_grammar))
{
}

ViterbiParser::ViterbiParser(Pcfg&& grammar)
    : m_grammar(std::move(grammar))
    , m_index(build_prod_index(m_grammar))
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

// For each begin position, maps a nonterminal id to the spans that begin there:
// a list of (end, trees) pairs kept sorted by end. This lets match_rhs jump
// straight to the splits where a symbol actually has a constituent, instead of
// probing every chart cell. The NodeSet pointers reference values stored in the
// chart; they stay valid because categories are never erased from a chart cell
// (only the trees within a NodeSet are pruned, which keeps the NodeSet itself).
using SpanList = std::vector<std::pair<int, NodeSet const*>>;
using StartsByBegin = std::vector<std::unordered_map<int, SpanList>>;

/**
 * @return a set of all the lists of children that cover `range`
 *  and that match `rhs`.
 */
auto match_rhs(std::span<const RhsType> rhs,
               Range range,
               StartsByBegin const& starts,
               std::vector<LetterType> const& tokens) -> std::vector<std::vector<TreeNode>>
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
            auto rights = match_rhs(rhs.subspan(1), {split, range.end}, starts, tokens);
            prepend(TreeNode {std::get<LetterType>(first)}, rights);
        }
        return childlists;
    }

    const int symbol = std::get<Nonterminal>(first).id();
    auto const& begin_starts = starts[static_cast<std::size_t>(range.begin)];
    auto sit = begin_starts.find(symbol);
    if (sit == begin_starts.end()) {
        return childlists;
    }
    SpanList const& spans = sit->second;  // sorted by end

    if (rhs.size() == 1) {
        // Last symbol: it must span exactly to range.end. Binary-search for it.
        auto const lb = std::lower_bound(
            spans.begin(), spans.end(), range.end, [](auto const& span, int e) { return span.first < e; });
        if (lb != spans.end() && lb->first == range.end) {
            for (auto&& left : *lb->second) {
                childlists.push_back(std::vector {left});
            }
        }
        return childlists;
    }

    // Non-final symbol: it must leave room for the rest, so its split must be
    // strictly before range.end. Spans are sorted, so stop at the first one that
    // reaches (or passes) range.end.
    for (auto const& [split, nodeset] : spans) {
        if (split >= range.end) {
            break;
        }
        auto rights = match_rhs(rhs.subspan(1), {split, range.end}, starts, tokens);
        if (rights.empty()) {
            continue;
        }
        for (auto&& left : *nodeset) {
            prepend(left, rights);
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
 * `range.begin` are considered: nonterminals that already have a constituent
 * beginning at `range.begin`, plus the single terminal at that position. This
 * avoids scanning the whole grammar.
 */
auto find_instantiations(Range range,
                         Chart const& chart,
                         StartsByBegin const& starts,
                         ProdIndex const& index,
                         std::vector<LetterType> const& tokens,
                         bool unary_only) -> std::vector<std::pair<LetterProd const*, std::vector<TreeNode>>>
{
    auto result = std::vector<std::pair<LetterProd const*, std::vector<TreeNode>>> {};

    auto consider = [&](LetterProd const* production)
    {
        auto childlists = match_rhs(std::span {production->rhs.begin(), production->rhs.end()}, range, starts, tokens);
        for (auto&& childlist : childlists) {
            result.emplace_back(production, std::move(childlist));
        }
    };

    // After the first pass only unary productions can produce anything new, so
    // restrict to those and skip the (unchanging) terminal- and binary-first
    // productions.
    auto const& nt_index = unary_only ? index.unary_by_first_nt : index.by_first_nt;

    // General path: NT-first (non-binary) productions whose first symbol is a
    // nonterminal present at range.begin.
    for (auto const& [cat, spans] : starts[static_cast<std::size_t>(range.begin)]) {
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

    // Binary NT-NT productions, CYK-style: for each split, pair the categories
    // actually present on the left sub-span with those on the right, and fire
    // only the productions registered for that (B, C) pair. This avoids touching
    // the thousands of productions that share a first symbol but whose second
    // symbol is absent.
    for (int split = range.begin + 1; split < range.end; ++split) {
        NtCell const& left = chart[static_cast<std::size_t>(range.begin)][static_cast<std::size_t>(split)];
        if (left.empty()) {
            continue;
        }
        NtCell const& right = chart[static_cast<std::size_t>(split)][static_cast<std::size_t>(range.end)];
        if (right.empty()) {
            continue;
        }
        for (auto const& [b, left_trees] : left) {
            auto bit = index.binary_nt.find(b);
            if (bit == index.binary_nt.end()) {
                continue;
            }
            auto const& by_second = bit->second;
            for (auto const& [c, right_trees] : right) {
                auto cit = by_second.find(c);
                if (cit == by_second.end()) {
                    continue;
                }
                for (auto const* production : cit->second) {
                    for (auto const& left_tree : left_trees) {
                        for (auto const& right_tree : right_trees) {
                            result.emplace_back(production, std::vector<TreeNode> {left_tree, right_tree});
                        }
                    }
                }
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
                               StartsByBegin& starts,
                               int top_k,
                               ProdIndex const& index,
                               std::vector<LetterType> const& tokens)
{
    NtCell& chart_cell = chart[static_cast<std::size_t>(range.begin)][static_cast<std::size_t>(range.end)];
    auto& begin_starts = starts[static_cast<std::size_t>(range.begin)];

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
        auto instantiations = find_instantiations(range, chart, starts, index, tokens, /*unary_only=*/!first_pass);
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
                auto [emplaced, _] = chart_cell.emplace(node.id(), NodeSet {tree_node});
                begin_starts[node.id()].emplace_back(range.end, &emplaced->second);
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

    // starts[i] records, per nonterminal category, the spans beginning at
    // position i (as (end, trees) pairs). It drives find_instantiations and lets
    // match_rhs jump to the splits that actually have constituents.
    StartsByBegin starts(static_cast<std::size_t>(num_tokens) + 1);

    // Consider each span of length 1, 2, ..., n; and add any trees
    // that might cover that span to the constituents dictionary.
    for (int length = 1; length <= num_tokens; ++length) {
        // Find the most likely constituent spanning `length` text elements
        for (int begin = 0; begin <= num_tokens - length; ++begin) {
            add_constituents_spanning({begin, begin + length}, chart, starts, top_k, m_index, tokens);
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
