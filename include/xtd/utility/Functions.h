#pragma once
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <concepts>

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
}


