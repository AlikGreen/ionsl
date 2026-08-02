#pragma once
#include <optional>
#include <span>
#include <unordered_set>
#include <variant>
#include <vector>

#include "ast.h"
#include "box.h"
#include "diagnostics.h"
#include "lexer.h"
#include "module.h"
#include "types.h"

namespace ionsl
{
class Parser
{
public:
    explicit Parser(std::span<Token> tokens);
    Module parse();

    static Module parse(std::span<Token> tokens);
private:
    std::span<Token> m_tokens;
    size_t m_pos;

    DiagnosticSink m_diagnostics;

    std::vector<DeclNode> m_decls;

    std::vector<Trivia> m_pendingTrivia{};
    std::vector<Attribute> m_pendingAttributes{};

    DeclNode parseDecl();

    StructDecl parseStructDecl();
    FunctionDecl parseFunctionDecl();
    VarDecl parseVarDecl();
    InterfaceDecl parseInterfaceDecl();

    ExprNode parseExpr(uint32_t minBP = 0);
    ExprNode parseInfixExpr(ExprNode left, const Token &opToken);
    ExprNode parsePrefixExpr();
    ExprNode parseIdentifierExpr();
    ExprNode createErrorExpr();

    ExprNode makeLiteralExpr(const Token &token);
    ExprNode makeUnaryPrefixExpr(const Token& operatorToken, ExprNode operand);

    StmtNode parseStmt();
    BlockStmt parseBlockStmt();
    IfStmt parseIfStmt();
    WhileStmt parseWhileStmt();
    ForStmt parseForStmt();
    ExprStmt parseExprStmt();
    DeclStmt parseDeclStmt();
    ReturnStmt parseReturnStmt();

    static Literal parseLiteral(const Token &token);

    void consumeTriviaAndAttribs();
    void consumeAttribute();

    Type parseType();

    Type parseTypeName(const Token &startToken, const std::vector<Trivia> &leadingTrivia);
    Type parseArraySuffix(Type elementType, const Token &startToken);
    Type parseVectorType(const Token& startToken, const std::vector<Trivia>& leadingTrivia);
    Type parseMatrixType(const Token& startToken, const std::vector<Trivia>& leadingTrivia);
    Type parseResourceType(ResourceBindingKind resourceKind, const Token& startToken, const std::vector<Trivia>& leadingTrivia, bool hasGenericArgs);
    Type parseCustomType(std::string_view baseName, const Token& startToken, const std::vector<Trivia>& leadingTrivia, bool hasGenericArgs);

    Type parseGenericTypeArgs(const Token &startToken, const std::vector<Trivia> &leadingTrivia);
    static Box<PrimitiveKind> parsePrimitiveKind(std::string_view name);
    static std::optional<std::pair<uint32_t, uint32_t>> getBindingPower(TokenKind kind);

    bool isLookaheadGenericArgs();

    std::vector<Trivia> takePendingTrivia();
    std::vector<Attribute> takePendingAttributes();

    [[nodiscard]] bool done() const;
    const Token& peek(uint32_t offset = 0) const;
    [[nodiscard]] const Token& previous() const;
    const Token& advance();
    [[nodiscard]] bool check(TokenKind kind) const;
    bool match(TokenKind kind);
    bool consume(TokenKind kind, std::string_view context = {}, Severity severity = Severity::Error);
};
}
