#include "semanticAnalyzer.h"

#include <iostream>
#include <ranges>
#include <unordered_set>

#include "constantEvaluator.h"
#include "../ast/declarations.h"
#include "../ast/module.h"

namespace ionsl
{
    void SemanticAnalyzer::analyze()
    {
        constexpr SemaContext ctx{};

        for(const auto decl : m_module.declarations())
        {
            checkDeclarationSignature(*decl, ctx);
        }

        for(const auto decl : m_module.declarations())
        {
            checkDeclaration(*decl, ctx);
        }
    }

    void SemanticAnalyzer::analyze(Module& module, SymbolTable& symbolTable, TypeSystem& typeSystem, ScopeTable& scopeTable, DeclAllocator& declAllocator, const std::unordered_map<DeclId, std::vector<TypeId>>& specializations)
    {
        return SemanticAnalyzer(module, symbolTable, typeSystem, scopeTable, declAllocator, specializations).analyze();
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
            return checkFieldAccessExpr(expression, *fieldAccessExpr, ctx);

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

        auto* typeSyntax = m_module.arena().create<NamedTypeSyntax>();
        typeSyntax->name = identifier->name;
        typeSyntax->span = identifier->span;
        typeSyntax->arguments = expression.genericArgs;

        for(const auto arg : expression.genericArgs)
            resolveTypeArg(*arg, ctx);

        if(resolveType(*typeSyntax, ctx) != TypeId::Error)
        {
            auto* construct = m_module.arena().create<ConstructExpr>();
            construct->type = typeSyntax;
            construct->args = expression.args;
            construct->span = expression.span;
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

        auto candidates = m_scopeTable.find(ctx.scope, identifier.name.parts[0]);
        if(candidates.empty())
            candidates = m_globalScope.find(identifier.name.parts[0]);

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
                    continue;

                if(bestConversionCost > *conversion)
                {
                    bestConversionCost = *conversion;
                    bestCandidate = decl;
                }
            }
        }

        if(bestCandidate == nullptr)
        {
            m_module.diagnostics().error(expression.span, "no matching function for call to '{}'", identifier.name.string(m_symbols));
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

        m_module.diagnostics().error(expression.span, "no matching function for call to '{}'", identifier.name.string(m_symbols));

        return TypeId::Error;
    }

    TypeId SemanticAnalyzer::checkIdentifierExpr(IdentifierExpr &expression, const SemaContext& ctx) const
    {
        auto candidates = m_scopeTable.find(ctx.scope, expression.name.parts[0]);
        if(candidates.empty())
            candidates = m_globalScope.find(expression.name.parts[0]);

        TypeId bestCandidateType = TypeId::Error;
        DeclId bestCandidateDecl = DeclId::Error;

        for(const auto& id : candidates)
        {
            const auto decl = m_declTable.get(id);
            if(const auto value = decl->as<ValueDecl>())
            {
                bestCandidateType = value->type->resolvedType;
                bestCandidateDecl = id;
            }
            if(const auto struc = decl->as<StructDecl>())
            {
                bestCandidateType = m_typeSystem.types().getStructType(struc->id);
                bestCandidateDecl = id;
            }
            if(const auto interface = decl->as<InterfaceDecl>())
            {
                bestCandidateType = m_typeSystem.types().getInterfaceType(interface->id);
                bestCandidateDecl = id;
            }
            if(const auto interface = decl->as<EnumDecl>())
            {
                bestCandidateType = m_typeSystem.types().getEnumType(interface->id);
                bestCandidateDecl = id;
            }
            // TODO other decl types
        }

        // TODO diagnostics if no candidates

        expression.decl = bestCandidateDecl;
        return expression.resultType = bestCandidateType;
    }

    TypeId SemanticAnalyzer::checkIndexExpr(IndexExpr &expression, const SemaContext& ctx)
    {
        checkExpression(expression.index, ctx);
        const TypeId arrayTypeId = checkExpression(expression.array, ctx);

        if (arrayTypeId == TypeId::Error)
            return TypeId::Error;

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
            if constexpr (std::is_same_v<T, ConstantInt>)
            {
                if (arg.kind == IntKind::Unsigned)
                    return TypeId::U64;

                return TypeId::I64;
            }
            if constexpr (std::is_same_v<T, ConstantFloat>)
            {
                if (arg.kind == FloatKind::Half)
                    return TypeId::F16;

                if (arg.kind == FloatKind::Float)
                    return TypeId::F32;

                return TypeId::F64;
            }
            if constexpr (std::is_same_v<T, std::string>)
                return TypeId::String;

            return TypeId::Error;

        }, expression.value);

        expression.resultType = typeId;
        return typeId;
    }

    TypeId SemanticAnalyzer::checkFieldAccessExpr(Expression*& slot, FieldAccessExpr &expression, const SemaContext& ctx)
    {
        const TypeId objTypeId = checkExpression(expression.object, ctx);

        if(objTypeId == TypeId::Error)
            return TypeId::Error;

        TypeInfo objType = m_typeSystem.types().getInfo(objTypeId);
        // TODO validation and interfaces
        if (const auto structType = objType.as<StructType>())
        {
            const StructDecl& structDecl = *m_declTable.get(structType->declId)->as<StructDecl>();
            // TODO methods
            const ValueDecl& field = *structDecl.findField(expression.memberName);
            expression.resultType = field.type->resolvedType;
            return expression.resultType;
        }
        if (const auto vectorType = objType.as<VectorType>())
        {
            // TODO allow for other vector operations like .length
            std::string fieldName = m_symbols.get(expression.memberName);

            std::vector<uint8_t> swizzleIndices{};
            for (const char c : fieldName)
            {
                const uint8_t index = componentIndex(c);
                if (index >= vectorType->dimension)
                    return TypeId::Error;
                swizzleIndices.push_back(index);
            }

            auto* newExpr = m_module.arena().create<SwizzleExpr>();
            newExpr->span = expression.span;
            newExpr->object = expression.object;
            newExpr->indices = swizzleIndices;
            slot = newExpr;

            if (fieldName.size() > 1)
                return m_typeSystem.types().getVectorType(vectorType->scalarType, fieldName.size());

            return m_typeSystem.types().getPrimitiveType(vectorType->scalarType);
        }

        return TypeId::Error;
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
            m_module.diagnostics().error(statement.condition->span, "condition in an if statement must resolve to a bool");
            return;
        }

        checkStatement(*statement.thenBranch, ctx);

        if(statement.elseBranch)
            checkStatement(*statement.elseBranch, ctx);
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
        for (const auto& attrib : declaration.attributes.attributes())
        {
            // TODO check attribute
        }

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
        SemaContext newCtx = ctx;
        std::unordered_map<DeclId, TypeId> subs;
        if(!declaration.genericParams.empty())
        {
            if(const auto it = m_specializations.find(declaration.id); it != m_specializations.end())
            {
                for(const auto [param, typeId] : std::ranges::zip_view(declaration.genericParams, it->second))
                    subs[param->id] = typeId;

                newCtx = ctx.forGenericDecl(declaration.genericParams, subs);
            }
        }

        if(declaration.body)
            checkBlockStmt(*declaration.body, newCtx);

        if(!declaration.genericParams.empty())
        {
            if(const auto it = m_specializations.find(declaration.id); it != m_specializations.end())
                m_module.declarations().push_back(m_genericInstantiator.instantiate(declaration, it->second));
        }
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
        resolveType(*declaration.type, ctx);

        if(declaration.initializer)
            checkExpression(declaration.initializer, ctx);
    }

    void SemanticAnalyzer::checkEnumDecl(EnumDecl &declaration, const SemaContext &ctx)
    {
        if (declaration.underlyingType)
            resolveType(*declaration.underlyingType, ctx); // TODO ensure type is an integer type

        for (auto& member : declaration.members)
        {
            if (member.initializer)
            {
                checkExpression(member.initializer, ctx);
                auto val = m_evaluator.evaluate(*member.initializer);
                if (!val || !std::holds_alternative<ConstantInt>(*val))
                    continue; // TODO diagnostics

                member.value = std::get<ConstantInt>(*val);
            }
        }

    }

    void SemanticAnalyzer::checkAttributeDecl(const AttributeDecl &declaration, const SemaContext &ctx)
    {
        for (const auto field : declaration.fields)
            checkValueDecl(*field, ctx);
    }

    void SemanticAnalyzer::checkDeclarationSignature(Declaration &declaration, const SemaContext &ctx)
    {
        if(const auto valueDecl = declaration.as<ValueDecl>())
            checkValueDecl(*valueDecl, ctx);
        if(const auto funcDecl = declaration.as<FunctionDecl>())
            checkFunctionDeclSignature(*funcDecl, ctx);
        if(const auto structDecl = declaration.as<StructDecl>())
            checkStructDeclSignature(*structDecl, ctx);
        if(const auto interfaceDecl = declaration.as<InterfaceDecl>())
            checkInterfaceDeclSignature(*interfaceDecl, ctx);
        // if(const auto aliasDecl = declaration.as<AliasDecl>())
        //     checkAliasDeclSignature(*aliasDecl);
    }

    void SemanticAnalyzer::checkFunctionDeclSignature(const FunctionDecl &declaration, const SemaContext &ctx)
    {
        resolveType(*declaration.returnType, ctx);

        for(const auto param : declaration.params)
            resolveType(*param->type, ctx);
    }

    void SemanticAnalyzer::checkStructDeclSignature(const StructDecl &declaration, const SemaContext &ctx)
    {
        for(const auto field : declaration.fields)
            resolveType(*field->type, ctx);

        for(const auto method : declaration.methods)
            checkFunctionDeclSignature(*method, ctx);
    }

    void SemanticAnalyzer::checkInterfaceDeclSignature(const InterfaceDecl &declaration, const SemaContext &ctx)
    {
        for(const auto method : declaration.methods)
            checkFunctionDeclSignature(*method, ctx);
    }

    TypeId SemanticAnalyzer::resolveType(TypeSyntax& syntax, const SemaContext& ctx)
    {
        if(auto* namedSyntax = syntax.as<NamedTypeSyntax>())
        {
            if(namedSyntax->name.string(m_symbols) == "vector")
                return resolveVectorType(*namedSyntax, ctx);
            if(namedSyntax->name.string(m_symbols) == "matrix")
                return resolveMatrixType(*namedSyntax, ctx);

            const PrimitiveKind primitiveKind = toPrimitiveKind(namedSyntax->name.string(m_symbols));
            if(primitiveKind != PrimitiveKind::Unknown)
                return syntax.resolvedType = m_typeSystem.types().getPrimitiveType(primitiveKind);


            return syntax.resolvedType = resolveNamedType(*namedSyntax, ctx);
        }

        if(auto* arraySyntax = syntax.as<ArrayTypeSyntax>())
            return syntax.resolvedType = resolveArrayType(*arraySyntax, ctx);

        return TypeId::Error; // TODO diagnostics
    }

    TypeId SemanticAnalyzer::resolveTypeArg(TypeArgument &arg, const SemaContext &ctx)
    {
        if(auto* typeArg = arg.as<TypeArgumentType>())
            return typeArg->resolvedType = resolveType(*typeArg->type, ctx);

        return TypeId::Error;
    }

    TypeId SemanticAnalyzer::resolveVectorType(NamedTypeSyntax &syntax, const SemaContext& ctx)
    {
        if(syntax.arguments.size() != 2)
        {
            // TODO diagnostics
            return TypeId::Error;
        }


        auto* elementTypeSyntax = syntax.arguments.at(0)->as<TypeArgumentType>()->type;
        const TypeId elementType = resolveType(*elementTypeSyntax, ctx);
        const auto primitiveElementType = m_typeSystem.types().getInfo(elementType).as<PrimitiveType>();

        if (!primitiveElementType)
            return TypeId::Error; // TODO diagnostics

        const auto res = m_evaluator.evaluate(*syntax.arguments.at(1)->as<TypeArgumentValue>()->expression);
        if(!res) return TypeId::Error; // TODO diagnostics

        const uint32_t dimension = std::get<ConstantInt>(*res).value; // TODO check signedness

        return syntax.resolvedType = m_typeSystem.types().getVectorType(primitiveElementType->kind, dimension);
    }

    TypeId SemanticAnalyzer::resolveMatrixType(NamedTypeSyntax &syntax, const SemaContext& ctx)
    {
        if(syntax.arguments.size() != 3)
        {
            // TODO diagnostics
            return TypeId::Error;
        }

        auto* elementTypeSyntax = syntax.arguments.at(0)->as<TypeArgumentType>()->type;
        const TypeId elementType = resolveType(*elementTypeSyntax, ctx);
        const auto primitiveElementType = m_typeSystem.types().getInfo(elementType).as<PrimitiveType>();

        if (!primitiveElementType)
            return TypeId::Error; // TODO diagnostics

        const auto rowsRes = m_evaluator.evaluate(*syntax.arguments.at(1)->as<TypeArgumentValue>()->expression);
        if(!rowsRes) return TypeId::Error; // TODO diagnostics
        const uint32_t rows =std::get<ConstantInt>(*rowsRes).value; // TODO check signedness

        const auto columnsRes = m_evaluator.evaluate(*syntax.arguments.at(2)->as<TypeArgumentValue>()->expression);
        if(!columnsRes) return TypeId::Error; // TODO diagnostics
        const uint32_t columns = std::get<ConstantInt>(*columnsRes).value; // TODO check signedness

        return syntax.resolvedType = m_typeSystem.types().getMatrixType(primitiveElementType->kind, rows, columns);
    }

    TypeId SemanticAnalyzer::resolveNamedType(NamedTypeSyntax &syntax, const SemaContext& ctx)
    {
        for(const auto* param : ctx.visibleGenericParams)
        {
            if(syntax.name.parts.size() != 1 || param->name != syntax.name.parts.back()) continue;

            if(ctx.substitutions)
                if(const auto it = ctx.substitutions->find(param->id); it != ctx.substitutions->end())
                    return it->second;

            return TypeId::Error;
        }

        // FIXME make sure to match the whole type
        auto decls = m_scopeTable.find(ctx.scope, syntax.name.parts[0]);
        if(decls.empty())
            decls = m_globalScope.find(syntax.name.parts[0]);

        for(const DeclId id : decls)
        {
            // TODO make sure type matches full signature when introducing generics
            Declaration* decl = m_declTable.get(id);
            if(decl->is<StructDecl>())
            {
                return syntax.resolvedType = m_typeSystem.types().getStructType(decl->id);
            }
            if(decl->is<InterfaceDecl>())
            {
                // TODO this probably needs to resolve it to a concrete type
                return syntax.resolvedType = m_typeSystem.types().getInterfaceType(decl->id);
            }
            if(decl->is<EnumDecl>())
            {
                return syntax.resolvedType = m_typeSystem.types().getEnumType(decl->id);
            }
            if(decl->is<TypeGenericParam>())
            {
                return syntax.resolvedType = m_typeSystem.types().getGenericType(decl->id);
            }
            if(const AliasDecl* alias = decl->as<AliasDecl>())
            {
                return resolveAliasType(syntax, *alias, ctx);
            }
        }

        return TypeId::Error; // TODO diagnostics
    }

    TypeId SemanticAnalyzer::resolveAliasType(NamedTypeSyntax& syntax, const AliasDecl& alias, const SemaContext& ctx)
    {

        if(alias.genericParams.size() != syntax.arguments.size())
            return TypeId::Error; // TODO diagnostics

        std::unordered_map<DeclId, TypeId> typeSubstitutions{};


        for(size_t i = 0; i < alias.genericParams.size(); ++i)
        {
            GenericParam* param = alias.genericParams[i];
            TypeArgument* arg = syntax.arguments[i];

            if(const auto* typeParam = param->as<TypeGenericParam>())
            {
                const auto* typeArg = arg->as<TypeArgumentType>();

                if(!typeArg)
                {
                    // TODO diagnostics
                    continue;
                }

                resolveType(*typeArg->type, ctx);
                typeSubstitutions[typeParam->id] = typeArg->type->resolvedType;
            }
        }

        resolveType(*alias.targetType, ctx.forGenericDecl(alias.genericParams, typeSubstitutions));
        return syntax.resolvedType = alias.targetType->resolvedType;
    }

    TypeId SemanticAnalyzer::resolveArrayType(ArrayTypeSyntax &syntax, const SemaContext& ctx)
    {
        resolveType(*syntax.elementType, ctx);

        std::optional<uint32_t> size = std::nullopt;

        if(syntax.size)
        {
            checkExpression(syntax.size, ctx);
            const auto sizeRes = m_evaluator.evaluate(*syntax.size);
            if(!sizeRes) return TypeId::Error; // TODO diagnostics
            size = std::get<ConstantInt>(*sizeRes).value; // TODO check signedness
        }

        return syntax.resolvedType = m_typeSystem.types().getArrayType(syntax.elementType->resolvedType, size);
    }

    PrimitiveKind SemanticAnalyzer::toPrimitiveKind(const std::string &name)
    {
        const std::unordered_map<std::string, PrimitiveKind> primitiveKinds = {
            {"void", PrimitiveKind::Void},
            {"bool", PrimitiveKind::Bool},

            {"i8", PrimitiveKind::Int8},
            {"i16", PrimitiveKind::Int16},
            {"i32", PrimitiveKind::Int32},
            {"i64", PrimitiveKind::Int64},

            {"u8", PrimitiveKind::UInt8},
            {"u16", PrimitiveKind::UInt16},
            {"u32", PrimitiveKind::UInt32},
            {"u64", PrimitiveKind::UInt64},

            {"f16", PrimitiveKind::Float16},
            {"f32", PrimitiveKind::Float32},
            {"f64", PrimitiveKind::Float64},

            {"string", PrimitiveKind::String}
        };

        if(const auto it = primitiveKinds.find(name); it != primitiveKinds.end())
            return it->second;

        return PrimitiveKind::Unknown;
    }


    Expression* SemanticAnalyzer::makeConversion(Expression* operand, const TypeId type) const
    {
        if(operand->resultType == type)
            return operand;

        auto* expr = m_module.arena().create<ConversionExpr>();
        expr->span = operand->span;
        expr->kind = ConversionKind::Implicit;
        expr->targetType = type;
        expr->operand = operand;
        return expr;
    }

    uint8_t SemanticAnalyzer::componentIndex(char c)
    {
        switch(c)
        {
            case 'x': case 'r': return 0;
            case 'y': case 'g': return 1;
            case 'z': case 'b': return 2;
            case 'w': case 'a': return 3;
            default: return -1;
        }
    }
}
