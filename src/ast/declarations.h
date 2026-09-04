#pragma once

#include "astNode.h"
#include "attribute.h"
#include "decl.h"
#include "type.h"

#include "../common/arena.h"

namespace ionsl
{
class TypeSyntax;

class Expression;
class BlockStmt;
class  Declaration : public AstNode
{
public:
    std::vector<Attribute> attributes{};
    DeclId id{};

    Declaration* clone(Arena& arena) const override = 0;
};

class ValueDecl  final : public Declaration
{
public:
    SymbolId name{};
    TypeSyntax* type{};
    TypeId resolvedType = TypeIdInvalid;
    Expression* initializer{};
    // TODO modifiers eg mutable

    ValueDecl* clone(Arena &arena) const override;
};

class FunctionDecl final : public Declaration
{
public:
    SymbolId name;
    TypeSyntax* returnType;
    TypeId resolvedReturnType;
    std::vector<ValueDecl*> params;
    BlockStmt* body;

    FunctionDecl* clone(Arena &arena) const override;
};

class InterfaceDecl final : public Declaration
{
public:
    SymbolId name;
    std::vector<FunctionDecl*> methods;

    InterfaceDecl* clone(Arena &arena) const override;
};

class StructDecl final : public Declaration
{
public:
    SymbolId name;
    std::vector<TypeId> resolvedInterfaces;
    std::vector<TypeSyntax*> interfaces;

    std::vector<ValueDecl*> fields;
    std::vector<FunctionDecl*> methods;

    ValueDecl* findField(SymbolId name) const;
    StructDecl* clone(Arena &arena) const override;
};

class ErrorDecl final : public Declaration
{
public:
    ErrorDecl* clone(Arena &arena) const override;
};
}
