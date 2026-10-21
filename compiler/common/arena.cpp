#include "arena.h"

namespace ionsl
{
    Arena::Arena()
    {
        m_pages.emplace_back(std::make_unique<Page>());
    }

    Arena::~Arena()
    {
        reset();
    }

    void * Arena::allocate(const size_t size, const size_t alignment)
    {
        if (size > Page::kPageSize)
            throw std::bad_alloc();


        Page* page = m_pages.back().get();

        size_t offset = page->offset;
        size_t alignedOffset = (offset + alignment - 1) & ~(alignment - 1);

        if (alignedOffset + size > Page::kPageSize)
        {
            m_pages.emplace_back(std::make_unique<Page>());
            page = m_pages.back().get();
            offset = page->offset;
            alignedOffset = (offset + alignment - 1) & ~(alignment - 1);
            std::cout << "page count: " << m_pages.size() << "\n";
        }

        if (alignedOffset + size > Page::kPageSize)
            throw std::bad_alloc();

        page->offset = alignedOffset + size;

        return &page->data[alignedOffset];
    }

    void Arena::reset()
    {
        for (const auto& dtor : m_dtors)
            dtor();

        m_dtors.clear();

        m_pages.clear();
        m_pages.emplace_back(std::make_unique<Page>());
    }
}
