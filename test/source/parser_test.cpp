#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "nonterminal.hpp"
#include "pcfg.hpp"
#include "viterbiparser.h"

namespace {

// Load a grammar fixture, parse a fixed sentence with it, and print the trees
// (and elapsed time). Shared by the prods.json / prods_1k / prods_10k cases,
// which differ only in the fixture path.
void parse_grammar_file(std::string const& path)
{
    const auto t0 = std::chrono::high_resolution_clock::now();

    auto prods_file = std::ifstream(path);
    const auto parser = parser::ViterbiParser(parser::pcfg_from_json(prods_file));

    std::vector<char> tokens {};
    for (char chr : std::string("hakeysssupnitaGipnita")) {
        tokens.push_back(chr);
    }

    const auto result = parser.parse(tokens, 10);
    const auto t1 = std::chrono::high_resolution_clock::now();

    for (auto&& tree : result) {
        std::cout << tree.str() << "\n";
    }
    std::cout << "Took " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << "ms\n";
    std::cout.flush();
}

}  // namespace

TEST_CASE("Test", "[test_basic]")
{
    using Symb = parser::Nonterminal;
    const auto parser = parser::ViterbiParser(parser::Pcfg(  //
        Symb("S"),
        {
            {Symb("S"), {Symb("A"), Symb("R")}, 1.0},
            {Symb("A"), {'A'}, 1.0},
            {Symb("R"), {Symb("R"), Symb("B")}, 0.5},
            {Symb("R"), {Symb("B")}, 0.5},
            {Symb("B"), {'B'}, 1.0},
        }));

    std::cout << "parsing..." << std::endl;

    auto result = parser.parse({'A', 'B', 'B', 'B'});

    for (auto&& tree : result) {
        std::cout << tree.str() << "\n";
    }
    std::cout.flush();
}

TEST_CASE("Test", "[test_complex]")
{
    parse_grammar_file("examples/prods.json");
}

TEST_CASE("Test", "[test_1k]")
{
    parse_grammar_file("examples/prods_1k.json");
}

TEST_CASE("Test", "[test_10k]")
{
    parse_grammar_file("examples/prods_10k.json");
}

TEST_CASE("Test", "[test_full]")
{
    parse_grammar_file("examples/prods_full.json");
}
