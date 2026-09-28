#pragma once
#include <cstdlib>
#include <cstdint>
#include <utility>
#include <limits>

#include "xtd/math/Utilities.h"
#include "xtd/math/UintOperations.h"
#include "xtd/containers/Array.h"

namespace xtd::math {

    template<size_t ByteCount, typename LimbType>
	class UintImpl : public containers::SpanFunctions<UintImpl<ByteCount, LimbType>, LimbType, size_t, containers::ContiguousIteratorBase<const LimbType>, UintBase<UintImpl<ByteCount, LimbType>>>  {
		template<size_t ByteCount, typename LimbType>
		friend class UintImpl;
	public:
		using UintTag = void;
		static inline const size_t storageSize = sizeof(LimbType) * 8;
		static inline const size_t limbCount = (ByteCount - 1) / sizeof(LimbType) + 1;
		static inline const size_t byteSize = limbCount * sizeof(LimbType);
		static inline const size_t bitSize = byteSize * 8;

        using ValueType = LimbType;
        using SizeType = size_t;
        using PointerType = ValueType*;
        using IteratorType = typename xtd::containers::Array<LimbType, limbCount>::IteratorType;
        using ConstIteratorType = typename xtd::containers::Array<LimbType, limbCount>::ConstIteratorType;

        using Self = UintImpl<byteSize, LimbType>;
        using Base = UintBase<UintImpl<ByteCount, LimbType>>;

	private:
		xtd::containers::Array<LimbType, limbCount> _limbs;

	public:
		constexpr UintImpl() noexcept = default;
		constexpr ~UintImpl() noexcept = default;

		constexpr UintImpl(const UintImpl&) noexcept = default;
		constexpr UintImpl(UintImpl&&) noexcept = default;

		constexpr UintImpl& operator=(const UintImpl&) noexcept = default;
		constexpr UintImpl& operator=(UintImpl&&) noexcept = default;

		template<size_t ByteCount2, typename LimbType2>
		constexpr UintImpl(const UintImpl<ByteCount2, LimbType2>& other) noexcept {
            convertNum(_limbs, other._limbs);
		}

		template<size_t ByteCount2, typename LimbType2>
		constexpr UintImpl(UintImpl<ByteCount2, LimbType2>&& other) noexcept {
            convertNum(_limbs, other._limbs);
		}

		template<size_t ByteCount2, typename LimbType2>
		constexpr UintImpl& operator=(const UintImpl<ByteCount2, LimbType2>& other) noexcept {
            convertNum(_limbs, other._limbs);
			return *this;
		}

		template<size_t ByteCount2, typename LimbType2>
		constexpr UintImpl& operator=(UintImpl<ByteCount2, LimbType2>&& other) noexcept {
            convertNum(_limbs, other._limbs);
			return *this;
		}

		template<size_t otherLimbCount>
		constexpr UintImpl(const xtd::containers::Array<LimbType, otherLimbCount>& other) noexcept : _limbs(other) {
		}

		template<size_t otherLimbCount>
		constexpr UintImpl(xtd::containers::Array<LimbType, otherLimbCount>&& other) noexcept : _limbs(std::move(other)) {
		}

		template<std::integral T>
		constexpr UintImpl(const T& other) noexcept : _limbs{} {
			convertNum(_limbs, other);
		}

		template<std::integral T>
		constexpr UintImpl& operator=(const T& other) noexcept {
			convertNum(_limbs, other);
			return *this;
		}

		template<size_t digits>
		constexpr UintImpl(xtd::containers::Span<const char, digits> str) noexcept : _limbs{} {
			_limbs = stringToBigInt<LimbType, digits>(str);
		}

		bool operator<(const UintImpl& other) const noexcept
		{
			return compare(_limbs, other._limbs) == CompareResult::Less;
		}

		bool operator>(const UintImpl& other) const noexcept
		{
			return compare(_limbs, other._limbs) == CompareResult::Greater;
		}

		bool operator<=(const UintImpl& other) const noexcept
		{
			return compare(_limbs, other._limbs) != CompareResult::Greater;
		}

		bool operator>=(const UintImpl& other) const noexcept
		{
			return compare(_limbs, other._limbs) != CompareResult::Less;
		}

		bool operator!=(const UintImpl& other) const noexcept
		{
			return compare(_limbs, other._limbs) != CompareResult::Equal;
		}

		bool operator==(const UintImpl& other) const noexcept
		{
			return compare(_limbs, other._limbs) == CompareResult::Equal;
		}

		inline constexpr LimbType& operator[](size_t index) noexcept { return _limbs[index]; };
		inline constexpr const LimbType& operator[](size_t index) const noexcept { return _limbs[index]; };

		void fromString(std::string_view str) {
			stringToBigInt(xtd::containers::Span<const char>(str.data(), str.size()), _limbs);
		}

		inline std::string toString() const {
			std::string str = bigIntToString(_limbs);
			return str;
		}

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
        constexpr inline ConstIteratorType end() const noexcept { _limbs.end(); }
        constexpr inline ConstIteratorType cbegin() const noexcept { return begin(); }
        constexpr inline ConstIteratorType cend() const noexcept { return end(); }
        
        static inline constexpr SizeType size() noexcept { return limbCount; };

        ValueType& back() { return _limbs.back(); }
        const ValueType& back() const { return _limbs.back(); }

        ValueType& front() { return _limbs.front(); }
        const ValueType& front() const { return _limbs.front(); }

		std::string toBinary() const;
	};

	template<size_t ByteCount>
	struct UintSelector;

	template<size_t ByteCount>
	requires (ByteCount <= sizeof(uint8_t))
	struct UintSelector<ByteCount> { using Type = UintImpl<ByteCount, uint8_t>; };

	template<size_t ByteCount>
	requires (ByteCount <= sizeof(uint16_t) && ByteCount > sizeof(uint8_t))
	struct UintSelector<ByteCount> { using Type = UintImpl<ByteCount, uint16_t>; };

	template<size_t ByteCount>
	requires (ByteCount <= sizeof(uint32_t) && ByteCount > sizeof(uint16_t))
	struct UintSelector<ByteCount> { using Type = UintImpl<ByteCount, uint32_t>; };

	template<size_t ByteCount>
	requires (ByteCount <= sizeof(uint64_t) && ByteCount > sizeof(uint32_t))
	struct UintSelector<ByteCount> { using Type = UintImpl<ByteCount, uint64_t>; };

// #if defined(__SIZEOF_INT128__)
// 	template<size_t ByteCount>
// 	requires (ByteCount <= sizeof(__uint128_t) && ByteCount > sizeof(uint64_t))
// 	struct UintSelector<ByteCount> { using Type = UintImpl<ByteCount, __uint128_t>; };
// #endif

	template<size_t ByteCount>
	requires (ByteCount > sizeof(uint64_t))
	struct UintSelector<ByteCount> { using Type = UintImpl<ByteCount, uint64_t>; };

	template<size_t ByteCount>
	using Uint = typename UintSelector<ByteCount>::Type;
}