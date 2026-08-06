#include "dependencyAnalyzer.h"

namespace ionsl
{
    Module DependencyAnalyzer::collectDependencies(const FunctionDecl &entry)
    {
        m_visitedNames.clear();
        m_closure.clear();
        visitByName(entry.name);

        Module module{
            m_module.path,
            m_closure,
            m_module.diagnostics
        };

        return module;
    }

    void DependencyAnalyzer::visitByName(const std::string &name)
    {
        if (m_visitedNames.contains(name)) return;
        m_visitedNames.insert(name);

        const DeclNode* decl = findDeclNode(name);
        if (!decl) return;
        m_closure.push_back(*decl);

        AstWalker walker;
        walker.onExpr = [this](const ExprNode& e)
        {
            if (auto* call = std::get_if<FunctionCallExpr>(&e.expr))
                if (auto* callee = std::get_if<IdentifierExpr>(&call->callee->expr))
                    visitByName(callee->name);
        };

        walker.onType = [this](const Type& t)
        {
            if (auto* s = std::get_if<StructType>(&t.kind))
                visitByName(s->decl->name);
        };

        walker.walk(*decl);
    }

    const DeclNode * DependencyAnalyzer::findDeclNode(const std::string &name)
    {
        for(const auto& decl : m_module.decls)
        {
            bool sameName = std::visit(
            [this, name]<typename T0>(T0&& d) -> bool
            {
                using T = std::decay_t<T0>;

                if constexpr (std::is_same_v<T, FunctionDecl>)
                {
                    if(d.name == name)
                        return true;
                }
                else if constexpr (std::is_same_v<T, StructDecl>)
                {
                    if(d.name == name)
                        return true;
                }
                else if constexpr (std::is_same_v<T, VarDecl>)
                {
                    if(d.name == name)
                        return true;
                }

                return false;
            },
            decl.decl);

            if(sameName)
                return &decl;
        }

        return nullptr;
    }
}
