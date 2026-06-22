"""Minimal usage example for the korean_verb_parser package.

Install the package first (from the project root):

    pip install .

Then run this from the project root (so examples/prods.json resolves):

    python bindings/example.py
"""

import json

import korean_verb_parser as kvp

with open("examples/prods.json", encoding="utf-8") as fh:
    grammar = kvp.pcfg_from_json(fh.read())

viterbi = kvp.ViterbiParser(grammar)

trees = viterbi.parse("hakeysssupnitaGipnita", top_k=10)

print(f"{len(trees)} parse(s) found")
for tree in sorted(trees, key=lambda t: t.log_prob, reverse=True):
    print(f"  {tree.label}  log_prob={tree.log_prob:.4f}")
    # Full structure is available as JSON:
    _ = json.loads(tree.to_json())
