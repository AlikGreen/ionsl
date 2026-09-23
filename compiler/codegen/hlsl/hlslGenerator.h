#pragma once
#include "../codeGenerator.h"
#include "../codeWriter.h"
#include "../../ast/declarations.h"
#include "../../ast/expressions.h"
#include "../../ast/statements.h"

namespace ionsl
{
class HlslGenerator final : public CodeGenerator
{
public:
    explicit HlslGenerator(const Module &module, const SymbolTable& symbolTable, const TypeTable& typeTable)
        : CodeGenerator(module, symbolTable, typeTable), m_writer(symbolTable) { }

    std::string generate() override;

private:
    CodeWriter m_writer;

    void genDecl(const Declaration& decl);
    bool genFunctionDecl(const FunctionDecl& decl);
    void genStructDecl(const StructDecl& decl);
    void genInterfaceDecl(const InterfaceDecl& decl);
    void genVarDecl(const ValueDecl& decl);
    void genEnumDecl(const EnumDecl& decl);

    void genStmt(Statement& stmt);
    void genExprStmt(ExprStmt& stmt);
    void genBlockStmt(BlockStmt& stmt);
    void genIfStmt(IfStmt& stmt);
    void genWhileStmt(WhileStmt& stmt);
    void genForStmt(ForStmt& stmt);
    void genReturnStmt(ReturnStmt& stmt);
    void genBreakStmt(BreakStmt& stmt);
    void genContinueStmt(ContinueStmt& stmt);

    void genType(TypeId id);
    void genVectorType(VectorType& type);
    void genMatrixType(MatrixType& type);
    void genPrimitiveType(PrimitiveKind kind);
    void genStructType(StructType type);

    void genExpr(Expression& expr, bool addParens = false);
    void genBinaryExpr(const BinaryExpr& expr, bool addParens = false);
    void genUnaryExpr(UnaryExpr& expr);
    void genCallExpr(const CallExpr& expr);
    void genConstructExpr(const ConstructExpr& expr);
    void genConversionExpr(const ConversionExpr& expr);
    void genIndexExpr(const IndexExpr& expr);
    void genLiteralExpr(LiteralExpr& expr);
    void genFieldAccessExpr(FieldAccessExpr& expr);
    void genIdentifierExpr(IdentifierExpr& expr);

    std::string opToString(BinaryOp op);
    std::string opToString(UnaryOp op);
    std::string constantIntToString(ConstantInt op);
    bool isPostfixOp(UnaryOp op);
    bool isAssignment(BinaryOp op);
};
}
