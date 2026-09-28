#pragma once
#include "xtd/containers/Vector.h"

#include <concepts>
#include <type_traits>
#include <utility>

namespace xtd::containers {
    template<std::integral T, typename Policy = VectorPolicy<xtd::allocators::AllocatorNew, VectorGrowthPowerOfTwo>>
    class StringBase : public SpanFunctions<Vector<T, Policy>, T, size_t, ContiguousIteratorBase<const T>> {
    public:
        using ValueType = T;
        using SizeType = size_t;
        using PointerType = ValueType*;
        using PolicyType = Policy;
        
        using AllocatorType = typename PolicyType::Alloc;
        using GrowthType = typename PolicyType::Growth;
        using IteratorType = ContiguousIteratorBase<ValueType>;
        using ConstIteratorType = ContiguousIteratorBase<const ValueType>;

        using Self = Vector<ValueType, PolicyType>;
        using Base = SpanFunctions<Self, ValueType, SizeType, ConstIteratorType>;

        struct Policy {
            [[no_unique_address]] AllocatorType alloc;
            [[no_unique_address]] GrowthType growth;
        };

        struct RawParts {
            PointerType begin;
            PointerType end;
            PointerType cap;

            [[no_unique_address]] Policy policy;

            void cleanup() {
                for(auto* it = begin; it < end; ++it) it->~ValueType();
                policy.alloc.deallocate(begin);
            }
        };

    private:
        PointerType _begin;
        PointerType _end;
        PointerType _cap;

        [[no_unique_address]] Policy _policy;

    public:
        StringBase() requires(std::is_default_constructible_v<Policy>) : _policy(Policy()) {
            init(alloc(1), 1, 1);
            ::new (_end) ValueType('\0');
        }

        explicit StringBase(Policy&& policy) : _policy(std::move(policy)) {
            init(alloc(1), 1, 1);
            ::new (_end) ValueType('\0');
        }

        StringBase(const StringBase& other) requires (std::is_copy_constructible_v<ValueType>)
        : _policy({allocatorCopyConstruction(other._policy.alloc), growthCopyConstruction(other._policy.growth)}) {
            auto size = other.size();
            auto cap = other.capacity();
            init(alloc(size), size, cap);
            copyFrom(other.data(), other.size());
        }

        // The moved from allocator must have valid state
        StringBase(StringBase&& other) requires (std::move_constructible<Policy>) : _policy(std::move(other._policy)) {
            _begin = std::exchange(other._begin, other.alloc(1));
            _end = std::exchange(other._end, other._begin);
            _cap = std::exchange(other._cap, other._begin);

            ::new (other._begin) ValueType('\0');
            other._end = _begin;
            other._cap = _begin + 1;
        }

        template<ArrayContainerType Other>
        StringBase& operator=(const Other& other) {
            if(this == &other) return *this;

            auto size = other.size();

            clearData();
            if(size > capacity()) {
                dealloc();
                auto cap = growSize(capacity(), size);
                _begin = alloc(cap);
                _cap = _begin + cap;
            }
            copyFrom(other.data(), size);
            _end = _begin + size;

            return *this;
        }


        // The moved from allocator must have valid state
        StringBase& operator=(StringBase&& other) requires (std::is_move_assignable_v<Policy>) {
            if(this == &other) return *this;

            clearData();
            dealloc();

            _policy = std::move(other._policy);

            _begin = std::exchange(other._begin, other.alloc(1));
            _end = std::exchange(other._end, other._begin);
            _cap = std::exchange(other._cap, other._begin);

            ::new (other._begin) ValueType('\0');
            other._end = _begin;
            other._cap = _begin + 1;

            return *this;
        }

        ~StringBase() {
            clearData();
            dealloc();
        }

        inline ValueType& operator[](SizeType i) { return const_cast<ValueType&>(std::as_const(*this)[i]); }
        inline const ValueType& operator[](SizeType i) const {
            XTD_DEBUG_VERIFY(_begin + i < _end, "out of bounds access to StringBase data");
            return _begin[i];
        }

        RawParts release() requires (std::is_default_constructible_v<Policy>) {
            return release(Policy());
        }

        RawParts release(Policy&& policy) {
            RawParts parts { _begin, _end, _cap, std::move(_policy) };
            _policy = policy;
            init(alloc(1), 1, 1);
            ::new (_end) ValueType('\0');
            return parts;
        }

        void assign(PointerType data, SizeType size) requires (std::is_default_constructible_v<Policy>) {
            assign(data, size, Policy());
        }

        // Size must include the string terminator value ('\0'), it will not be automatically assigned
        void assign(PointerType data, SizeType size, Policy&& policy) {
            clear();
            dealloc();
            _begin = data;
            _end = _begin + size - 1;
            _cap = _end;
            _policy = std::move(policy);
        }

        inline IteratorType begin() { return IteratorType(_begin); }
        inline IteratorType end() { return IteratorType(_end); }
        inline ConstIteratorType begin() const { return ConstIteratorType(_begin); }
        inline ConstIteratorType end() const { return ConstIteratorType(_end); }
        inline ConstIteratorType cbegin() const { return begin(); }
        inline ConstIteratorType cend() const { return end(); }
        
        inline SizeType size() const { return _end - _begin; }
        inline SizeType capacity() const { return _end - _cap; }
        inline PointerType data() { return _begin; }
        inline const PointerType data() const { return _begin; }

        ValueType& back() { return _end[-1]; }
        const ValueType& back() const { return _end[-1]; }

        ValueType& front() { return _begin[0]; }
        const ValueType& front() const { return _begin[0]; }

        AllocatorType& allocator() { return _policy.alloc; }
        const AllocatorType& allocator() const { return _policy.alloc; }

        GrowthType& growth() { return _policy.growth; }
        const GrowthType& growth() const { return _policy.growth; }
    
        void clear() {
            clearData();
            _end = _begin;
            ::new (_end) ValueType('\0');
        }

        template<typename... Args>
        IteratorType pushBack(Args&&... args) {
            if(_end >= _cap) migrate(_policy.growth.grow(size() + 1), size());
            _end->~ValueType();
            ::new (_end) ValueType(std::forward<Args>(args)...);
            ::new (++_end) ValueType('\0');
            return IteratorType(_end - 1);
        }

        //Push back without checking for capacity, used when the caller can guarantee the addition will fit
        template<typename... Args>
        IteratorType pushBackUnchecked(Args&&... args) {
            XTD_DEBUG_VERIFY(_end < _cap, "call to StringBase::pushBackUnchecked with no capacity to spare");
            _end->~ValueType();
            ::new (_end) ValueType(std::forward<Args>(args)...);
            ::new (++_end) ValueType('\0');
            return IteratorType(_end - 1);
        }

        IteratorType popBack() {
            XTD_DEBUG_VERIFY(_end > _begin, "call to StringBase::popBack with zero size");
            _end->~ValueType();
            --_end;
            _end->~ValueType();
            ::new (_end) ValueType('\0');
            return IteratorType(_end - 1); 
        }

        IteratorType insert(IteratorType target, IteratorType b, IteratorType e) {
            auto cap = capacity();
            auto s = size();
            auto addition = e - b;
            auto ns = s + addition;
            _end->~ValueType();
            if(cap <= ns + 1) {
                cap = growSize(cap, ns + 1);
                auto newBegin = alloc(cap);
                PointerType it = _begin;
                PointerType newIt = newBegin;
                for(; it < target; ++it, ++newIt) {
                    constructAt(newIt, *it);
                    it->~ValueType();
                }
                
                for(; b < e; ++b, ++newIt) {
                    constructAt(newIt, *b);
                }

                for(; it < _end; ++it, ++newIt) {
                    constructAt(newIt, *it);
                    it->~ValueType();
                }
                auto res = newBegin + target - _begin;
                dealloc();
                init(newBegin, ns, cap);
                ::new (_end) ValueType('\0');
                return res;
            }

            auto elemsAfter = _end - target;
            if(elemsAfter > addition) {
                PointerType itFirst = _end - 1;
                PointerType itLast = itFirst + addition;
                
                for(; itLast >= _end; --itLast, --itFirst) {
                    constructAt(itLast, *itFirst);
                }
                for(; itFirst >= target; --itLast, --itFirst) {
                    itLast->~ValueType();
                    constructAt(itLast, *itFirst);
                }
                for(IteratorType src = e - 1; itLast >= target; --itLast, --src) {
                    itLast->~ValueType();
                    constructAt(itLast, *src);
                }
                _end += addition;
            } else {
                PointerType itFirst = _end - 1;
                PointerType itLast = itFirst + addition;
                PointerType itNewEnd = itFirst;

                for(; itLast >= itNewEnd; --itLast, --itFirst) {
                    constructAt(itLast, *itFirst);
                }
                IteratorType src = e - 1;
                for(; itLast >= _end; --itLast, --src) {
                    constructAt(itLast, *src);
                }
                for(; itLast >= target; --itLast, --src) {
                    itLast->~ValueType();
                    constructAt(itLast, *src);
                }
                _end += addition;
            }
            ::new (_end) ValueType('\0');
            return target;
        }

        void resize(SizeType newSize) {
            auto priorSize = size();
            if(priorSize == newSize) return;
            if(priorSize < newSize) grow(newSize);
            if(priorSize > newSize) shrink(newSize);
        }

        void grow(SizeType newSize) {
            auto priorCap = capacity();
            _end->~ValueType();
            if(priorCap >= newSize + 1) {
                for(auto it = _end; it < _begin + newSize; ++it) {
                    ::new (it) ValueType();
                }
                _end = _begin + newSize;
                ::new (_end) ValueType('\0');
            }
            else {
                auto newCap = growSize(priorCap, newSize + 1);
                auto itNew = migrate(newCap, newSize);
                for(; itNew < _end; ++itNew) {
                    ::new (itNew) ValueType();
                }
                ::new (_end) ValueType('\0');
            }
        }

        void shrink(SizeType newSize) {
            _end->~ValueType();
            auto newEnd = _begin + newSize;
            for(auto it = newEnd; it < _end; ++it) it->~ValueType();
            _end = newEnd;
            ::new (_end) ValueType('\0');
            return;
        }

    private:

        inline void init(PointerType ptr, SizeType size, SizeType cap) {
            _begin = ptr;
            _end = _begin + size;
            _cap = _begin + cap;
        }
        inline PointerType alloc(SizeType size) {
            return reinterpret_cast<PointerType>(_policy.alloc.allocate(size * sizeof(ValueType), alignof(ValueType)));
        }
        inline void dealloc() {
            return _policy.alloc.deallocate(_begin, size() * sizeof(ValueType), alignof(ValueType));
        }
        inline SizeType growSize(SizeType current, SizeType req) {
            while(current < req) current = _policy.growth.grow(current);
            return current;
        }
        inline auto migrate(SizeType cap, SizeType s) {
            auto newBegin = alloc(cap);
            PointerType itNew = newBegin;
            PointerType it = _begin;
            for(; it < _end; ++it, ++itNew) {
                constructAt(itNew, *it);
                it->~ValueType();
            }
            it->~ValueType();
            dealloc();
            init(newBegin, s, cap);
            return itNew;
        }

        inline void clearData() {
            for(auto* it = _begin; it < _end; ++it) {
                it->~ValueType();
            }
            _end->~ValueType();
        }

        inline void copyFrom(PointerType src, size_t size) {
            for(PointerType it; it < _begin + size; ++it, ++src) {
                ::new (it) ValueType(*src);
            }
        }

        static AllocatorType allocatorCopyConstruction(const AllocatorType& other) {
            if constexpr (requires (const AllocatorType& other) {
                { AllocatorType::onCopyConstruction(other) } -> std::convertible_to<AllocatorType>;
            }) {
                return AllocatorType::onCopyConstruction(other);
            }
            else if constexpr (std::is_copy_constructible_v<AllocatorType>) {
                return other.allocator();
            }
            else {
                static_assert(false, "AllocatorType is not copy constructible and doesn't implement onCopyConstruction method");
            }
        }

        static GrowthType growthCopyConstruction(const GrowthType& other) {
            if constexpr (requires (const GrowthType& other) {
                { GrowthType::onCopyConstruction(other) } -> std::convertible_to<GrowthType>;
            }) {
                return GrowthType::onCopyConstruction(other);
            }
            else if constexpr (std::is_copy_constructible_v<GrowthType>) {
                return other.growth();
            }
            else {
                static_assert(false, "GrowthType is not copy constructible and doesn't implement onCopyConstruction method");
            }
        }

        template<typename Dst, typename Src>
            requires std::is_constructible_v<Dst, const Src&> ||
                    std::is_constructible_v<Dst, Src&&>
        void constructAt(Dst* dst, Src& src) {
            if constexpr (std::is_constructible_v<Dst, Src&&>) {
                ::new (dst) Dst(std::move(src));
            }
            else if constexpr (std::is_constructible_v<Dst, const Src&>) {
                ::new (dst) Dst(src);
            }
        }

        template<typename Dst> requires(std::is_default_constructible_v<Dst>)
        void constructAt(Dst* dst) { ::new (dst) Dst(); }
    };

    static_assert(VectorContainerType<StringBase<char>>, "StringBase must satisfy ContiguousContainerType");

    template<typename Policy = VectorPolicy<xtd::allocators::AllocatorNew, VectorGrowthPowerOfTwo>>
    using CustomString = StringBase<char, Policy>;

    template<typename Policy = VectorPolicy<xtd::allocators::AllocatorNew, VectorGrowthPowerOfTwo>>
    using CustomWString = StringBase<wchar_t, Policy>;
    
    using String = CustomString<>;
    using WString = CustomWString<>;
}