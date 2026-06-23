#include <string>
#include <vector>

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "pcfg.hpp"
#include "tree.h"
#include "viterbiparser.h"

namespace nb = nanobind;
using namespace nb::literals;

NB_MODULE(_core, m)
{
    m.doc() = "Python bindings for the Korean verb PCFG Viterbi parser.";

    nb::class_<parser::Pcfg>(m, "Pcfg", "A probabilistic context-free grammar.");

    m.def(
        "pcfg_from_json",
        [](std::string const& json) -> parser::Pcfg { return parser::pcfg_from_json(json); },
        "json"_a,
        "Build a Pcfg from a JSON string of the form "
        R"({"start_symbol": "...", "rules": [{"lhs": "...", "rhs": [...], "prob": ...}, ...]}.)");

    nb::class_<parser::Tree>(m, "Tree", "A parse tree.")
        .def_ro("log_prob", &parser::Tree::log_prob)
        .def_prop_ro("label", [](parser::Tree const& tree) { return tree.symbol.name(); })
        .def(
            "to_json",
            [](parser::Tree const& tree) { return tree.json().dump(); },
            "Return the tree serialized as a JSON string.")
        .def("__str__", [](parser::Tree const& tree) { return tree.str(); })
        .def("__repr__",
             [](parser::Tree const& tree) {
                 return "<korean_verb_parser.Tree " + tree.symbol.name() + " log_prob=" + std::to_string(tree.log_prob)
                     + ">";
             });

    nb::class_<parser::ViterbiParser>(m, "ViterbiParser", "Top-k Viterbi parser for a Pcfg.")
        .def(nb::init<parser::Pcfg const&>(), "grammar"_a)
        .def(
            "parse",
            [](parser::ViterbiParser const& self, std::string const& text, int top_k)
            {
                // Each byte of the string is treated as one terminal token, which
                // matches how the C++ parser consumes char tokens.
                std::vector<parser::LetterType> tokens(text.begin(), text.end());
                auto result = self.parse(tokens, top_k);
                return std::vector<parser::Tree>(result.begin(), result.end());
            },
            "text"_a,
            "top_k"_a = 1,
            "Parse `text` (one token per byte) and return up to `top_k` most-likely parse trees.");
}
