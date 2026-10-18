#pragma once
#include "xtd/allocators/NewAllocators.h"
#include "xtd/allocators/Traits.h"
#include "xtd/containers/Vector.h"
#include "xtd/debug/Checks.h"
#include "xtd/containers/Traits.h"
#include "xtd/containers/PointerIterators.h"
#include "xtd/containers/Span.h"
#include "xtd/memory/Lifetime.h"
#include "xtd/Macros.h"
#include "xtd/utility/Functions.h"

#include <concepts>
#include <type_traits>
#include <utility>

namespace xtd::containers {
    
    template<typename T, typename Alloc = xtd::allocators::AllocatorNew>
    class Buffer : public SpanFunctions<Buffer<T, Alloc>, T, size_t, ContiguousIteratorBase<const T>> {
    public:
        using ValueType = T;
        using SizeType = size_t;
        using PointerType = ValueType*;
        
        using AllocatorType = Alloc;
        using IteratorType = ContiguousIteratorBase<ValueType>;
        using ConstIteratorType = ContiguousIteratorBase<const ValueType>;

        using Self = Buffer<ValueType, AllocatorType>;
        using Base = SpanFunctions<Self, ValueType, SizeType, ConstIteratorType>;

        struct RawParts {
            PointerType begin;
            PointerType end;

            XTD_NO_UNIQUE_ADDRESS AllocatorType alloc;

            void cleanup() { alloc.alloc.deallocate(begin); }
        };

    private:
        PointerType _begin;
        PointerType _end;
        XTD_NO_UNIQUE_ADDRESS AllocatorType _alloc;

    public:
        Buffer() requires(std::is_default_constructible_v<AllocatorType>) : _alloc(AllocatorType()) {
            init(allocate(0), 0);
        }

        explicit Buffer(AllocatorType&& alloc) : _alloc(std::move(alloc)) {
            init(allocate(0), 0);
        }

        Buffer(SizeType size) requires(std::is_default_constructible_v<AllocatorType>) : _alloc(AllocatorType()) {
            init(allocate(size), size);
        }

        explicit Buffer(SizeType size, AllocatorType&& alloc) : _alloc(std::move(alloc)) {
            init(allocate(size), size);
        }

        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;

        // The moved from allocator must have valid state
        Buffer(Buffer&& other) requires (std::move_constructible<AllocatorType>) : _alloc(std::move(other._alloc)) {
            _begin = std::exchange(other._begin, other.allocate(0));
            _end = std::exchange(other._end, other._begin);
        }

        // The moved from allocator must have valid state
        Buffer& operator=(Buffer&& other) requires (std::is_move_assignable_v<AllocatorType>) {
            if(this == &other) return *this;

            deallocate();

            _alloc = std::move(other._alloc);

            _begin = std::exchange(other._begin, other.alloc(0));
            _end = std::exchange(other._end, other._begin);

            return *this;
        }

        ~Buffer() { deallocate(); }

        inline ValueType& operator[](SizeType i) { return const_cast<ValueType&>(std::as_const(*this)[i]); }
        inline const ValueType& operator[](SizeType i) const {
            XTD_DEBUG_VERIFY(_begin + i < _end, "out of bounds access to Buffer data");
            return _begin[i];
        }

        RawParts release() requires (std::is_default_constructible_v<AllocatorType>) {
            return release(AllocatorType());
        }

        RawParts release(AllocatorType&& alloc) {
            RawParts parts { _begin, _end, std::move(_alloc) };
            _alloc = std::move(alloc);
            init(allocate(0), 0);
            return parts;
        }

        void assign(PointerType data, SizeType size) requires (std::is_default_constructible_v<AllocatorType>) {
            assign(data, size, AllocatorType());
        }

        void assign(PointerType data, SizeType size, AllocatorType&& alloc) {
            deallocate();
            _begin = data;
            _end = _begin + size;
            _alloc = std::move(alloc);
        }

        inline IteratorType begin() { return IteratorType(_begin); }
        inline IteratorType end() { return IteratorType(_end); }
        inline ConstIteratorType begin() const { return ConstIteratorType(_begin); }
        inline ConstIteratorType end() const { return ConstIteratorType(_end); }
        inline ConstIteratorType cbegin() const { return begin(); }
        inline ConstIteratorType cend() const { return end(); }
        
        inline SizeType size() const { return _end - _begin; }
        inline PointerType data() { return _begin; }
        inline const PointerType data() const { return _begin; }

        ValueType& back() { return _end[-1]; }
        const ValueType& back() const { return _end[-1]; }

        ValueType& front() { return _begin[0]; }
        const ValueType& front() const { return _begin[0]; }

        AllocatorType& allocator() { return _alloc.alloc; }
        const AllocatorType& allocator() const { return _alloc.alloc; }

        void resize(SizeType newSize) {
            deallocate();
            init(allocate(newSize), newSize);
        }

        template<typename F>
        void migrate(size_t newSize, F&& cleanup) {
            auto newBegin = allocate(newSize);
            auto newEnd = newBegin + newSize;
            cleanup(Span<T>(newBegin, newEnd), Span<T>(_begin, _end));
            deallocate();
            _begin = newBegin;
            _end = newEnd;
        }

    private:
        inline void init(PointerType ptr, SizeType size) {
            _begin = ptr;
            _end = _begin + size;
        }
        inline PointerType allocate(SizeType size, SizeType alignment = alignof(ValueType)) {
            return reinterpret_cast<PointerType>(_alloc.allocate(size * sizeof(ValueType), alignment));
        }
        inline void deallocate(SizeType alignment = alignof(ValueType)) {
            return _alloc.deallocate(_begin, size() * sizeof(ValueType), alignment);
        }
    };
}


