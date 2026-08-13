#pragma once
#include <stdint.h>
#include <new>
#include <utility>

#include "xtd/allocators/Traits.h"

namespace xtd::allocators {
    struct AllocatorNew;

    // Allocator - Deallocator pairs are required to have this structure to work with types like UniquePtr (can also implement deallocate separately for NewAllocator)
    struct DeallocatorNew {
        using Allocator = AllocatorNew;
        inline void deallocate(void* ptr, size_t size, size_t align) noexcept {
            ::operator delete(ptr, size, std::align_val_t(align));
        }
    };

    struct AllocatorNew : public DeallocatorNew {
        using Deallocator = DeallocatorNew;
        [[nodiscard]]
        inline void* allocate(size_t size, size_t align) noexcept {
            return (::operator new(size, std::align_val_t(align), std::nothrow));
        }
        inline Deallocator deallocator() { return Deallocator(); } 
    };


    // Allocates AND constructs the objects
    struct TypedAllocatorNew;

    struct TypedDeallocatorNew {
        using Allocator = TypedAllocatorNew;
        template<typename T>
        inline void deallocate(T* ptr) noexcept { delete ptr; }
    };

    struct TypedAllocatorNew {
        using Deallocator = TypedDeallocatorNew;

        template<typename T, typename... Args>
        [[nodiscard]]
        inline std::pair<Deallocator, T*> allocate(Args&&... args) noexcept {
            return std::make_pair<Deallocator, T*>(Deallocator(), new T(std::forward<Args>(args)...));
        }
    };

    struct TypedArrayAllocatorNew;

    struct TypedArrayDeallocatorNew {
        using Allocator = TypedArrayAllocatorNew;
        template<typename T>
        inline void deallocate(T* ptr) noexcept { delete[] ptr; }
    };

    struct TypedArrayAllocatorNew {
        using Deallocator = TypedArrayDeallocatorNew;

        template<typename T, typename... Args>
        [[nodiscard]]
        inline std::pair<Deallocator, T*> allocate(size_t size, Args&&... args) noexcept {
            return std::make_pair<Deallocator, T*>(Deallocator(), new T[size](std::forward<Args>(args)...));
        }
    };

    static_assert(TypedDeallocatorT<TypedDeallocatorNew>);
    static_assert(TypedAllocatorT<TypedAllocatorNew>);
    static_assert(TypedDeallocatorT<TypedArrayDeallocatorNew>);
    static_assert(TypedArrayAllocatorT<TypedArrayAllocatorNew>);
}