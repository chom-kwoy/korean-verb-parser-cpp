#include <string>
#include <utility>

#include "nonterminal.hpp"

namespace parser
{

Nonterminal::Nonterminal(std::string name)
    : m_name {std::move(name)}
{
}

auto Nonterminal::name() const -> std::string
{
    return m_name;
}

auto Nonterminal::operator<(const Nonterminal& other) const -> bool
{
    return m_name < other.m_name;
}

}  // namespace parser
