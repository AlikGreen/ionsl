#pragma once
#include "constantEvaluator.h"
#include "../ast/scopeTable.h"
#include "../ast/type.h"
#include "../ast/typeSystem.h"

namespace ionsl
{
class AliasDecl;
class ArrayTypeSyntax;
class NamedTypeSyntax;

class TypeResolver
{
public:
    TypeResolver(TypeSystem& typeSystem, ConstantEvaluator& evaluator, SymbolTable& symbols, ScopeTable& scopeTable, DeclTable& decls);

    TypeId resolveType(TypeSyntax& syntax, ScopeId scope, std::unordered_map<DeclId, TypeId>& typeSubstitutions);
private:
    TypeSystem& m_typeSystem;
    ConstantEvaluator& m_evaluator;
    SymbolTable& m_symbols;
    ScopeTable& m_scopeTable;
    DeclTable& m_decls;

    TypeId resolveVectorType(NamedTypeSyntax& syntax, ScopeId scope, std::unordered_map<DeclId, TypeId>& typeSubstitutions);
    TypeId resolveMatrixType(NamedTypeSyntax& syntax, ScopeId scope, std::unordered_map<DeclId, TypeId>& typeSubstitutions);
    TypeId resolveNamedType(NamedTypeSyntax& syntax, ScopeId scope, std::unordered_map<DeclId, TypeId>& typeSubstitutions);
    TypeId resolveAliasType(NamedTypeSyntax& syntax, const AliasDecl& alias, std::unordered_map<DeclId, TypeId>& typeSubstitutions);
    TypeId resolveArrayType(ArrayTypeSyntax& syntax, ScopeId scope, std::unordered_map<DeclId, TypeId>& typeSubstitutions);

    static PrimitiveKind toPrimitiveKind(const std::string &name);
};
}
