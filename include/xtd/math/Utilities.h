#pragma once
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

	static inline std::pair<uint64_t, uint64_t> divModFallback(uint64_t high, uint64_t low, uint64_t divisor) {
		//std::pair<uint64_t, uint64_t> res;
		// Handle division by zero
		if (divisor == 0) {
			// Division by zero - behavior is undefined, return 0 and remainder 0
			return {0, 0};
		}
		
		// Special case: if high < divisor, we can use standard 64-bit division
		if (high < divisor) {
			//uint64_t quotient;
			
			// Combine high and low into a 128-bit number and divide
			// Using binary long division algorithm
			uint64_t remainder_high = high;
			uint64_t result = 0;
			
			for (int i = 0; i < 64; i++) {
				// Shift result left by 1
				result <<= 1;
				
				// Shift the high bit of low into remainder_high
				remainder_high = (remainder_high << 1) | ((low >> 63) & 1);
				low <<= 1;
				
				// If remainder_high >= divisor, subtract and set result bit
				if (remainder_high >= divisor) {
					remainder_high -= divisor;
					result |= 1;
				}
			}
			
			return {result, remainder_high};
		}
		else {
			// When high >= divisor, we need full 128-bit division
			// Using binary long division algorithm for 128 bits
			
			uint64_t result = 0;
			
			// Combine high and low into a 128-bit number by shifting and adding bits
			// Process all 128 bits
			for (int i = 0; i < 128; i++) {
				// Shift result left by 1
				result <<= 1;
				
				// Get the most significant bit of the current dividend
				uint64_t msb = (high >> 63) & 1;
				
				// Shift the 128-bit dividend left by 1
				high = (high << 1) | ((low >> 63) & 1);
				low <<= 1;
				
				// Check if we can subtract divisor from high
				// For the first 64 bits, we use high as the remainder
				// For the last 64 bits, we need to be careful about overflow
				
				if (i < 64) {
					// First 64 bits: high contains the working remainder
					if (high >= divisor) {
						high -= divisor;
						result |= 1;
					}
				} else {
					// Last 64 bits: high might be large, but we can still compare
					// We need to handle the case where high >= divisor
					if (high >= divisor || (high == divisor && msb)) {
						high -= divisor;
						result |= 1;
					}
				}
			}
			
			return {result, high};
		}
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

	template <typename T>
		requires std::is_unsigned_v<T>
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
		static const size_t bits = sizeof(T) * 8;
		using WideT = typename NextSizedInteger<T>::Type;
		WideT left = (static_cast<WideT>(hiLeft) << bits) | loLeft;
		WideT right = (static_cast<WideT>(hiRight) << bits) | loRight;
		return left > right;
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
	static inline std::pair<T, T> addWithCarry(T a, T b, bool carry) // returns a pair where first is result and second is carry
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
			auto carryIntermediate = __builtin_uaddll_overflow(a, b, &res);
			carry = __builtin_uaddll_overflow(res, static_cast<T>(carry), &res) | carryIntermediate;
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
			return { sum2, carry1 + carry2 };
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
	static inline std::pair<T, T> subWithBorrow(T a, T b, bool borrow) // returns a pair where first is result and second is borrow
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
		if constexpr (sizeof(T) == 8) {
			T res;
			auto borrowIntermediate = __builtin_usubll_overflow(a, b, &res);
			borrow = __builtin_usubll_overflow(res, static_cast<T>(borrow), &res) | borrowIntermediate;
			return { res, borrow };
		}
		else if constexpr (sizeof(T) == 4) {
			T res;
			auto borrowIntermediate = __builtin_usub_overflow(a, b, &res);
			borrow = __builtin_usub_overflow(res, static_cast<T>(borrow), &res) | borrowIntermediate;
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
		return { res2, borrow1 + borrow2 };
	}

	template <typename T>
	requires std::is_unsigned_v<T>
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
		T res = (lo >> shift) | (hi << (sizeof(T) - shift));
		return res;
	}

	template <typename T>
	requires std::is_unsigned_v<T>
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
		T res = (lo >> (sizeof(T) - shift)) | (hi << shift);
		return res;
	}

	// Convenience wrappers around std utilities
	template <typename T>
	requires std::is_unsigned_v<T>
	static inline uint8_t countLeadingZeroes(T num) {
		return std::countl_zero(num);
	}

	template <typename T>
	requires std::is_unsigned_v<T>
	static inline uint8_t countLeadingOnes(T num) {
		return std::countl_one(num);
	}

	template <typename T>
	requires std::is_unsigned_v<T>
	static inline uint8_t countRightZeroes(T num) {
		return std::countr_zero(num);
	}

	template <typename T>
	requires std::is_unsigned_v<T>
	static inline uint8_t countRightOnes(T num) {
		return std::countr_one(num);
	}

	template <typename T>
	requires std::is_unsigned_v<T>
	static inline std::pair<T, T> divMod(T hi, T lo, T divisor) // returns pair where first is quotient and second is remainder
	{
		if constexpr (sizeof(T) == 8) {
	#if defined(_MSC_VER) && !defined(__SIZEOF_INT128__)
			std::pair<T, T> res;
			res.first = _udiv128(hi, lo, divisor, &res.second);
			return res;
	#elif defined(__SIZEOF_INT128__)
			std::pair<T, T> res;
			__uint128_t divident = static_cast<__uint128_t>(hi << 64) | lo;			
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
			WideT divident = static_cast<WideT>(hi << bits) | lo;			
			res.first = divident / divisor;
			res.second = divident % divisor;
			return res;
		}
	}

	template<size_t s_digitCount>
	static inline constexpr size_t estimateByteCount() {
		return (s_digitCount * 3321928ULL / 1000000ULL + 7) / 8;
	}

	template<typename T, size_t s_digitCount>
	static inline constexpr size_t estimateLimbCount() {
		return (estimateByteCount<s_digitCount>() - 1) / sizeof(T) + 1;
	}

	template<typename T, size_t N>
		requires std::is_unsigned_v<T>
	xtd::containers::Array<T, N> stringToNumArray(xtd::containers::Span<const char> str, xtd::containers::Array<T, N>& limbs) {

		{
			T digit = str[0] - '0';
			limbs[0] = digit;
		}

		for (size_t i = 1; i < str.size(); ++i) {
			if (!std::isdigit(str[i])) {
				return {0};
			}

			// Multiply current number by 10
			auto temp = multiply(limbs, xtd::containers::Array<T, 1>{ 10 });

			// Add current digit
			T digit = str[i] - '0';
			if (digit != 0) {
				temp = add(temp, xtd::containers::Array<T, 1>{ digit });
			}

			limbs = temp;
		}

		return limbs;
	}

    template<typename LimbDst, auto sizeDst, typename LimbSrc, auto sizeSrc>
    void convertNum(xtd::containers::Array<LimbDst, sizeDst>& dst, const xtd::containers::Array<LimbSrc, sizeSrc>& src) {
        if constexpr(sizeof(LimbDst) == sizeof(LimbSrc)) {
            dst = src;
        }
        else if constexpr(sizeof(LimbDst) > sizeof(LimbSrc)) {
            dst = {};
            constexpr auto step = sizeof(LimbDst) / sizeof(LimbSrc);
            constexpr size_t limit = dst.size() * step >= src.size() ? src.size() / step : dst.size();
            for(size_t i = 0; i < limit; ++i) {
                LimbDst limb;
                for(size_t j = 0; j < step; ++j) {
                    limb |= src[i * step + j] << (j * sizeof(LimbSrc) * 8);
                }
                dst[i] = limb;
            }
        }
        else {
            dst = {};
            constexpr auto step = sizeof(LimbSrc) / sizeof(LimbDst);
            constexpr size_t limit = dst.size() >= src.size() * step ? src.size() : dst.size() / step;
            for(size_t i = 0; i < limit; ++i) {
                for(size_t j = 0; j < step; ++j) {
                    dst[i * step + j] = (src[i] >> (j * sizeof(LimbDst) * 8)) & ((LimbSrc(1) << (sizeof(LimbDst) * 8)) - 1);
                }
            }
        }
    }

    template<typename LimbDst, auto sizeDst, typename LimbSrc>
    void convertNum(xtd::containers::Array<LimbDst, sizeDst>& dst, const LimbSrc& src) {
        if constexpr(sizeof(LimbDst) >= sizeof(LimbSrc)) {
			dst = {};
            dst[0] = src;
        }
        else {
            dst = {};
            constexpr auto step = sizeof(LimbSrc) / sizeof(LimbDst);
            constexpr size_t limit = dst.size() >= step ? step : dst.size();
			for(size_t i = 0; i < limit; ++i) {
				dst[i] = src >> (i * sizeof(LimbDst) * 8) & ((LimbSrc(1) << (sizeof(LimbDst) * 8)) - 1);
			}
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