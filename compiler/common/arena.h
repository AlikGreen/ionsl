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
    explicit Arena();
    ~Arena();

    template<typename T, typename... Args>
    requires std::is_constructible_v<T, Args...>
    T* create(Args&&... args)
    {
        // TODO record destructors and destruct correctly
        void* allocation = allocate(sizeof(T), alignof(T));

        T* obj = new (allocation) T(std::forward<Args>(args)...);

        m_dtors.emplace_back([obj]()
        {
            obj->~T();
        });

        return obj;
    }

    void* allocate(size_t size, size_t alignment);
    void reset();
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
