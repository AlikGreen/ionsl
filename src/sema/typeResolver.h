#pragma once
#include "constantEvaluator.h"
#include "../ast/scopeTable.h"
#include "../ast/type.h"
#include "../ast/typeSystem.h"

namespace ionsl
{
struct SemaContext;
class AliasDecl;
class ArrayTypeSyntax;
class NamedTypeSyntax;

class TypeResolver
{
public:
    TypeResolver(TypeSystem& typeSystem, ConstantEvaluator& evaluator, SymbolTable& symbols, ScopeTable& scopeTable, DeclTable& decls);

    TypeId resolveType(TypeSyntax& syntax, const SemaContext& ctx);
    TypeId resolveTypeArg(TypeArgument& arg, const SemaContext& ctx);
private:
    TypeSystem& m_typeSystem;
    ConstantEvaluator& m_evaluator;
    SymbolTable& m_symbols;
    ScopeTable& m_scopeTable;
    DeclTable& m_decls;

    TypeId resolveVectorType(NamedTypeSyntax& syntax, const SemaContext& ctx);
    TypeId resolveMatrixType(NamedTypeSyntax& syntax, const SemaContext& ctx);
    TypeId resolveNamedType(NamedTypeSyntax& syntax, const SemaContext& ctx);
    TypeId resolveAliasType(NamedTypeSyntax& syntax, const AliasDecl& alias, const SemaContext& ctx);
    TypeId resolveArrayType(ArrayTypeSyntax& syntax, const SemaContext& ctx);

    static PrimitiveKind toPrimitiveKind(const std::string &name);
};
}
