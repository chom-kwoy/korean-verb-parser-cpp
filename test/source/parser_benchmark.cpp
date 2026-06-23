#include <fstream>
#include <string>
#include <vector>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "pcfg.hpp"
#include "viterbiparser.h"

namespace {

// Loads and parses a grammar fixture into a JSON document, ready to hand to
// parser::pcfg_from_json. Path is relative, so the benchmark must be run with
// the project root as the working directory.
auto load_grammar_json(std::string const& path) -> nlohmann::json
{
    auto grammar_file = std::ifstream(path);
    return nlohmann::json::parse(grammar_file);
}

auto to_tokens(std::string const& text) -> std::vector<parser::LetterType>
{
    std::vector<parser::LetterType> tokens {};
    for (char chr : text) {
        tokens.push_back(chr);
    }
    return tokens;
}

} // namespace

TEST_CASE("Viterbi parser benchmark", "[.][benchmark]")
{
    const auto grammar = load_grammar_json("examples/prods.json");
    const auto tokens = to_tokens("hakeysssupnitaGipnita");
    constexpr int top_k = 10;

    // Sanity check that the workload actually produces parses, so we never
    // benchmark a silently-empty parse.
    {
        const auto parser = parser::ViterbiParser(parser::pcfg_from_json(grammar));
        const auto result = parser.parse(tokens, top_k);
        REQUIRE(!result.empty());
    }

    BENCHMARK("Pcfg construction")
    {
        return parser::pcfg_from_json(grammar);
    };

    BENCHMARK_ADVANCED("parse (grammar prebuilt)")(Catch::Benchmark::Chronometer meter)
    {
        const auto parser = parser::ViterbiParser(parser::pcfg_from_json(grammar));
        meter.measure([&] { return parser.parse(tokens, top_k); });
    };

    BENCHMARK("Pcfg construction + parse")
    {
        const auto parser = parser::ViterbiParser(parser::pcfg_from_json(grammar));
        return parser.parse(tokens, top_k);
    };
}

TEST_CASE("Viterbi parser benchmark - 1K", "[.][benchmark-1k]")
{
    const auto grammar = load_grammar_json("examples/prods_1k.json");
    const auto tokens = to_tokens("hakeysssupnitaGipnita");
    constexpr int top_k = 10;

    // Sanity check that the workload actually produces parses, so we never
    // benchmark a silently-empty parse.
    {
        const auto parser = parser::ViterbiParser(parser::pcfg_from_json(grammar));
        const auto result = parser.parse(tokens, top_k);
        REQUIRE(!result.empty());
    }

    BENCHMARK("Pcfg construction")
    {
        return parser::pcfg_from_json(grammar);
    };

    BENCHMARK_ADVANCED("parse (grammar prebuilt)")(Catch::Benchmark::Chronometer meter)
    {
        const auto parser = parser::ViterbiParser(parser::pcfg_from_json(grammar));
        meter.measure([&] { return parser.parse(tokens, top_k); });
    };

    BENCHMARK("Pcfg construction + parse")
    {
        const auto parser = parser::ViterbiParser(parser::pcfg_from_json(grammar));
        return parser.parse(tokens, top_k);
    };
}
