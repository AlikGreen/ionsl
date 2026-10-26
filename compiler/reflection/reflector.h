#pragma once

#include "reflection.h"
#include "../ast/declarations.h"
#include "../ast/statements.h"
#include "../ast/expressions.h"
#include "../ast/typeTable.h"

namespace ionsl
{
class Reflector
{
public:
    Reflector(const SymbolTable &symbols, const TypeTable &types, const Module &module);
    ShaderReflection reflect();
private:
    const SymbolTable& m_symbols;
    const TypeTable& m_types;
    const DeclTable m_decls;
    const Module& m_module;

    void reflectDeclaration(const Declaration& declaration, ShaderReflection& data);
    void reflectFunctionDecl(const FunctionDecl& declaration, ShaderReflection& data);
    void reflectResourceDecl(const ResourceDecl& declaration, ShaderReflection &data);

    ReflectedType reflectType(TypeId id);
    ReflectedType reflectPrimitiveType(const PrimitiveType& type);
    ReflectedType reflectStructType(const StructType& type);
    ReflectedType reflectArrayType(const ArrayType& type);
    ReflectedType reflectMatrixType(const MatrixType& type);
    ReflectedType reflectVectorType(const VectorType& type);
    ReflectedType reflectEnumType(const EnumType& type);

    ReflectedAttributes reflectAttributes(const Attributes& attributes);
    ReflectedAttribute reflectAttribute(const Attribute& attribute);

    static ShaderStage convertStage(const std::string &name);
    static uint32_t primitiveSize(PrimitiveKind kind);
};
}
