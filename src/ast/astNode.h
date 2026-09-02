#pragma once
#include "../common/arena.h"
#include "../common/diagnostics.h"

namespace ionsl
{
class AstNode
{
public:
    SourceSpan span;

    virtual ~AstNode() = default;

    virtual AstNode* clone(Arena& arena) const = 0;

    template<typename T>
    requires std::is_base_of_v<AstNode, T>
    T* as()
    {
        return dynamic_cast<T*>(this);
    }

    template<typename T>
    requires std::is_base_of_v<AstNode, T>
    bool is()
    {
        return dynamic_cast<T*>(this) != nullptr;
    }
};
}
