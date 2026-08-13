#pragma once
#include <concepts>
#include <stdint.h>

namespace xtd::containers {
    template<typename C>
    concept ContiguousContainerType = requires(C c) {
        typename C::ValueType;
        typename C::SizeType;

        { c.data() } -> std::convertible_to<typename C::ValueType*>;
        { c.size() } -> std::convertible_to<typename C::SizeType>;
    };

    template<typename C>
    concept ArrayContainerType = ContiguousContainerType<C> &&  requires() {
        { C::size() } -> std::convertible_to<typename C::SizeType>;
    };

    template<typename C>
    concept VectorContainerType = ContiguousContainerType<C> &&  requires(C c) {
        { c.capacity() } -> std::convertible_to<typename C::SizeType>;
    };

    template<typename I>
    concept InputIteratorType = requires(const I cit) {
        typename I::ValueType;
        { *cit } -> std::convertible_to<const typename I::ValueType>;
        { cit.operator->() } -> std::convertible_to<const typename I::ValueType*>;
    };

    template<typename I>
    concept OutputIteratorType = InputIteratorType<I> && requires(I it) {
        typename I::ValueType;
        { *it } -> std::convertible_to<typename I::ValueType>;
        { it.operator->() } -> std::convertible_to<typename I::ValueType*>;
    };

    template<typename I>
    concept ComparableIteratorType = requires(I it) {
        typename I::ValueType;
        { it == it } -> std::convertible_to<bool>;
        { it != it } -> std::convertible_to<bool>;
    };

    template<typename I>
    concept ForwardIteratorType = InputIteratorType<I> && ComparableIteratorType<I> && requires(I it) {
        { ++it } -> std::same_as<I&>;
        { it++ } -> std::convertible_to<I>;
    };

    template<typename I>
    concept BackwardIteratorType = InputIteratorType<I> && ComparableIteratorType<I> && requires(I it) {
        { --it } -> std::same_as<I&>;
        { it-- } -> std::convertible_to<I>;
    };

    template<typename I>
    concept BidirectionalIteratorType = ForwardIteratorType<I> && BackwardIteratorType<I>;

    template<typename I>
    concept RandomAccessIteratorType = BidirectionalIteratorType<I> && requires(I it, size_t val) {
        { it + val } -> std::convertible_to<I>;
        { it - val } -> std::convertible_to<I>;

        { it += val } -> std::same_as<I&>;
        { it -= val } -> std::same_as<I&>;

        { it[val] } -> std::convertible_to<typename I::ValueType>;

        { it < it } -> std::convertible_to<bool>;
        { it > it } -> std::convertible_to<bool>;
        { it <= it } -> std::convertible_to<bool>;
        { it >= it } -> std::convertible_to<bool>;
    };

    template<typename I>
    concept ContiguousIteratorType = RandomAccessIteratorType<I> && requires(I it) {
        typename I::ContiguousIteratorTag;
        { it + it } -> std::convertible_to<I>;
        { it - it } -> std::convertible_to<I>;
    };
}