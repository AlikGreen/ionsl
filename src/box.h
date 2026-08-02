#pragma once
#include <memory>

namespace ionsl
{
template <typename T>
class Box
{
    std::unique_ptr<T> m_ptr{};
public:
    Box() = default;
    Box(std::nullptr_t) noexcept : m_ptr(nullptr) {}

    Box(const Box& other) : m_ptr(other.m_ptr ? std::make_unique<T>(*other.m_ptr) : nullptr) {}
    Box& operator=(const Box& other)
    {
        m_ptr = other.m_ptr ? std::make_unique<T>(*other.m_ptr) : nullptr;
        return *this;
    }

    Box(Box&&) noexcept = default;
    Box& operator=(Box&&) noexcept = default;
    ~Box() = default;

    template <typename... Args>
    static Box make(Args&&... args)
    {
        Box box;
        box.m_ptr = std::make_unique<T>(std::forward<Args>(args)...);
        return box;
    }

    T& operator*() { return *m_ptr; }
    const T& operator*() const { return *m_ptr; }
    T* operator->() { return m_ptr.get(); }
    const T* operator->() const { return m_ptr.get(); }

    explicit operator bool() const noexcept { return m_ptr != nullptr; }
    T* get() { return m_ptr.get(); }
    const T* get() const { return m_ptr.get(); }
};
}
