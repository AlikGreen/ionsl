#include "decl.h"

#include "declarations.h"

namespace ionsl
{
    void DeclTable::regenerate(const Module& module)
    {
        m_map.clear();
        AstWalker walker;
        walker.on([this](Declaration& decl)
        {
            m_map[decl.id] = &decl;
        });
        walker.walk(module);
    }

    Declaration * DeclTable::get(const DeclId id) const
    {
        if(const auto it = m_map.find(id); it != m_map.end())
            return it->second;

        return nullptr;
    }
}
