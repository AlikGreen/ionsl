#pragma once
#include <sstream>

#include "../ast/symbolTable.h"

namespace ionsl
{
class CodeWriter
{
public:
    CodeWriter() = default;
    explicit CodeWriter(const SymbolTable& symbolTable);

    void writeSymbol(SymbolId symbol);

    void write(std::string_view text);

    template<typename... Args>
    void write(std::format_string<Args...> fmt, Args&&... args)
    {
        write(std::format(fmt, std::forward<Args>(args)...));
    }

    void writeLine(std::string_view text = {});

    template<typename... Args>
    void writeLine(std::format_string<Args...> fmt, Args&&... args)
    {
        writeLine(std::format(fmt, std::forward<Args>(args)...));
    }

    void space();
    void newline();

    void indent();
    void unindent();

    void beginBlock();
    void endBlock();

    void writeIndent();

    template<typename Container, typename Func>
    void writeSeparated(
        const Container& values,
        std::string_view separator,
        Func&& func)
    {
        if(m_atLineStart)
            writeIndent();

        bool first = true;
        m_atLineStart = false;

        for(const auto& value : values)
        {
            if(!first)
                write(separator);

            first = false;
            func(value);
        }
    }

    [[nodiscard]] std::string string() const;
private:
    const SymbolTable* m_symbolTable{};
    std::ostringstream m_output;
    uint32_t m_indent = 0;
    bool m_atLineStart = true;
};
}
