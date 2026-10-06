#pragma once
#include <cstdlib>
#include <cstdint>
#include <utility>
#include <limits>

#include "xtd/math/Utilities.h"
#include "xtd/math/UintOperations.h"
#include "xtd/containers/Array.h"
#include "xtd/containers/String.h"

namespace xtd::math {
    template<size_t limbCount>
	class BigUint : public containers::SpanFunctions<BigUint<limbCount>, uint64_t, size_t, containers::ContiguousIteratorBase<const uint64_t>, BigUintBase<BigUint<limbCount>>>  {
		template<size_t limbCount>
		friend class BigUint;
	public:
		using UintTag = void;
		static inline const size_t storageSize = sizeof(uint64_t) * 8;
		static inline const size_t byteSize = limbCount * sizeof(uint64_t);
		static inline const size_t bitSize = byteSize * 8;

        using ValueType = uint64_t;
        using SizeType = size_t;
        using PointerType = ValueType*;
        using IteratorType = typename xtd::containers::Array<uint64_t, limbCount>::IteratorType;
        using ConstIteratorType = typename xtd::containers::Array<uint64_t, limbCount>::ConstIteratorType;

        using Self = BigUint<limbCount>;
        using Base = containers::SpanFunctions<BigUint<limbCount>, uint64_t, size_t, containers::ContiguousIteratorBase<const uint64_t>, BigUintBase<BigUint<limbCount>>>;

	private:
		xtd::containers::Array<uint64_t, limbCount> _limbs;

	public:
		constexpr BigUint() noexcept = default;
		constexpr ~BigUint() noexcept = default;

		constexpr BigUint(const BigUint&) noexcept = default;
		constexpr BigUint(BigUint&&) noexcept = default;

		constexpr BigUint& operator=(const BigUint&) noexcept = default;
		constexpr BigUint& operator=(BigUint&&) noexcept = default;

		template<size_t limbCount2>
		constexpr BigUint(const BigUint<limbCount2>& other) noexcept {
            convertNum(_limbs, other._limbs);
		}

		template<size_t limbCount2>
		constexpr BigUint(BigUint<limbCount2>&& other) noexcept {
            convertNum(_limbs, other._limbs);
		}

		template<size_t limbCount2>
		constexpr BigUint& operator=(const BigUint<limbCount2>& other) noexcept {
            convertNum(_limbs, other._limbs);
			return *this;
		}

		template<size_t limbCount2>
		constexpr BigUint& operator=(BigUint<limbCount2>&& other) noexcept {
            convertNum(_limbs, other._limbs);
			return *this;
		}

		template<size_t otherLimbCount>
		constexpr BigUint(const xtd::containers::Array<uint64_t, otherLimbCount>& other) noexcept : _limbs(other) {
		}

		template<size_t otherLimbCount>
		constexpr BigUint(xtd::containers::Array<uint64_t, otherLimbCount>&& other) noexcept : _limbs(std::move(other)) {
		}

		template<std::integral T>
		constexpr BigUint(const T& other) noexcept : _limbs{} {
			convertNum(_limbs, other);
		}

		template<std::integral T>
		constexpr BigUint& operator=(const T& other) noexcept {
			convertNum(_limbs, other);
			return *this;
		}

		template<std::integral T>
		constexpr BigUint(xtd::containers::StringView str) noexcept : _limbs{} {
			stringToInt(str, *this);
		}

		template<std::integral T>
		constexpr BigUint& operator=(xtd::containers::StringView str) noexcept {
			stringToInt(str, *this);
			return *this;
		}

		bool operator<(const BigUint& other) const noexcept
		{
			return compare(*this, other) == CompareResult::Less;
		}

		bool operator>(const BigUint& other) const noexcept
		{
			return compare(*this, other) == CompareResult::Greater;
		}

		bool operator<=(const BigUint& other) const noexcept
		{
			return compare(*this, other) != CompareResult::Greater;
		}

		bool operator>=(const BigUint& other) const noexcept
		{
			return compare(*this, other) != CompareResult::Less;
		}

		bool operator!=(const BigUint& other) const noexcept
		{
			return compare(*this, other) != CompareResult::Equal;
		}

		bool operator==(const BigUint& other) const noexcept
		{
			return compare(*this, other) == CompareResult::Equal;
		}

		inline constexpr uint64_t& operator[](size_t index) noexcept { return _limbs[index]; };
		inline constexpr const uint64_t& operator[](size_t index) const noexcept { return _limbs[index]; };

		void fromString(xtd::containers::StringView str) {
			stringToInt(str, *this);
		}

		// inline xtd::containers::String toString() const {
		// 	xtd::containers::String str = bigIntToString(_limbs);
		// 	return str;
		// }

		inline constexpr bool isOdd() const noexcept {
			return !(_limbs.front() & 1);
		}

		inline constexpr bool isEven() const noexcept {
			return _limbs.front() & 1;
		}

		static inline constexpr SizeType limbByteSize() { return sizeof(ValueType); }
		static inline constexpr SizeType limbBitSize() { return limbByteSize() * 8; }

		static inline constexpr SizeType byteCount() { return limbByteSize() * size(); }
		static inline constexpr SizeType bitCount() { return limbBitSize() * size(); }

		inline constexpr auto& data() noexcept { return _limbs.data(); };
		inline constexpr const auto& data() const noexcept { return _limbs.data(); };

        constexpr inline IteratorType begin() noexcept { return _limbs.begin(); }
        constexpr inline IteratorType end() noexcept { return _limbs.end(); }
        constexpr inline ConstIteratorType begin() const noexcept { return _limbs.begin(); }
        constexpr inline ConstIteratorType end() const noexcept { return _limbs.end(); }
        constexpr inline ConstIteratorType cbegin() const noexcept { return begin(); }
        constexpr inline ConstIteratorType cend() const noexcept { return end(); }
        
        static inline constexpr SizeType size() noexcept { return limbCount; };

		auto& limbs() { return _limbs; }
		const auto& limbs() const { return _limbs; }

        ValueType& back() { return _limbs.back(); }
        const ValueType& back() const { return _limbs.back(); }

        ValueType& front() { return _limbs.front(); }
        const ValueType& front() const { return _limbs.front(); }

		std::string toBinary() const;
	};

	using u128 = BigUint<2>;
	using u256 = BigUint<4>;
	using u512 = BigUint<8>;

	template<char... Cs>
	constexpr auto operator""_u128() requires (deduceByteSize<Cs...>() <= 16)  { return stringToInt<BigUint<2>, Cs...>(); };
	template<char... Cs>
	constexpr auto operator""_u256() requires (deduceByteSize<Cs...>() <= 32)  { return stringToInt<BigUint<4>, Cs...>(); };
	template<char... Cs>
	constexpr auto operator""_u512() requires (deduceByteSize<Cs...>() <= 64) { return stringToInt<BigUint<8>, Cs...>(); };
	template<char... Cs>
	constexpr auto operator""_u() { return stringToInt<BigUint<(deduceByteSize<Cs...>() - 1) / 8 + 1>, Cs...>(); };
}