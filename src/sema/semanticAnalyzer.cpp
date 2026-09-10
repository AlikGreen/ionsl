#include "semanticAnalyzer.h"

#include <iostream>
#include <ranges>

#include "constantEvaluator.h"
#include "signatureResolutionPass.h"
#include "../ast/declarations.h"
#include "../ast/module.h"

namespace ionsl
{
    void SemanticAnalyzer::analyze()
    {
        SemaContext ctx{};
        ctx.scope = m_module.scope;

        SignatureResolutionPass(m_typeResolver, m_module.declarations).run(ctx);

        for(const auto decl : m_module.declarations)
        {
            checkDeclaration(*decl, ctx);
        }
    }

    void SemanticAnalyzer::analyze(Module& module, SymbolTable& symbolTable, TypeSystem& typeSystem, DeclTable& declTable, ScopeTable& scopeTable)
    {
        return SemanticAnalyzer(module, symbolTable, typeSystem, declTable, scopeTable).analyze();
    }

    TypeId SemanticAnalyzer::checkExpression(Expression*& expression, const SemaContext& ctx)
    {
        if(auto* binaryExpr = expression->as<BinaryExpr>())
            return checkBinaryExpr(*binaryExpr, ctx);
        if(auto* unaryExpr = expression->as<UnaryExpr>())
            return checkUnaryExpr(*unaryExpr, ctx);
        if(auto* callExpr = expression->as<CallExpr>())
            return checkCallExpr(expression, *callExpr, ctx);
        if(auto* identifierExpr = expression->as<IdentifierExpr>())
            return checkIdentifierExpr(*identifierExpr, ctx);
        if(auto* indexExpr = expression->as<IndexExpr>())
            return checkIndexExpr(*indexExpr, ctx);
        if(auto* literalExpr = expression->as<LiteralExpr>())
            return checkLiteralExpr(*literalExpr);
        if(auto* fieldAccessExpr = expression->as<FieldAccessExpr>())
            return checkFieldAccessExpr(*fieldAccessExpr, ctx);

        return TypeId::Error;
    }

    TypeId SemanticAnalyzer::checkBinaryExpr(BinaryExpr &expression, const SemaContext& ctx)
    {
        const auto leftType = checkExpression(expression.left, ctx);
        const auto rightType = checkExpression(expression.right, ctx);

        if(leftType == TypeId::Error || rightType == TypeId::Error) return TypeId::Error;

        const auto result = m_typeSystem.resolveBinaryTypes(expression.op, leftType, rightType);
        if(!result) return TypeId::Error; // TODO diagnostics

        expression.left = makeConversion(expression.left, result->leftType);
        expression.right = makeConversion(expression.right, result->rightType);

        expression.resultType = result->resultType;

        return expression.resultType;
    }

    TypeId SemanticAnalyzer::checkUnaryExpr(UnaryExpr &expression, const SemaContext& ctx)
    {
        const auto operandType = checkExpression(expression.operand, ctx);

        if(operandType == TypeId::Error) return TypeId::Error;

        const auto result = m_typeSystem.resolveUnaryType(expression.op, operandType);
        if(!result) return TypeId::Error;

        expression.operand = makeConversion(expression.operand, result->operandType);

        expression.resultType = result->resultType;

        return expression.resultType;
    }

    TypeId SemanticAnalyzer::checkCallExpr(Expression*& slot, CallExpr &expression, const SemaContext& ctx)
    {
        auto* identifier = expression.callee->as<IdentifierExpr>();
        if(!identifier)
            return TypeId::Error; // TODO check non identifier call

        auto* typeSyntax = m_module.arena.create<NamedTypeSyntax>();
        typeSyntax->name = identifier->name;
        typeSyntax->span = identifier->span;
        typeSyntax->arguments = expression.genericArgs;

        for(const auto arg : expression.genericArgs)
            m_typeResolver.resolveTypeArg(*arg, ctx);

        if(m_typeResolver.resolveType(*typeSyntax, ctx) != TypeId::Error)
        {
            auto* construct = m_module.arena.create<ConstructExpr>();
            construct->type = typeSyntax;
            construct->args = expression.args;
            slot = construct;
            return typeSyntax->resolvedType;
        }

        return checkIdentifierCall(expression, *identifier, ctx);
    }

    TypeId SemanticAnalyzer::checkIdentifierCall(CallExpr &expression, const IdentifierExpr &identifier, const SemaContext& ctx)
    {
        // TODO check non identifier calls eg
        // var func = () -> { return 2; };
        // var x = func()

        std::vector<TypeId> argumentTypes;

        for(auto& argument : expression.args)
        {
            TypeId type = checkExpression(argument, ctx);

            if(type == TypeId::Error)
                return TypeId::Error;

            argumentTypes.push_back(type);
        }

        auto candidates = m_scopeTable.findDecls(ctx.scope, identifier.name);
        uint32_t bestConversionCost = ~0u;
        Declaration* bestCandidate = nullptr;

        for(const auto candidate : candidates)
        {
            Declaration* decl = m_declTable.get(candidate);

            if(auto* funcDecl = decl->as<FunctionDecl>())
            {
                std::vector<TypeId> paramTypes;
                for(const auto* param : funcDecl->params)
                    paramTypes.push_back(param->type->resolvedType);

                auto conversion = m_typeSystem.conversionCost(argumentTypes, paramTypes);
                if(!conversion)
                {
                    continue;
                }

                if(bestConversionCost > *conversion)
                {
                    bestConversionCost = *conversion;
                    bestCandidate = decl;
                }
            }
        }

        if(bestCandidate == nullptr)
        {
            m_module.diagnostics.error(expression.span, "no matching function for call to '{}'", identifier.name.string(m_symbols));
            return TypeId::Error;
        }

        if(auto* funcDecl = bestCandidate->as<FunctionDecl>())
        {
            for(const auto& [arg, param] : std::views::zip(expression.args, funcDecl->params))
            {
                arg = makeConversion(arg, param->type->resolvedType);
            }

            return funcDecl->returnType->resolvedType;
        }


        // TODO methods and variables

        m_module.diagnostics.error(expression.span, "no matching function for call to '{}'", identifier.name.string(m_symbols));

        return TypeId::Error;
    }

    TypeId SemanticAnalyzer::checkIdentifierExpr(IdentifierExpr &expression, const SemaContext& ctx) const
    {
        auto candidates = m_scopeTable.findDecls(ctx.scope, expression.name);

        TypeId bestCandidateType  = TypeId::Error;

        for(const auto& id : candidates)
        {
            const auto decl = m_declTable.get(id);
            if(const auto value = decl->as<ValueDecl>())
                bestCandidateType = value->type->resolvedType;
            if(const auto struc = decl->as<StructDecl>())
                bestCandidateType = m_typeSystem.types().getStructType(struc->id);
            if(const auto interface = decl->as<InterfaceDecl>())
                bestCandidateType = m_typeSystem.types().getInterfaceType(interface->id);
            // TODO other decl types
        }

        // TODO diagnostics if no candidates

        return expression.resultType = bestCandidateType;
    }

    TypeId SemanticAnalyzer::checkIndexExpr(IndexExpr &expression, const SemaContext& ctx)
    {
        checkExpression(expression.index, ctx);
        const TypeId arrayTypeId = checkExpression(expression.array, ctx);

        TypeInfo arrayType = m_typeSystem.types().getInfo(arrayTypeId);

        // TODO validation
        return expression.resultType = arrayType.as<ArrayType>()->elementType;
    }

    TypeId SemanticAnalyzer::checkLiteralExpr(LiteralExpr &expression) const
    {
        const auto typeId = std::visit([](auto&& arg) -> TypeId
        {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, bool>)
                return TypeId::Bool;
            if constexpr (std::is_same_v<T, uint64_t>)
                return TypeId::U64;
            if constexpr (std::is_same_v<T, int64_t>)
                return TypeId::I64;
            if constexpr (std::is_same_v<T, double>)
                return TypeId::F64;
            if constexpr (std::is_same_v<T, std::string>)
                return TypeId::String;

            return TypeId::Error;

        }, expression.literal);

        expression.resultType = typeId;
        return typeId;
    }

    TypeId SemanticAnalyzer::checkFieldAccessExpr(FieldAccessExpr &expression, const SemaContext& ctx)
    {
        const TypeId objTypeId = checkExpression(expression.object, ctx);

        if(objTypeId == TypeId::Error)
            return TypeId::Error;

        TypeInfo objType = m_typeSystem.types().getInfo(objTypeId);
        // TODO validation and interfaces
        const DeclId structDeclId = objType.as<StructType>()->declId;
        const StructDecl& structDecl = *m_declTable.get(structDeclId)->as<StructDecl>();
        const ValueDecl& field = *structDecl.findField(expression.memberName);
        expression.resultType = field.type->resolvedType;
        return expression.resultType;
    }

    void SemanticAnalyzer::checkStatement(Statement &statement, const SemaContext& ctx)
    {
        if(const auto exprStmt = statement.as<ExprStmt>())
            checkExpression(exprStmt->expr, ctx);
        if(const auto declStmt = statement.as<DeclStmt>())
            checkDeclaration(*declStmt->decl, ctx);
        if(const auto blockStmt = statement.as<BlockStmt>())
            checkBlockStmt(*blockStmt, ctx);
        if(const auto ifStmt = statement.as<IfStmt>())
            checkIfStmt(*ifStmt, ctx);
        if(const auto whileStmt = statement.as<WhileStmt>())
            checkWhileStmt(*whileStmt, ctx);
        if(const auto forStmt = statement.as<ForStmt>())
            checkForStmt(*forStmt, ctx);
        if(const auto returnStmt = statement.as<ReturnStmt>())
            checkReturnStmt(*returnStmt, ctx);
        if(statement.is<BreakStmt>() || statement.is<ContinueStmt>())
            checkBreakContinueStmt();
    }

    void SemanticAnalyzer::checkBlockStmt(const BlockStmt &statement, const SemaContext& ctx)
    {
        SemaContext scopeCtx = ctx.withScope(statement.scope);

        for(const auto stmt : statement.statements)
        {
            checkStatement(*stmt, scopeCtx);
        }
    }

    void SemanticAnalyzer::checkIfStmt(IfStmt &statement, const SemaContext& ctx)
    {
        const TypeId conditionType = checkExpression(statement.condition, ctx);
        if(conditionType != TypeId::Bool)
        {
            m_module.diagnostics.add("condition in an if statement must resolve to a bool", statement.condition->span, Severity::Error);
            return;
        }

        checkBlockStmt(*statement.thenBranch, ctx);

        if(statement.elseBranch)
            checkBlockStmt(*statement.elseBranch, ctx);
    }

    void SemanticAnalyzer::checkForStmt(ForStmt &statement, const SemaContext& ctx)
    {
        checkStatement(*statement.init, ctx);
        checkExpression(statement.condition, ctx);
        checkExpression(statement.increment, ctx);
        checkBlockStmt(*statement.body, ctx);
    }

    void SemanticAnalyzer::checkWhileStmt(WhileStmt &statement, const SemaContext& ctx)
    {
        checkExpression(statement.condition, ctx);
        checkBlockStmt(*statement.body, ctx);
    }

    void SemanticAnalyzer::checkReturnStmt(ReturnStmt &statement, const SemaContext& ctx)
    {
        TypeId returnType = TypeId::Void;

        if(statement.expr)
            returnType = checkExpression(statement.expr, ctx);

        // TODO check return type against function type
    }

    void SemanticAnalyzer::checkBreakContinueStmt()
    {
        // TODO make sure in for or while loop
    }

    void SemanticAnalyzer::checkDeclaration(Declaration &declaration, const SemaContext& ctx)
    {
        if(const auto funcDecl = declaration.as<FunctionDecl>())
            checkFunctionDecl(*funcDecl, ctx);
        if(const auto interfaceDecl = declaration.as<InterfaceDecl>())
            checkInterfaceDecl(*interfaceDecl, ctx);
        if(const auto structDecl = declaration.as<StructDecl>())
            checkStructDecl(*structDecl, ctx);
        if(const auto valDecl = declaration.as<ValueDecl>())
            checkValueDecl(*valDecl, ctx);
    }

    void SemanticAnalyzer::checkFunctionDecl(const FunctionDecl &declaration, const SemaContext& ctx)
    {
        if(declaration.body)
            checkBlockStmt(*declaration.body, ctx);
    }

    void SemanticAnalyzer::checkStructDecl(const StructDecl &declaration, const SemaContext& ctx)
    {
        for(const auto method : declaration.methods)
            checkFunctionDecl(*method, ctx);

        for(const auto field : declaration.fields)
            checkValueDecl(*field, ctx);
    }

    void SemanticAnalyzer::checkInterfaceDecl(const InterfaceDecl &declaration, const SemaContext& ctx)
    {
        for(const auto method : declaration.methods)
            checkFunctionDecl(*method, ctx);
    }

    void SemanticAnalyzer::checkValueDecl(ValueDecl &declaration, const SemaContext& ctx)
    {
        m_typeResolver.resolveType(*declaration.type, ctx);

        if(declaration.initializer)
            checkExpression(declaration.initializer, ctx);
    }


    Expression* SemanticAnalyzer::makeConversion(Expression* operand, const TypeId type) const
    {
        if(operand->resultType == type)
            return operand;

        auto* expr = m_module.arena.create<ConversionExpr>();
        expr->kind = ConversionKind::Implicit;
        expr->targetType = type;
        expr->operand = operand;
        return expr;
    }
}
