"""Viterbi parser for Korean verb morphology (PCFG).

The heavy lifting is implemented in C++ and exposed through the compiled
``_core`` extension module.
"""

from ._core import Pcfg, Tree, ViterbiParser, pcfg_from_json

__all__ = ["Pcfg", "Tree", "ViterbiParser", "pcfg_from_json"]
