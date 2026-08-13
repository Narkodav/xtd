#pragma once
#include <concepts>
#include <stdint.h>
#include <utility>

namespace xtd::allocators {

    template<typename D>
    concept DeallocatorT = requires(D d, void* ptr, size_t size, size_t align) {
        typename D::Allocator;
        { d.deallocate(ptr, size, align) } -> std::same_as<void>;
    } && std::same_as<typename D::Allocator::Deallocator, D>;

    // Move operator must leave the moved from allocator in a valid state
    template<typename A>
    concept AllocatorT = requires(A a, void* ptr, size_t size, size_t align) {
        typename A::Deallocator;
        requires DeallocatorT<typename A::Deallocator>;
        { a.allocate(size, align) } -> std::convertible_to<void*>;
        { a.deallocate(ptr, size, align) } -> std::same_as<void>;
        { a.deallocator() } -> std::convertible_to<typename A::Deallocator>;
    } && std::same_as<typename A::Deallocator::Allocator, A>;

    // A deallocator must expose the Allocator it pairs with, and a noexcept
    // templated deallocate(T*) -> void. (This shape is identical for both the
    // scalar and array deallocators, so one concept covers both.)
    template<typename D>
    concept TypedDeallocatorT = requires(D d, int* p) {
        typename D::Allocator;
        { d.template deallocate<int>(p) } noexcept -> std::same_as<void>;
    } && std::same_as<typename D::Allocator::Deallocator, D>;

    // A scalar allocator must expose its Deallocator type (which must itself
    // satisfy TypedDeallocator), produce a noexcept templated
    // allocate<T>(Args...) -> pair<Deallocator, T*>, and round-trip back to
    // itself via Deallocator::Allocator.
    template<typename A>
    concept TypedAllocatorT =
        requires(A a) {
            typename A::Deallocator;
            requires TypedDeallocatorT<typename A::Deallocator>;
            { a.template allocate<int>() } noexcept -> std::same_as<std::pair<typename A::Deallocator, int*>>;
        } && std::same_as<typename A::Deallocator::Allocator, A>;

    // Array allocator: same idea, but allocate() also takes a size_t.
    template<typename A>
    concept TypedArrayAllocatorT =
        requires(A a, size_t n) {
            typename A::Deallocator;
            requires TypedDeallocatorT<typename A::Deallocator>;
            { a.template allocate<int>(n) } noexcept -> std::same_as<std::pair<typename A::Deallocator, int*>>;
        } && std::same_as<typename A::Deallocator::Allocator, A>;
}