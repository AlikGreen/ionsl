#include "signatureResolutionPass.h"

#include "../ast/statements.h"

namespace ionsl
{
    SignatureResolutionPass::SignatureResolutionPass(TypeResolver &typeResolver, std::vector<Declaration*>& declarations)
        : m_typeResolver(typeResolver), m_declarations(declarations)
    {
    }

    void SignatureResolutionPass::run(const SemaContext& ctx)
    {
        for(const auto decl : m_declarations)
        {
            checkDeclaration(*decl, ctx);
        }
    }

    void SignatureResolutionPass::checkDeclaration(Declaration &declaration, const SemaContext& ctx)
    {
        if(const auto funcDecl = declaration.as<FunctionDecl>())
            checkFunctionDecl(*funcDecl, ctx);
        if(const auto structDecl = declaration.as<StructDecl>())
            checkStructDecl(*structDecl, ctx);
        if(const auto interfaceDecl = declaration.as<InterfaceDecl>())
            checkInterfaceDecl(*interfaceDecl, ctx);
        // if(const auto aliasDecl = declaration.as<AliasDecl>())
        //     checkAliasDecl(*aliasDecl);
    }

    void SignatureResolutionPass::checkFunctionDecl(const FunctionDecl &declaration, const SemaContext& ctx)
    {
        m_typeResolver.resolveType(*declaration.returnType, ctx);

        for(const auto param : declaration.params)
            m_typeResolver.resolveType(*param->type, ctx);
    }

    void SignatureResolutionPass::checkStructDecl(const StructDecl &declaration, const SemaContext& ctx)
    {
        for(const auto field : declaration.fields)
            m_typeResolver.resolveType(*field->type, ctx);

        for(const auto method : declaration.methods)
            checkFunctionDecl(*method, ctx);
    }

    void SignatureResolutionPass::checkInterfaceDecl(const InterfaceDecl &declaration, const SemaContext& ctx)
    {
        for(const auto method : declaration.methods)
            checkFunctionDecl(*method, ctx);
    }
}
