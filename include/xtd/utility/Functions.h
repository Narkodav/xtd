#pragma once
#include <cstdint>
#include <cstring>
#include <type_traits>

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
}


