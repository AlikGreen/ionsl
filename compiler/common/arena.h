#pragma once
#include <functional>
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
    explicit Arena()
    {
        m_pages.emplace_back();
    }

    template<typename T, typename... Args>
    requires std::is_constructible_v<T, Args...>
    T* create(Args&&... args)
    {
        // TODO record destructors and destruct correctly
        void* allocation = allocate(sizeof(T), alignof(T));

        T* obj = new (allocation) T(std::forward<Args>(args)...);

        m_dtors.emplace_back([&obj]()
        {
            delete obj;
        });

        return obj;
    }

    void* allocate(const size_t size, const size_t alignment)
    {
        if (size > Page::kPageSize)
            throw std::bad_alloc();

        size_t space = m_pages.back().space();
        if (space < size)
            m_pages.emplace_back();

        Page& page = m_pages.back();

        space = page.space();
        const size_t offset = page.data().capacity();
        const size_t alignedOffset = (offset + alignment - 1) & ~(alignment - 1);

        if (space < offset - alignedOffset + size)
            throw std::bad_alloc();

        page.data().resize(alignedOffset + size);

        return &page.data()[alignedOffset];
    }

    void reset()
    {
        for (const auto& dtor : m_dtors)
            dtor();

        m_pages.clear();
    }
private:
    class Page
    {
    public:
        static constexpr size_t kPageSize = 64*1024;

        explicit Page() { m_data.reserve(kPageSize); }
        [[nodiscard]] size_t space() const { return m_data.capacity() - m_data.size(); }
        std::vector<std::byte>& data() { return m_data; }
    private:
        std::vector<std::byte> m_data;
    };

    std::vector<Page> m_pages;
    std::vector<std::function<void()>> m_dtors;

};
}
