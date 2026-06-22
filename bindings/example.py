"""Minimal usage example for the `parser` extension module.

Build the module first (from the project root):

    cmake -S . -B build -D parser_BUILD_PYTHON=ON -D CMAKE_BUILD_TYPE=Release
    cmake --build build

Then run this from the directory containing the built `parser` module, e.g.:

    PYTHONPATH=build python bindings/example.py
"""

import json

import parser

with open("examples/prods.json", encoding="utf-8") as fh:
    grammar = parser.pcfg_from_json(fh.read())

viterbi = parser.ViterbiParser(grammar)

trees = viterbi.parse("hakeysssupnitaGipnita", top_k=10)

print(f"{len(trees)} parse(s) found")
for tree in sorted(trees, key=lambda t: t.log_prob, reverse=True):
    print(f"  {tree.label}  log_prob={tree.log_prob:.4f}")
    # Full structure is available as JSON:
    _ = json.loads(tree.to_json())
