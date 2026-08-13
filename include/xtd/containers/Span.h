#pragma once
#include "xtd/debug/Checks.h"
#include "xtd/containers/Traits.h"
#include "xtd/containers/Iterators.h"

#include <limits>
#include <utility>

namespace xtd::containers {

    static constexpr size_t spanDynamicSize = std::numeric_limits<size_t>::max();

    // Bolt this on to have span integration, Bases can be empty
    template<typename Derived, typename T, typename S, typename IC, typename... Bases>
    class SpanFunctions : public Bases... {
    private:
        constexpr Derived& derived() { return static_cast<Derived*>(this); }
        constexpr const Derived& derived() const { return static_cast<const Derived*>(this); }
    public:
        using ValueType = T;
        using SizeType = S;
        using ConstIteratorType = IC;

        auto subspan(ConstIteratorType&& begin, ConstIteratorType&& end) {
            XTD_DEBUG_VERIFY(begin >= derived().begin() && begin < derived().end(), "using iterators that dont blong to this span to make a subspan");
            XTD_DEBUG_VERIFY(end >= derived().begin() && end < derived().end(), "using iterators that dont blong to this span to make a subspan");
            return Span<ValueType>(begin, end); 
        }
        auto subspan(SizeType pos, SizeType size) {
            XTD_DEBUG_VERIFY(pos + size < derived().size(), "out of bounds while trying to make a subspan");
            auto begin = derived().begin() + pos;
            return Span<ValueType>(begin, begin + size); 
        }

        auto subspan(ConstIteratorType begin, SizeType size) {
            XTD_DEBUG_VERIFY(begin >= derived().begin() && begin < derived().end(), "using iterators that dont blong to this span to make a subspan");
            XTD_DEBUG_VERIFY(derived().begin() + size < derived().end(), "out of bounds size while trying to make a subspan");
            return Span<ValueType>(begin, begin + size); 
        }

        template<SizeType size>
        auto subspan(SizeType pos) {
            XTD_DEBUG_VERIFY(pos + size < derived().size(), "out of bounds while trying to make a subspan");
            auto begin = derived().begin() + pos;
            return Span<ValueType, size>(begin); 
        }
        template<SizeType size>
        auto subspan(ConstIteratorType begin) {
            XTD_DEBUG_VERIFY(begin >= derived().begin() && begin < derived().end(), "using iterators that dont blong to this span to make a subspan");
            XTD_DEBUG_VERIFY(derived().begin() + size < derived().end(), "out of bounds size while trying to make a subspan");
            return Span<ValueType, size>(begin); 
        }
    };

    template<typename T, size_t _size = spanDynamicSize>
    class Span : public SpanFunctions<Span<T, _size>, T, size_t, ContiguousIteratorBase<const T>> {
    public:
        using ValueType = T;
        using SizeType = size_t;
        using PointerType = ValueType*;
        using IteratorType = ContiguousIteratorBase<ValueType>;
        using ConstIteratorType = ContiguousIteratorBase<const ValueType>;
        using Self = Span<T, spanDynamicSize>;
        using Base = SpanFunctions<Self, ValueType, SizeType, ConstIteratorType>;

    private:
        T* _begin;

    public:
        constexpr Span() noexcept = default;
        constexpr ~Span() noexcept = default;
        constexpr Span(const Span&) noexcept = default;
        constexpr Span(Span&&) noexcept = default;
        constexpr Span& operator=(const Span&) noexcept = default;
        constexpr Span& operator=(Span&&) noexcept = default;

        template<ArrayContainerType C>
        constexpr Span(const C& other) noexcept requires(C::size() == _size) : _begin(other.data()) {}

        template<ContiguousIteratorType IteratorT>
        constexpr Span(IteratorT begin) noexcept : _begin(begin) {}

        constexpr inline ValueType& operator[](SizeType i) noexcept { return const_cast<ValueType&>(std::as_const(*this)[i]); }
        constexpr inline const ValueType& operator[](SizeType i) const noexcept {
            XTD_DEBUG_VERIFY(i < _size, "out of bounds access to Span data");
            return _begin[i];
        }

        constexpr inline IteratorType begin() noexcept { return IteratorType(_begin); }
        constexpr inline IteratorType end() noexcept { return IteratorType(_begin + _size); }
        constexpr inline ConstIteratorType begin() const noexcept { return ConstIteratorType(_begin); }
        constexpr inline ConstIteratorType end() const noexcept { return ConstIteratorType(_begin + _size); }
        constexpr inline ConstIteratorType cbegin() const noexcept { return begin(); }
        constexpr inline ConstIteratorType cend() const noexcept { return end(); }
        
        static constexpr inline SizeType size() noexcept { return _size; }
        constexpr inline PointerType data() { return _begin; }
        constexpr inline const PointerType data() const { return _begin; }

        ValueType& back() { return end()[-1]; }
        const ValueType& back() const { return end()[-1]; }

        ValueType& front() { return _begin[0]; }
        const ValueType& front() const { return _begin[0]; }
    };

    static_assert(ArrayContainerType<Span<int, 1>>, "Sized Span must satisfy ArrayContainerType");

    template<typename T>
    class Span<T, spanDynamicSize> : public SpanFunctions<Span<T, spanDynamicSize>, T, size_t, ContiguousIteratorBase<const T>> {
    public:
        using ValueType = T;
        using SizeType = size_t;
        using PointerType = ValueType*;
        using IteratorType = ContiguousIteratorBase<ValueType>;
        using ConstIteratorType = ContiguousIteratorBase<const ValueType>;
        using Self = Span<T, spanDynamicSize>;
        using Base = SpanFunctions<Self, ValueType, SizeType, ConstIteratorType>;

    private:
        T* _begin;
        T* _end;

    public:
        constexpr Span() noexcept = default;
        constexpr ~Span() noexcept = default;
        constexpr Span(const Span&) noexcept = default;
        constexpr Span(Span&&) noexcept = default;
        constexpr Span& operator=(const Span&) noexcept = default;
        constexpr Span& operator=(Span&&) noexcept = default;

        template<ContiguousContainerType C>
        constexpr Span(const C& other) noexcept : _begin(other.data()), _end(_begin + other.size()) {}

        template<ContiguousIteratorType IteratorT>
        constexpr Span(IteratorT begin, IteratorT end) noexcept : _begin(begin), _end(end) {}
    
        template<ContiguousIteratorType IteratorT>
        constexpr Span(ValueType begin, SizeType size) noexcept : _begin(begin), _end(begin + size) {}

        constexpr inline ValueType& operator[](SizeType i) noexcept { return const_cast<ValueType&>(std::as_const(*this)[i]); }
        constexpr inline const ValueType& operator[](SizeType i) const noexcept {
            XTD_DEBUG_VERIFY(_begin + i < _end, "out of bounds access to Span data");
            return _begin[i];
        }

        constexpr inline IteratorType begin() noexcept { return IteratorType(_begin); }
        constexpr inline IteratorType end() noexcept { return IteratorType(_end); }
        constexpr inline ConstIteratorType begin() const noexcept { return ConstIteratorType(_begin); }
        constexpr inline ConstIteratorType end() const noexcept { return ConstIteratorType(_end); }
        constexpr inline ConstIteratorType cbegin() const noexcept { return begin(); }
        constexpr inline ConstIteratorType cend() const noexcept { return end(); }
        
        constexpr inline SizeType size() const noexcept { return _end - _begin; }
        constexpr inline PointerType data() { return _begin; }
        constexpr inline const PointerType data() const { return _begin; }

        ValueType& back() { return _end[-1]; }
        const ValueType& back() const { return _end[-1]; }

        ValueType& front() { return _begin[0]; }
        const ValueType& front() const { return _begin[0]; }
    };

    static_assert(ContiguousContainerType<Span<int>>, "Span must satisfy ContiguousContainerType");
}


