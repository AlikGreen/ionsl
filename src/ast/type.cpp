#include "type.h"

namespace ionsl
{
    constexpr TypeId TypeId::Error   = TypeId(0);
    constexpr TypeId TypeId::Void    = TypeId(1);
    constexpr TypeId TypeId::Bool    = TypeId(2);
    constexpr TypeId TypeId::I8      = TypeId(3);
    constexpr TypeId TypeId::I16     = TypeId(4);
    constexpr TypeId TypeId::I32     = TypeId(5);
    constexpr TypeId TypeId::I64     = TypeId(6);
    constexpr TypeId TypeId::U8      = TypeId(7);
    constexpr TypeId TypeId::U16     = TypeId(8);
    constexpr TypeId TypeId::U32     = TypeId(9);
    constexpr TypeId TypeId::U64     = TypeId(10);
    constexpr TypeId TypeId::F16     = TypeId(11);
    constexpr TypeId TypeId::F32     = TypeId(12);
    constexpr TypeId TypeId::F64     = TypeId(13);
    constexpr TypeId TypeId::String  = TypeId(14);
}