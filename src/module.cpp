#include "module.h"

namespace ionsl
{
    const FunctionDecl* Module::findFunctionByName(std::string_view name) const
    {
        for(auto& decl : m_decls)
        {
            if(const auto funcDecl = std::get_if<FunctionDecl>(&decl.decl))
            {
                if(funcDecl->name == name)
                {
                    return funcDecl;
                }
            }
        }

        return nullptr;
    }

    const StructDecl* Module::findStructByName(const std::string_view name) const
    {
        for(auto& decl : m_decls)
        {
            if(const auto structDecl = std::get_if<StructDecl>(&decl.decl))
            {
                if(structDecl->name == name)
                {
                    return structDecl;
                }
            }
        }

        return nullptr;
    }
}
