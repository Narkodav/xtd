#pragma once
#include <new>

namespace xtd::memory {
        template<typename Dst, typename Src>
        requires std::is_constructible_v<Dst, const Src&> || std::is_constructible_v<Dst, Src&&>
        inline Dst* moveOrCopyTo(Dst* dst, Src& src) {
            if constexpr (std::is_constructible_v<Dst, Src&&>) { return moveTo(dst, src); }
            else if constexpr (std::is_constructible_v<Dst, const Src&>) { return copyTo(dst, src); }
        }

        template<typename Dst, typename Src> requires std::is_constructible_v<Dst, Src&&>
        inline Dst* moveTo(Dst* dst, Src& src) { return ::new (dst) Dst(std::move(src)); }

        template<typename Dst, typename Src> requires std::is_constructible_v<Dst, const Src&>
        inline Dst* copyTo(Dst* dst, Src& src) { return ::new (dst) Dst(src); }

        template<typename Dst, typename... Args> requires(std::is_constructible_v<Dst, Args...>)
        inline Dst* constructAt(Dst* dst, Args&&... args) { return ::new (dst) Dst(std::forward<Args>(args)...); }

        template<typename Dst>
        inline void destroyAt(Dst* dst) { dst->~Dst(); }
}