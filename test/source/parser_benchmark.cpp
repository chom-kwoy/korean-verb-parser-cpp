#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include "pcfg.hpp"
#include "viterbiparser.h"

namespace {

// Slurps a grammar fixture into a string, ready to hand to parser::pcfg_from_json.
// Path is relative, so the benchmark must be run with the project root as the
// working directory. Reading once keeps JSON parsing inside the timed loop.
auto load_grammar_text(std::string const& path) -> std::string
{
    auto grammar_file = std::ifstream(path);
    std::ostringstream buffer;
    buffer << grammar_file.rdbuf();
    return buffer.str();
}

auto to_tokens(std::string const& text) -> std::vector<parser::LetterType>
{
    std::vector<parser::LetterType> tokens {};
    for (char chr : text) {
        tokens.push_back(chr);
    }
    return tokens;
}

// Benchmark grammar construction and parsing for one fixture. Shared by every
// grammar-size case, which differ only in the fixture path.
void run_benchmarks(std::string const& grammar_path)
{
    const auto grammar = load_grammar_text(grammar_path);
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

}  // namespace

TEST_CASE("Viterbi parser benchmark - prods", "[.][benchmark]")
{
    run_benchmarks("examples/prods.json");
}

TEST_CASE("Viterbi parser benchmark - 1K", "[.][benchmark-1k]")
{
    run_benchmarks("examples/prods_1k.json");
}

TEST_CASE("Viterbi parser benchmark - 10K", "[.][benchmark-10k]")
{
    run_benchmarks("examples/prods_10k.json");
}

TEST_CASE("Viterbi parser benchmark - 10K", "[.][benchmark-full]")
{
    run_benchmarks("examples/prods_full.json");
}
