#include "semanticAnalyzer.h"

#include <iostream>
#include <ranges>
#include <unordered_set>

#include "constantEvaluator.h"
#include "../compiler.h"
#include "../ast/declarations.h"
#include "../ast/module.h"

namespace ionsl
{
    void SemanticAnalyzer::analyze()
    {
        const SemaContext ctx{};

        for(auto it = m_module.declarations().begin(); it != m_module.declarations().end(); ++it)
        {
            checkDeclarationSignature(*(*it), ctx);

            for (auto* resource : m_pendingTopLevelDecls)
            {
                it = m_module.declarations().insert(it, resource);
                ++it;
            }

            m_pendingTopLevelDecls.clear();
        }

        for(auto it = m_module.declarations().begin(); it != m_module.declarations().end(); ++it)
        {
            checkDeclaration(*(*it), ctx);

            for (auto* resource : m_pendingTopLevelDecls)
            {
                it = m_module.declarations().insert(it, resource);
                ++it;
            }

            m_pendingTopLevelDecls.clear();
        }
    }

    void SemanticAnalyzer::analyze(Module& module, SymbolTable& symbolTable, TypeSystem& typeSystem, ScopeTable& scopeTable, DeclAllocator& declAllocator, const std::vector<EntrypointSpecialization>& specializations)
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
        if(auto* conversionExpr = expression->as<ConversionExpr>())
            return checkConversionExpr(*conversionExpr, ctx);
        if(auto* constructExpr = expression->as<ConstructExpr>())
            return constructExpr->type->resolvedType;

        return TypeId::Error;
    }

    TypeId SemanticAnalyzer::checkBinaryExpr(BinaryExpr &expression, const SemaContext& ctx)
    {
        const auto leftType = checkExpression(expression.left, ctx);
        const auto rightType = checkExpression(expression.right, ctx);

        if(leftType == TypeId::Error || rightType == TypeId::Error) return TypeId::Error;

        const auto result = m_typeSystem.resolveBinaryTypes(expression.op, leftType, rightType);
        if(!result)
        {
            error(expression.span, "operator '{}' cannot be applied to operands of type '{}' and '{}'",
              opToString(expression.op),
              getTypeName(leftType),
              getTypeName(rightType));
            return TypeId::Error;
        }

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
        if(!result)
        {
            error(expression.span, "unary operator '{}' cannot be applied to type '{}'",
                  opToString(expression.op),
                  getTypeName(operandType));
            return TypeId::Error;
        }


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

        m_speculative = true;
        if(resolveType(*typeSyntax, ctx) != TypeId::Error)
        {
            m_speculative = false;
            auto* construct = m_module.arena().create<ConstructExpr>();
            construct->type = typeSyntax;
            construct->args = expression.args;
            construct->span = expression.span;
            slot = construct;
            return typeSyntax->resolvedType;
        }
        m_speculative = false;

        // TODO check non identifier calls eg
        // var func = () -> { return 2; };
        // var x = func()
        return checkIdentifierCall(expression, *identifier, ctx);
    }

    TypeId SemanticAnalyzer::checkIdentifierCall(CallExpr& expression, IdentifierExpr& identifier, const SemaContext& ctx)
    {
        auto candidates = find(identifier.name, ctx);
        if (candidates.empty())
        {
            error(expression.span, "no function named '{}' exists", identifier.name.string(m_symbols));
            return TypeId::Error;
        }

        std::vector<TypeId> argumentTypes;
        bool hasArgError = false;

        for (auto& argument : expression.args)
        {
            TypeId type = checkExpression(argument, ctx);
            if (type == TypeId::Error)
                hasArgError = true;

            argumentTypes.push_back(type);
        }

        if (hasArgError)
            return TypeId::Error;

        struct Candidate
        {
            uint32_t conversionCost = ~0u;
            Declaration* decl{};
            std::vector<TypeId> paramTypes;
        };

        Candidate bestCandidate{};

        for(const auto candidate : candidates)
        {
            Declaration* decl = m_declTable.get(candidate);

            if(auto* funcDecl = decl->as<FunctionDecl>())
            {
                if (funcDecl->attributes->contains("shader", m_symbols))
                    continue;

                std::vector<TypeId> paramTypes;
                for(const auto* param : funcDecl->params)
                {
                    paramTypes.push_back(resolveType(*param->type, ctx.forGenericDecl(funcDecl->genericParams).withScope(funcDecl->paramsScope)));
                }

                auto conversion = m_typeSystem.conversionCost(argumentTypes, paramTypes);

                if(!conversion)
                    continue;

                if(bestCandidate.conversionCost > *conversion)
                {
                    bestCandidate.conversionCost = *conversion;
                    bestCandidate.decl = decl;
                    bestCandidate.paramTypes = paramTypes;
                }
            }
        }

        if(bestCandidate.decl == nullptr)
        {
            m_module.diagnostics().error(expression.span, "no matching function for call to '{}'", identifier.name.string(m_symbols));
            return TypeId::Error;
        }

        if(auto* funcDecl = bestCandidate.decl->as<FunctionDecl>())
        {
            if (!funcDecl->genericParams.empty())
            {
                std::unordered_map<DeclId, TypeId> subs;
                for(const auto [param, arg] : std::ranges::zip_view(funcDecl->genericParams, expression.genericArgs))
                    subs[param->id] = arg->resolvedType;

                SemaContext genericCtx = ctx.forGenericDecl(funcDecl->genericParams).withSubs(subs);

                funcDecl = instantiateFunction(*funcDecl, genericCtx);
            }

            for(const auto& [arg, param] : std::views::zip(expression.args, funcDecl->params))
            {
                arg = makeConversion(arg, param->type->resolvedType);
            }

            identifier.decl = funcDecl->id;
            return funcDecl->returnType->resolvedType;
        }


        // TODO methods and variables

        m_module.diagnostics().error(expression.span, "no matching function for call to '{}'", identifier.name.string(m_symbols));

        return TypeId::Error;
    }

    TypeId SemanticAnalyzer::checkIdentifierExpr(IdentifierExpr &expression, const SemaContext& ctx)
    {
        const auto candidates = find(expression.name, ctx);

        TypeId bestCandidateType = TypeId::Error;
        DeclId bestCandidateDecl = DeclId::Error;

        for(const auto& id : candidates)
        {
            const auto decl = m_declTable.get(id);
            if(const auto value = decl->as<ValueDecl>())
            {
                bestCandidateType = resolveType(*value->type, ctx);
                bestCandidateDecl = id;
            }
            else if(const auto struc = decl->as<StructDecl>())
            {
                bestCandidateType = m_typeSystem.types().getStructType(struc->id);
                bestCandidateDecl = id;
            }
            else if(const auto interface = decl->as<InterfaceDecl>())
            {
                bestCandidateType = m_typeSystem.types().getInterfaceType(interface->id);
                bestCandidateDecl = id;
            }
            else if(const auto enumeration = decl->as<EnumDecl>())
            {
                bestCandidateType = m_typeSystem.types().getEnumType(enumeration->id);
                bestCandidateDecl = id;
            }
            else if(const auto resource = decl->as<ResourceDecl>())
            {
                bestCandidateType = m_typeSystem.types().getResourceType(resource->id, resource->type);
                bestCandidateDecl = id;
            }
            else if(const auto enumMember = decl->as<EnumMember>())
            {
                bestCandidateType = m_typeSystem.types().getEnumType(enumMember->parentId);
                bestCandidateDecl = id;
            }
            else
            {
                return TypeId::Error;
            }
        }

        // TODO diagnostics if no candidates
        if (bestCandidateDecl == DeclId::Error)
            error(expression.span, "cannot resolve name {}", expression.name.string(m_symbols));


        expression.decl = bestCandidateDecl;
        return expression.resultType = bestCandidateType;
    }

    TypeId SemanticAnalyzer::checkIndexExpr(IndexExpr &expression, const SemaContext& ctx)
    {
        const TypeId indexType = checkExpression(expression.index, ctx);
        const TypeId arrayTypeId = checkExpression(expression.array, ctx);

        if (arrayTypeId == TypeId::Error)
            return TypeId::Error;

        if (!m_typeSystem.types().isIntegral(indexType))
        {
            error(expression.index->span, "array index must be an integer");
            return TypeId::Error;
        }

        TypeInfo type = m_typeSystem.types().getInfo(arrayTypeId);

        if (const auto arrayType = type.as<ArrayType>())
        {
            return expression.resultType = arrayType->elementType;
        }
        if (const auto resourceType = type.as<ResourceType>())
        {
            // TODO improve this
            const auto resourceDecl = m_declTable.get(resourceType->declId)->as<ResourceDecl>();
            if (!resourceDecl) return TypeId::Error;

            return expression.resultType = resourceDecl->type;
        }

        error(expression.array->span, "cannot index into non-array type");
        return TypeId::Error;
    }

    TypeId SemanticAnalyzer::checkConversionExpr(ConversionExpr &expression, const SemaContext &ctx)
    {
        // TODO check that operand type can go to target type
        return expression.targetType;
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
            const StructDecl* structDecl = m_declTable.get(structType->declId)->as<StructDecl>();
            // TODO methods
            const ValueDecl* field = structDecl->findField(expression.memberName);

            if (!field)
            {
                error(expression.span, "struct '{}' has no field named '{}'",
                      m_symbols.get(structDecl->name),
                      m_symbols.get(expression.memberName));
                return TypeId::Error;
            }

            expression.resultType = field->type->resolvedType;
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
                {
                    error(expression.span, "component '{}' is out of bounds for vector of dimension {}",
                          c, vectorType->dimension);
                    return TypeId::Error;
                }

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

        error(expression.span, "cannot access member '{}' on non-aggregate type",
          m_symbols.get(expression.memberName));
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

        if(ctx.expectedReturnType != TypeId::Error)
        {
            if(!m_typeSystem.conversionCost(returnType, ctx.expectedReturnType))
            {
                error(statement.span, "cannot return value of type '{}' from function returning '{}'",
                      getTypeName(returnType),
                      getTypeName(ctx.expectedReturnType));
            }
        }
    }

    void SemanticAnalyzer::checkBreakContinueStmt()
    {
        // TODO make sure in for or while loop
    }

    void SemanticAnalyzer::checkDeclaration(Declaration &declaration, const SemaContext& ctx)
    {
        for (const auto attrib : declaration.attributes->attributes)
        {
            checkAttribute(*attrib, ctx);
        }

        if(const auto funcDecl = declaration.as<FunctionDecl>())
            checkFunctionDecl(*funcDecl, ctx);
        else if(const auto interfaceDecl = declaration.as<InterfaceDecl>())
            checkInterfaceDecl(*interfaceDecl, ctx);
        else if(const auto structDecl = declaration.as<StructDecl>())
            checkStructDecl(*structDecl, ctx);
        else if(const auto valDecl = declaration.as<ValueDecl>())
            checkValueDecl(*valDecl, ctx);
        else if(const auto enumDecl = declaration.as<EnumDecl>())
            checkEnumDecl(*enumDecl, ctx);
        else if(const auto aliasDecl = declaration.as<AliasDecl>())
            checkAliasDecl(*aliasDecl, ctx);
    }

    void SemanticAnalyzer::checkFunctionDecl(const FunctionDecl &declaration, const SemaContext& ctx)
    {
        if (!declaration.body)
             return;

        SemaContext bodyCtx = ctx.withExpectedReturnType(declaration.returnType->resolvedType);

        if (declaration.genericParams.empty())
        {
            checkBlockStmt(*declaration.body, bodyCtx);
            return;
        }

        // check the generic function for errors
        checkBlockStmt(*declaration.body, bodyCtx.forGenericDecl(declaration.genericParams));

        for (const auto& specialization : m_specializations)
        {
            if (specialization.entrypoint != declaration.id)
                continue;

            std::unordered_map<DeclId, TypeId> subs;
            for(const auto [param, typeId] : std::ranges::zip_view(declaration.genericParams, specialization.typeArgs))
                subs[param->id] = typeId;

            SemaContext genericCtx = bodyCtx.forGenericDecl(declaration.genericParams).withSubs(subs);

            const SymbolId newName = specialization.newName.empty() ? declaration.name : m_symbols.intern(specialization.newName);

            instantiateFunction(declaration, genericCtx, newName);
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

        uint64_t nextValue = 0;

        for (auto& member : declaration.members)
        {
            if (member->initializer)
            {
                checkExpression(member->initializer, ctx);
                auto val = m_evaluator.evaluate(*member->initializer);
                if (!val || !std::holds_alternative<ConstantInt>(*val))
                    continue; // TODO diagnostics

                member->value = std::get<ConstantInt>(*val);
                nextValue = member->value.value + 1;
            }else
            {
                member->value = ConstantInt{ nextValue++ , IntKind::Signed };
            }
        }

    }

    void SemanticAnalyzer::checkAliasDecl(AliasDecl &declaration, const SemaContext &ctx)
    {
        resolveType(*declaration.targetType, ctx.forGenericDecl(declaration.genericParams));
    }

    void SemanticAnalyzer::checkAttributeDecl(const AttributeDecl &declaration, const SemaContext &ctx)
    {
        for (const auto field : declaration.fields)
        {
            checkValueDecl(*field, ctx);

            if (field->initializer)
            {
                if (!m_evaluator.evaluate(*field->initializer))
                {
                    error(field->initializer->span, "attribute parameter initializer must be a compile-time constant");
                }
            }
        }
    }

    void SemanticAnalyzer::checkDeclarationSignature(Declaration &declaration, const SemaContext &ctx)
    {
        for (const auto attrib : declaration.attributes->attributes)
        {
            checkAttribute(*attrib, ctx);
        }

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

    void SemanticAnalyzer::checkFunctionDeclSignature(FunctionDecl &declaration, const SemaContext &ctx)
    {
        const auto genericCtx = ctx.forGenericDecl(declaration.genericParams);

        resolveType(*declaration.returnType, genericCtx);

        for(const auto param : declaration.params)
            checkDeclarationSignature(*param, genericCtx);

        // for(auto it = declaration.params.begin(); it != declaration.params.end();)
        // {
        //     const auto param = *it;
        //     checkDeclarationSignature(*param, genericCtx);
        //
        //     if (tryExtractResourceDecl(*param, declaration))
        //     {
        //         m_scopeTable.unregisterDecl(declaration.paramsScope, (*it)->name, (*it)->id);
        //         it = declaration.params.erase(it);
        //     }else
        //     {
        //         ++it;
        //     }
        // }
    }

    void SemanticAnalyzer::checkStructDeclSignature(const StructDecl &declaration, const SemaContext &ctx)
    {
        for(const auto field : declaration.fields)
            checkDeclarationSignature(*field, ctx);

        for(const auto method : declaration.methods)
            checkDeclarationSignature(*method, ctx);
    }

    void SemanticAnalyzer::checkInterfaceDeclSignature(const InterfaceDecl &declaration, const SemaContext &ctx)
    {
        for(const auto method : declaration.methods)
            checkDeclarationSignature(*method, ctx);
    }

    void SemanticAnalyzer::checkAttribute(Attribute &attribute, const SemaContext &ctx)
    {
        AttributeDecl* decl = nullptr;
        for(const DeclId id : find(attribute.name, ctx))
            if(auto* a = m_declTable.get(id)->as<AttributeDecl>()) { decl = a; break; }

        if(!decl) { error(attribute.span, "unknown attribute '{}'", attribute.name.string(m_symbols)); return; }
        attribute.decl = decl->id;

        // TODO check this is a valid target

        std::vector<AttributeArg*> foundArgs(decl->fields.size());
        size_t nextPositional = 0;
        bool sawNamed = false;

        for(auto* arg : attribute.args)
        {
            size_t idx;
            if(arg->name == SymbolId::Invalid)
            {
                if(sawNamed)
                {
                    error(arg->span, "positional argument after named argument");
                    return;
                }

                if(nextPositional >= foundArgs.size())
                {
                    error(arg->span, "too many arguments");
                    return;
                }

                idx = nextPositional++;
            }
            else
            {
                sawNamed = true;
                auto it = std::ranges::find(decl->fields, arg->name, &ValueDecl::name);
                if(it == decl->fields.end())
                {
                    error(arg->span, "no parameter named '{}'", m_symbols.get(arg->name));
                    return;
                }

                idx = it - decl->fields.begin();

                if(foundArgs[idx])
                {
                    error(arg->span, "parameter already supplied", m_symbols.get(arg->name));
                    return;
                }
            }

            checkExpression(arg->expression, ctx);
            arg->value = m_evaluator.evaluate(*arg->expression);

            if(!arg->value)
            {
                error(arg->span, "attribute arguments must be constant");
                return;
            }
            foundArgs[idx] = arg;
        }

        for(size_t i = 0; i < foundArgs.size(); ++i)
        {
            const ValueDecl& p = *decl->fields[i];

            if(foundArgs[i])
            {
                if (foundArgs[i]->name == SymbolId::Invalid)
                    foundArgs[i]->name = p.name;

                continue;
            }

            if(!p.initializer) { error(decl->fields[i]->span, "missing required argument '{}'", m_symbols.get(decl->fields[i]->name)); return; }

            const auto def = m_evaluator.evaluate(*p.initializer);

            auto* arg = m_module.arena().create<AttributeArg>();
            arg->name = p.name;
            arg->value = def;
            attribute.args.push_back(arg);
        }
    }

    TypeId SemanticAnalyzer::resolveType(TypeSyntax& syntax, const SemaContext& ctx)
    {
        if (syntax.resolvedType != TypeId::Error && !m_typeSystem.types().containsGenericParam(syntax.resolvedType))
            return syntax.resolvedType;

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
            error(syntax.span, "vector type expects 2 arguments");
            return TypeId::Error;
        }


        auto* elementTypeSyntax = syntax.arguments.at(0)->as<TypeArgumentType>()->type;
        const TypeId elementType = resolveType(*elementTypeSyntax, ctx);

        const auto res = m_evaluator.evaluate(*syntax.arguments.at(1)->as<TypeArgumentValue>()->expression);
        if(!res || !std::holds_alternative<ConstantInt>(*res))
        {
            error(syntax.arguments.at(1)->span, "vector dimension must be a compile-time constant integer");
            return TypeId::Error;
        }

        const uint32_t dimension = std::get<ConstantInt>(*res).value; // TODO check signedness

        if (dimension < 2 || dimension > 4)
        {
            error(syntax.arguments.at(1)->span, "invalid vector dimension ({}), dimension must be between 2 and 4", dimension);
            return TypeId::Error;
        }

        if (const auto primitiveElementType = m_typeSystem.types().getInfo(elementType).as<PrimitiveType>())
        {
            return syntax.resolvedType = m_typeSystem.types().getVectorType(primitiveElementType->kind, dimension);
        }
        if (m_typeSystem.types().getInfo(elementType).is<GenericType>())
        {
            return syntax.resolvedType = m_typeSystem.types().getGenericVectorType(elementType, dimension);
        }

        error(syntax.span, "vector element type must be a scalar type");
        return TypeId::Error;
    }

    TypeId SemanticAnalyzer::resolveMatrixType(NamedTypeSyntax &syntax, const SemaContext& ctx)
    {
        if(syntax.arguments.size() != 3)
        {
            error(syntax.span, "matrix type expects 3 arguments");
            return TypeId::Error;
        }


        const auto rowsRes = m_evaluator.evaluate(*syntax.arguments.at(1)->as<TypeArgumentValue>()->expression);

        if(!rowsRes || !std::holds_alternative<ConstantInt>(*rowsRes))
        {
            error(syntax.arguments.at(1)->span, "matrix rows must be a compile-time constant integer");
            return TypeId::Error;
        }

        const uint32_t rows = std::get<ConstantInt>(*rowsRes).value;

        if (rows < 1 || rows > 4)
        {
            error(syntax.arguments.at(1)->span, "invalid matrix rows ({}), rows must be between 1 and 4", rows);
            return TypeId::Error;
        }

        const auto columnsRes = m_evaluator.evaluate(*syntax.arguments.at(2)->as<TypeArgumentValue>()->expression);
        if(!columnsRes || !std::holds_alternative<ConstantInt>(*columnsRes))
        {
            error(syntax.arguments.at(2)->span, "matrix columns must be a compile-time constant integer");
            return TypeId::Error;
        }

        const uint32_t columns = std::get<ConstantInt>(*columnsRes).value; // TODO check signedness

        if (columns < 2 || columns > 4)
        {
            error(syntax.arguments.at(2)->span, "invalid matrix columns ({}), columns must be between 2 and 4", columns);
            return TypeId::Error;
        }

        auto* elementTypeSyntax = syntax.arguments.at(0)->as<TypeArgumentType>()->type;
        const TypeId elementType = resolveType(*elementTypeSyntax, ctx);

        if (const auto primitiveElementType = m_typeSystem.types().getInfo(elementType).as<PrimitiveType>())
        {
            return syntax.resolvedType = m_typeSystem.types().getMatrixType(primitiveElementType->kind, rows, columns);
        }
        if (m_typeSystem.types().getInfo(elementType).is<GenericType>())
        {
            return syntax.resolvedType = m_typeSystem.types().getGenericMatrixType(elementType, rows, columns);
        }

        error(syntax.span, "matrix element type must be a scalar type");
        return TypeId::Error;
    }

    TypeId SemanticAnalyzer::resolveNamedType(NamedTypeSyntax &syntax, const SemaContext& ctx)
    {
        const SymbolId name = syntax.name.parts[0];
        for (const auto* param : ctx.visibleGenericParams)
        {
            if (param->name != name) continue;

            if (ctx.substitutions)
            {
                const auto it = ctx.substitutions->find(param->id);
                if (it != ctx.substitutions->end())
                    return syntax.resolvedType = it->second;
            }
            return syntax.resolvedType = m_typeSystem.types().getGenericType(param->id);
        }

        auto decls = find(syntax.name, ctx);

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

        error(syntax.span, "unknown type '{}'", syntax.name.string(m_symbols));
        return TypeId::Error;
    }

    TypeId SemanticAnalyzer::resolveAliasType(NamedTypeSyntax& syntax, const AliasDecl& alias, const SemaContext& ctx)
    {
        if (alias.genericParams.size() != syntax.arguments.size())
        {
            error(syntax.span, "type alias '{}' expects {} arguments, but {} were provided",
                  m_symbols.get(alias.name),
                  alias.genericParams.size(),
                  syntax.arguments.size());
            return TypeId::Error;
        }

        std::vector<TypeId> genericArgTypes{};
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

                genericArgTypes.push_back(resolveType(*typeArg->type, ctx));
                typeSubstitutions[typeParam->id] = typeArg->type->resolvedType;
            }else
            {
                // TODO
            }
        }


        size_t key = hashGenericInst(alias.id, genericArgTypes);

        TypeId resolvedType = resolveType(*alias.targetType, ctx.forGenericDecl(alias.genericParams).withSubs(typeSubstitutions));

        if (!alias.genericParams.empty())
        {
            alias.targetType->resolvedType = TypeId::Error;
            m_aliasInstCache.emplace(key, resolvedType);
        }

        return syntax.resolvedType = resolvedType;
    }

    TypeId SemanticAnalyzer::resolveArrayType(ArrayTypeSyntax &syntax, const SemaContext& ctx)
    {
        resolveType(*syntax.elementType, ctx);

        std::optional<uint32_t> size = std::nullopt;

        if(syntax.size)
        {
            checkExpression(syntax.size, ctx);
            const auto sizeRes = m_evaluator.evaluate(*syntax.size);
            if(!sizeRes || !std::holds_alternative<ConstantInt>(*sizeRes))
            {
                error(syntax.size->span, "array size must be a compile-time constant integer");
                return TypeId::Error;
            }
            size = std::get<ConstantInt>(*sizeRes).value;
            if(*size < 1)
            {
                error(syntax.size->span, "array size must be a non-zero positive integer");
                return TypeId::Error;
            }
        }

        return syntax.resolvedType = m_typeSystem.types().getArrayType(syntax.elementType->resolvedType, size);
    }


    FunctionDecl* SemanticAnalyzer::instantiateFunction(const FunctionDecl &genericFunction, const SemaContext& ctx, SymbolId newName)
    {
        std::vector<TypeId> typeArgs{};
        for (const auto* param : genericFunction.genericParams)
        {
            if (auto it = ctx.substitutions->find(param->id); it != ctx.substitutions->end())
                typeArgs.push_back(it->second);
            else
                return nullptr; // TODO error
        }

        newName = newName == SymbolId::Invalid ? genericFunction.name : newName;

        const size_t key = hashGenericInst(genericFunction.id, typeArgs, newName);
        if (const auto it = m_funcInstCache.find(key); it != m_funcInstCache.end())
            return it->second;

        FunctionDecl* concreteFunc = genericFunction.clone(m_module.arena());
        concreteFunc->id = m_declAllocator.allocate();
        concreteFunc->name = newName;

        checkFunctionDeclSignature(*concreteFunc, ctx);
        checkBlockStmt(*concreteFunc->body, ctx.withExpectedReturnType(concreteFunc->returnType->resolvedType));

        concreteFunc->genericParams.clear();

        addGlobalDecl(*concreteFunc);

        m_funcInstCache.emplace(key, concreteFunc);

        return concreteFunc;
    }

    std::vector<DeclId> SemanticAnalyzer::findInScopeChain(const SymbolId name, const ScopeId scope) const
    {
        for(ScopeId s = scope; s != ScopeId::None; s = m_scopeTable.getScope(s).parent)
        {
            auto found = m_scopeTable.find(s, name);
            if(!found.empty())
                return found;
        }
        return {};
    }

    std::vector<DeclId> SemanticAnalyzer::findUnqualified(const SymbolId name, const SemaContext& ctx) const
    {
        auto found = findInScopeChain(name, ctx.scope);
        if(!found.empty())
            return found;

        return m_globalScope.find(name);
    }

    std::vector<DeclId> SemanticAnalyzer::find(const QualifiedName& name, const SemaContext &ctx) const
    {
        if(name.parts.empty())
            return {};

        auto current = findUnqualified(name.parts[0], ctx);

        for(size_t i = 1; i < name.parts.size() && !current.empty(); ++i)
        {
            ScopeId members = ScopeId::None;
            int owners = 0;

            for(const DeclId id : current)
            {
                const ScopeId s = memberScopeOf(*m_declTable.get(id));
                if(s != ScopeId::None) { members = s; ++owners; }
            }

            if(owners != 1)
                return {};

            current = m_scopeTable.find(members, name.parts[i]);
        }

        if (current.empty())
            return {};

        return current;
    }

    ScopeId SemanticAnalyzer::memberScopeOf(const Declaration& decl) const
    {
        // TODO return namespaces scope or struct scope for nested types
        if (auto enumDecl = decl.as<EnumDecl>())
        {
            return enumDecl->membersScope;
        }

        return ScopeId::None;
    }

    void SemanticAnalyzer::addGlobalDecl(Declaration& decl)
    {
        m_pendingTopLevelDecls.push_back(&decl);
        m_scopeTable.registerDecl(ScopeId::None, decl.name, decl.id);
        m_globalScope.registerDecl(decl.name, decl.id);
    }

    size_t SemanticAnalyzer::hashGenericInst(const DeclId genericId, const std::vector<TypeId>& typeArgs, const SymbolId name) const
    {
        size_t h = std::hash<DeclId>{}(genericId);
        for(const TypeId t : typeArgs)
            hashCombine(h, t);

        hashCombine(h, name.value());
        return h;
    }

    std::string SemanticAnalyzer::getTypeName(const TypeId id) const
    {
        if (id == TypeId::Error)
            return "<error>";
        if (id == TypeId::Void)
            return "void";

        const TypeInfo info = m_typeSystem.types().getInfo(id);

        if (const auto* p = info.as<PrimitiveType>())
        {
            switch (p->kind)
            {
                case PrimitiveKind::Void:    return "void";
                case PrimitiveKind::Bool:    return "bool";
                case PrimitiveKind::Int8:    return "i8";
                case PrimitiveKind::Int16:   return "i16";
                case PrimitiveKind::Int32:   return "i32";
                case PrimitiveKind::Int64:   return "i64";
                case PrimitiveKind::UInt8:   return "u8";
                case PrimitiveKind::UInt16:  return "u16";
                case PrimitiveKind::UInt32:  return "u32";
                case PrimitiveKind::UInt64:  return "u64";
                case PrimitiveKind::Float16: return "f16";
                case PrimitiveKind::Float32: return "f32";
                case PrimitiveKind::Float64: return "f64";
                case PrimitiveKind::String:  return "string";
                default:                     return "unknown";
            }
        }

        if (const auto* v = info.as<VectorType>())
        {
            std::string elem = getTypeName(m_typeSystem.types().getPrimitiveType(v->scalarType));
            return std::format("vector<{}, {}>", elem, v->dimension);
        }

        if (const auto* m = info.as<MatrixType>())
        {
            std::string elem = getTypeName(m_typeSystem.types().getPrimitiveType(m->scalarType));
            return std::format("matrix<{}, {}, {}>", elem, m->rows, m->columns);
        }

        if (const auto* gv = info.as<GenericVectorType>())
        {
            return std::format("vector<{}, {}>", getTypeName(gv->elementType), gv->dimension);
        }

        if (const auto* gm = info.as<GenericMatrixType>())
        {
            return std::format("matrix<{}, {}, {}>", getTypeName(gm->elementType), gm->rows, gm->columns);
        }

        if (const auto* a = info.as<ArrayType>())
        {
            if (a->size)
                return std::format("{}[{}]", getTypeName(a->elementType), *a->size);
            return std::format("{}[]", getTypeName(a->elementType));
        }

        if (const auto* s = info.as<StructType>())
        {
            if (const auto* decl = m_declTable.get(s->declId))
                return m_symbols.get(decl->name);
            return "<struct>";
        }

        if (const auto* e = info.as<EnumType>())
        {
            if (const auto* decl = m_declTable.get(e->declId))
                return m_symbols.get(decl->name);
            return "<enum>";
        }

        if (const auto* i = info.as<InterfaceType>())
        {
            if (const auto* decl = m_declTable.get(i->declId))
                return m_symbols.get(decl->name);
            return "<interface>";
        }

        if (const auto* g = info.as<GenericType>())
        {
            if (const auto* decl = m_declTable.get(g->declId))
                return m_symbols.get(decl->name);
            return "<generic>";
        }

        if (const auto* r = info.as<ResourceType>())
        {
            if (const auto* decl = m_declTable.get(r->declId))
                return std::format("{}<{}>", m_symbols.get(decl->name), getTypeName(r->containedType));
            return std::format("Resource<{}>", getTypeName(r->containedType));
        }

        return "<unknown>";
    }

    std::string_view SemanticAnalyzer::opToString(BinaryOp op)
    {
        switch (op)
        {
            case BinaryOp::Add:             return "+";
            case BinaryOp::Subtract:        return "-";
            case BinaryOp::Multiply:        return "*";
            case BinaryOp::Divide:          return "/";
            case BinaryOp::Modulo:          return "%";

            case BinaryOp::Equal:           return "==";
            case BinaryOp::NotEqual:        return "!=";
            case BinaryOp::Less:            return "<";
            case BinaryOp::LessEqual:       return "<=";
            case BinaryOp::Greater:         return ">";
            case BinaryOp::GreaterEqual:    return ">=";

            case BinaryOp::LogicalAnd:      return "&&";
            case BinaryOp::LogicalOr:       return "||";

            case BinaryOp::BitwiseAnd:      return "&";
            case BinaryOp::BitwiseOr:       return "|";
            case BinaryOp::BitwiseXor:      return "^";
            case BinaryOp::ShiftLeft:       return "<<";
            case BinaryOp::ShiftRight:      return ">>";

            case BinaryOp::Assign:          return "=";
            case BinaryOp::ModuloAssign:    return "%=";
            case BinaryOp::AddAssign:       return "+=";
            case BinaryOp::SubAssign:       return "-=";
            case BinaryOp::MulAssign:       return "*=";
            case BinaryOp::DivAssign:       return "/=";
            case BinaryOp::BitwiseAndAssign: return "&=";
            case BinaryOp::BitwiseOrAssign:  return "|=";
            case BinaryOp::BitwiseXorAssign: return "^=";
            case BinaryOp::ShiftLeftAssign:  return "<<=";
            case BinaryOp::ShiftRightAssign: return ">>=";
        }

        return "<invalid>";
    }

    std::string_view SemanticAnalyzer::opToString(UnaryOp op)
    {
        switch (op)
        {
            case UnaryOp::Negate:         return "-";
            case UnaryOp::LogicalNot:     return "!";
            case UnaryOp::BitwiseNot:     return "~";
            case UnaryOp::PreIncrement:   return "++";
            case UnaryOp::PreDecrement:   return "--";
            case UnaryOp::PostIncrement:  return "++";
            case UnaryOp::PostDecrement:  return "--";
        }

        return "<invalid>";
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
            default: return 0xFF;
        }
    }
}
