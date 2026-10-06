#pragma once
#include "xtd/containers/Traits.h"
#include "xtd/containers/Iterators.h"
#include "xtd/containers/Span.h"

#include "xtd/debug/Checks.h"
#include "xtd/utility/Functions.h"

#include <initializer_list>
#include <type_traits>
#include <utility>

namespace xtd::containers {
    template<typename T, size_t _size, size_t _alignment = alignof(T)>
    class Array : public SpanFunctions<Array<T, _size, _alignment>, T, size_t, ContiguousIteratorBase<const T>> {
    public:
        static_assert(_alignment != 0 && (_alignment & (_alignment - 1)) == 0, "Alignment must be a power of two");
        using ValueType = T;
        using SizeType = size_t;
        using PointerType = ValueType*;
        using IteratorType = ContiguousIteratorBase<ValueType>;
        using ConstIteratorType = ContiguousIteratorBase<const ValueType>;

        using Self = Array<ValueType, _size, _alignment>;
        using Base = SpanFunctions<Self, ValueType, SizeType, ConstIteratorType>;

    private:
        alignas(_alignment) T _data[_size];

    public:
        constexpr Array() noexcept = default;
        constexpr ~Array() noexcept = default;
        constexpr Array(const Array&) noexcept = default;
        constexpr Array(Array&&) noexcept = default;
        constexpr Array& operator=(const Array&) noexcept = default;
        constexpr Array& operator=(Array&&) noexcept = default;

        constexpr Array(std::initializer_list<ValueType> list) noexcept {
            XTD_DEBUG_VERIFY(list.size() <= size(), "list size must be smaller than or equal array size");
            xtd::utility::copy(list.begin(), list.end(), _data);
        }

        constexpr Array& operator=(std::initializer_list<ValueType> list) noexcept {
            XTD_DEBUG_VERIFY(list.size() <= size(), "list size must be smaller than or equal array size");
            list = {};
            xtd::utility::copy(list.begin(), list.end(), _data);
            return *this;
        }

        template<typename... Args>
        constexpr Array(Args&&... args) noexcept {
            XTD_DEBUG_VERIFY(sizeof...(args) <= size(), "Args count must be smaller than or equal array size");
            _data = { args... }; 
        }

        template<typename... Args>
        constexpr Array& operator=(Args&&... args) noexcept {
            XTD_DEBUG_VERIFY(sizeof...(args) <= size(), "Args count must be smaller than or equal array size");
            _data = { args... }; 
            return *this;
        }

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
        
        static constexpr inline SizeType size() noexcept { return _size; }
        constexpr inline PointerType data() { return _data; }
        constexpr inline const PointerType data() const { return _data; }

        constexpr ValueType& back() { return _data[_size - 1]; }
        constexpr const ValueType& back() const { return _data[_size - 1]; }

        constexpr ValueType& front() { return _data[0]; }
        constexpr const ValueType& front() const { return _data[0]; }
    };

    static_assert(ArrayContainerType<Array<int, 1>>, "Array must satisfy ArrayContainerType");
}