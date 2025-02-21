#include <chrono>
#include <exception>
#include <iostream>
#include <string>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

#include "nonterminal.hpp"
#include "pcfg.hpp"
#include "viterbiparser.h"

void parse_from_stream(std::istream& istream)
{
    using Symb = parser::Nonterminal;

    auto start_time = std::chrono::high_resolution_clock::now();

    const auto input = nlohmann::json::parse(istream);

    std::vector<parser::LetterProd> productions {};
    for (auto&& rule : input["rules"]) {
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

    const auto start_symbol = input["start_symbol"].get<std::string>();
    const auto parser = parser::ViterbiParser(parser::Pcfg(Symb(start_symbol), productions));

    std::vector<char> tokens {};
    for (const char chr : input["sentence"].get<std::string>()) {
        tokens.push_back(chr);
    }

    auto trees = parser.parse(tokens, input["num_trees"].get<int>());

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
