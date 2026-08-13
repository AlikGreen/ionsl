#pragma once
#include <sstream>

#include "declTable.h"
#include "parser.h"

namespace ionsl
{
class CodeGen
{
public:
    explicit CodeGen(Module &module);
    std::string generate();

    static std::string generate(Module& module);
private:
    DeclTable m_declTable;
    const std::vector<DeclNode>& m_declNodes;
    std::ostringstream m_source;
    uint32_t m_indent{};

    std::unordered_map<char, uint32_t> m_registerCounters;

    void genTrivia(const std::vector<Trivia> &trivias);

    void genDecl(const DeclNode& decl);
    void genFunctionDecl(const FunctionDecl& decl);
    void genVarDecl(const VarDecl& decl);
    void genStructDecl(const StructDecl& decl);

    void genConstantBufferDecl(const ParamDecl& decl);
    void genStorageBufferDecl(const ParamDecl& decl);

    void genForwardDecl(const DeclNode& decl);
    void genFunctionSignature(const FunctionDecl& decl, bool genResources = true);
    void genStructSignature(const StructDecl& decl);

    void genExpr(const ExprNode& expr, bool topLevel = false);
    void genBinaryExpr(const BinaryExpr& expr, bool topLevel = false);
    void genIdentifierExpr(const IdentifierExpr& expr);
    void genIndexExpr(const IndexExpr& expr);
    void genUnaryExpr(const UnaryExpr& expr);
    void genFieldAccessExpr(const FieldAccessExpr& expr);
    void genFunctionCallExpr(const FunctionCallExpr& expr);
    void genTypeExpr(const TypeExpr& expr);
    void genLiteralExpr(const Literal& expr);
    void genFloatLiteral(FloatLiteral literal);
    void genIntegerLiteral(IntegerLiteral literal);

    void genStmt(const StmtNode& stmt);
    void genBlockStmt(const BlockStmt& block);
    void genForStmt(const ForStmt& stmt);
    void genWhileStmt(const WhileStmt& stmt);
    void genIfStmt(const IfStmt& stmt);
    void genReturnStmt(const ReturnStmt& stmt);
    void genBreakStmt(const BreakStmt& stmt);
    void genContinueStmt(const ContinueStmt& stmt);
    void genExprStmt(const ExprStmt& stmt);

    void genType(const Type& type);
    void genPrimitiveType(const PrimitiveKind& type);
    void genCustomType(const CustomType& type);
    void genStructType(const StructType& type);
    void genMatrixType(const MatrixType& type);
    void genVectorType(const VectorType &type);

    void genAttribute(const std::vector<Attribute>& attributes);
    static bool hasAttribute(const std::vector<Attribute>& attributes, const std::string& name);

    void newLine();

    static std::string opToString(BinaryOp op);
    static std::string opToString(UnaryOp op);
    static bool isPostfixOp(UnaryOp op);
};
}
