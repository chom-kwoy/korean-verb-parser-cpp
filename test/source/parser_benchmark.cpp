#include <fstream>
#include <string>
#include <variant>
#include <vector>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "nonterminal.hpp"
#include "pcfg.hpp"
#include "viterbiparser.h"

namespace
{

// Loads the Korean-verb grammar fixture. Path is relative, so the benchmark
// must be run with the project root as the working directory.
auto load_productions(std::string const& path) -> std::vector<parser::LetterProd>
{
    using Symb = parser::Nonterminal;

    auto prods_file = std::ifstream(path);
    auto rules = nlohmann::json::parse(prods_file);

    std::vector<parser::LetterProd> productions {};
    for (auto&& rule : rules) {
        auto lhs = rule["lhs"].get<std::string>();
        auto prob = rule["prob"].get<float>();
        auto rhs = std::vector<std::variant<parser::Nonterminal, parser::LetterType>> {};
        for (auto&& item : rule["rhs"]) {
            if (item.is_object()) {
                rhs.emplace_back(Symb(item["name"].get<std::string>()));
            } else {
                rhs.emplace_back(item.get<std::string>()[0]);
            }
        }
        productions.push_back({Symb(lhs), rhs, prob});
    }

    return productions;
}

auto to_tokens(std::string const& text) -> std::vector<parser::LetterType>
{
    std::vector<parser::LetterType> tokens {};
    for (char chr : text) {
        tokens.push_back(chr);
    }
    return tokens;
}

}  // namespace

TEST_CASE("Viterbi parser benchmark", "[.][benchmark]")
{
    using Symb = parser::Nonterminal;

    const auto productions = load_productions("examples/prods.json");
    const auto tokens = to_tokens("hakeysssupnitaGipnita");
    constexpr int top_k = 10;

    // Sanity check that the workload actually produces parses, so we never
    // benchmark a silently-empty parse.
    {
        const auto parser = parser::ViterbiParser(parser::Pcfg(Symb("Noun"), productions));
        const auto result = parser.parse(tokens, top_k);
        REQUIRE(!result.empty());
    }

    BENCHMARK("Pcfg construction")
    {
        return parser::Pcfg(Symb("Noun"), productions);
    };

    BENCHMARK_ADVANCED("parse (grammar prebuilt)")(Catch::Benchmark::Chronometer meter)
    {
        const auto parser = parser::ViterbiParser(parser::Pcfg(Symb("Noun"), productions));
        meter.measure([&] { return parser.parse(tokens, top_k); });
    };

    BENCHMARK("Pcfg construction + parse")
    {
        const auto parser = parser::ViterbiParser(parser::Pcfg(Symb("Noun"), productions));
        return parser.parse(tokens, top_k);
    };
}
