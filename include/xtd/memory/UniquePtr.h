#pragma once
#include "xtd/allocators/NewAllocators.h"
#include "xtd/allocators/Traits.h"
#include "xtd/debug/Checks.h"
#include "xtd/containers/Iterators.h"
#include "xtd/containers/Span.h"

#include "xtd/Concepts.h"

#include <initializer_list>
#include <type_traits>
#include <utility>

namespace xtd::memory {
    template<typename T, xtd::allocators::TypedDeallocatorT D = 
        std::conditional_t<std::is_array_v<T>, xtd::allocators::TypedArrayDeallocatorNew, xtd::allocators::TypedDeallocatorNew>>
    class UniquePtr {
    public:
        using ValueType = T;
        using SizeType = size_t;
        using PointerType = ValueType*;
        using DeallocatorType = D;
        using AllocatorType = typename DeallocatorType::Allocator;
        using Self = UniquePtr<ValueType, DeallocatorType>;

        struct RawParts {
            PointerType data;
            [[no_unique_address]] DeallocatorType dealloc;

            void cleanup() { dealloc.deallocate(data); }
        };

    private:
        PointerType _data;
        [[no_unique_address]] DeallocatorType _dealloc;

    public:
        UniquePtr() requires(std::is_default_constructible_v<DeallocatorType>) : _data(nullptr), _dealloc(DeallocatorType()) {}

        UniquePtr(PointerType data) requires(std::is_default_constructible_v<DeallocatorType>) 
            : _data(data), _dealloc(DeallocatorType()) {}

        template<xtd::allocators::DeallocatorT U>
        UniquePtr(PointerType data, U&& dealloc) requires(std::convertible_to<D, DeallocatorType>) 
            : _data(data), _dealloc(std::forward<U>(dealloc)) {}

        UniquePtr(const UniquePtr&) = delete;
        UniquePtr& operator=(const UniquePtr&) = delete;

        UniquePtr(UniquePtr&& other) requires (std::move_constructible<DeallocatorType>)
        : _data(std::exchange(other._data, nullptr)), _dealloc(std::move(other._dealloc)) {}

        UniquePtr& operator=(UniquePtr&& other) requires (std::is_move_assignable_v<DeallocatorType>) {
            if(this == &other) return *this;

            reset();

            _dealloc = std::move(other._dealloc);
            _data = std::exchange(other._data, nullptr);

            return *this;
        }

        ~UniquePtr() { reset(); }

        template<typename U>
        explicit operator UniquePtr<U, DeallocatorType>() noexcept { 
            return UniquePtr<U, DeallocatorType>(
                std::exchange(static_cast<U*>(_data), nullptr), std::move(_dealloc)); 
        }

        template<typename U>
        explicit operator UniquePtr<const U, DeallocatorType>() const noexcept { 
            return UniquePtr<const U, DeallocatorType>(
                std::exchange(static_cast<const U*>(_data), nullptr), std::move(_dealloc)); 
        }

        inline PointerType data() { return _data; }
        inline const PointerType data() const { return _data; }

        DeallocatorType& deallocator() { return _dealloc; }
        const DeallocatorType& deallocator() const { return _dealloc; }
    
        void reset() {
            if(_data == nullptr) return;
            dealloc();
            _data = nullptr;
        }

        ValueType& operator*() {
            XTD_DEBUG_VERIFY(_data != nullptr, "Dereferencing a nullptr");
            return *_data;
        }
        const ValueType& operator*() const  {
            XTD_DEBUG_VERIFY(_data != nullptr, "Dereferencing a nullptr");
            return *_data;
        }

        PointerType operator->() {
            XTD_DEBUG_VERIFY(_data != nullptr, "Dereferencing a nullptr");
            return _data;
        }
        const PointerType operator->() const  {
            XTD_DEBUG_VERIFY(_data != nullptr, "Dereferencing a nullptr");
            return _data;
        }

        RawParts release() {
            XTD_DEBUG_VERIFY(_data != nullptr, "Releasing a nullptr");
            RawParts parts { _data, std::move(_dealloc) };
            _data = nullptr;
            return parts;
        }

        void assign(PointerType data, DeallocatorType&& d) {
            reset();
            _data = data;
            _dealloc = std::move(d);
        }

        template<typename... Args>
        static Self make(Args&&... args) requires std::is_default_constructible_v<AllocatorType> {
            auto alloc = AllocatorType();
            return make(alloc, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static Self make(AllocatorType& alloc, Args&&... args) {
            auto* data = reinterpret_cast<ValueType*>(alloc.allocate(sizeof(ValueType), alignof(ValueType)));
            ::new (data) ValueType(std::forward<Args>(args)...);
            return Self(data, alloc.deallocator());
        }

        std::strong_ordering operator<=>(const Self& other) const { return _data <=> other._data; }
        bool operator==(const Self& other) const { return _data == other._data; }

        template<typename U>
        std::strong_ordering operator<=>(const U* other) const { return _data <=> other; }
        template<typename U>
        bool operator==(const U* other) const { return _data == other; }

        std::strong_ordering operator<=>(std::nullptr_t) const { return _data <=> nullptr; }
        bool operator==(std::nullptr_t) const { return _data == nullptr; }

    private:
        inline void dealloc() { _dealloc.deallocate(_data); }
    };


    template<typename T, xtd::allocators::DeallocatorT D>
    class UniquePtr<T[], D> : public xtd::containers::SpanFunctions<UniquePtr<T[], D>, T, size_t, xtd::containers::ContiguousIteratorBase<const T>>  {
    public:
        using ValueType = T;
        using SizeType = size_t;
        using PointerType = ValueType*;
        using DeallocatorType = D;
        using AllocatorType = typename DeallocatorType::Allocator;
        using Self = UniquePtr<ValueType[], DeallocatorType>;

        using IteratorType = xtd::containers::ContiguousIteratorBase<ValueType>;
        using ConstIteratorType = xtd::containers::ContiguousIteratorBase<const ValueType>;

        struct RawParts {
            PointerType begin;
            PointerType end;
            [[no_unique_address]] DeallocatorType dealloc;

            void cleanup() { dealloc.deallocate(begin); }
        };

    private:
        PointerType _begin;
        PointerType _end;
        [[no_unique_address]] DeallocatorType _dealloc;

    public:
        UniquePtr() requires(std::is_default_constructible_v<DeallocatorType>) : _begin(nullptr), _dealloc(DeallocatorType()) {}

        UniquePtr(PointerType begin, PointerType end) requires(std::is_default_constructible_v<DeallocatorType>) 
            : _begin(begin), _end(end), _dealloc(DeallocatorType()) {}

        template<xtd::allocators::DeallocatorT U>
        UniquePtr(PointerType begin, PointerType end, U&& dealloc) requires(std::convertible_to<D, DeallocatorType>) 
            : _begin(begin), _end(end), _dealloc(std::forward<U>(dealloc)) {}

        UniquePtr(PointerType begin, SizeType size) requires(std::is_default_constructible_v<DeallocatorType>) 
            : _begin(begin), _end(_begin + size), _dealloc(DeallocatorType()) {}

        template<xtd::allocators::DeallocatorT U>
        UniquePtr(PointerType begin, SizeType size, U&& dealloc) requires(std::convertible_to<D, DeallocatorType>) 
            : _begin(begin), _end(_begin + size), _dealloc(std::forward<U>(dealloc)) {}

        UniquePtr(const UniquePtr&) = delete;
        UniquePtr& operator=(const UniquePtr&) = delete;

        UniquePtr(UniquePtr&& other) requires (std::move_constructible<DeallocatorType>)
        : _begin(std::exchange(other._begin, nullptr)), _end(std::exchange(other._end, nullptr)), 
            _dealloc(std::move(other._dealloc)) {}

        UniquePtr& operator=(UniquePtr&& other) requires (std::is_move_assignable_v<DeallocatorType>) {
            if(this == &other) return *this;

            reset();

            _dealloc = std::move(other._dealloc);
            _begin = std::exchange(other._begin, nullptr);
            _end = std::exchange(other._end, nullptr);

            return *this;
        }

        ~UniquePtr() { reset(); }

        inline PointerType data() { return _begin; }
        inline const PointerType data() const { return _begin; }

        inline PointerType begin() { return _begin; }
        inline const PointerType begin() const { return _begin; }

        inline PointerType end() { return _end; }
        inline const PointerType end() const { return _end; }

        inline const PointerType cbegin() const { return _begin; }
        inline const PointerType cend() const { return _end; }

        DeallocatorType& deallocator() { return _dealloc; }
        const DeallocatorType& deallocator() const { return _dealloc; }
    
        void reset() {
            if(_begin == nullptr) return;
            dealloc();
            _begin = nullptr;
        }

        ValueType& operator[](SizeType i) {
            XTD_DEBUG_VERIFY(_begin != nullptr, "Dereferencing a nullptr");
            XTD_DEBUG_VERIFY(_begin + i < _end, "Index out of scope");
            return _begin[i];
        }
        const ValueType& operator[](SizeType i) const  {
            XTD_DEBUG_VERIFY(_begin != nullptr, "Dereferencing a nullptr");
            XTD_DEBUG_VERIFY(_begin + i < _end, "Index out of scope");
            return _begin[i];
        }

        RawParts release() {
            XTD_DEBUG_VERIFY(_begin != nullptr, "Releasing a nullptr");
            RawParts parts { _begin, _end, std::move(_dealloc) };
            _begin = nullptr;
            return parts;
        }

        void assign(PointerType begin, PointerType end, DeallocatorType&& d) {
            reset();
            _begin = begin;
            _end = end;
            _dealloc = std::move(d);
        }

        void assign(PointerType begin, SizeType size, DeallocatorType&& d) {
            reset();
            _begin = begin;
            _end = _begin + size;
            _dealloc = std::move(d);
        }

        static Self make(std::initializer_list<ValueType> vals) requires std::is_default_constructible_v<AllocatorType> {
            auto alloc = AllocatorType();
            return make(alloc, vals);
        }

        static Self make(AllocatorType& alloc, std::initializer_list<ValueType> vals) {
            auto* begin = reinterpret_cast<ValueType*>(alloc.allocate(sizeof(ValueType) * vals.size(), alignof(ValueType)));
            auto* end = begin + vals.size();
            auto* it = begin;
            auto* val = vals.begin();
            for(; it < end; ++it, ++val) ::new (it) ValueType(*val);
            return Self(begin, end, alloc.deallocator());
        }

        static Self make(size_t size) requires std::is_default_constructible_v<AllocatorType> && std::is_default_constructible_v<ValueType> {
            auto alloc = AllocatorType();
            auto val = ValueType();
            return make(alloc, size, val);
        }

        static Self make(size_t size, const ValueType& val) requires std::is_default_constructible_v<AllocatorType> {
            auto alloc = AllocatorType();
            return make(alloc, size, val);
        }

        static Self make(AllocatorType& alloc, size_t size) requires std::is_default_constructible_v<ValueType> {
            auto val = ValueType();
            return make(alloc, size, val);
        }

        static Self make(AllocatorType& alloc, size_t size, const ValueType& val) {
            auto* begin = reinterpret_cast<ValueType*>(alloc.allocate(sizeof(ValueType) * size, alignof(ValueType)));
            auto* end = begin + size;
            for(auto* it = begin; it < end; ++it) ::new (it) ValueType(val);
            return Self(begin, end, alloc.deallocator());
        }

        SizeType size() const { 
            XTD_DEBUG_VERIFY(_begin != nullptr, "Dereferencing a nullptr");
            return _end - _begin; 
        }

        std::strong_ordering operator<=>(const Self& other) const { return _begin <=> other._begin; }
        bool operator==(const Self& other) const { return _begin == other._begin; }

        template<typename U>
        std::strong_ordering operator<=>(const U* other) const { return _begin <=> other; }
        template<typename U>
        bool operator==(const U* other) const { return _begin == other; }

        std::strong_ordering operator<=>(std::nullptr_t) const { return _begin <=> nullptr; }
        bool operator==(std::nullptr_t) const { return _begin == nullptr; }

    private:
        inline void dealloc() { _dealloc.deallocate(_begin); }
    };
}


