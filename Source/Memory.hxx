#pragma once

#include <vector>
#include <memory>
#include <cstring>
#include <mutex>
#include <memory_resource>
#include <cstddef>
#include <array>

static constexpr std::size_t HeapSize = 1024 * 1024 * 16;

class CInlineResource final : public std::pmr::memory_resource
{
private:
    struct alignas(alignof(std::max_align_t)) SAllocationHeader
    {
        std::size_t Length{};
        SAllocationHeader* PreviousBlock{};
        SAllocationHeader* NextBlock{};
    };

    alignas(alignof(std::max_align_t))
        std::array<std::byte, HeapSize> Buffer{};
    SAllocationHeader* FirstAllocation{};
    std::pmr::memory_resource* Upstream = std::pmr::new_delete_resource();
    std::mutex Mutex;

    void DestroyBlock(SAllocationHeader* AllocationHeader);

    static constexpr std::size_t DoAlign(std::size_t Num, std::size_t Alignment)
    {
        return (Num + (Alignment - 1)) & ~(Alignment - 1);
    }

    static inline void* AlignPtr(void* Ptr, std::size_t Alignment)
    {
        return (void*)(DoAlign((size_t)Ptr, Alignment));
    }

    static bool IsAlignedPtr(void* Ptr, std::size_t Alignment)
    {
        return ((std::size_t)Ptr & (Alignment - 1)) == 0;
    }

public:
    explicit CInlineResource(std::pmr::memory_resource* up = std::pmr::new_delete_resource());

    ~CInlineResource() final;

    size_t NumberOfBlocks();

    void* do_allocate(size_t bytes, size_t alignment) override;

    void do_deallocate(void* ptr, size_t bytes, size_t alignment) override;

    void* do_reallocate(void* ptr, size_t bytes);

    void* AllocateInline(void* SrcPtr, size_t Bytes, size_t Alignment = alignof(std::max_align_t));

    [[nodiscard]] inline bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
    {
        return this == &other;
    }
};

class CTopmostResource final : public std::pmr::memory_resource
{
    void* do_allocate(size_t Bytes, size_t Align) override;

    void do_deallocate(void* Pointer, size_t Bytes, size_t Align) noexcept override;

    [[nodiscard]] inline bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
    {
        return this == &other;
    }
};

namespace Memory
{
    std::pmr::memory_resource* GetInlineResource();
    std::pmr::memory_resource* GetPoolResource();

    template <typename T>
    inline static std::shared_ptr<T> MakeShared()
    {
        return std::allocate_shared<T, std::pmr::polymorphic_allocator<T>>(GetInlineResource());
    }

    template <typename T>
    inline static auto GetVector()
    {
        return std::pmr::vector<T>(GetPoolResource());
    }

    void* Malloc(size_t Bytes);
    void* Calloc(size_t Num, size_t Bytes);
    void* Realloc(void* Ptr, size_t Bytes);
    void Free(void* Ptr);

    std::size_t NumberOfBlocks();
}
