#pragma once

#include <string>

namespace parser
{

class Nonterminal
{
    std::string m_name;

  public:
    explicit Nonterminal(std::string name);

    Nonterminal() = default;

    Nonterminal(Nonterminal const&) = default;

    Nonterminal(Nonterminal&&) = default;

    auto operator=(Nonterminal const&) -> Nonterminal& = default;

    auto operator=(Nonterminal&&) -> Nonterminal& = default;

    ~Nonterminal() = default;

    auto name() const -> std::string;

    auto operator<(Nonterminal const& other) const -> bool;
};

}  // namespace parser
