#pragma once
#include <sstream>

namespace ions
{
class CodeWriter
{
public:
    void write(std::string_view text);
    void writeLine(std::string_view text = {});

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
    std::ostringstream m_output;
    uint32_t m_indent = 0;
    bool m_atLineStart = true;
};
}
