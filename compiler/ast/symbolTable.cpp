#include "symbolTable.h"

namespace ionsl
{
    SymbolTable::SymbolTable()
    {
        m_names.emplace_back("invalid");
    }

    SymbolId SymbolTable::intern(const std::string &text)
    {
        if(const auto it = m_lookup.find(text); it != m_lookup.end())
            return it->second;

        const SymbolId id = SymbolId(m_names.size());
        m_lookup[text] = id;
        m_names.push_back(text);
        return id;
    }

    SymbolId SymbolTable::intern(const std::string_view text)
    {
        return intern(std::string(text));
    }

    std::optional<SymbolId> SymbolTable::find(const std::string &text)
    {
        if(const auto it = m_lookup.find(text); it != m_lookup.end())
            return it->second;

        return std::nullopt;
    }

    std::string SymbolTable::get(const SymbolId id) const
    {
        if(id.value() >= m_names.size()) return "";
        return m_names.at(id.value());
    }
}
