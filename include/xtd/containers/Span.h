#pragma once
#include "xtd/debug/Checks.h"
#include "xtd/containers/Traits.h"
#include "xtd/containers/PointerIterators.h"

#include <limits>
#include <utility>

namespace xtd::containers {

    static constexpr size_t spanDynamicSize = std::numeric_limits<size_t>::max();

    template<typename T, size_t _size = spanDynamicSize>
    class Span;

    // Bolt this on to have span integration, Bases can be empty
    template<typename Derived, typename T, typename S, typename IC, typename... Bases>
    class SpanFunctions : public Bases... {
    private:
        constexpr Derived& derived() { return *static_cast<Derived*>(this); }
        constexpr const Derived& derived() const { return *static_cast<const Derived*>(this); }
    public:
        using ValueType = T;
        using SizeType = S;
        using ConstIteratorType = IC;

        constexpr auto subspan(ConstIteratorType begin, ConstIteratorType end) {
            XTD_DEBUG_VERIFY(begin >= derived().begin() && begin <= derived().end(), "using iterators that dont belong to this span to make a subspan");
            XTD_DEBUG_VERIFY(end >= derived().begin() && end <= derived().end(), "using iterators that dont belong to this span to make a subspan");
            XTD_DEBUG_VERIFY(begin <= end, "end iterator must be greater than or equal to the begin iterator");
            return Span<ValueType>(begin, end); 
        }
        constexpr auto subspan(SizeType pos, SizeType size) {
            return subspan(derived().begin() + pos, derived().begin() + pos + size);
        }

        constexpr auto subspan(ConstIteratorType begin, SizeType size) {
            return subspan(begin, begin + size);
        }

        template<SizeType size>
        constexpr auto subspan(ConstIteratorType begin) {
            XTD_DEBUG_VERIFY(begin >= derived().begin() && begin <= derived().end(), "using iterators that dont blong to this span to make a subspan");
            XTD_DEBUG_VERIFY(derived().begin() + size <= derived().end(), "out of bounds size while trying to make a subspan");
            return Span<ValueType, size>(begin); 
        }

        template<SizeType size>
        constexpr auto subspan(SizeType pos) {
            return subspan<size>(derived().begin() + pos);
        }

        constexpr auto subspan() {
            return Span<ValueType>(derived().begin(), derived().end());
        }
    };

    template<typename T, size_t _size>
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

        template<std::size_t size>
        requires(size >= _size)
        constexpr Span(ValueType (&data)[size]) noexcept : _begin(data) {}

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

        constexpr ValueType& back() { return end()[-1]; }
        constexpr const ValueType& back() const { return end()[-1]; }

        constexpr ValueType& front() { return _begin[0]; }
        constexpr const ValueType& front() const { return _begin[0]; }
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
    
        constexpr Span(PointerType begin, SizeType size) noexcept : _begin(begin), _end(begin + size) {}

        template<std::size_t size>
        constexpr Span(ValueType (&data)[size]) noexcept : _begin(data), _end(data + size) {}

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

        constexpr ValueType& back() { return _end[-1]; }
        constexpr const ValueType& back() const { return _end[-1]; }

        constexpr ValueType& front() { return _begin[0]; }
        constexpr const ValueType& front() const { return _begin[0]; }
    };

    static_assert(ContiguousContainerType<Span<int>>, "Span must satisfy ContiguousContainerType");
}


