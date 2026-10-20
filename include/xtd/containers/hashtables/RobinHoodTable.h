#pragma once
#include "xtd/containers/Traits.h"
#include "xtd/containers/Slot.h"
#include "xtd/Macros.h"
#include "xtd/debug/Checks.h"
#include "xtd/utility/Functions.h"
#include "xtd/allocators/Traits.h"

namespace xtd::containers {

    template<typename T>
	class RobinHoodIteratorBase {
	public:
		using TableType = T;
		using BucketType = typename TableType::BucketType;
		using ValueType = typename BucketType::ValueType;
		using PointerType = ValueType*;

	private:
		BucketType* _bucket;

	public:
		constexpr RobinHoodIteratorBase() noexcept = default;
		constexpr RobinHoodIteratorBase(BucketType *bucket, TableType&) noexcept : _bucket(bucket) {
			while(_bucket->unoccupied()) ++_bucket;
		}

		template <typename U>
		requires std::convertible_to<const U *, BucketType *>
		constexpr RobinHoodIteratorBase(U *bucket, TableType&) noexcept : _bucket(bucket) {
			while(_bucket->unoccupied()) ++_bucket;
		};

		template <typename U>
		requires std::convertible_to<const U *, ValueType *>
		constexpr RobinHoodIteratorBase(RobinHoodIteratorBase<U> it) noexcept : _bucket(it.bucket()) {
			while(_bucket->unoccupied()) ++_bucket;
		};

		template <typename U>
		requires std::convertible_to<const U *, ValueType *>
		constexpr RobinHoodIteratorBase operator=(RobinHoodIteratorBase<U> it) noexcept {
			_bucket = it.bucket();
		};

		constexpr ValueType &operator*() const noexcept { return value(); }
		constexpr ValueType *operator->() const noexcept { return data(); }
		constexpr RobinHoodIteratorBase &operator++() noexcept {
			++_bucket;
			while(_bucket->unoccupied()) ++_bucket;
			return *this;
		}
		constexpr RobinHoodIteratorBase operator++(int) noexcept {
			RobinHoodIteratorBase temp = *this;
			++_bucket;
			while(_bucket->unoccupied()) ++_bucket;
			return temp;
		}

		constexpr bool operator==(const RobinHoodIteratorBase &other) const noexcept { return _bucket == other._bucket; }
		constexpr bool operator!=(const RobinHoodIteratorBase &other) const noexcept { return _bucket != other._bucket; }
		constexpr bool operator<(const RobinHoodIteratorBase &other) const noexcept { return _bucket < other._bucket; }
		constexpr bool operator<=(const RobinHoodIteratorBase &other) const noexcept { return _bucket <= other._bucket; }
		constexpr bool operator>(const RobinHoodIteratorBase &other) const noexcept { return _bucket > other._bucket; }
		constexpr bool operator>=(const RobinHoodIteratorBase &other) const noexcept { return _bucket >= other._bucket; }
		constexpr ValueType* data() noexcept { return _bucket->data(); }
		constexpr const ValueType* data() const noexcept { return _bucket->data(); };
		constexpr ValueType* value() noexcept { return _bucket->value(); }
		constexpr const ValueType* value() const noexcept { return _bucket->value(); };
		constexpr BucketType* bucket() noexcept { return _bucket; }
		constexpr const BucketType* bucket() const noexcept { return _bucket; };
	};

	template<typename S>
	struct RobinHoodSentinelBucket {
		using SizeType = S;
		static inline const S _sentinel = std::numeric_limits<S>::max() - 1;
		S distance;
		bool sentinel() const { return distance != _sentinel; }
		void setSentinel() { this->distance = _sentinel; }
	};

	template<typename T, typename S>
	struct RobinHoodBucket : RobinHoodSentinelBucket<S> {
		using ValueType = T;
		using SizeType = S;
		static inline const S _unoccupied = std::numeric_limits<S>::max();
		Slot<T> data;
		S hash;
		bool unoccupied() const { return this->distance == _unoccupied; }
		bool occupied() const { return this->distance != _unoccupied; }
		void setUnoccupied() { this->distance = _unoccupied; }
		void setDistance(S d) { this->distance = d; }
	};

    template<typename H, typename C, xtd::allocators::AllocatorT A, GrowthStrategyT G>
    struct HashTableRobinHoodPolicy {
		using HasherType = H;
		using ComparatorType = C;
        using AllocatorType = A;
        using GrowthType = G;
    };

	template<typename T, typename Policy, size_t initialCapacity = 16, float loadThreshold = 0.75f>
	class RobinHoodTable {
		static_assert(loadThreshold > 0 && loadThreshold <= 1.0f, "loadThreshold must be between 0 and 1 and cannot be exactly 0");
		static_assert(initialCapacity > 0, "initialCapacity must be greater than 0");
		
	public:
        using ValueType = T;
        using SizeType = size_t;
        using PointerType = ValueType*;
		using BucketType = RobinHoodBucket<T, size_t>;
		using SentinelBucketType = RobinHoodSentinelBucket<size_t>;
        
        using HasherType = typename Policy::HasherType;
		using ComparatorType = typename Policy::ComparatorType;
		using AllocatorType = typename Policy::AllocatorType;
		using GrowthType = typename Policy::GrowthType;

		using IteratorType = RobinHoodIteratorBase<BucketType>;
		using ConstIteratorType = RobinHoodIteratorBase<const BucketType>;

        using Self = RobinHoodTable<ValueType, Policy, initialCapacity, loadThreshold>;

        struct Policy {
            XTD_NO_UNIQUE_ADDRESS HasherType hasher;
            XTD_NO_UNIQUE_ADDRESS ComparatorType comparator;
			XTD_NO_UNIQUE_ADDRESS AllocatorType alloc;
			XTD_NO_UNIQUE_ADDRESS GrowthType growth;
        };

	private:
		BucketType* _begin;
		BucketType* _end;
		SizeType _size;
		XTD_NO_UNIQUE_ADDRESS Policy _policy;

	public:
		RobinHoodTable() requires std::is_default_constructible_v<Policy> : _size(0), _begin(allocate(initialCapacity)), _end(_begin + initialCapacity) {
			sentinel()->setSentinel();
		}
		RobinHoodTable(Policy&& policy) requires std::is_default_constructible_v<Policy> 
			: _policy(std::move(policy)), _size(0), _begin(allocate(initialCapacity)), _end(_begin + initialCapacity) {
			sentinel()->setSentinel();
		}

		~RobinHoodTable() {
			clear();
			deallocate(_begin, _end);
		}

		RobinHoodTable(const RobinHoodTable& other) requires std::is_default_constructible_v<Policy> 
			: _size(other.size()), _begin(allocate(other.capacity())), _end(_begin + other.capacity()) {
			copyOther(other);
			sentinel()->setSentinel();
		}

		RobinHoodTable& operator=(const RobinHoodTable& other) {
			if(this == &other) return *this;
			clear();
			deallocate(_begin, _end);

			_size = other.size();
			_begin = allocate(other.capacity());
			_end = _begin + other.capacity();
			sentinel()->setSentinel();

			copyOther(other);
			return *this;
		}

		RobinHoodTable(RobinHoodTable&& other) requires std::is_default_constructible_v<Policy>  {
			_policy = std::exchange(other._policy, Policy());
			_begin = std::exchange(other._begin, other.allocate(initialCapacity));
			_end = std::exchange(other._end, other._begin);
			_size = std::exchange(other._size, 0);
			other.sentinel()->setSentinel();
		}

		RobinHoodTable& operator=(RobinHoodTable&& other) {
			if(this == &other) return *this;
			clear();
			deallocate(_begin, _end);

			_policy = std::exchange(other._policy, Policy());
			_begin = std::exchange(other._begin, other.allocate(initialCapacity));
			_end = std::exchange(other._end, other._begin);
			_size = std::exchange(other._size, 0);
			other.sentinel()->setSentinel();

			return *this;
		}

		template<typename... Args>
		BucketType* insert(Args&&... args) {
			ValueType value(std::forward<Args>(args)...);
			return insertImpl(value, _policy.hasher(value));
		}

		template<typename... Args>
		BucketType* insert(SizeType hash, Args&&... args) {
			ValueType value(std::forward<Args>(args)...);
			XTD_DEBUG_VERIFY(hash == _policy.hasher(value), "Method was given an incorrect hash hint");
			return insertImpl(value, hash);
		}

		template<typename U>
		constexpr BucketType* find(const U& value) {
			return const_cast<BucketType*>(std::as_const(*this).find(value));
		}

		template<typename U>
		constexpr BucketType* find(SizeType hash, const U& value) {
			return const_cast<BucketType*>(std::as_const(*this).find(value, hash));
		}

		template<typename Key>
		constexpr const BucketType* find(const Key& key) const { return findImpl(key, _policy.hasher(key)); }
		template<typename Key>
		constexpr const BucketType* find(const Key& key, SizeType hash) const  { 
			XTD_DEBUG_VERIFY(hash == _policy.hasher(key), "Method was given an incorrect hash hint");
			return findImpl(key, hash); 
		}
		

		constexpr BucketType* erase(BucketType* it) { return eraseImpl(it); }
		
		template<typename Key>
		constexpr BucketType* erase(const Key& key) { return eraseImpl(key, _policy.hasher(key)); }

		template<typename Key>
		constexpr BucketType* erase(SizeType hash, const Key& key) { 
			XTD_DEBUG_VERIFY(hash == _policy.hasher(key), "Method was given an incorrect hash hint");
			return eraseImpl(key, hash); 
		}

		[[nodiscard]] constexpr BucketType* eraseAndAdvance(BucketType* it) {
			eraseImpl(it);
			if(it->unoccupied() && it < _end) ++it;
			return it;
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

        constexpr BucketType* begin() { return _begin; }
        constexpr BucketType* end() { return _end; }
        constexpr const BucketType* begin() const { return _begin; }
        constexpr const BucketType* end() const { return _end; }
        constexpr const BucketType* cbegin() const { return _begin; }
        constexpr const BucketType* cend() const { return _end; }

		constexpr float loadFactor() const { return static_cast<float>(_size) / static_cast<float>(capacity()); }

	private:
		constexpr void rehashImpl(size_t newCapacity) {
			BucketType* newBegin = allocate(newCapacity);
			BucketType* newEnd = newBegin + newCapacity;

			for(BucketType* it = _begin; it < _end; ++it) {
				if(it->occupied()) {
					SizeType hash = it->hash;
					auto ideal = newBegin + (hash % newCapacity);
					auto tmp = ideal;
					SizeType distance = 0;
					insertLoop<false, true>(*(it->data), distance, it->hash, tmp, newEnd, ideal);
					it->data.destroy();
				}
			}
			
			deallocate(_begin, _end);
			_begin = newBegin;
			_end = newEnd;
			sentinel()->setSentinel();
		}

		constexpr inline BucketType* insertImpl(ValueType& value, SizeType hash) {
			if(loadFactor() >= loadThreshold) {
				rehashImpl(_policy.growth.grow(capacity()));
			}
			++_size;
			auto ideal = _begin + (hash % capacity());
			auto it = ideal;
			SizeType distance = 0;
			// insert loop cannot fail because capacity is guaranteed to be present
			return insertLoop<true, true>(value, distance, hash, it, _end, ideal);
		}

		template<bool checkUnique, bool firstHalf>
		constexpr inline BucketType* insertLoop(ValueType& value, SizeType& distance, SizeType& hash, BucketType*& it, BucketType* end, BucketType* ideal) {
			for(; it < end; ++it) {
				if(it->unoccupied()) {
					it->distance = distance;
					it->hash = hash;
					it->data.moveOrCopy(value);
					return it;
				}
				if constexpr (checkUnique) {
					if(it->hash == hash && _policy.comparator(*(it->data), value)) {
						it->data->destroy();
						it->data.moveOrCopy(value);
						return it;
					}
				}
				if(it->distance < distance) {
					xtd::utility::swap(it->distance, distance);
					xtd::utility::swap(it->hash, hash);
					xtd::utility::swap(*(it->data), value);
					if constexpr (checkUnique) {
						auto ret = it;
						insertLoop<false, firstHalf>(value, distance, hash, it, ideal, ideal);
						return ret;
					}
					else {
						continue;
					}
				}
			}
			if constexpr (firstHalf) {
				it = _begin;
				insertLoop<checkUnique, false>(value, distance, hash, it, ideal, ideal);
			}
		}

		template<typename Key>
		constexpr const BucketType* findImpl(const Key& key, SizeType hash) const {
			auto ideal = _begin + (hash % capacity());
			return findLoop<Key, true>(key, hash, 0, ideal, _end, ideal);
		}

		template<typename Key, bool firstPass>
		constexpr inline const BucketType* findLoop(const Key& key, SizeType hash, SizeType distance, BucketType* it, BucketType* end, BucketType* ideal) const {
			for(; it < end; ++it, ++distance) {
				if(it->unoccupied() || it->distance < distance) return _end;
				if(it->hash == hash && _policy.comparator(key, *(it->data))) return it;
			}
			if constexpr(firstPass) {
				it = _begin;
				return findLoop<Key, false>(key, hash, distance, it, ideal, ideal);
			}
			return _end;
		}

		template<typename Key>
		constexpr BucketType* eraseImpl(const Key& key, SizeType hash) {
			return eraseImpl(findImpl(key, hash));
		}

		template<typename Key>
		constexpr BucketType* eraseImpl(BucketType* it) {
			--_size;
			eraseLoop<Key, true>(it, _end - 1, it);
			return it;
		}

		template<typename Key, bool firstPass>
		constexpr inline BucketType* eraseLoop(BucketType* it, BucketType* end, BucketType* border) const {
			for(;it < end; ++it) {
				auto next = it + 1;
				if(next->unoccupied() || next->distance == 0) {
					it->data.destroy();
					it->setUnoccupied();
					return next;
				}
				it->distance = next->distance - 1;
				it->hash = next->hash;
				it->data.destroy();
				it->data.moveOrCopy(*(next->data));
			}
			if constexpr(firstPass) {
				auto next = _begin;
				if(next->unoccupied() || next->distance == 0) {
					it->data.destroy();
					it->setUnoccupied();
					return next;
				}
				it->distance = next->distance - 1;
				it->hash = next->hash;
				it->data.destroy();
				it->data.moveOrCopy(*(next->data));
				return eraseLoop<Key, false>(_begin, border, border);
			}
			return _end;
		}

		BucketType* allocate(SizeType size) {
			return  _policy.alloc.allocate(size * sizeof(BucketType) + sizeof(SentinelBucketType), alignof(BucketType));
		}

		BucketType* deallocate(BucketType* begin, BucketType* end) {
			return _policy.alloc.deallocate(begin, (end - begin) * sizeof(BucketType) + sizeof(SentinelBucketType), alignof(BucketType));
		}

		SentinelBucketType* sentinel() { return static_cast<SentinelBucketType*>(_end); }
		const SentinelBucketType* sentinel() const { return static_cast<const SentinelBucketType*>(_end); }

		void copyOther(const RobinHoodTable& other) {
			auto it = _begin;
			auto itOther = other._begin;
			for(; it < _end; ++it, ++itOther) {
				if(it->occupied()) {
					it->hash = itOther->hash;
					it->distance = itOther->distance;
					it->data.copy(*(itOther->data));
				}
				else {
					it->setUnoccupied();
				}
			}
		}
	};
}