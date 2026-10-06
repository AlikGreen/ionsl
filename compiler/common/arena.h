#pragma once
#include <functional>
#include <iostream>
#include <memory>
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
        m_pages.emplace_back(std::make_unique<Page>());
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

        size_t space = m_pages.back()->space();
        if (space < size)
        {
            m_pages.emplace_back(std::make_unique<Page>());
            std::cout << "page count: " << m_pages.size() << "\n";
        }

        Page& page = *m_pages.back();

        space = page.space();
        const size_t offset = page.offset;
        const size_t alignedOffset = (offset + alignment - 1) & ~(alignment - 1);

        if (space < offset - alignedOffset + size)
            throw std::bad_alloc();

        page.offset = alignedOffset + size;

        return &page.data[alignedOffset];
    }

    void reset()
    {
        for (const auto& dtor : m_dtors)
            dtor();

        m_pages.clear();
    }
private:
    struct Page
    {
        static constexpr size_t kPageSize = 32*1024;

        [[nodiscard]] size_t space() const { return kPageSize - offset; }

        size_t offset = 0;
        std::array<std::byte, kPageSize> data{};
    };

    std::vector<std::unique_ptr<Page>> m_pages;
    std::vector<std::function<void()>> m_dtors;

};
}
