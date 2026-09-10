#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "symbol.h"

namespace ionsl
{
class SymbolTable
{
public:
    SymbolTable();
    SymbolId intern(const std::string &text);
    SymbolId intern(std::string_view text);
    [[nodiscard]] std::string get(SymbolId id) const;
private:
    std::vector<std::string> m_names;
    std::unordered_map<std::string, SymbolId> m_lookup;
};
}
