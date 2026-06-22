#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

#include "nonterminal.hpp"
#include "pcfg.hpp"

namespace parser
{

struct Tree;

}  // namespace parser

// Overload hash function for Tree
template<>
struct std::hash<parser::Tree>
{
    auto operator()(const parser::Tree& tree) const -> std::size_t;
};

namespace parser
{

struct Tree;

// A child is either a subtree or a terminal. Subtrees are held by shared_ptr so
// that combining constituents only copies pointers, not whole subtrees.
using TreeNode = std::variant<std::shared_ptr<const Tree>, LetterType>;

// Structural hash and equality for TreeNode: subtrees are compared by value
// (dereferencing the pointer), not by pointer identity. This is what lets the
// chart deduplicate structurally-identical derivations.
struct TreeNodeHash
{
    auto operator()(TreeNode const& node) const -> std::size_t;
};

struct TreeNodeEq
{
    auto operator()(TreeNode const& lhs, TreeNode const& rhs) const -> bool;
};

struct Tree
{
    Nonterminal symbol;
    std::vector<TreeNode> children;
    float log_prob = 0.F;
    // Structural hash, computed once at construction. Because children are shared
    // and carry their own cached hash, this is O(arity) rather than O(subtree),
    // and std::hash<Tree> just returns it.
    std::size_t hash_code = 0;

    Tree() = default;
    Tree(Nonterminal symbol, std::vector<TreeNode> children, float log_prob);

    Tree(Tree const&) = default;
    Tree(Tree&&) = default;

    auto operator=(Tree const&) -> Tree& = default;
    auto operator=(Tree&&) -> Tree& = default;

    ~Tree() = default;

    auto operator<(Tree const& rhs) const -> bool;
    auto operator==(Tree const& rhs) const -> bool;

    auto str(int indent_level = 0) const -> std::string;
    auto json() const -> nlohmann::json;
};

}  // namespace parser
