#include "symbolTable.h"

namespace ionsl
{
    SymbolId SymbolTable::intern(const std::string &text)
    {
        if(const auto it = m_lookup.find(text); it != m_lookup.end())
            return it->second;

        const SymbolId id = m_names.size();
        m_lookup[text] = id;
        m_names.push_back(text);
        return id;
    }

    SymbolId SymbolTable::intern(const std::string_view text)
    {
        return intern(std::string(text));
    }

    std::string SymbolTable::get(const SymbolId id) const
    {
        if(id >= m_names.size()) return "";
        return m_names.at(id);
    }
}
