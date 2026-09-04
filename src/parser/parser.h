#pragma once
#include <span>
#include <unordered_set>

#include "../ast/module.h"
#include "../ast/declarations.h"
#include "../ast/expressions.h"
#include "../ast/statements.h"
#include "../ast/typeSyntax.h"
#include "../lexer/token.h"

namespace ionsl
{
struct ParserState
{
    size_t m_tokenIndex;
    size_t m_diagnosticCount;
};

class Parser
{
public:
    explicit Parser(std::span<Token> tokens, DeclarationIdAllocator& declAllocator, SymbolTable& symbolTable, ScopeTable& scopeTable, DeclTable& declTable);
    static Module parse(std::span<Token> tokens, DeclarationIdAllocator& declAllocator, SymbolTable& symbolTable, ScopeTable& scopeTable, DeclTable& declTable);

    Module parse();
private:
    std::span<Token> m_tokens{};
    uint32_t m_pos{};
    Module m_ast{10*1024*1024}; // 10Mb
    DeclarationIdAllocator& m_declAllocator;

    SymbolTable& m_symbolTable;
    ScopeTable& m_scopeTable;
    DeclTable& m_declTable;

    ScopeId m_currentScope = 0;
    std::vector<Attribute> m_pendingAttributes;

    Declaration* parseDeclaration();
    FunctionDecl* parseFunctionDecl();
    StructDecl* parseStructDecl();
    InterfaceDecl* parseInterfaceDecl();
    ValueDecl* parseVarDecl();
    ValueDecl* parseValueDecl();

    Statement* parseStatement();
    BlockStmt* parseBlockStmt();
    WhileStmt* parseWhileStmt();
    ForStmt* parseForStmt();
    IfStmt* parseIfStmt();
    ReturnStmt* parseReturnStmt();
    BreakStmt* parseBreakStmt();
    ContinueStmt* parseContinueStmt();
    DeclStmt* parseDeclStmt();
    ExprStmt* parseExprStmt();


    Expression* parseExpression(uint32_t minBindingPower = 0, const std::unordered_set<TokenKind>& stopTokens = {});
    Expression* parseInfixExpr(Expression* left, TokenKind opKind);
    Expression* parsePrefixExpr();
    Expression* makeUnaryPrefixExpr(const Token &operatorToken, Expression *operand);

    LiteralExpr* parseLiteralExpr();
    IdentifierExpr* parseIdentifierExpr();

    TypeSyntax* parseType();
    NamedTypeSyntax* parseNamedType(QualifiedName name, const SourceSpan &start);
    ArrayTypeSyntax* parseArrayType(TypeSyntax* elementType, const SourceSpan &start);

    TypeArgument* parseGenericArg();
    std::vector<TypeArgument*> parseGenericArgs();

    NamedTypeSyntax* createVoidType(const SourceSpan &span);

    [[nodiscard]] bool startsUnambiguousConst() const;

    QualifiedName parseName();
    LiteralValue parseLiteral();

    void parseAttributes();
    std::vector<Attribute> takeAttributes();
    AttributeArg parseAttribArg();

    static std::optional<std::pair<uint32_t, uint32_t>> getBindingPower(TokenKind kind);

    template<typename T, typename... Args>
    requires std::is_constructible_v<T, Args...>
    T* create(Args&&... args)
    {
        return m_ast.arena.create<T>(std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    requires std::is_constructible_v<T, Args...> && std::is_base_of_v<Declaration ,T>
    T* createDecl(Args&&... args)
    {
        T* decl = m_ast.arena.create<T>(std::forward<Args>(args)...);
        static_cast<Declaration*>(decl)->id = m_declAllocator.allocate();
        return decl;
    }

    void reportError(const SourceSpan& span, const std::string &message);

    [[nodiscard]] ParserState saveState() const;
    void restoreState(const ParserState& state);

    [[nodiscard]] bool atEnd() const;

    [[nodiscard]] const Token& peek() const;
    [[nodiscard]] const Token& previous() const;
    const Token& advance();

    [[nodiscard]] bool check(TokenKind kind) const;
    bool match(TokenKind kind);
    bool expect(TokenKind kind);
    const Token& consume(TokenKind kind, std::string_view message = "");
};
}
