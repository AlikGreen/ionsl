#pragma once
#include "genericInstantiator.h"
#include "semaContext.h"
#include "typeResolver.h"
#include "../ast/declarations.h"

namespace ionsl
{
class SignatureResolutionPass
{
public:
    SignatureResolutionPass(TypeResolver& typeResolver, std::vector<Declaration*>& declarations);
    void run(const SemaContext& ctx);
private:
    TypeResolver& m_typeResolver;
    std::vector<Declaration*>& m_declarations;

    void checkDeclaration(Declaration& declaration, const SemaContext& ctx);

    void checkFunctionDecl(const FunctionDecl& declaration, const SemaContext& ctx);
    void checkStructDecl(const StructDecl& declaration, const SemaContext& ctx);
    void checkInterfaceDecl(const InterfaceDecl& declaration, const SemaContext& ctx);
};
}
