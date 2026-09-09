#pragma once

#include "astNode.h"
#include "attribute.h"
#include "decl.h"
#include "scopeTable.h"
#include "type.h"
#include "typeSyntax.h"

#include "../common/arena.h"

namespace ionsl
{
    class GenericParam;
    class TypeSyntax;
class TypeArgument;

class Expression;
class BlockStmt;
class  Declaration : public AstNode
{
public:
    std::vector<Attribute> attributes{};
    DeclId id{};
    SymbolId name{};

    Declaration* clone(Arena& arena) const override = 0;
};

class ValueDecl  final : public Declaration
{
public:
    TypeSyntax* type{};
    Expression* initializer{};
    // TODO modifiers eg mutable

    ValueDecl* clone(Arena &arena) const override;
};

class FunctionDecl final : public Declaration
{
public:
    TypeSyntax* returnType;
    std::vector<ValueDecl*> params;
    BlockStmt* body;

    FunctionDecl* clone(Arena &arena) const override;
};

class InterfaceDecl final : public Declaration
{
public:
    std::vector<FunctionDecl*> methods;

    InterfaceDecl* clone(Arena &arena) const override;
};

class StructDecl final : public Declaration
{
public:
    std::vector<TypeId> resolvedInterfaces;
    std::vector<TypeSyntax*> interfaces;

    std::vector<ValueDecl*> fields;
    std::vector<FunctionDecl*> methods;

    [[nodiscard]] ValueDecl* findField(SymbolId name) const;
    StructDecl* clone(Arena &arena) const override;
};


// e.g. type vec3<T> = vector<T, 3>
class AliasDecl final : public Declaration
{
public:
    std::vector<GenericParam*> genericParams;
    TypeSyntax* targetType = nullptr;

    AliasDecl* clone(Arena &arena) const override;
};

class ErrorDecl final : public Declaration
{
public:
    ErrorDecl* clone(Arena &arena) const override;
};

class GenericParam : public Declaration
{
public:
    GenericParam* clone(Arena& arena) const override = 0;
};

class TypeGenericParam final : public GenericParam
{
public:
    TypeGenericParam* clone(Arena& arena) const override;
};

class ValueGenericParam final : public GenericParam
{
public:
    TypeSyntax* type{};
    TypeId resolvedType = TypeId::Error;

    ValueGenericParam* clone(Arena& arena) const override;
};
}
