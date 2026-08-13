#pragma once
#include "xtd/allocators/NewAllocators.h"
#include "xtd/allocators/Traits.h"
#include "xtd/debug/Checks.h"
#include "xtd/containers/Traits.h"
#include "xtd/containers/Iterators.h"
#include "xtd/containers/Span.h"
#include <concepts>
#include <type_traits>
#include <utility>

namespace xtd::containers {
    template<typename G>
    concept GrowthStrategyT = requires(G g, size_t cap) {
        { g.grow(cap) } -> std::convertible_to<size_t>;
    };

    template<xtd::allocators::AllocatorT A, GrowthStrategyT G>
    struct VectorPolicy {
        using Alloc = A;
        using Growth = G;
    };

    template<typename A>
    concept VectorPolicyT = requires() {
        typename A::Alloc;
        typename A::Growth;
    };

    struct VectorGrowthPowerOfTwo {
        inline size_t grow(size_t cap) noexcept {
            // Prevent stalling at cap 0
            return (cap + 1) * 2;
        }
    };

    struct VectorGrowthGoldenRatio {
        inline size_t grow(size_t cap) noexcept {
            // Prevent stalling at cap 0
            return (cap + 1) + (cap + 1) / 2;
        }
    };

    // template<typename T, VectorPolicyT Policy = VectorPolicy<NewAllocator, VectorGrowthPowerOfTwo>>
    // class Vector;

    // template<typename T, typename Policy> 
    // struct DerivedTraits<Vector<T, Policy>> { 
    //     using ValueType = T;
    //     using Alloc = typename Policy::Alloc;
    //     using Growth = typename Policy::Growth;
    // };
    
    template<typename T, typename Policy = VectorPolicy<xtd::allocators::AllocatorNew, VectorGrowthPowerOfTwo>>
    class Vector : public SpanFunctions<Vector<T, Policy>, T, size_t, ContiguousIteratorBase<const T>> {
    public:
        using ValueType = T;
        using SizeType = size_t;
        using PointerType = ValueType*;
        using PolicyType = Policy;
        using Self = Vector<ValueType, PolicyType>;
        using AllocatorType = typename PolicyType::Alloc;
        using GrowthType = typename PolicyType::Growth;
        using IteratorType = ContiguousIteratorBase<ValueType>;
        using ConstIteratorType = ContiguousIteratorBase<const ValueType>;

        using Self = Vector<ValueType, PolicyType>;
        using Base = SpanFunctions<Self, ValueType, SizeType, ConstIteratorType>;

        struct RawParts {
            PointerType begin;
            PointerType end;
            PointerType cap;

            [[no_unique_address]] AllocatorType alloc;
            [[no_unique_address]] GrowthType growth;

            void cleanup() {
                for(auto* it = begin; it < end; ++it) it->~ValueType();
                alloc.deallocate(begin);
            }
        };

    private:
        PointerType _begin;
        PointerType _end;
        PointerType _cap;

        [[no_unique_address]] AllocatorType _alloc;
        [[no_unique_address]] GrowthType _growth;

    public:
        Vector() requires(std::is_default_constructible_v<AllocatorType> && std::is_default_constructible_v<GrowthType>) 
        : _alloc(AllocatorType()), _growth(GrowthType()) {
            init(alloc(0), 0, 0);
        }

        template<xtd::allocators::AllocatorT A>
        explicit Vector(A&& alloc) requires(std::is_default_constructible_v<GrowthType> && std::convertible_to<A, AllocatorType>) 
        : _alloc(std::forward<A>(alloc)), _growth(GrowthType()) {
            init(alloc(0), 0, 0);
        }

        template<typename G>
        explicit Vector(G&& growth) requires(std::is_default_constructible_v<AllocatorType> && std::convertible_to<G, GrowthType>) 
        : _alloc(AllocatorType()), _growth(std::forward<G>(growth)) {
            init(alloc(0), 0, 0);
        }

        template<typename A, typename G>
        explicit Vector(A&& alloc, G&& growth) requires(std::convertible_to<A, AllocatorType> && std::convertible_to<G, GrowthType>)
            : _alloc(std::forward<A>(alloc)), _growth(std::forward<G>(growth)) {
            init(alloc(0), 0, 0);
        }

        Vector(const Vector& other) : _alloc(allocatorCopyConstruction(other.allocator())), _growth(growthCopyConstruction(other.growth())) {
            auto size = other.size();
            auto cap = other.capacity();
            init(alloc(size), size, cap);
            copyFrom(other.data(), other.size());
        }

        // The moved from allocator must have valid state
        Vector(Vector&& other) requires (std::move_constructible<AllocatorType> && std::move_constructible<GrowthType>)
        : _alloc(std::move(other.allocator())), _growth(std::move(other.growth())) {
            _begin = std::exchange(other._begin, other.alloc(0));
            _end = std::exchange(other._end, other._begin);
            _cap = std::exchange(other._cap, other._begin);
        }

        template<ArrayContainerType Other>
        Vector& operator=(const Other& other) {
            if(this == &other) return *this;

            auto size = other.size();

            clearData();
            if(size > capacity()) {
                dealloc();
                auto cap = grow(capacity(), size);
                _begin = alloc(cap);
                _cap = _begin + cap;
            }
            copyFrom(other.data(), size);
            _end = _begin + size;

            return *this;
        }


        // The moved from allocator must have valid state
        Vector& operator=(Vector&& other) requires (std::is_move_assignable_v<AllocatorType> && std::is_move_assignable_v<GrowthType>) {
            if(this == &other) return *this;

            clearData();
            dealloc();

            _alloc = std::move(other.allocator());
            _growth = std::move(other.growth());

            _begin = std::exchange(other._begin, other.alloc(0));
            _end = std::exchange(other._end, other._begin);
            _cap = std::exchange(other._cap, other._begin);

            return *this;
        }

        ~Vector() {
            clearData();
            dealloc();
        }

        inline ValueType& operator[](SizeType i) { return const_cast<ValueType&>(std::as_const(*this)[i]); }
        inline const ValueType& operator[](SizeType i) const {
            XTD_DEBUG_VERIFY(_begin + i < _end, "out of bounds access to Vector data");
            return _begin[i];
        }

        RawParts release() requires (std::is_default_constructible_v<AllocatorType> && std::is_default_constructible_v<GrowthType>) {
            return release(AllocatorType(), GrowthType());
        }
        RawParts release(AllocatorType&& alloc) requires (std::is_default_constructible_v<GrowthType>) {
            return release(alloc, GrowthType());
        }
        RawParts release(GrowthType&& growth) requires (std::is_default_constructible_v<AllocatorType>) {
            return release(AllocatorType(), growth);
        }

        RawParts release(AllocatorType&& alloc, GrowthType&& growth) {
            RawParts parts { _begin, _end, _cap, std::move(_alloc), std::move(_growth) };
            _alloc = alloc;
            _growth = growth;
            init(alloc(0), 0, 0);
            return parts;
        }

        void assign(PointerType data, SizeType size) requires (std::is_default_constructible_v<AllocatorType> && std::is_default_constructible_v<GrowthType>) {
            assign(data, size);
        }

        void assign(PointerType data, SizeType size, AllocatorType&& alloc) requires (std::is_default_constructible_v<GrowthType>) {
            assign(data, size, std::move(alloc), GrowthType());
        }

        void assign(PointerType data, SizeType size, GrowthType&& growth) requires (std::is_default_constructible_v<AllocatorType>) {
            assign(data, size, AllocatorType(), std::move(growth));
        }

        void assign(PointerType data, SizeType size, AllocatorType&& alloc, GrowthType&& growth) {
            clear();
            dealloc();
            _begin = data;
            _end = _begin + size;
            _cap = _end;
            _alloc = std::move(alloc);
            _growth = std::move(growth);
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

        AllocatorType& allocator() { return _alloc; }
        const AllocatorType& allocator() const { return _alloc; }

        GrowthType& growth() { return _growth; }
        const GrowthType& growth() const { return _growth; }
    
        void clear() {
            clearData();
            _end = _begin;
        }

        template<typename... Args>
        IteratorType pushBack(Args&&... args) {
            if(_end >= _cap) migrate(_growth.grow(capacity()), size());
            ::new (_end) ValueType(std::forward<Args>(args)...);
            return IteratorType(_end++);
        }

        //Push back without checking for capacity, used when the caller can guarantee the addition will fit
        template<typename... Args>
        IteratorType pushBackUnchecked(Args&&... args) {
            XTD_DEBUG_VERIFY(_end < _cap, "call to Vector::pushBackUnchecked with no capacity to spare");
            ::new (_end) ValueType(std::forward<Args>(args)...);
            return IteratorType(_end++);
        }

        IteratorType popBack() { --_end; return IteratorType(_end - 1); }

        IteratorType insert(IteratorType target, IteratorType b, IteratorType e) {
            auto cap = capacity();
            auto s = size();
            auto addition = e - b;
            auto ns = s + addition;
    
            if(cap <= ns) {
                cap = grow(cap, ns);
                auto newBegin = alloc(cap);
                PointerType it = _begin;
                PointerType newIt = newBegin;
                for(; it < target; ++it, ++newIt) {
                    ::new (newIt) ValueType(*it);
                    it->~ValueType();
                }
                
                for(; b < e; ++b, ++newIt) {
                    ::new (newIt) ValueType(*b);
                }

                for(; it < _end; ++it, ++newIt) {
                    ::new (newIt) ValueType(*it);
                    it->~ValueType();
                }
                auto res = newBegin + target - _begin;
                dealloc();
                init(newBegin, ns, cap);
                return res;
            }

            auto elemsAfter = _end - target;
            if(elemsAfter > addition) {
                PointerType itFirst = _end - 1;
                PointerType itLast = itFirst + addition;
                
                for(; itLast >= _end; --itLast, --itFirst) {
                    ::new (itLast) ValueType(*itFirst);
                }
                for(; itFirst >= target; --itLast, --itFirst) {
                    itLast->~ValueType();
                    ::new (itLast) ValueType(*itFirst);
                }
                for(IteratorType src = e - 1; itLast >= target; --itLast, --src) {
                    itLast->~ValueType();
                    ::new (itLast) ValueType(*src);
                }
                _end += addition;
            } else {
                PointerType itFirst = _end - 1;
                PointerType itLast = itFirst + addition;
                PointerType itNewEnd = itFirst;

                for(; itLast >= itNewEnd; --itLast, --itFirst) {
                    ::new (itLast) ValueType(*itFirst);
                }
                IteratorType src = e - 1;
                for(; itLast >= _end; --itLast, --src) {
                    ::new (itLast) ValueType(*src);
                }
                for(; itLast >= target; --itLast, --src) {
                    itLast->~ValueType();
                    ::new (itLast) ValueType(*src);
                }
                _end += addition;
            }
            return target;
        }

    private:
        inline void init(PointerType ptr, SizeType size, SizeType cap) {
            _begin = ptr;
            _end = _begin + size;
            _cap = _begin + cap;
        }
        inline PointerType alloc(SizeType size) {
            return reinterpret_cast<PointerType>(_alloc.allocate(size * sizeof(ValueType), alignof(ValueType)));
        }
        inline void dealloc() {
            return _alloc.deallocate(_begin, size() * sizeof(ValueType), alignof(ValueType));
        }
        inline SizeType grow(SizeType current, SizeType req) {
            while(current < req) current = _growth.grow(current);
            return current;
        }
        inline void migrate(SizeType cap, SizeType s) {
            auto newBegin = alloc(cap);
            PointerType itNew = newBegin;
            PointerType it = _begin;
            for(; it < _end; ++it, ++itNew) {
                ::new (itNew) ValueType(*it);
                it->~ValueType();
            }
            dealloc();
            init(newBegin, s, cap);
        }

        inline void clearData() {
            for(auto* it = _begin; it < _end; ++it) {
                it->~ValueType();
            }
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
    };

    static_assert(VectorContainerType<Vector<int>>, "Vector must satisfy ContiguousContainerType");
}


