#pragma once

#include "semaContext.h"
#include "typeResolver.h"
#include "../ast/declarations.h"

namespace ionsl
{


class SignatureResolutionPass
{
public:
    explicit SignatureResolutionPass(SemanticAnalyzer& analyzer);
    void run(const SemaContext& ctx);
private:
    TypeResolver& m_typeResolver;
    SemanticAnalyzer& m_analyzer;
    std::vector<Declaration*>& m_declarations;

    void checkDeclaration(Declaration& declaration, const SemaContext& ctx);

    void checkFunctionDecl(const FunctionDecl& declaration, const SemaContext& ctx);
    void checkStructDecl(const StructDecl& declaration, const SemaContext& ctx);
    void checkInterfaceDecl(const InterfaceDecl& declaration, const SemaContext& ctx);
};
}
