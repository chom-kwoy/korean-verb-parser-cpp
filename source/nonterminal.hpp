#pragma once

#include <string>

namespace parser
{

// A grammar nonterminal symbol. Each distinct name is interned to a small
// integer id (see nonterminal.cpp), so comparison, hashing and ordered-map
// lookups are integer operations rather than string operations. The name is
// only needed for output and is recovered from the intern table on demand.
class Nonterminal
{
    int m_id;

  public:
    explicit Nonterminal(std::string const& name);

    Nonterminal();

    Nonterminal(Nonterminal const&) = default;

    Nonterminal(Nonterminal&&) = default;

    auto operator=(Nonterminal const&) -> Nonterminal& = default;

    auto operator=(Nonterminal&&) -> Nonterminal& = default;

    ~Nonterminal() = default;

    auto name() const -> std::string;

    auto id() const -> int;

    auto operator<(Nonterminal const& other) const -> bool;

    auto operator==(Nonterminal const& other) const -> bool;
};

}  // namespace parser
