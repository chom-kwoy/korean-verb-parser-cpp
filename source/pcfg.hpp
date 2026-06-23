#pragma once

#include <istream>
#include <vector>

#include <nlohmann/json.hpp>

#include "nonterminal.hpp"
#include "production.hpp"

namespace parser
{

using LetterType = char;
using LetterProd = Production<LetterType>;

class Pcfg
{
    Nonterminal m_start;
    std::vector<LetterProd> m_productions;

  public:
    Pcfg(Nonterminal start, std::vector<LetterProd> productions);

    auto start() const -> Nonterminal;
    auto productions() const -> std::vector<LetterProd> const&;
};

// Build a Pcfg from a JSON document of the form
//   {"start_symbol": "...", "rules": [{"lhs": "...", "rhs": [...], "prob": ...}, ...]}
// where each rhs entry is either {"name": "<Nonterminal>"} or a single-character
// string terminal. The istream overload parses the stream first.
auto pcfg_from_json(nlohmann::json const& input) -> Pcfg;
auto pcfg_from_json(std::istream& istream) -> Pcfg;

}  // namespace parser
