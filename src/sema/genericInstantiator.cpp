#include "genericInstantiator.h"

#include "semaContext.h"
#include "../ast/module.h"

namespace ionsl
{
    FunctionDecl* GenericInstantiator::instantiate(const FunctionDecl& genericDecl, const std::vector<TypeArgument*> &typeArgs)
    {
        std::vector<TypeId> typeArgTypes{};
        for(const auto arg : typeArgs)
            typeArgTypes.push_back(arg->resolvedType);

        return instantiate(genericDecl, typeArgTypes);
    }

    FunctionDecl * GenericInstantiator::instantiate(const FunctionDecl &genericDecl, const std::vector<TypeId> &typeArgs)
    {
        if(typeArgs.size() != genericDecl.genericParams.size())
            return nullptr;

        size_t key = hashFuncInst(genericDecl.id, typeArgs);

        if(const auto it = m_cache.find(key); it != m_cache.end())
            return it->second;


        AstWalker walker{};
        walker.on([&](TypeSyntax& type)
        {
            auto* genericType = m_types.types().getInfo(type.resolvedType).as<GenericType>();
            if(!genericType) return;

            int index = -1;

            for(int i = 0; i < genericDecl.genericParams.size(); i++)
            {
                if(genericDecl.genericParams[i]->id == genericType->declId)
                {
                    index = i;
                    break;
                }
            }

            if(index < 0) return;

            type.resolvedType = typeArgs[index];
        });

        // TODO resolve expressions

        FunctionDecl* clone = genericDecl.clone(m_module.arena());
        clone->id = m_declAllocator.allocate();
        clone->genericParams.clear();
        walker.walk(*clone);

        m_cache.emplace(key, clone);

        return clone;
    }

    size_t GenericInstantiator::hashFuncInst(const DeclId genericId, const std::vector<TypeId>& typeArgs) const
    {
        size_t h = std::hash<DeclId>{}(genericId);
        for(const TypeId t : typeArgs)
            hashCombine(h, t);
        return h;
    }
}
