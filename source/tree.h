#pragma once

#include <cstddef>
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

// Overload hash function for TreeNode
template<>
struct std::hash<parser::Tree>
{
    auto operator()(const parser::Tree& tree) const -> std::size_t;
};

namespace parser
{

struct Tree;
using TreeNode = std::variant<Tree, LetterType>;

struct Tree
{
    Nonterminal symbol;
    std::vector<TreeNode> children;
    float log_prob = 0.F;

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
