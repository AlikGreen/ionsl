#include "decl.h"

#include "declarations.h"

namespace ionsl
{
    constexpr DeclId DeclId::Error   = DeclId(0);

    DeclTable::DeclTable(const Module &module)
    {
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

    DeclId DeclAllocator::allocate()
    {
        return DeclId{m_nextId++};
    }
}
