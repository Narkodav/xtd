#pragma once
#include "xtd/containers/Traits.h"
#include "xtd/containers/Slot.h"
#include "xtd/Macros.h"
#include "xtd/debug/Checks.h"
#include "xtd/utility/Functions.h"
#include "xtd/allocators/Traits.h"

namespace xtd::containers {
    template<typename B>
	class FlatIteratorBase {
	private:
		using BucketType = B;
		BucketType* _bucket;
	public:
		using ValueType = typename BucketType::ValueType;
		using PointerType = ValueType*;

		constexpr FlatIteratorBase() noexcept = default;
		constexpr FlatIteratorBase(BucketType *bucket) noexcept : _bucket(bucket) {
			while(_bucket->unoccupied()) ++_bucket;
		}

		template <typename U>
		requires std::convertible_to<const U *, BucketType *>
		constexpr FlatIteratorBase(U *bucket) noexcept : _bucket(bucket) {
			while(_bucket->unoccupied()) ++_bucket;
		};

		template <typename U>
		requires std::convertible_to<const U *, ValueType *>
		constexpr FlatIteratorBase(FlatIteratorBase<U> it) noexcept : _bucket(it.bucket()) {
			while(_bucket->unoccupied()) ++_bucket;
		};

		template <typename U>
		requires std::convertible_to<const U *, ValueType *>
		constexpr FlatIteratorBase operator=(FlatIteratorBase<U> it) noexcept {
			_bucket = it.bucket();
		};

		constexpr ValueType &operator*() const noexcept { return value(); }
		constexpr ValueType *operator->() const noexcept { return data(); }
		constexpr FlatIteratorBase &operator++() noexcept {
			++_bucket;
			while(_bucket->unoccupied()) ++_bucket;
			return *this;
		}
		constexpr FlatIteratorBase operator++(int) noexcept {
			FlatIteratorBase temp = *this;
			++_bucket;
			while(_bucket->unoccupied()) ++_bucket;
			return temp;
		}

		constexpr bool operator==(const FlatIteratorBase &other) const noexcept { return _bucket == other._bucket; }
		constexpr bool operator!=(const FlatIteratorBase &other) const noexcept { return _bucket != other._bucket; }
		constexpr bool operator<(const FlatIteratorBase &other) const noexcept { return _bucket < other._bucket; }
		constexpr bool operator<=(const FlatIteratorBase &other) const noexcept { return _bucket <= other._bucket; }
		constexpr bool operator>(const FlatIteratorBase &other) const noexcept { return _bucket > other._bucket; }
		constexpr bool operator>=(const FlatIteratorBase &other) const noexcept { return _bucket >= other._bucket; }
		constexpr ValueType* data() noexcept { return _bucket->data(); }
		constexpr const ValueType* data() const noexcept { return _bucket->data(); };
		constexpr ValueType* value() noexcept { return _bucket->value(); }
		constexpr const ValueType* value() const noexcept { return _bucket->value(); };
		constexpr BucketType* bucket() noexcept { return _bucket; }
		constexpr const BucketType* bucket() const noexcept { return _bucket; };

		constexpr bool occupied() const { return _bucket->occupied(); }
		constexpr bool unoccupied() const { return _bucket->unoccupied(); }
	};
}