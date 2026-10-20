#pragma once
#include "xtd/containers/Traits.h"
#include "xtd/containers/Slot.h"
#include "xtd/Macros.h"
#include "xtd/debug/Checks.h"
#include "xtd/memory/Lifetime.h"
#include "xtd/utility/Functions.h"
#include "xtd/allocators/Traits.h"
#include "xtd/simd/Functions.h"

#include <limits>
#include <bit>

namespace xtd::containers {

    template<typename T>
	class SwissTableIteratorBase {
	public:
		using TableType = T;
		using ValueType = typename TableType::ValueType;
		using PointerType = ValueType*;

		static constexpr std::byte _empty = static_cast<std::byte>(std::numeric_limits<uint8_t>::max()); 				// 0b11111111
		static constexpr std::byte _tombstone = static_cast<std::byte>(std::numeric_limits<uint8_t>::max() - 1); 		// 0b11111110
		static constexpr std::byte _sentinel = static_cast<std::byte>(std::numeric_limits<uint8_t>::max() - 2); 		// 0b11111101

	private:
		ValueType* _bucket;
		std::byte* _slot;

	public:
		constexpr SwissTableIteratorBase() noexcept = default;
		constexpr SwissTableIteratorBase(ValueType *bucket, TableType& table) noexcept 
			: _bucket(bucket), _slot(reinterpret_cast<std::byte*>(table.beginSlots()) + (_bucket - table.begin())) {
			while(*_slot > _sentinel) { ++_bucket; ++_slot; }
		}

		template <typename U>
		requires std::convertible_to<const U *, ValueType *>
		constexpr SwissTableIteratorBase(U *bucket, TableType& table) noexcept 
			: _bucket(bucket), _slot(reinterpret_cast<std::byte*>(table.beginSlots()) + (_bucket - table.begin())) {
			while(*_slot > _sentinel) { ++_bucket; ++_slot; }
		};

		template <typename U>
		requires std::convertible_to<const U *, ValueType *>
		constexpr SwissTableIteratorBase(SwissTableIteratorBase<U> it) noexcept : _bucket(it.bucket()), _slot(it.slot()) {
			while(*_slot > _sentinel) { ++_bucket; ++_slot; }
		};

		template <typename U>
		requires std::convertible_to<const U *, ValueType *>
		constexpr SwissTableIteratorBase operator=(SwissTableIteratorBase<U> it) noexcept {
			_bucket = it.bucket();
			_slot = it.slot();
		};

		constexpr ValueType &operator*() const noexcept { return value(); }
		constexpr ValueType *operator->() const noexcept { return data(); }
		constexpr SwissTableIteratorBase &operator++() noexcept {
			++_bucket;
			++_slot;
			while(*_slot > _sentinel) { ++_bucket; ++_slot; }
			return *this;
		}
		constexpr SwissTableIteratorBase operator++(int) noexcept {
			SwissTableIteratorBase temp = *this;
			++_bucket;
			++_slot;
			while(*_slot > _sentinel) { ++_bucket; ++_slot; }
			return temp;
		}

		constexpr bool operator==(const SwissTableIteratorBase &other) const noexcept { return _bucket == other._bucket; }
		constexpr bool operator!=(const SwissTableIteratorBase &other) const noexcept { return _bucket != other._bucket; }
		constexpr bool operator<(const SwissTableIteratorBase &other) const noexcept { return _bucket < other._bucket; }
		constexpr bool operator<=(const SwissTableIteratorBase &other) const noexcept { return _bucket <= other._bucket; }
		constexpr bool operator>(const SwissTableIteratorBase &other) const noexcept { return _bucket > other._bucket; }
		constexpr bool operator>=(const SwissTableIteratorBase &other) const noexcept { return _bucket >= other._bucket; }
		constexpr ValueType* data() noexcept { return _bucket->data(); }
		constexpr const ValueType* data() const noexcept { return _bucket->data(); };
		constexpr ValueType* value() noexcept { return _bucket->value(); }
		constexpr const ValueType* value() const noexcept { return _bucket->value(); };
		constexpr ValueType* bucket() noexcept { return _bucket; }
		constexpr const ValueType* bucket() const noexcept { return _bucket; };
		constexpr std::byte* slot() noexcept { return _slot; }
		constexpr const std::byte* slot() const noexcept { return _slot; };
	};

    template<typename H, typename C, xtd::allocators::AllocatorT A>
    struct SwissTablePolicy {
		using HasherType = H;
		using ComparatorType = C;
        using AllocatorType = A;
    };

	template<typename T, typename Policy, float _loadThreshold = 0.75f>
	class SwissTable {
		static_assert(_loadThreshold > 0 && _loadThreshold <= 1.0f, "_loadThreshold must be between 0 and 1 and cannot be exactly 0");
		static constexpr size_t _groupSize = sizeof(xtd::simd::I8x16);
        static constexpr size_t _initialCapacity = _groupSize;

		static constexpr std::byte _empty = static_cast<std::byte>(std::numeric_limits<uint8_t>::max()); 				// 0b11111111
		static constexpr std::byte _tombstone = static_cast<std::byte>(std::numeric_limits<uint8_t>::max() - 1); 		// 0b11111110
		static constexpr std::byte _sentinel = static_cast<std::byte>(std::numeric_limits<uint8_t>::max() - 2); 	// 0b11111101

		static constexpr xtd::simd::I8x16 _emptyMask = xtd::simd::I8x16(static_cast<int8_t>(_empty));
		static constexpr xtd::simd::I8x16 _tombstoneMask = xtd::simd::I8x16(static_cast<int8_t>(_tombstone));
		static constexpr xtd::simd::I8x16 _emptyThresholdMask = xtd::simd::I8x16(static_cast<int8_t>(_sentinel));

	public:
        using ValueType = T;
        using SizeType = size_t;
        using PointerType = ValueType*;
		using BucketType = ValueType;
        
        using HasherType = typename Policy::HasherType;
		using ComparatorType = typename Policy::ComparatorType;
		using AllocatorType = typename Policy::AllocatorType;

		using IteratorType = SwissTableIteratorBase<BucketType>;
		using ConstIteratorType = SwissTableIteratorBase<const BucketType>;

        using Self = SwissTable<ValueType, Policy, _loadThreshold>;

        struct Policy {
            XTD_NO_UNIQUE_ADDRESS HasherType hasher;
            XTD_NO_UNIQUE_ADDRESS ComparatorType comparator;
			XTD_NO_UNIQUE_ADDRESS AllocatorType alloc;
        };

	private:
		PointerType _begin;
		PointerType _end;
        xtd::simd::I8x16* _slotsBegin;
		xtd::simd::I8x16* _slotsEnd;

		SizeType _size;
		SizeType _tombstoneCount;
		XTD_NO_UNIQUE_ADDRESS Policy _policy;

	public:
		SwissTable() requires std::is_default_constructible_v<Policy> : _begin(allocate(_initialCapacity)), _end(_begin + _initialCapacity), 
			_slotsBegin(allocateSlots(_initialCapacity / _groupSize)),  _slotsEnd(_slotsBegin + _initialCapacity / _groupSize),
			_size(0), _tombstoneCount(0), _policy() {}

		SwissTable(Policy&& policy) requires std::is_default_constructible_v<Policy> 
			: _begin(allocate(_initialCapacity)), _end(_begin + _initialCapacity), 
			_slotsBegin(allocateSlots(_initialCapacity / _groupSize)),  _slotsEnd(_slotsBegin + _initialCapacity / _groupSize),
			_size(0), _tombstoneCount(0), _policy(std::move(policy)) {}

		~SwissTable() {
			clear();
			deallocateSlots(_slotsBegin, _slotsEnd);
			deallocate(_begin, _end);
		}

		SwissTable(const SwissTable& other) requires std::is_default_constructible_v<Policy> 
			: _begin(allocate(other.capacity())), _end(_begin + other.capacity()),
			_slotsBegin(allocateSlots(other.capacity() / _groupSize)),  _slotsEnd(_slotsBegin + other.capacity() / _groupSize),
			_size(other.size()), _tombstoneCount(other._tombstoneCount), _policy(other._policy) {
			copyOther(other);
		}

		SwissTable& operator=(const SwissTable& other) {
			if(this == &other) return *this;
			clear();
			deallocateSlots(_slotsBegin, _slotsEnd);
			deallocate(_begin, _end);

			_tombstoneCount = other._tombstoneCount;
			_size = other.size();
			_begin = allocate(other.capacity());
			_end = _begin + other.capacity();
			_slotsBegin = allocateSlots(other.capacity() / _groupSize);
			_slotsEnd = _slotsBegin + other.capacity() / _groupSize;

			copyOther(other);
			return *this;
		}

		SwissTable(SwissTable&& other) requires std::is_default_constructible_v<Policy>  {
			_policy = std::exchange(other._policy, Policy());
			_begin = std::exchange(other._begin, other.allocate(_initialCapacity));
			_end = std::exchange(other._end, other._begin);
			_slotsBegin = std::exchange(other._slotsBegin, other.allocateSlots(_initialCapacity / _groupSize));
			_slotsEnd = std::exchange(other._slotsEnd, other._slotsBegin + _initialCapacity / _groupSize);
			_tombstoneCount = std::exchange(other._tombstoneCount, 0);
			_size = std::exchange(other._size, 0);
		}

		SwissTable& operator=(SwissTable&& other) {
			if(this == &other) return *this;
			clear();
			deallocate(_begin, _end);

			_policy = std::exchange(other._policy, Policy());
			_begin = std::exchange(other._begin, other.allocate(_initialCapacity));
			_end = std::exchange(other._end, other._begin);
			_slotsBegin = std::exchange(other._slotsBegin, other.allocateSlots(_initialCapacity / _groupSize));
			_slotsEnd = std::exchange(other._slotsEnd, other._slotsBegin + _initialCapacity / _groupSize);
			_tombstoneCount = std::exchange(other._tombstoneCount, 0);
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
			auto group = _slotsBegin;
			auto it = _begin;
			for(; group < _slotsEnd; it += 16, ++group) {
				uint32_t occupied = ~static_cast<uint16_t>(xtd::simd::movemask(xtd::simd::cmpgt(group, _emptyThresholdMask)));
				while(occupied) {
					int i = std::countr_zero(occupied);
					auto ins = it + i;
					xtd::memory::destroyAt(ins);
					occupied &= occupied - 1u; //Clears lowest 1 bit
				}
			}
			std::memset(_slotsBegin, std::numeric_limits<uint64_t>::max(), capacity() * sizeof(xtd::simd::I8x16));
			_tombstoneCount = 0;
			_size = 0;
		}

		constexpr void rehash(size_t newCapacity) {
			if(newCapacity == capacity()) return;
			else if(newCapacity >= _size) rehashImpl(xtd::utility::alignNextPowerOf2(newCapacity));
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

		constexpr auto beginSlots() { return _slotsBegin; }
        constexpr auto endSlots() { return _slotsEnd; }
        constexpr const auto beginSlots() const { return _slotsBegin; }
        constexpr const auto endSlots() const { return _slotsEnd; }
        constexpr const auto cbeginSlots() const { return _slotsBegin; }
        constexpr const auto cendSlots() const { return _slotsEnd; }

		constexpr float loadFactor() const { return static_cast<float>(_size) / static_cast<float>(capacity()); }
		constexpr float tombstoneFactor() const { return static_cast<float>(_size + _tombstoneCount) / static_cast<float>(capacity()); }

	private:
		inline void rehashImpl(SizeType newCap) {
			auto newBegin = allocate(newCap);
			auto newEnd = newBegin + newCap;
			auto newSlots = allocateSlots(newCap / _groupSize);
			auto newSlotsEnd = newSlots + newCap / _groupSize;
			auto group = _slotsBegin;
			auto it = _begin;

			for(; group < _slotsEnd; it += 16, ++group) {
				uint32_t occupied = ~static_cast<uint16_t>(xtd::simd::movemask(xtd::simd::cmpgt(group, _emptyThresholdMask)));

				while(occupied) {
					int i = std::countr_zero(occupied);
					auto ins = it + i;
					auto hash = _policy.hasher(*ins);
					auto H1 = getH1(hash);
					auto H2 = getH2(hash);
					auto offset = (H1 & (capacity() - 1)) / _groupSize;
					auto ideal = newSlots + offset;
					auto newIt = newBegin + offset * 16;
					insertLoop<false>(*ins, ideal, newIt, newSlotsEnd, ideal);
					xtd::memory::destroyAt(ins);
					occupied &= occupied - 1u; //Clears lowest 1 bit
				}
			}
			deallocateSlots(_slotsBegin, _slotsEnd);
			deallocate(_begin, _end);

			_tombstoneCount = 0;
			_slotsBegin = newSlots;
			_slotsEnd = newSlotsEnd;
			_begin = newBegin;
			_end = newEnd;
		}

		template<typename Key>
		inline PointerType findImpl(const Key& key, SizeType hash) {
			auto H1 = getH1(hash);
			auto H2 = getH2(hash);
			auto offset = (H1 & (capacity() - 1)) / _groupSize;
			auto ideal = _slotsBegin + offset;
			auto it = _begin + offset * 16;
			return findLoop<false, Key>(key, H2, ideal, it, _slotsEnd, ideal);
		}

		template<bool wrappedAround, typename Key>
		inline PointerType findLoop(const Key& key, xtd::simd::I8x16 H2Mask, xtd::simd::I8x16* group, PointerType it, xtd::simd::I8x16* end, xtd::simd::I8x16* ideal) {
			for(; group < end; ++group, it += 16) {
				uint32_t matches = xtd::simd::movemask(xtd::simd::cmpeq(group, H2Mask));

				while (matches) {
					int i = std::countr_zero(matches);
					auto ptr = it + i;
					if (_policy.comparator(*ptr, key)) return ptr;
					matches &= matches - 1u; //Clears lowest 1 bit
				}

				if (xtd::simd::movemask(xtd::simd::cmpeq(group, _emptyMask))) return _end;
			}
			if constexpr(!wrappedAround) {
				return findLoop<true, Key>(key, H2Mask, _slotsBegin, _begin, ideal, ideal);
			}
			else {
				return _end;
			}
		}

		inline PointerType insertImpl(ValueType& val, SizeType hash) {
			if(loadFactor() >= _loadThreshold) rehashImpl(capacity() * 2);
			if(tombstoneFactor() >= _loadThreshold) rehashImpl(capacity());
			auto H1 = getH1(hash);
			auto H2 = getH2(hash);
			auto offset = (H1 & (capacity() - 1)) / _groupSize;
			auto ideal = _slotsBegin + offset;
			auto it = _begin + offset * 16;
			++_size;
			return insertLoop<false>(val, ideal, it, _slotsEnd, ideal);
		}

		template<bool wrappedAround>
		inline PointerType insertLoop(ValueType& val, xtd::simd::I8x16* group, PointerType it, xtd::simd::I8x16* end, xtd::simd::I8x16* ideal) {
			for(; group < end; ++group, it += 16) {
				uint32_t unoccupied = xtd::simd::movemask(xtd::simd::cmpgt(group, _emptyThresholdMask));
				if(unoccupied) {
					int i = std::countr_zero(unoccupied);
					auto ins = it + i;
					auto byte = reinterpret_cast<std::byte*>(group) + i;
					if(*byte == _tombstone) --_tombstoneCount;
					xtd::memory::moveOrCopyTo(ins, val);
					return ins;
				}
			}
			if constexpr(!wrappedAround) {
				return findLoop<true>(val, _slotsBegin, _begin, ideal, ideal);
			}
			else {
				return _end;
			}
		}

		template<typename Key>
		constexpr PointerType eraseImpl(const Key& key, SizeType hash) {
			return eraseImpl(findImpl(key, hash));
		}

		inline PointerType eraseImpl(PointerType it) {
			--_size;
			auto offset = (it - _begin);
			auto group = _slotsBegin + (offset / _groupSize);
			auto slot = reinterpret_cast<std::byte*>(_slotsBegin) + offset;

			uint32_t empties = xtd::simd::movemask(xtd::simd::cmpeq(group, _emptyMask));
			if(empties) *slot = _empty;
			else {
				*slot = _tombstone;
				++_tombstoneCount;
			}
			
			xtd::memory::destroyAt(it);
			return it;
		}

		inline std::byte getH2(SizeType hash) {
			constexpr const SizeType mask = ((SizeType(1) << 8) - 1);
			return static_cast<std::byte>(hash & mask);
		}

		inline SizeType getH1(SizeType hash) {
			return hash >> 7;
		}

		inline PointerType allocate(SizeType size) {
			return _policy.alloc.allocate(size * sizeof(ValueType), alignof(ValueType));
		}

		inline void deallocate(PointerType begin, PointerType end) {
			_policy.alloc.deallocate(begin, (end - begin) * sizeof(ValueType), alignof(ValueType));
		}

		inline xtd::simd::I8x16* allocateSlots(SizeType size) {
			auto bytes = size * sizeof(xtd::simd::I8x16) + sizeof(std::byte);
			auto slots = _policy.alloc.allocate(bytes, alignof(xtd::simd::I8x16));
			std::memset(slots, std::numeric_limits<uint64_t>::max(), bytes);
			*(slots + bytes - 1) = _sentinel;
			return slots;
		}

		inline void deallocateSlots(xtd::simd::I8x16* begin, xtd::simd::I8x16* end) {
			_policy.alloc.deallocate(begin, (end - begin) * sizeof(xtd::simd::I8x16) + sizeof(std::byte), alignof(xtd::simd::I8x16));
		}
	};
}