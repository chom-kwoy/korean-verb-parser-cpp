#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include "nonterminal.hpp"

namespace parser
{

namespace
{

// Global intern table mapping each distinct nonterminal name to a small integer
// id. Interning makes nonterminal identity an int, which is what the parser's
// hot paths (chart keys, tree hashing/equality) rely on. Single-threaded use is
// assumed, matching the rest of the parser.
struct InternTable
{
    std::unordered_map<std::string, int> ids;
    std::vector<std::string> names;

    auto intern(std::string const& name) -> int
    {
        auto it = ids.find(name);
        if (it != ids.end()) {
            return it->second;
        }
        const int id = static_cast<int>(names.size());
        names.push_back(name);
        ids.emplace(name, id);
        return id;
    }
};

auto table() -> InternTable&
{
    static InternTable instance;
    return instance;
}

}  // namespace

Nonterminal::Nonterminal(std::string const& name)
    : m_id {table().intern(name)}
{
}

Nonterminal::Nonterminal()
    : m_id {table().intern(std::string {})}
{
}

auto Nonterminal::name() const -> std::string
{
    return table().names[static_cast<std::size_t>(m_id)];
}

auto Nonterminal::id() const -> int
{
    return m_id;
}

auto Nonterminal::operator<(const Nonterminal& other) const -> bool
{
    return m_id < other.m_id;
}

auto Nonterminal::operator==(const Nonterminal& other) const -> bool
{
    return m_id == other.m_id;
}

}  // namespace parser
