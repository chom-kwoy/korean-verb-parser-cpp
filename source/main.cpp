#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>
#include <simdjson.h>

#include "nonterminal.hpp"
#include "pcfg.hpp"
#include "viterbiparser.h"

void parse_from_stream(std::istream& istream)
{
    auto start_time = std::chrono::high_resolution_clock::now();

    std::ostringstream buffer;
    buffer << istream.rdbuf();
    const std::string request = buffer.str();

    // The grammar (start_symbol + rules) is parsed straight into productions by
    // pcfg_from_json (simdjson); we only need the sentence and tree count from
    // the rest of the request, which a second lightweight pass reads cheaply.
    const auto parser = parser::ViterbiParser(parser::pcfg_from_json(request));

    namespace oj = simdjson::ondemand;
    oj::parser json_parser;
    auto padded = simdjson::padded_string(request);
    oj::document doc = json_parser.iterate(padded);
    const std::string sentence {std::string_view {doc["sentence"].get_string()}};
    const auto num_trees = static_cast<int>(std::int64_t {doc["num_trees"].get_int64()});

    std::vector<char> tokens(sentence.begin(), sentence.end());

    auto trees = parser.parse(tokens, num_trees);

    auto json_trees = std::vector<nlohmann::json> {};
    for (auto&& tree : trees) {
        json_trees.push_back(tree.json());
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    auto result = nlohmann::json {
        {"status", "success"},
        {"elapsed_ms", elapsed.count()},
        {"trees", json_trees},
    };
    std::cout << result.dump() << "\n";
}

auto main() -> int
{
    try {
        parse_from_stream(std::cin);
    } catch (std::exception const& e) {
        auto result = nlohmann::json {
            {"status", "error"},
            {"message", e.what()},
        };
        std::cout << result.dump() << "\n";
        std::cerr << "Uncaught error:\n";
        std::cerr << e.what() << "\n";
        return 1;
    } catch (...) {
        auto result = nlohmann::json {
            {"status", "error"},
            {"message", "Unknown error"},
        };
        std::cout << result.dump() << "\n";
        std::cerr << "Uncaught unknown error.\n";
        return 1;
    }

    return 0;
}
