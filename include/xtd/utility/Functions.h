#pragma once
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <concepts>

#include "xtd/memory/Lifetime.h"

namespace xtd::utility {
    template<typename IteratorSrc, typename IteratorDst>
    requires std::is_pointer_v<IteratorSrc> && std::is_pointer_v<IteratorDst>
    constexpr void copy(IteratorSrc begin, IteratorSrc end, IteratorDst dst) noexcept {
        if (std::is_constant_evaluated()) {
            for(auto it = begin; it < end; ++it, ++dst) *dst = *it;
        }
        else {
            memcpy(dst, begin, end - begin);
        }
    }

    template<typename T>
    constexpr void memset(T* begin, T* end, T val) noexcept {
        if (std::is_constant_evaluated()) {
            for(; begin < end; ++begin) *begin = val;
        }
        else {
            std::memset(begin, val, end - begin);
        }
    }

    template<std::integral T>
    constexpr T alignPowerOf2(T numberToAlign, T alignment) {
        return ( numberToAlign + alignment - 1 ) & ~( alignment - 1 );
    }

    //Undefined for 0
    template<std::integral T>
    constexpr T alignNextPowerOf2(T n) {
            n--;
            n |= n >> 1;
            n |= n >> 2;
            if constexpr(sizeof(T) >= 1) {
                n |= n >> 4;
            }
            if constexpr(sizeof(T) >= 2) {
                n |= n >> 8;
            }
            if constexpr(sizeof(T) >= 4) {
                n |= n >> 16;
            }
            if constexpr(sizeof(T) >= 8) {
                n |= n >> 32;
            }
            if constexpr(sizeof(T) >= 16) {
                n |= n >> 64;
            }
            n++;
            return n;
    }

    // Default constructs
    template<typename IteratorType, typename... Args>
    constexpr IteratorType constructRange(IteratorType begin, IteratorType end, Args&&... args) {
        for(;begin < end; ++begin) xtd::memory::constructAt(begin, std::forward<Args>(args)...);
        return begin;
    }

    template<typename IteratorType, typename SizeType, typename... Args>
    constexpr IteratorType constructRange(IteratorType begin, SizeType size, Args&&... args) {
        return constructRange(begin, begin + size, std::forward<Args>(args)...);
    }

    template<typename IteratorType, typename OtherIterator, typename... Args>
    constexpr IteratorType copyConstructRange(OtherIterator begin, OtherIterator end, IteratorType dst) {
        for(;begin < end; ++begin, ++dst) xtd::memory::copyTo(dst, *begin);
        return dst;
    }

    template<typename IteratorType, typename OtherIterator, typename... Args>
    constexpr IteratorType moveConstructRange(OtherIterator begin, OtherIterator end, IteratorType dst) {
        for(;begin < end; ++begin, ++dst) xtd::memory::moveTo(dst, *begin);
        return dst;
    }

    template<typename IteratorType, typename OtherIterator, typename... Args>
    constexpr IteratorType moveOrCopyConstructRange(OtherIterator begin, OtherIterator end, IteratorType dst) {
        for(;begin < end; ++begin, ++dst) xtd::memory::moveOrCopyTo(dst, *begin);
        return dst;
    }

    template<typename IteratorType>
    constexpr IteratorType destroyRange(IteratorType begin, IteratorType end) {
        validateIterators(begin, end);
        for(;begin < end; ++begin) xtd::memory::destroyAt(begin);
        return begin;
    }

    template<typename IteratorType, typename SizeType>
    constexpr IteratorType destroyRange(IteratorType begin, SizeType size) {
        return destroyRange(begin, begin + size);
    }

    template<typename R, typename L>
    constexpr void swap(R& r, L& l) noexcept(std::is_nothrow_move_constructible_v<R> || (std::is_nothrow_move_assignable_v<R> && std::is_nothrow_move_assignable_v<L>)) {
        if constexpr (std::is_nothrow_move_constructible_v<R> && std::is_nothrow_move_assignable_v<R> && std::is_nothrow_move_assignable_v<L>) {
            auto tmp = std::move(r);
            r = std::move(l);
            l = std::move(tmp);
        } else if(std::is_nothrow_move_constructible_v<R>){
            auto tmp = std::move(r);
            r = l;
            l = tmp;
        } else if(std::is_nothrow_move_assignable_v<R> && std::is_nothrow_move_assignable_v<L>){
            auto tmp = r;
            r = std::move(l);
            l = std::move(tmp);
        }
    }

    template<typename Dst, typename Src>
    requires std::is_assignable_v<Dst, const Src&> || std::is_assignable_v<Dst, Src&&>
    inline void moveOrCopyExisting(Dst& dst, Src& src) {
        if constexpr (std::is_assignable_v<Dst, Src&&>) { return dst = std::move(src); }
        else if constexpr (std::is_assignable_v<Dst, const Src&>) { return dst = src; }
    }
}