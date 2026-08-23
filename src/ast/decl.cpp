#include "decl.h"

#include "declarations.h"

namespace ionsl
{
    DeclTable::DeclTable(Module &module)
    {
        AstWalker walker;
        walker.on([this](Declaration& decl)
        {
            m_map[decl.id] = &decl;
        });
    }

    Declaration * DeclTable::get(const DeclId id) const
    {
        if(const auto it = m_map.find(id); it != m_map.end())
            return it->second;

        return nullptr;
    }
}
