#pragma once
#include "xtd/containers/Traits.h"
#include "xtd/containers/Slot.h"
#include "xtd/Macros.h"
#include "xtd/debug/Checks.h"
#include "xtd/memory/Lifetime.h"
#include "xtd/utility/Functions.h"
#include "xtd/allocators/Traits.h"
#include <limits>

namespace xtd::containers {
    template<typename H, typename C, xtd::allocators::AllocatorT A>
    struct SwissTablePolicy {
		using HasherType = H;
		using ComparatorType = C;
        using AllocatorType = A;
    };

	template<typename T, typename Policy, float loadThreshold = 0.75f>
	class SwissTable {
		static_assert(loadThreshold > 0 && loadThreshold <= 1.0f, "loadThreshold must be between 0 and 1 and cannot be exactly 0");
        static constexpr size_t _initialCapacity = 16;

		static constexpr std::byte _empty = static_cast<std::byte>(std::numeric_limits<uint8_t>::max()); 			// 0b11111111
		static constexpr std::byte _tombstone = static_cast<std::byte>(std::numeric_limits<uint8_t>::max() - 1); 	// 0b11111110

	public:
        using ValueType = T;
        using SizeType = size_t;
        using PointerType = ValueType*;
		using BucketType = ValueType;
        
        using HasherType = typename Policy::HasherType;
		using ComparatorType = typename Policy::ComparatorType;
		using AllocatorType = typename Policy::AllocatorType;

        using Self = SwissTable<ValueType, Policy, loadThreshold>;

        struct Policy {
            XTD_NO_UNIQUE_ADDRESS HasherType hasher;
            XTD_NO_UNIQUE_ADDRESS ComparatorType comparator;
			XTD_NO_UNIQUE_ADDRESS AllocatorType alloc;
        };

	private:
		PointerType _begin;
		PointerType _end;
        std::byte* _slots;

		SizeType _size;
		XTD_NO_UNIQUE_ADDRESS Policy _policy;

	public:
		SwissTable() requires std::is_default_constructible_v<Policy> : _size(0), _begin(allocate(_initialCapacity)), _end(_begin + _initialCapacity), _slots(allocateSlots(_initialCapacity)) {}
		SwissTable(Policy&& policy) requires std::is_default_constructible_v<Policy> 
			: _policy(std::move(policy)), _size(0), _begin(allocate(_initialCapacity)), _end(_begin + _initialCapacity), _slots(allocateSlots(_initialCapacity)) {}

		~SwissTable() {
			clear();
			deallocateSlots(_slots, _slots + capacity());
			deallocate(_begin, _end);
		}

		SwissTable(const SwissTable& other) requires std::is_default_constructible_v<Policy> 
			: _size(other.size()), _begin(allocate(other.capacity())), _end(_begin + other.capacity()), _slots(allocateSlots(other.capacity())) {
			copyOther(other);
		}

		SwissTable& operator=(const SwissTable& other) {
			if(this == &other) return *this;
			clear();
			deallocateSlots(_slots, _slots + capacity());
			deallocate(_begin, _end);

			_size = other.size();
			_begin = allocate(other.capacity());
			_end = _begin + other.capacity();
			_slots = allocateSlots(other.capacity());

			copyOther(other);
			return *this;
		}

		SwissTable(SwissTable&& other) requires std::is_default_constructible_v<Policy>  {
			_policy = std::exchange(other._policy, Policy());
			_begin = std::exchange(other._begin, other.allocate(_initialCapacity));
			_end = std::exchange(other._end, other._begin);
			_slots = std::exchange(other._slots, other.allocateSlots(_initialCapacity));
			_size = std::exchange(other._size, 0);
		}

		SwissTable& operator=(SwissTable&& other) {
			if(this == &other) return *this;
			clear();
			deallocate(_begin, _end);

			_policy = std::exchange(other._policy, Policy());
			_begin = std::exchange(other._begin, other.allocate(_initialCapacity));
			_end = std::exchange(other._end, other._begin);
			_slots = std::exchange(other._slots, other.allocateSlots(_initialCapacity));
			_size = std::exchange(other._size, 0);

			return *this;
		}

		template<typename... Args>
		PointerType insert(Args&&... args) {
			ValueType value(std::forward<Args>(args)...);
			return insertImpl(value, _policy.hasher(value));
		}

		template<typename... Args>
		PointerType insert(SizeType hash, Args&&... args) {
			ValueType value(std::forward<Args>(args)...);
			XTD_DEBUG_VERIFY(hash == _policy.hasher(value), "Method was given an incorrect hash hint");
			return insertImpl(value, hash);
		}

		template<typename U>
		constexpr PointerType find(const U& value) {
			return const_cast<PointerType>(std::as_const(*this).find(value));
		}

		template<typename U>
		constexpr PointerType find(SizeType hash, const U& value) {
			return const_cast<PointerType>(std::as_const(*this).find(value, hash));
		}

		template<typename Key>
		constexpr const PointerType find(const Key& key) const { return findImpl(key, _policy.hasher(key)); }
		template<typename Key>
		constexpr const PointerType find(const Key& key, SizeType hash) const  { 
			XTD_DEBUG_VERIFY(hash == _policy.hasher(key), "Method was given an incorrect hash hint");
			return findImpl(key, hash); 
		}
		

		constexpr PointerType erase(PointerType it) { return eraseImpl(it); }
		
		template<typename Key>
		constexpr PointerType erase(const Key& key) { return eraseImpl(key, _policy.hasher(key)); }

		template<typename Key>
		constexpr PointerType erase(SizeType hash, const Key& key) { 
			XTD_DEBUG_VERIFY(hash == _policy.hasher(key), "Method was given an incorrect hash hint");
			return eraseImpl(key, hash); 
		}

		void clear() {
			for(auto it = _begin; it < _end; ++it) {
				if(it->occupied()) {
					it->data.destroy();
					it->setUnoccupied();
				}
			}
			_size = 0;
		}

		constexpr void rehash(size_t newCapacity) {
			if(newCapacity == capacity()) return;
			else if(newCapacity >= _size) rehashImpl(newCapacity);
			else rehashImpl(_size);
		}

        constexpr bool empty() const { return _size == 0; }
        constexpr SizeType size() const { return _size; }
		constexpr SizeType capacity() const { return _end - _begin; }

        constexpr PointerType begin() { return _begin; }
        constexpr PointerType end() { return _end; }
        constexpr const PointerType begin() const { return _begin; }
        constexpr const PointerType end() const { return _end; }
        constexpr const PointerType cbegin() const { return _begin; }
        constexpr const PointerType cend() const { return _end; }

		constexpr float loadFactor() const { return static_cast<float>(_size) / static_cast<float>(capacity()); }

	private:
		void rehashImpl(SizeType newCap) {
			auto newBegin = allocate(newCap);
			auto newEnd = newBegin + newCap;
			auto newSlots = allocateSlots(newCap);
			auto newSlotsEnd = newSlots + newCap;
			auto slot = _slots;

			for(auto it = _begin; it < _end; ++it, ++slot) {
				if(*slot != _empty && *slot != _tombstone) {
					auto hash = _policy.hasher(*it);
					auto H1 = getH1(hash);
					auto H2 = getH2(hash);
					auto offset = (H1 & (newCap - 1));
					auto ideal = newSlots + offset;
					auto newIt = newBegin + offset;
					insertLoop<false>(*it, H2, slot, newIt, newSlotsEnd, ideal);
					xtd::memory::destroyAt(it);
				}
			}
			deallocateSlots(_slots, _slots + capacity());
			deallocate(_begin, _end);

			_slots = newSlots;
			_begin = newBegin;
			_end = newEnd;
		}

		template<typename Key>
		PointerType findImpl(const Key& key, SizeType hash) {
			auto H1 = getH1(hash);
			auto H2 = getH2(hash);
			auto offset = (H1 & (capacity() - 1));
			auto ideal = _slots + offset;
			auto it = _begin + offset;
			auto slotsEnd = _slots + capacity();
			return findLoop<false, Key>(key, H2, ideal, it, slotsEnd, ideal);
		}

		template<bool wrappedAround, typename Key>
		PointerType findLoop(const Key& key, std::byte H2, std::byte* slot, PointerType it, std::byte* end, std::byte* ideal) {
			// TODO: add SIMD path
			for(; slot < end; ++slot, ++it) {
				if (*slot == _empty) return _end;
				if (*slot == _tombstone) continue;
				if (*slot == H2 && _policy.comparator(*it, key)) return it;
			}
			if constexpr(!wrappedAround) {
				return findLoop<true, Key>(key, H2, _slots, _begin, ideal, ideal);
			}
			else {
				return _end;
			}
		}

		PointerType insertImpl(ValueType& val, SizeType hash) {
			if(loadFactor() >= loadThreshold) rehashImpl(capacity() * 2);
			auto H1 = getH1(hash);
			auto H2 = getH2(hash);
			auto offset = (H1 & (capacity() - 1));
			auto ideal = _slots + offset;
			auto it = _begin + offset;
			auto slotsEnd = _slots + capacity();
			++_size;
			return insertLoop<false>(val, H2, ideal, it, slotsEnd, ideal);
		}

		template<bool wrappedAround>
		PointerType insertLoop(ValueType& val, std::byte H2, std::byte* slot, PointerType it, std::byte* end, std::byte* ideal) {
			// TODO: add SIMD path
			for(; slot < end; ++slot, ++it) {
				if (*slot == _empty || *slot == _tombstone) {
					xtd::memory::moveOrCopyTo(slot, val);
					return slot;
				}
				if (*slot == H2 && _policy.comparator(*it, val)) {
					*it = val;
					return slot;
				}
			}
			if constexpr(!wrappedAround) {
				return insertLoop<true>(val, H2, _slots, _begin, ideal, ideal);
			}
			else {
				return _end; // This shouldn't be possible to reach
			}
		}

		PointerType eraseImpl(PointerType it) {
			// TODO: add SIMD path
			--_size;
			auto slot = _slots + (it - _begin);
			auto next = slot + 1;
			if(*next == _empty) *slot = _empty;
			else *slot = _tombstone;
			xtd::memory::destroyAt(it);
			return it;
		}

		std::byte getH2(SizeType hash) {
			constexpr const SizeType mask = ((SizeType(1) << 8) - 1);
			return static_cast<std::byte>(hash & mask);
		}

		SizeType getH1(SizeType hash) {
			return hash >> 7;
		}

		SizeType grow(SizeType requiredCapacity) {
			return xtd::utility::alignNextPowerOf2(requiredCapacity);
		}

		PointerType allocate(SizeType size) {
			return _policy.alloc.allocate(size * sizeof(ValueType), alignof(ValueType));
		}

		void deallocate(PointerType begin, PointerType end) {
			_policy.alloc.deallocate(begin, (end - begin) * sizeof(ValueType), alignof(ValueType));
		}

		std::byte* allocateSlots(SizeType size) {
			return _policy.alloc.allocate(size * sizeof(std::byte), alignof(std::byte));
		}

		void deallocateSlots(std::byte* begin, std::byte* end) {
			_policy.alloc.deallocate(begin, (end - begin) * sizeof(std::byte), alignof(std::byte));
		}
	};
}