#include "hlslGenerator.h"

#include "../../ast/typeTable.h"

namespace ionsl
{
    void HlslGenerator::genDecl(const Declaration &decl)
    {
        bool emitted = true;

        if(const auto funcDecl = decl.as<FunctionDecl>())
            emitted = genFunctionDecl(*funcDecl);
        else if(const auto interfaceDecl = decl.as<InterfaceDecl>())
            genInterfaceDecl(*interfaceDecl);
        else if(const auto structDecl = decl.as<StructDecl>())
            genStructDecl(*structDecl);
        else if(const auto valDecl = decl.as<ValueDecl>())
            genVarDecl(*valDecl);
        else if(const auto enumDecl = decl.as<EnumDecl>())
            genEnumDecl(*enumDecl);
        else if(const auto resourceDecl = decl.as<ResourceDecl>())
            genResourceDecl(*resourceDecl);
        else
            emitted = false;

        if(emitted && m_writer.getIndent() == 0)
            m_writer.newline();
    }

    bool HlslGenerator::genFunctionDecl(const FunctionDecl &decl)
    {
        if(!decl.genericParams.empty() || (decl.attributes && decl.attributes->contains("hlsl", m_symbols)))
            return false;

        genType(decl.returnType->resolvedType);
        m_writer.space();
        m_writer.writeSymbol(decl.name);
        m_writer.write("(");

        m_writer.writeSeparated(decl.params, ", ", [this](const ValueDecl* param)
        {
            genType(param->type->resolvedType);
            m_writer.space();
            m_writer.writeSymbol(param->name);

            if (param->attributes)
                for (const auto* attrib : param->attributes->attributes)
                {
                    std::string hlslName = convertAttribute(*attrib);
                    if (!hlslName.empty())
                    {
                        m_writer.write(" : {}", hlslName);
                        break;
                    }
                }

            if(param->initializer)
            {
                m_writer.write(" = ");
                genExpr(*param->initializer);
            }
        });

        m_writer.write(")");

        if(decl.body)
            genBlockStmt(*decl.body);
        else
            m_writer.write(";");

        return true;
    }

    void HlslGenerator::genStructDecl(const StructDecl &decl)
    {
        for(auto interface : decl.resolvedInterfaces)
        {
            m_writer.write("// Implements interface '");
            genType(interface);
            m_writer.write("'");
            m_writer.newline();
        }

        m_writer.write("struct ");
        m_writer.writeSymbol(decl.name);
        m_writer.beginBlock();

        for(const auto field : decl.fields)
        {
            genType(field->type->resolvedType);
            m_writer.space();
            m_writer.writeSymbol(field->name);
            m_writer.write(";");

            if(field->initializer)
            {
                m_writer.write("// = ");
                genExpr(*field->initializer);
            }

            m_writer.newline();
        }

        for(const auto method : decl.methods)
        {
            genFunctionDecl(*method);
            m_writer.newline();
        }

        m_writer.unindent();
        m_writer.writeLine("};");

    }

    void HlslGenerator::genInterfaceDecl(const InterfaceDecl &decl)
    {
        m_writer.write("// interface ");
        m_writer.writeSymbol(decl.name);
        m_writer.newline();
    }

    void HlslGenerator::genVarDeclHeader(const ValueDecl &decl)
    {
        genType(decl.type->resolvedType);
        m_writer.space();
        m_writer.writeSymbol(decl.name);

        if(decl.initializer)
        {
            m_writer.write(" = ");
            genExpr(*decl.initializer);
        }

    }

    void HlslGenerator::genVarDecl(const ValueDecl &decl)
    {
        genVarDeclHeader(decl);
        m_writer.write(";");
        m_writer.newline();
    }

    void HlslGenerator::genEnumDecl(const EnumDecl &decl)
    {
        m_writer.writeLine("enum {}", m_symbols.get(decl.name));
        m_writer.beginBlock();

        for (const auto member : decl.members)
        {
            m_writer.writeLine("{} = {},", m_symbols.get(member->name), constantIntToString(member->value));
        }

        m_writer.endBlock();
    }

    void HlslGenerator::genResourceDecl(const ResourceDecl& decl)
    {
        if (decl.kind == ResourceKind::ConstantBuffer)
        {
            m_writer.write("ConstantBuffer<");
            genType(decl.type);
            m_writer.write("> ");
            m_writer.writeSymbol(decl.name);

            m_writer.write(" : register(b{}, space{});", decl.index, decl.space);
        }
        else if (decl.kind == ResourceKind::StorageBuffer)
        {
            char bindingPrefix = 't';
            std::string resourceTypePrefix;

            // TODO if the type is byte array then use ByteAddressBuffer
            if (decl.access == ResourceAccess::ReadWrite)
            {
                resourceTypePrefix = "RW";
                bindingPrefix = 'u';
            }


            m_writer.write("{}StructuredBuffer<", resourceTypePrefix);
            genType(decl.type);
            m_writer.write("> ");
            m_writer.writeSymbol(decl.name);
            m_writer.write(" : register({}{}, space{});", bindingPrefix, decl.index, decl.space);
        }

        m_writer.newline();
    }

}