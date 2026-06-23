#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "nonterminal.hpp"
#include "pcfg.hpp"
#include "viterbiparser.h"

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
    using Symb = parser::Nonterminal;
    auto t0 = std::chrono::high_resolution_clock::now();

    auto prods_file = std::ifstream("examples/prods.json");
    const auto parser = parser::ViterbiParser(parser::pcfg_from_json(prods_file));

    std::vector<char> tokens {};
    for (char chr : "hakeysssupnitaGipnita") {
        tokens.push_back(chr);
    }
    tokens.pop_back();

    auto result = parser.parse(tokens, 10);
    auto t1 = std::chrono::high_resolution_clock::now();

    for (auto&& tree : result) {
        std::cout << tree.str() << "\n";
    }
    std::cout << "Took " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << "ms\n";
    std::cout.flush();
}

TEST_CASE("Test", "[test_1k]")
{
    using Symb = parser::Nonterminal;
    auto t0 = std::chrono::high_resolution_clock::now();

    auto prods_file = std::ifstream("examples/prods_1k.json");
    const auto parser = parser::ViterbiParser(parser::pcfg_from_json(prods_file));

    std::vector<char> tokens {};
    for (char chr : "hakeysssupnitaGipnita") {
        tokens.push_back(chr);
    }
    tokens.pop_back();

    auto result = parser.parse(tokens, 10);
    auto t1 = std::chrono::high_resolution_clock::now();

    for (auto&& tree : result) {
        std::cout << tree.str() << "\n";
    }
    std::cout << "Took " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << "ms\n";
    std::cout.flush();
}
