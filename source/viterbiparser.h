#pragma once

#include <unordered_set>
#include <vector>

#include "pcfg.hpp"
#include "tree.h"

namespace parser
{

class ViterbiParser
{
    Pcfg m_grammar;

  public:
    explicit ViterbiParser(Pcfg const& grammar);
    explicit ViterbiParser(Pcfg&& grammar);

    auto parse(std::vector<LetterType> const& tokens, int top_k = 1) const -> std::unordered_set<Tree>;
};

}  // namespace parser
