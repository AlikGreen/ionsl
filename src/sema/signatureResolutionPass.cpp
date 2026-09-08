#include "signatureResolutionPass.h"

namespace ionsl
{
    void SignatureResolutionPass::checkDeclaration(Declaration &declaration)
    {
        if(const auto funcDecl = declaration.as<FunctionDecl>())
            checkFunctionDecl(*funcDecl);
        if(const auto structDecl = declaration.as<StructDecl>())
            checkStructDecl(*structDecl);
        if(const auto interfaceDecl = declaration.as<InterfaceDecl>())
            checkInterfaceDecl(*interfaceDecl);
        // if(const auto aliasDecl = declaration.as<AliasDecl>())
        //     checkAliasDecl(*aliasDecl);
    }

    void SignatureResolutionPass::checkFunctionDecl(FunctionDecl &declaration)
    {
        resolveType(*declaration.returnType);

        for(const auto param : declaration.params)
            checkValueDecl(*param);
    }

    void SignatureResolutionPass::checkStructDecl(const StructDecl &declaration)
    {
        for(const auto field : declaration.fields)
            checkValueDecl(*field);

        for(const auto method : declaration.methods)
            checkFunctionDecl(*method);
    }

    void SignatureResolutionPass::checkInterfaceDecl(const InterfaceDecl &declaration)
    {
        for(const auto method : declaration.methods)
            checkFunctionDecl(*method);
    }
}
