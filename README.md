# korean-verb-parser

A fast [Viterbi parser][viterbi] for **probabilistic context-free grammars
(PCFGs)**, written in C++ and applied to Korean verb morphology. Given a weighted
grammar and a string of single-character tokens, it returns the *top-k* most
likely parse trees, with probabilities combined in log space.

The algorithm is a bottom-up CYK-style chart parser (originally ported from
[NLTK][nltk]'s `ViterbiParser`, then heavily optimized — parsing the bundled
~2800-rule example grammar takes a few milliseconds).

It ships three ways to use it:

- a **C++ library** (`parser_lib`),
- a **command-line tool** (`parser`) that reads a JSON request on stdin, and
- a **Python package** (`korean_verb_parser`) via nanobind.

## Building

Requires a C++20 compiler and CMake ≥ 3.28. Third-party dependencies (fmt,
nlohmann_json, and Catch2 for tests) are fetched automatically at configure
time, so no package manager is needed (a network connection is required the
first time you configure). See [BUILDING.md](BUILDING.md) for details.

```sh
cmake -S . -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build
```

This produces the `parser` executable in `build/`.

## Command-line usage

`parser` reads a single JSON request on stdin and writes a single JSON result on
stdout. The request bundles the grammar, the sentence to parse, and how many
trees to return:

```json
{
  "start_symbol": "S",
  "sentence": "ABBB",
  "num_trees": 1,
  "rules": [
    {"lhs": "S", "rhs": [{"name": "A"}, {"name": "R"}], "prob": 1.0},
    {"lhs": "A", "rhs": ["A"], "prob": 1.0},
    {"lhs": "R", "rhs": [{"name": "R"}, {"name": "B"}], "prob": 0.5},
    {"lhs": "R", "rhs": [{"name": "B"}], "prob": 0.5},
    {"lhs": "B", "rhs": ["B"], "prob": 1.0}
  ]
}
```

```sh
./build/parser < request.json
```

The grammar format:

- **`start_symbol`** — the root nonterminal a full parse must span.
- **`rules`** — a list of weighted productions, each with an `lhs` nonterminal,
  a `prob` (relative weight), and an `rhs` list whose entries are either a
  nonterminal object `{"name": "X"}` or a single-character string terminal
  (e.g. `"A"`).
- **`sentence`** — the input string; each character is one terminal token.
- **`num_trees`** — the number of most-likely trees to return (the *k* in top-k).

On success the result is `{"status": "success", "elapsed_ms": <int>, "trees":
[...]}`, where each tree is `{"label", "children", "log_prob"}` (children are
nested trees or single-character strings). On failure it is `{"status":
"error", "message": "..."}` and the process exits non-zero.

A realistic Korean-morphology grammar is included at
[`examples/prods.json`](examples/prods.json).

## Python usage

The bindings are packaged with [scikit-build-core][skbuild]; installing builds
the extension via CMake (Python development headers required):

```sh
pip install .
```

```python
import korean_verb_parser as kvp

with open("examples/prods.json") as f:
    grammar = kvp.pcfg_from_json(f.read())

parser = kvp.ViterbiParser(grammar)
trees = parser.parse("hakeysssupnitaGipnita", top_k=10)

for tree in sorted(trees, key=lambda t: t.log_prob, reverse=True):
    print(tree.label, tree.log_prob)
    print(tree.to_json())   # full structure as a JSON string
```

The package exposes `pcfg_from_json(json_str)`, `ViterbiParser` with
`parse(text, top_k=1)`, and a minimal `Tree` (`.log_prob`, `.label`,
`.to_json()`, `str()`). See [`bindings/example.py`](bindings/example.py).

## Contributing

See the [CONTRIBUTING](CONTRIBUTING.md) and [HACKING](HACKING.md) documents.

## Licensing

No license has been chosen for this project yet.

[viterbi]: https://en.wikipedia.org/wiki/Viterbi_algorithm
[nltk]: https://www.nltk.org/
[skbuild]: https://scikit-build-core.readthedocs.io/
