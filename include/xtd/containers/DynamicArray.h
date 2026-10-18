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
#include "xtd/containers/Buffer.h"

#include <concepts>
#include <type_traits>
#include <utility>

namespace xtd::containers {
    
    template<typename T, typename Alloc = xtd::allocators::AllocatorNew>
    class DynamicArray : public SpanFunctions<DynamicArray<T, Alloc>, T, size_t, ContiguousIteratorBase<const T>> {
    public:
        using ValueType = T;
        using SizeType = size_t;
        using PointerType = ValueType*;
        
        using AllocatorType = Alloc;
        using IteratorType = ContiguousIteratorBase<ValueType>;
        using ConstIteratorType = ContiguousIteratorBase<const ValueType>;

        using Self = DynamicArray<ValueType, AllocatorType>;
        using Base = SpanFunctions<Self, ValueType, SizeType, ConstIteratorType>;

        using RawParts = typename Buffer<ValueType, AllocatorType>::RawParts;
        using BufferType = Buffer<ValueType, AllocatorType>;

    private:
        BufferType _buffer;

    public:
        DynamicArray() : _buffer() { xtd::utility::constructRange(_buffer.begin(), _buffer.end()); }

        explicit DynamicArray(AllocatorType&& alloc) : _buffer(std::move(alloc)) { xtd::utility::constructRange(_buffer.begin(), _buffer.end()); }

        DynamicArray(SizeType size) : _buffer(size) { xtd::utility::constructRange(_buffer.begin(), _buffer.end()); }

        explicit DynamicArray(SizeType size, AllocatorType&& alloc) : _buffer(size, std::move(alloc)) {
            xtd::utility::constructRange(_buffer.begin(), _buffer.end());
        }

        DynamicArray(const DynamicArray& other) : _buffer(other.size()) {
            xtd::utility::copyConstructRange(other.begin(), other.end(), _buffer.begin());
        }

        DynamicArray& operator=(const DynamicArray& other) {
            if(this == &other) return *this;

            xtd::utility::destroyRange(_buffer.begin(), _buffer.end());
            _buffer.resize(other.size());
            xtd::utility::copyConstructRange(other.begin(), other.end(), _buffer.begin());

            return *this;
        }

        DynamicArray(DynamicArray&& other) : _buffer(std::move(other._buffer)) {}

        // The moved from allocator must have valid state
        DynamicArray& operator=(DynamicArray&& other) requires (std::is_move_assignable_v<AllocatorType>) {
            if(this == &other) return *this;

            _buffer = std::exchange(other._buffer, BufferType());

            return *this;
        }

        ~DynamicArray() = default;

        inline ValueType& operator[](SizeType i) { return _buffer[i]; }
        inline const ValueType& operator[](SizeType i) const { return _buffer[i]; }

        RawParts release() { return _buffer.release(); }
        RawParts release(AllocatorType&& alloc) { return _buffer.release(std::move(alloc)); }

        void assign(PointerType data, SizeType size) { _buffer.assign(data, size); }
        void assign(PointerType data, SizeType size, AllocatorType&& alloc) { _buffer.assign(data, size, std::move(alloc)); }

        inline IteratorType begin() { return _buffer.begin(); }
        inline IteratorType end() { return _buffer.end(); }
        inline ConstIteratorType begin() const { return _buffer.begin(); }
        inline ConstIteratorType end() const { return _buffer.end(); }
        inline ConstIteratorType cbegin() const { return _buffer.cbegin(); }
        inline ConstIteratorType cend() const { return _buffer.cend(); }
        
        inline SizeType size() const { return _buffer.size(); }
        inline PointerType data() { return _buffer.data(); }
        inline const PointerType data() const { return _buffer.data(); }

        ValueType& back() { return _buffer.back(); }
        const ValueType& back() const { return _buffer.back(); }

        ValueType& front() { return _buffer.front(); }
        const ValueType& front() const { return _buffer.front(); }

        AllocatorType& allocator() { return _buffer.allocator(); }
        const AllocatorType& allocator() const { return _buffer.allocator(); }

        void resize(SizeType newSize) {
            if(newSize > size()) grow(newSize);
            else if(newSize < size()) shrink(newSize);
        }

        void grow(SizeType newSize) {
            XTD_DEBUG_VERIFY(newSize >= size(), "New size must be greater than old size to grow");
            _buffer.migrate(newSize, [&](Span<ValueType> newRange, Span<ValueType> oldRange) {
                auto oldEnd = xtd::utility::copyConstructRange(oldRange.begin(), oldRange.end(), newRange.begin());
                xtd::utility::constructRange(oldEnd, newRange.end());
                xtd::utility::destroyRange(oldRange.begin(), oldRange.end());
            });
        }

        void shrink(SizeType newSize) {
            XTD_DEBUG_VERIFY(newSize <= size(), "New size must be greater than old size to grow");
            _buffer.migrate(newSize, [&](Span<ValueType> newRange, Span<ValueType> oldRange) {
                auto oldEnd = xtd::utility::copyConstructRange(oldRange.begin(), oldRange.begin() + newRange.size(), newRange.begin());
                xtd::utility::destroyRange(oldRange.begin(), oldRange.end());
            });
        }
    };
}


