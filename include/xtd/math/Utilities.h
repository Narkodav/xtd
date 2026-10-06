#pragma once
#include <concepts>
#include <cstdlib>
#include <cstdint>
#include <type_traits>
#include <limits>
#include <bit>
#include <utility>

#include "xtd/containers/Array.h"

// Intrinsic like helpers
namespace xtd::math {
    template <typename T>
    requires std::is_unsigned_v<T>
	static inline std::pair<T, T> multFallback(T a, T b) {
		static const size_t halfBits = sizeof(T) * 4;
		static const T lowMask = (T(1) << halfBits) - 1;
	
		T aHigh = a >> halfBits;
		T bHigh = b >> halfBits;
	
		T aLow = a & lowMask;
		T bLow = b & lowMask;
	
		T x = aLow * bLow;
		T s0 = x & lowMask;
	
		x = aHigh * bLow + (x >> halfBits);
	
		T s1 = x & lowMask;
		T s2 = x >> halfBits;
	
		x = s1 + aLow * bHigh;
		s1 = x & lowMask;
	
		x = s2 + aHigh * bHigh + (x >> halfBits);
		s2 = x & lowMask;
		T s3 = x >> halfBits;
	
		T high = s3 << halfBits | s2;
		T low = s1 << halfBits | s0;
	
		return { low, high };
	}

	static inline std::pair<uint64_t, uint64_t> divModFallback(uint64_t hi, uint64_t lo, uint64_t divisor) {
		//std::pair<uint64_t, uint64_t> res;
		// Handle division by zero
		if (divisor == 0 || hi >= divisor) {
			// Division by zero or hi > divisor - behavior is undefined, return 0 and remainder 0
			return {0, 0};
		}
		
		uint64_t q = 0, r = hi;
		for (int i = 0; i < 64; ++i) {
			bool top = r >> 63;
			r = (r << 1) | (lo >> 63);
			lo <<= 1;
			q <<= 1;
			if (top || r >= divisor) { r -= divisor; q |= 1; }
		}
		return { q, r };
	}

	//helper struct to get the next greater sized type for example if T = uint16_t, NextSizedInteger::Type = uint32_t
	template<typename T>
		requires std::is_unsigned_v<T>
	struct NextSizedInteger {
		static_assert(false, "Unknown integer type");
	};

	template<>
	struct NextSizedInteger<uint8_t> {
		using Type = uint16_t;
	};

	template<>
	struct NextSizedInteger<uint16_t> {
		using Type = uint32_t;
	};

	template<>
	struct NextSizedInteger<uint32_t> {
		using Type = uint64_t;
	};

	template <std::unsigned_integral T>
	static inline bool compareGreater(T hiLeft, T loLeft, T hiRight, T loRight) {		
		if constexpr (sizeof(T) == 8) {
	#if defined(__SIZEOF_INT128__)
			__uint128_t left = (static_cast<__uint128_t>(hiLeft) << 64) | loLeft;
			__uint128_t right = (static_cast<__uint128_t>(hiRight) << 64) | loRight;
			return left > right;
	#else
			return hiLeft > hiRight || ((hiLeft == hiRight) && (loLeft > loRight));
	#endif
		}
		else {
			static const size_t bits = sizeof(T) * 8;
			using WideT = typename NextSizedInteger<T>::Type;
			WideT left = (static_cast<WideT>(hiLeft) << bits) | loLeft;
			WideT right = (static_cast<WideT>(hiRight) << bits) | loRight;
			return left > right;
		}
	}

	template <typename T>
		requires std::is_unsigned_v<T>
	static inline std::pair<T, T> mult(T a, T b) {
		static const size_t bits = sizeof(T) * 8;
		using WideT = typename NextSizedInteger<T>::Type;
		WideT res = static_cast<WideT>(a) * b;
		return { static_cast<T>(res), static_cast<T>(res >> bits) };
	}

	template<>
	inline std::pair<uint64_t, uint64_t> mult<uint64_t>(uint64_t a, uint64_t b) {
    // ---------------- MSVC intrinsic ----------------
	#if defined(_MSC_VER) && !defined(__SIZEOF_INT128__)
		uint64_t hi;
		uint64_t lo = _umul128(a, b, &hi);
		return { lo, hi };
	#endif
	
		// ---------------- GCC / Clang __uint128_t ----------------
	#if defined(__SIZEOF_INT128__)
		__uint128_t res = static_cast<__uint128_t>(a) * b;
		uint64_t lo = static_cast<uint64_t>(res);
		uint64_t hi = static_cast<uint64_t>(res >> 64);
		return { lo, hi };
	#endif

		// ---------------- Branchless fallback (manual 64-bit split) ----------------
		return multFallback(a, b);
	}

	// ================== Addition with carry ==================
	template <typename T>
	requires std::is_unsigned_v<T>
	static inline std::pair<T, bool> addWithCarry(T a, T b, bool carry) // returns a pair where first is result and second is carry
	{
		// ---------------- MSVC intrinsics ----------------
	#if defined(_MSC_VER) && !defined(__SIZEOF_INT128__)
		if constexpr (sizeof(T) == 8) {
			T res;
			carry = _addcarry_u64(static_cast<unsigned char>(carry), a, b, &res);
			return { res, carry };
		}
		else if constexpr (sizeof(T) == 4) {
			T res;
			carry = _addcarry_u32(static_cast<unsigned char>(carry), a, b, &res);
			return { res, carry };
		}
	#endif

		// ---------------- __uint128_t ----------------	
		if constexpr (sizeof(T) == 8) {
	#if defined(__SIZEOF_INT128__)
			__uint128_t sum = static_cast<__uint128_t>(a) + b + carry;
			carry = static_cast<bool>(sum >> 64);
			return { static_cast<uint64_t>(sum), carry };
		// ---------------- Fallback: GCC / Clang builtins ----------------
	#elif defined(__GNUC__) || defined(__clang__)
			T res;
			auto carryIntermediate = __builtin_add_overflow(a, b, &res);
			carry = __builtin_add_overflow(res, static_cast<T>(carry), &res) | carryIntermediate;
			return { res, carry };
	#else
			// ---------------- Branchless fallback ----------------
			T res = a + b + carry;
			carry = (res < a) || (carry && res == a);
			return { res, carry };
	#endif
		}
		else {
			static const size_t bits = sizeof(T) * 8;
			using WideT = typename NextSizedInteger<T>::Type;
			WideT res = static_cast<WideT>(a) + b + carry;
			carry = static_cast<bool>(res >> bits);
			return { static_cast<T>(res), carry };
		}
	}

	template <typename T>
	requires std::is_unsigned_v<T>
	static inline std::pair<T, T> addWithBigCarry(T a, T b, T carry) // returns a pair where first is result and second is carry
	{
		if constexpr (sizeof(T) == 8) {
	#if defined(__SIZEOF_INT128__)
			__uint128_t sum = static_cast<__uint128_t>(a) + b + carry;
			return { static_cast<uint64_t>(sum), static_cast<uint64_t>(sum >> 64) };
	#else
			// ---------------- Branchless fallback ----------------
			auto [ sum1, carry1 ] = addWithCarry(a, b, 0);
			auto [ sum2, carry2 ] = addWithCarry(sum1, carry, 0);
			return { sum2, T(carry1) + T(carry2) };
	#endif
		}
		else {
			static const size_t bits = sizeof(T) * 8;
			using WideT = typename NextSizedInteger<T>::Type;
			WideT sum = static_cast<WideT>(a) + b + carry;
			return { static_cast<T>(sum), static_cast<T>(sum >> bits) };
		}
	}

	// ================== Subtraction with borrow ==================
	template <typename T>
	requires std::is_unsigned_v<T>
	static inline std::pair<T, bool> subWithBorrow(T a, T b, bool borrow) // returns a pair where first is result and second is borrow
	{
		// ---------------- MSVC intrinsics ----------------
	#if defined(_MSC_VER) && !defined(__clang__)
		if constexpr (sizeof(T) == 8) {
			T res;
			borrow = _subborrow_u64(static_cast<unsigned char>(borrow), a, b, &res);
			return { res, borrow };
		}
		else if constexpr (sizeof(T) == 4) {
			T res;
			borrow = _subborrow_u32(static_cast<unsigned char>(borrow), a, b, &res);
			return { res, borrow };
		}
	#endif

		// ---------------- GCC / Clang builtins ----------------
	#if defined(__GNUC__) || defined(__clang__)
		{
			T res;
			auto borrowIntermediate = __builtin_sub_overflow(a, b, &res);
			borrow = __builtin_sub_overflow(res, static_cast<T>(borrow), &res) | borrowIntermediate;
			return { res, borrow };
		}
	#endif

		// ---------------- Branchless fallback ----------------
		T t = a - b;
		T res = t - borrow;
		borrow = (a < b) || (borrow && a == b);
		return { res, borrow };
	}

	template <typename T>
	requires std::is_unsigned_v<T>
	static inline std::pair<T, T> subWithBigBorrow(T a, T b, T borrow) // returns a pair where first is result and second is borrow
	{
		auto [ res1, borrow1 ] = subWithBorrow(a, b, 0);
		auto [ res2, borrow2 ] = subWithBorrow(res1, borrow, 0);
		return { res2, T(borrow1) + T(borrow2) };
	}

	template <std::unsigned_integral T>
	static inline T shiftRight(T hi, T lo, uint8_t shift) // returns low
	{
		// ---------------- MSVC intrinsics ----------------
	#if defined(_MSC_VER) && !defined(__clang__)
		if constexpr (sizeof(T) == 8) {
			T res;
			res = __shiftright128(lo, hi, shift);
			return res;
		}
	#endif

		// ---------------- Branchless fallback ----------------
		constexpr unsigned bits = sizeof(T) * 8;
		return static_cast<T>((lo >> shift) | (hi << (bits - shift)));
	}

	template <std::unsigned_integral T>
	static inline T shiftLeft(T hi, T lo, uint8_t shift) // returns high
	{
		// ---------------- MSVC intrinsics ----------------
	#if defined(_MSC_VER) && !defined(__clang__)
		if constexpr (sizeof(T) == 8) {
			T res;
			res = __shiftleft128(lo, hi, shift);
			return res;
		}
	#endif

		// ---------------- Branchless fallback ----------------
		constexpr unsigned bits = sizeof(T) * 8;
		return static_cast<T>((hi << shift) | (lo >> (bits - shift)));
	}

	// Convenience wrappers around std utilities
	template <std::unsigned_integral T>
	static inline uint8_t countLeadingZeroes(T num) {
		return std::countl_zero(num);
	}

	template <std::unsigned_integral T>
	static inline uint8_t countLeadingOnes(T num) {
		return std::countl_one(num);
	}

	template <std::unsigned_integral T>
	static inline uint8_t countRightZeroes(T num) {
		return std::countr_zero(num);
	}

	template <std::unsigned_integral T>
	static inline uint8_t countRightOnes(T num) {
		return std::countr_one(num);
	}

	template <std::unsigned_integral T>
	static inline std::pair<T, T> divMod(T hi, T lo, T divisor) // returns pair where first is quotient and second is remainder
	{
		if constexpr (sizeof(T) == 8) {
	#if defined(_MSC_VER) && !defined(__SIZEOF_INT128__)
			std::pair<T, T> res;
			res.first = _udiv128(hi, lo, divisor, &res.second);
			return res;
	#elif defined(__SIZEOF_INT128__)
			std::pair<T, T> res;
			__uint128_t divident = (static_cast<__uint128_t>(hi) << 64) | lo;			
			res.first = divident / divisor;
			res.second = divident % divisor;
			return res;
	#else
			return divModFallback(hi, lo, divisor);
	#endif
		}
		else {
			static const size_t bits = sizeof(T) * 8;
			using WideT = typename NextSizedInteger<T>::Type;
			std::pair<T, T> res;
			WideT divident = (static_cast<WideT>(hi) << bits) | lo;			
			res.first = divident / divisor;
			res.second = divident % divisor;
			return res;
		}
	}

	template<typename T>
	struct Base10Divisor {
		static_assert(false, "Type not implemented");
	};

	template<>
	struct Base10Divisor<uint64_t> {
		static inline const uint64_t s_value = 1'000'000'000'000'000'000; //10^18
	};

	template<>
	struct Base10Divisor<uint32_t> {
		static inline const uint32_t s_value = 1'000'000'000; //10^9
	};
}