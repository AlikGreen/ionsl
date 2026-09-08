#pragma once
#include "../ast/declarations.h"

namespace ionsl
{
class SignatureResolutionPass
{
public:
private:
    void checkDeclaration(Declaration& declaration);

    void checkFunctionDecl(FunctionDecl& declaration);
    void checkStructDecl(const StructDecl& declaration);
    void checkInterfaceDecl(const InterfaceDecl& declaration);
};
}
