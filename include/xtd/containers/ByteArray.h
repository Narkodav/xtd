#pragma once
#include "xtd/containers/Array.h"

#include "xtd/debug/Checks.h"
#include "xtd/utility/Functions.h"
#include "xtd/Macros.h"

#include <initializer_list>
#include <type_traits>
#include <utility>

namespace xtd::utility {
    using U16Alias = uint16_t XTD_MAY_ALIAS;
    using U32Alias = uint32_t XTD_MAY_ALIAS;
    using U64Alias = uint64_t XTD_MAY_ALIAS;

    template<typename T>
    struct Alias;

    template<>
    struct Alias<uint8_t> { using Type = uint8_t; };

    template<>
    struct Alias<uint16_t> { using Type = U16Alias; };

    template<>
    struct Alias<uint32_t> { using Type = U32Alias; };

    template<>
    struct Alias<uint64_t> { using Type = U64Alias; };

    template<typename T>
    using AliasType = typename Alias<T>::Type;
}

namespace xtd::containers {
    template<size_t _size, size_t _alignment>
    class ByteArray : public Array<std::byte, _size, _alignment> {
    public:
        using Self = ByteArray<_size, _alignment>;
        using Base = Array<std::byte, _size, _alignment>;

        using ValueType = typename Base::ValueType;
        using SizeType = typename Base::SizeType;
        using PointerType = typename Base::PointerType;
        using IteratorType = typename Base::IteratorType;
        using ConstIteratorType = typename Base::ConstIteratorType;

        template<typename T>
        using AliasIteratorType = ContiguousIteratorBase<xtd::utility::AliasType<T>>;

        template<typename T>
        using AliasConstIteratorType = ContiguousIteratorBase<const xtd::utility::AliasType<T>>;

        // Explicit wide views
        template <typename T>
        static constexpr inline SizeType size() { 
            static_assert(alignof(T) <= _alignment, "The view type alignment must be smaller or equal to the array alignment");
            return size() / sizeof(T); 
        }

        template <typename T>
        constexpr inline xtd::utility::AliasType<T>* data() {
            static_assert(alignof(T) <= _alignment, "The view type alignment must be smaller or equal to the array alignment");
            static_assert(size() > 0, "The view type size must be smaller than total byte size");
            return reinterpret_cast<xtd::utility::AliasType<T>*>(data());
        }
        template <typename T>
        constexpr inline const xtd::utility::AliasType<T>* data() const {
            static_assert(alignof(T) <= _alignment, "The view type alignment must be smaller or equal to the array alignment");
            static_assert(size() > 0, "The view type size must be smaller than total byte size");
            return reinterpret_cast<const xtd::utility::AliasType<T>*>(data());
        }

        // Restrict means that the container data can only be accessed through the returned pointer until its lifetime ends
        template <typename T>
        constexpr inline xtd::utility::AliasType<T>* XTD_RESTRICT dataRestrict() {
            static_assert(alignof(T) <= _alignment, "The view type alignment must be smaller or equal to the array alignment");
            static_assert(size() > 0, "The view type size must be smaller than total byte size");
            return reinterpret_cast<xtd::utility::AliasType<T>* XTD_RESTRICT>(data());
        }
        template <typename T>
        constexpr inline const xtd::utility::AliasType<T>* XTD_RESTRICT dataRestrict() const {
            static_assert(alignof(T) <= _alignment, "The view type alignment must be smaller or equal to the array alignment");
            static_assert(size() > 0, "The view type size must be smaller than total byte size");
            return reinterpret_cast<const xtd::utility::AliasType<T>* XTD_RESTRICT>(data());
        }

        template <typename T>
        constexpr inline AliasIteratorType<T> begin() noexcept { return AliasIteratorType<T>(data<T>()); }

        template <typename T>
        constexpr inline AliasIteratorType<T> end() noexcept { return AliasIteratorType<T>(data<T>() + size<T>()); }

        template <typename T>
        constexpr inline AliasConstIteratorType<T> begin() const noexcept { return AliasConstIteratorType<T>(data<T>()); }

        template <typename T>
        constexpr inline AliasConstIteratorType<T> end() const noexcept { return AliasConstIteratorType<T>(data<T>() + size<T>()); }

        template <typename T>
        constexpr inline AliasConstIteratorType<T> cbegin() const noexcept { return begin<T>(); }

        template <typename T>
        constexpr inline AliasConstIteratorType<T> cend() const noexcept { return end<T>(); }

        template <typename T>
        auto subspan(AliasConstIteratorType<T>&& b, AliasConstIteratorType<T>&& e) {
            XTD_DEBUG_VERIFY(b >= begin<T>() && b < end<T>(), "using iterators that dont blong to this span to make a subspan");
            XTD_DEBUG_VERIFY(e >= begin<T>() && e < end<T>(), "using iterators that dont blong to this span to make a subspan");
            return Span<xtd::utility::AliasType<T>>(b, e);
        }

        // template <typename T>
        // auto subspan(SizeType pos, SizeType size) {
        //     XTD_DEBUG_VERIFY(pos + size < derived().size(), "out of bounds while trying to make a subspan");
        //     auto begin = derived().begin() + pos;
        //     return Span<ValueType>(begin, begin + size); 
        // }

        // auto subspan(ConstIteratorType begin, SizeType size) {
        //     XTD_DEBUG_VERIFY(begin >= derived().begin() && begin < derived().end(), "using iterators that dont blong to this span to make a subspan");
        //     XTD_DEBUG_VERIFY(derived().begin() + size < derived().end(), "out of bounds size while trying to make a subspan");
        //     return Span<ValueType>(begin, begin + size); 
        // }

        // template<SizeType size>
        // auto subspan(SizeType pos) {
        //     XTD_DEBUG_VERIFY(pos + size < derived().size(), "out of bounds while trying to make a subspan");
        //     auto begin = derived().begin() + pos;
        //     return Span<ValueType, size>(begin); 
        // }
        // template<SizeType size>
        // auto subspan(ConstIteratorType begin) {
        //     XTD_DEBUG_VERIFY(begin >= derived().begin() && begin < derived().end(), "using iterators that dont blong to this span to make a subspan");
        //     XTD_DEBUG_VERIFY(derived().begin() + size < derived().end(), "out of bounds size while trying to make a subspan");
        //     return Span<ValueType, size>(begin); 
        // }

        // auto subspan() {
        //     return Span<ValueType>(derived().begin(), derived().end());
        // }
    };

}