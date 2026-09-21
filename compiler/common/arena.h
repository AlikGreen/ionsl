#pragma once
#include <vector>

namespace ionsl
{

template <typename T, typename... Args>
concept Initializable = requires(Args&&... args)
{
    T{ std::forward<Args>(args)... };
};

class Arena
{
public:
    explicit Arena(const size_t size)
    {
        m_buffer.clear();
        m_buffer.reserve(size);
    }

    template<typename T, typename... Args>
    requires std::is_constructible_v<T, Args...>
    T* create(Args&&... args)
    {
        // TODO record destructors and destruct correctly
        void* allocation = allocate(sizeof(T), alignof(T));
        return new (allocation) T(std::forward<Args>(args)...);
    }

    void* allocate(const size_t size, const size_t alignment)
    {
        const size_t space = m_buffer.capacity() - m_buffer.size();
        const size_t offset = m_buffer.size();
        const size_t alignedOffset = (offset + alignment - 1) & ~(alignment - 1);

        if (space < offset - alignedOffset + size)
            throw std::bad_alloc();

        m_buffer.resize(alignedOffset + size);

        return &m_buffer[alignedOffset];
    }

    [[nodiscard]] size_t capacity() const
    {
        return m_buffer.capacity();
    }

    void reset()
    {
        // TODO call destructors
        m_buffer.clear();
    }
private:
    std::vector<std::byte> m_buffer;

};
}
