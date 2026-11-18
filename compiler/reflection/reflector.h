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
    refl::ReflectionData reflect();
private:
    const SymbolTable& m_symbols;
    const TypeTable& m_types;
    const DeclTable m_decls;
    const Module& m_module;

    std::unordered_map<DeclId, ShaderStage> m_stageMap;

    void reflectFunctionDecl(const FunctionDecl& declaration, refl::ReflectionData& data);
    void reflectResourceDecl(const ResourceDecl& declaration, refl::ReflectionData &data);

    refl::TypeRef reflectType(TypeId idl, uint32_t* location = nullptr);
    refl::TypeRef reflectPrimitiveType(const PrimitiveType& type);
    refl::TypeRef reflectStructType(const StructType& type, uint32_t* location);
    refl::TypeRef reflectArrayType(const ArrayType& type);
    refl::TypeRef reflectMatrixType(const MatrixType& type);
    refl::TypeRef reflectVectorType(const VectorType& type);
    refl::TypeRef reflectEnumType(const EnumType& type);

    refl::Attributes reflectAttributes(const Attributes& attributes);
    refl::Attribute reflectAttribute(const Attribute& attribute);

    static ShaderStage convertStage(const std::string &name);
    static refl::BufferKind convertBufferKind(ResourceKind kind);
    static uint32_t primitiveSize(PrimitiveKind kind);
};
}
