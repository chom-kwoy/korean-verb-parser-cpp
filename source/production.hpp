#pragma once

#include <tuple>
#include <variant>
#include <vector>

#include "nonterminal.hpp"

namespace parser
{

template<typename TerminalT>
struct Production
{
    using Terminal = TerminalT;
    using RhsType = std::variant<Nonterminal, Terminal>;

    Nonterminal lhs;
    std::vector<RhsType> rhs;
    float prob = 1.0F;

    // Compare operator
    auto operator<(Production const& other) const -> bool
    {
        return std::tie(lhs, rhs) < std::tie(other.lhs, other.rhs);
    }
};

}  // namespace parser
