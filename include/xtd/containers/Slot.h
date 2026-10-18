#pragma once
#include "xtd/memory/Lifetime.h"
#include <type_traits>

namespace xtd::containers {
    template<typename T>
    class alignas(T) Slot {
    public:
        using ValueType = T;
        using PointerType = ValueType*;
    private:
        alignas(T) std::byte _storage[sizeof(T)];
    public:
        constexpr Slot() = default;
        constexpr ~Slot() = default;

        template<typename Src>
        constexpr ValueType& copy(Src& value) noexcept { return *xtd::memory::copyTo(data(), value); }

        template<typename Src>
        constexpr ValueType& move(Src& value) noexcept { return *xtd::memory::moveTo(data(), value); }

        template<typename Src>
        constexpr ValueType& moveOrCopy(Src& value) noexcept { return *xtd::memory::moveOrCopyTo(data(), value); }

        template <typename... Args>
        constexpr ValueType& construct(Args&&... args) noexcept {
            return *xtd::memory::constructAt(data(), std::forward<Args>(args)...);
        }

        constexpr void destroy() noexcept { xtd::memory::destroyAt(data()); }

        constexpr PointerType data() noexcept { return reinterpret_cast<PointerType>(_storage); }
        constexpr const PointerType data() const noexcept { return reinterpret_cast<const PointerType>(_storage); }

        constexpr ValueType& operator*() noexcept { return *data(); }
        constexpr const ValueType& operator*() const noexcept { return *data(); }

        constexpr ValueType& operator->() noexcept { return data(); }
        constexpr const ValueType& operator->() const noexcept { return data(); }
    };

    template <typename T>
    inline constexpr bool slotLayoutOkay =
        sizeof(Slot<T>)   == sizeof(T) &&
        alignof(Slot<T>)  == alignof(T) &&
        std::is_standard_layout_v<Slot<T>>;

    static_assert(slotLayoutOkay<uint64_t>, "Slot has incompatible layout");
}


