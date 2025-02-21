#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <map>
#include <span>
#include <tuple>
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
using ConstituentKey = std::tuple<int, int, std::variant<Nonterminal, LetterType>>;  // (begin, end, symbol)
using ConstituentMap = std::map<ConstituentKey, std::unordered_set<TreeNode>>;

/**
 * @return a set of all the lists of children that cover `range`
 *  and that match `rhs`.
 */
auto match_rhs(std::span<const std::variant<Nonterminal, LetterType>> rhs,
               Range range,
               ConstituentMap const& constituents) -> std::vector<std::vector<TreeNode>>
{
    // Base case
    if (range.begin >= range.end and rhs.empty()) {
        return {std::vector<TreeNode> {}};
    }
    if (range.begin >= range.end or rhs.empty()) {
        return {};
    }

    auto childlists = std::vector<std::vector<TreeNode>> {};
    for (int split = range.begin; split <= range.end; ++split) {
        if (constituents.contains({range.begin, split, rhs[0]})) {
            auto const& lefts = constituents.at({range.begin, split, rhs[0]});
            auto rights = match_rhs(rhs.subspan(1), {split, range.end}, constituents);
            for (auto&& left : lefts) {
                for (auto&& right : rights) {
                    auto new_child = std::vector {left};
                    new_child.reserve(1 + right.size());
                    std::copy(right.begin(), right.end(), std::back_inserter(new_child));
                    childlists.push_back(new_child);
                }
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
 */
auto find_instantiations(Range range,
                         ConstituentMap const& constituents,
                         Pcfg const& grammar) -> std::vector<std::pair<LetterProd, std::vector<TreeNode>>>
{
    auto result = std::vector<std::pair<LetterProd, std::vector<TreeNode>>> {};

    for (auto&& production : grammar.productions()) {
        auto childlists = match_rhs(std::span {production.rhs.begin(), production.rhs.end()}, range, constituents);

        for (auto&& childlist : childlists) {
            result.emplace_back(production, childlist);
        }
    }

    return result;
}

/**
 * Find any constituents that might cover `range`, and add them
 * to the most likely constituents table.
 */
auto add_constituents_spanning(Range range, ConstituentMap& constituents, int top_k, Pcfg const& grammar)
{
    // Since some of the grammar productions may be unary, we need to
    // repeatedly try all of the productions until none of them add any
    // new constituents.
    bool changed = true;
    while (changed) {
        changed = false;

        // Find all ways instantiations of the grammar productions that
        // cover the span.
        auto instantiations = find_instantiations(range, constituents, grammar);

        // For each production instantiation, add a new
        // Tree whose probability is the product of the
        // children's probabilities and the production's
        // probability.
        for (auto&& [production, children] : instantiations) {
            float log_p = std::log(production.prob);
            for (auto&& child : children) {
                if (std::holds_alternative<Tree>(child)) {
                    log_p += std::get<Tree>(child).log_prob;
                }
            }

            auto node = production.lhs;
            auto tree = Tree {node, children, log_p};

            // If it's a new constituent, then add it to the
            // constituents dictionary.
            auto key = std::tuple {range.begin, range.end, production.lhs};
            if (not constituents.contains(key)) {
                constituents[key] = {tree};
                changed = true;
            } else {
                auto& constituent = constituents.at(key);
                const bool contains = constituent.contains(tree);
                if (not contains) {
                    const int c_size = static_cast<int>(constituent.size());
                    float min_log_prob = std::numeric_limits<float>::max();
                    for (auto&& c_tree : constituent) {
                        min_log_prob = std::min(min_log_prob, std::get<Tree>(c_tree).log_prob);
                    }

                    if (c_size < top_k or min_log_prob < tree.log_prob) {
                        constituent.insert(tree);
                        if (c_size > top_k) {
                            constituent.erase(std::min_element(
                                constituent.begin(),
                                constituent.end(),
                                [](auto&& lhs, auto&& rhs)
                                { return std::get<Tree>(lhs).log_prob < std::get<Tree>(rhs).log_prob; }));
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
    // The most likely constituent table.  This table specifies the
    // most likely constituent for a given span and type.
    // Constituents can be either Trees or tokens.  For Trees,
    // the "type" is the Nonterminal for the tree's root node
    // value.  For Tokens, the "type" is the token's type.
    // The table is stored as a dictionary, since it is sparse.
    ConstituentMap constituents {};

    // Initialize the constituents dictionary with the words from the text.
    int index = 0;
    for (auto token : tokens) {
        constituents[{index, index + 1, token}] = {{token}};
        index++;
    }

    // Consider each span of length 1, 2, ..., n; and add any trees
    // that might cover that span to the constituents dictionary.
    const int num_tokens = static_cast<int>(tokens.size());
    for (int length = 1; length <= num_tokens; ++length) {
        // Find the most likely constituent spanning `length` text elements
        for (int begin = 0; begin <= num_tokens - length; ++begin) {
            add_constituents_spanning({begin, begin + length}, constituents, top_k, m_grammar);
        }
    }

    // Return the tree that spans the entire text & have the right cat
    if (constituents.contains({0, num_tokens, m_grammar.start()})) {
        auto result_nodes = constituents[{0, num_tokens, m_grammar.start()}];
        std::unordered_set<Tree> result {};
        for (auto&& node : result_nodes) {
            result.insert(std::get<Tree>(node));
        }
        return result;
    }

    // No solution found
    return {};
}

}  // namespace parser
