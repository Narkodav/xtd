#pragma once
#include "xtd/containers/Traits.h"
#include "xtd/containers/Iterators.h"
#include "xtd/containers/Span.h"

#include "xtd/debug/Checks.h"
#include <utility>

namespace xtd::containers {
    template<typename T, size_t _size>
    class Array : public SpanFunctions<Array<T, _size>, T, size_t, ContiguousIteratorBase<const T>> {
    public:
        using ValueType = T;
        using SizeType = size_t;
        using PointerType = ValueType*;
        using IteratorType = ContiguousIteratorBase<ValueType>;
        using ConstIteratorType = ContiguousIteratorBase<const ValueType>;

        using Self = Array<ValueType, _size>;
        using Base = SpanFunctions<Self, ValueType, SizeType, ConstIteratorType>;

    private:
        T _data[_size];

    public:
        constexpr Array() noexcept = default;
        constexpr ~Array() noexcept = default;
        constexpr Array(const Array&) noexcept = default;
        constexpr Array(Array&&) noexcept = default;
        constexpr Array& operator=(const Array&) noexcept = default;
        constexpr Array& operator=(Array&&) noexcept = default;

        constexpr inline ValueType& operator[](SizeType i) noexcept { return const_cast<ValueType&>(std::as_const(*this)[i]); }
        constexpr inline const ValueType& operator[](SizeType i) const noexcept {
            XTD_DEBUG_VERIFY(i < _size, "out of bounds access to Array data");
            return _data[i];
        }

        constexpr inline IteratorType begin() noexcept { return IteratorType(_data); }
        constexpr inline IteratorType end() noexcept { return IteratorType(_data + _size); }
        constexpr inline ConstIteratorType begin() const noexcept { return ConstIteratorType(_data); }
        constexpr inline ConstIteratorType end() const noexcept { return ConstIteratorType(_data + _size); }
        constexpr inline ConstIteratorType cbegin() const noexcept { return begin(); }
        constexpr inline ConstIteratorType cend() const noexcept { return end(); }
        
        static inline SizeType size() noexcept { return _size; }
        inline PointerType data() { return _data; }
        inline const PointerType data() const { return _data; }

        ValueType& back() { return _data[_size - 1]; }
        const ValueType& back() const { return _data[_size - 1]; }

        ValueType& front() { return _data[0]; }
        const ValueType& front() const { return _data[0]; }
    };

    static_assert(ArrayContainerType<Array<int, 1>>, "Array must satisfy ArrayContainerType");
}


