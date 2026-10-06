#pragma once
#include <cstdlib>
#include <cstdint>
#include <type_traits>
#include <limits>
#include <bit>
#include <utility>
#include <iostream>

#include "xtd/containers/String.h"
#include "xtd/math/Utilities.h"

#include "xtd/containers/ByteArray.h"

// Intrinsic like helpers
namespace xtd::math {

    template<typename Derived>
	class StaticBigNumBase {
    public:
		Derived& derived() { return *static_cast<Derived*>(this); }
		const Derived& derived() const { return *static_cast<const Derived*>(this); }
	};

    template<typename Derived>
	class BigIntBase : public StaticBigNumBase<Derived> {};

    template<typename Derived>
	class BigUintBase : public StaticBigNumBase<Derived> {};

	template<char... Cs>
	constexpr size_t deduceByteSize() {
		// count significant digits (skip leading zeros and ' separators)
		// hex: 4 bits/digit, binary: 1, octal: 3
		// decimal: digits * 3322 / 1000 + 1   (log2(10) ~ 3.3219, rounded up)
		// then round up to a multiple of your word size
		char chars[] = { Cs... };
		if(sizeof...(Cs) <= 2) return 1;
		else if(chars[0] == '0' && chars[1] == 'b') {
			size_t digitCount = 0;
			size_t i = 2;
			for(; i < (sizeof...(Cs)); ++i) {
				if(chars[i] == '1') break;
			}
			for(; i < (sizeof...(Cs)); ++i) {
				if(chars[i] >= '0' && chars[i] <= '1') ++digitCount;
			}
			return (xtd::utility::alignNextPowerOf2(digitCount) - 1) / 8 + 1;
		}
		else if(chars[0] == '0' && chars[1] == 'x') {
			size_t digitCount = 0;
			size_t i = 2;
			for(; i < (sizeof...(Cs)); ++i) {
				if((chars[i] > '0' && chars[i] <= '9') || (chars[i] >= 'A' && chars[i] <= 'F')|| (chars[i] >= 'a' && chars[i] <= 'f')) break;
			}
			for(; i < (sizeof...(Cs)); ++i) {
				if((chars[i] >= '0' && chars[i] <= '7') || (chars[i] >= 'A' && chars[i] <= 'F')|| (chars[i] >= 'a' && chars[i] <= 'f')) ++digitCount;
			}
			return (xtd::utility::alignNextPowerOf2(digitCount * 4) - 1) / 8 + 1;
		}
		else if(chars[0] == '0' && chars[1] == 'o') {
			size_t digitCount = 0;
			size_t i = 2;
			for(; i < (sizeof...(Cs)); ++i) {
				if(chars[i] > '0' && chars[i] <= '7') break;
			}
			for(; i < (sizeof...(Cs)); ++i) {
				if(chars[i] >= '0' && chars[i] <= '7') ++digitCount;
			}
			return (xtd::utility::alignNextPowerOf2(digitCount * 3) - 1) / 8 + 1;
		}
		else {
			size_t digitCount = 0;
			size_t i;
			if(chars[0] == '0' && chars[1] == 'd') i = 2;
			else i = 0;
			for(; i < (sizeof...(Cs)); ++i) {
				if(chars[i] > '0' && chars[i] <= '9') break;
			}
			for(; i < (sizeof...(Cs)); ++i) {
				if(chars[i] >= '0' && chars[i] <= '9') ++digitCount;
			}
			return (xtd::utility::alignNextPowerOf2(digitCount * 3322 / 1000 + 1) - 1) / 8 + 1;
		}
	};

	constexpr bool validateHex(char c) { return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'); }
	constexpr bool validateDec(char c) { return c >= '0' && c <= '9'; }
	constexpr bool validateOct(char c) { return c >= '0' && c <= '8'; }
	constexpr bool validateBin(char c) { return c >= '0' && c <= '1'; }

	constexpr uint8_t getHex(char c) { 
		if(c >= '0' && c <= '9') return c - '0';
		else if(c >= 'A' && c <= 'F') return c - 'A' + 10;
		else return c - 'a' + 10;
	}
	constexpr uint8_t getDec(char c) { return c - '0'; }
	constexpr uint8_t getOct(char c) { return c - '0'; }
	constexpr uint8_t getBin(char c) { return c - '0'; }

	template<size_t multiplier, bool sign, typename DigitValidateFunc, typename GetDigitFunc, typename Num>
	constexpr void stringToIntParseLoop(xtd::containers::StringView str, Num& num, DigitValidateFunc&& validate, GetDigitFunc&& getDigit) {
		using ValueType = typename Num::ValueType;
		while((str.front() == '0' || str.front() == '\'') && str.size() > 0) {
			if (!validate(str.front())) throw "invalid digit";
			str = str.subspan(str.begin() + 1, str.end());
		}
		if(str.size() == 0) return;
		{
			ValueType digit = getDigit(str.front());
			if constexpr (sign == true) { num = add(num, digit).first; }
			else { num = subtract(num, digit).first; }
			str = str.subspan(str.begin() + 1, str.end());
		}
		while(str.front() != '\0' && str.size() > 0) {
			if (str.front() == '\'') continue;
			ValueType digit = getDigit(str.front());
			num = multiply(num, multiplier).first;
			if constexpr (sign == true) { num = add(num, digit).first; }
			else { num = subtract(num, digit).first; }
			str = str.subspan(str.begin() + 1, str.end());
		}
	}

	template<typename Num>
	constexpr void stringToInt(xtd::containers::StringView str, StaticBigNumBase<Num>& num) {
		auto& numDer = num.derived();
		numDer = {};

		if(str.front() == '-') {
			str = str.subspan(str.begin() + 1, str.end());
			if(str.size() <= 2) {
				stringToIntParseLoop<10, false>(str, numDer, validateDec, getDec);
			}
			else {
				if(str[0] == '0' && str[1] == 'b') {
					str = str.subspan(str.begin() + 2, str.end());
					stringToIntParseLoop<2, false>(str, numDer, validateBin, getBin);
				}
				else if(str[0] == '0' && str[1] == 'o') {
					str = str.subspan(str.begin() + 2, str.end());
					stringToIntParseLoop<8, false>(str, numDer, validateOct, getOct);
				}
				else if(str[0] == '0' && str[1] == 'd') {
					str = str.subspan(str.begin() + 2, str.end());
					stringToIntParseLoop<10, false>(str, numDer, validateDec, getDec);
				}
				else if(str[0] == '0' && str[1] == 'x') {
					str = str.subspan(str.begin() + 2, str.end());
					stringToIntParseLoop<16, false>(str, numDer, validateHex, getHex);
				}
				else {
					stringToIntParseLoop<10, false>(str, numDer, validateDec, getDec);
				}
			}
		}
		else {
			if(str.front() == '+') {
				str = str.subspan(str.begin() + 1, str.end());
			}

			if(str.size() <= 2) {
				stringToIntParseLoop<10, true>(str, numDer, validateDec, getDec);
			}
			else {
				if(str[0] == '0' && str[1] == 'b') {
					str = str.subspan(str.begin() + 2, str.end());
					stringToIntParseLoop<2, true>(str, numDer, validateBin, getBin);
				}
				else if(str[0] == '0' && str[1] == 'o') {
					str = str.subspan(str.begin() + 2, str.end());
					stringToIntParseLoop<8, true>(str, numDer, validateOct, getOct);
				}
				else if(str[0] == '0' && str[1] == 'd') {
					str = str.subspan(str.begin() + 2, str.end());
					stringToIntParseLoop<10, true>(str, numDer, validateDec, getDec);
				}
				else if(str[0] == '0' && str[1] == 'x') {
					str = str.subspan(str.begin() + 2, str.end());
					stringToIntParseLoop<16, true>(str, numDer, validateHex, getHex);
				}
				else {
					stringToIntParseLoop<10, true>(str, numDer, validateDec, getDec);
				}
			}
		}
	}

	template<typename Num, char... Cs>
	constexpr void stringToInt(StaticBigNumBase<Num>& num) {
		char chars[] = { Cs..., '\0' };
		stringToInt(chars, num);
	}

	template<typename Num, char... Cs>
	constexpr auto stringToInt() {
		Num num;
		stringToInt<Num, Cs...>(num);
		return num;
	}

    template<typename Dst, typename Src>
    constexpr void convertNum(BigUintBase<Dst>& dst, const BigUintBase<Src>& src) {
		static_assert(std::endian::native == std::endian::little, "convertNum: byte-copy path assumes a little-endian target");
		static_assert(Src::byteCount() <= Dst::byteCount(), "convertNum: source is larger than destination");

		if constexpr (Dst::byteCount() > Src::byteCount()) {
			dst.derived().limbs() = {};   // zero the high limbs before copying in the low ones
		}
		xtd::utility::copy(static_cast<const uint8_t*>(src.derived().data()), static_cast<const uint8_t*>(src.derived().data()) + Src::byteCount(),
			static_cast<uint8_t*>(dst.derived().data()));
    }

    template<typename Dst, typename Src>
    constexpr void convertNum(BigUintBase<Dst>& dst, const Src& src) {
		static_assert(std::endian::native == std::endian::little, "convertNum: byte-copy path assumes a little-endian target");
		static_assert(sizeof(Src) <= Dst::byteCount(), "convertNum: source is larger than destination");

		if constexpr (Dst::byteCount() > sizeof(Src)) {
			dst.derived().limbs() = {};   // zero the high limbs before copying in the low ones
		}
		xtd::utility::copy(static_cast<const uint8_t*>(&src), static_cast<const uint8_t*>(&src) + sizeof(Src),
			static_cast<uint8_t*>(dst.derived().data()));
    }

	// Shifting by a number greater than the bit count is UB, mirroring the c++ convention
	template<typename Num>
	constexpr void shiftLeft(StaticBigNumBase<Num>& val, uint64_t shift)
	{
        auto& derVal = val.derived();
        if constexpr (Num::size() == 1) {
            derVal[0] << shift;
        }
        else {
            if (shift == 0) return;
            auto limbShift = shift / Num::limbBitSize();
            auto bitShift = shift % Num::limbBitSize();
			auto it = derVal.end() - 1;
			if(bitShift == 0) {
				for(; it >= derVal.begin() + limbShift; --it) *it = *(it - limbShift);
			}
			else {
				for(; it >= derVal.begin() + limbShift + 1; --it) {
					auto correspond = it - limbShift;
					*it = *correspond << bitShift;
					*it |= *(correspond - 1) >> (Num::limbBitSize() - bitShift);
				}
				auto correspond = it - limbShift;
				*it = *correspond << bitShift;
				--it;
			}
            for(; it >= derVal.begin(); --it) *it = 0;
        }
	}

	// Shifting by a number greater than the bit count is UB, mirroring the c++ convention
	template<typename Num>
	constexpr void shiftRight(BigUintBase<Num>& val, uint64_t shift)
	{
        auto& derVal = val.derived();
        if constexpr (Num::size() == 1) {
            derVal[0] >> shift;
        }
        else {
            if (shift == 0) return;
            auto limbShift = shift / Num::limbBitSize();
            auto bitShift = shift % Num::limbBitSize();

            auto it = derVal.begin();

			if(bitShift == 0) {
				for(; it < derVal.end() - limbShift; ++it) *it = *(it + limbShift);
			}
			else {
				for(; it < derVal.end() - limbShift - 1; ++it) {
					auto correspond = it + limbShift;
					*it = *correspond >> bitShift;
					*it |= *(correspond + 1) << (Num::limbBitSize() - bitShift);
				}
				auto correspond = it + limbShift;
				*it = *correspond >> bitShift;
				++it;
			}
            for(; it < derVal.end(); ++it) *it = 0;
        }
	}

	enum class CompareResult
	{
		Less = -1,
		Equal = 0,
		Greater = 1
	};

	template<typename Left, typename Right>
	constexpr CompareResult compare(const BigUintBase<Left>& l, const BigUintBase<Right>& r) {
		auto& derL = l.derived();
		auto& derR = r.derived();
		size_t i;
		if constexpr (Left::size() > Right::size()) {
			i = derL.size() - 1;
            for (; i != derR.size() - 1; --i)
                if (derL[i] != 0)
                    return CompareResult::Greater;
        }
		else if constexpr (Left::size() < Right::size()) {
			i = derR.size() - 1;
            for (; i != derL.size() - 1; --i)
                if (derR[i] != 0)
                    return CompareResult::Less;
		}
		else {
			i = derL.size() - 1;
		}
		for (; i != std::numeric_limits<size_t>::max(); --i)
			if (derL[i] > derR[i])
				return CompareResult::Greater;
			else if (derL[i] < derR[i])
				return CompareResult::Less;
		return CompareResult::Equal;
	}

	template<typename Left, typename Right>
	using BiggerNum = std::conditional_t<Left::byteCount() >= Right::byteCount(), Left, Right>;

	template<typename Left, typename Right>
	constexpr std::pair<BiggerNum<Left, Right>, bool> add(const StaticBigNumBase<Left>& l, const StaticBigNumBase<Right>& r, bool carry = 0) {
		BiggerNum<Left, Right> result = {};
		using ValueType = typename BiggerNum<Left, Right>::ValueType;
		auto& derL = l.derived();
		auto& derR = r.derived();

		auto itL = derL.begin();
		auto itR = derR.begin();
		auto itRes = result.begin();

		if constexpr (Left::size() > Right::size()) {
			for (; itR < derR.end(); itL++, itR++, itRes++)
				std::tie(*itRes, carry) = xtd::math::addWithCarry(*itL, *itR, carry);
			for (; itL < derL.end(); itL++, itRes++)
				std::tie(*itRes, carry) = xtd::math::addWithCarry(*itL, ValueType(0), carry);
		}
		else if constexpr (Left::size() < Right::size()) {
			for (; itL < derL.end(); itL++, itR++, itRes++)
				std::tie(*itRes, carry) = xtd::math::addWithCarry(*itL, *itR, carry);
			for (; itR < derR.end(); itR++, itRes++)
				std::tie(*itRes, carry) = xtd::math::addWithCarry(ValueType(0),*itR,  carry);
		}
		else {
			for (; itL < derL.end(); itL++, itR++, itRes++)
				std::tie(*itRes, carry) = xtd::math::addWithCarry(*itL, *itR, carry);
		}
		return std::pair<BiggerNum<Left, Right>, bool>(result, carry);
	}

	template<typename Left, typename Right>
	requires std::same_as<typename Left::ValueType, typename Right::ValueType>
	constexpr std::pair<BiggerNum<Left, Right>, bool> subtract(const StaticBigNumBase<Left>& l, const StaticBigNumBase<Right>& r, bool borrow = 0) {
		BiggerNum<Left, Right> result = {};
		using ValueType = typename Left::ValueType;
		auto& derL = l.derived();
		auto& derR = r.derived();

		auto itL = derL.begin();
		auto itR = derR.begin();
		auto itRes = result.begin();

		if constexpr (Left::size() > Right::size()) {
			for (; itR < derR.end(); itL++, itR++, itRes++)
				std::tie(*itRes, borrow) = xtd::math::subWithBorrow(*itL, *itR, borrow);
			for (; itL < derL.end(); itL++, itRes++)
				std::tie(*itRes, borrow) = xtd::math::subWithBorrow(*itL, ValueType(0), borrow);
		}
		else if constexpr (Left::size() < Right::size()) {
			for (; itL < derL.end(); itL++, itR++, itRes++)
				std::tie(*itRes, borrow) = xtd::math::subWithBorrow(*itL, *itR, borrow);
			for (; itR < derR.end(); itR++, itRes++)
				std::tie(*itRes, borrow) = xtd::math::subWithBorrow(ValueType(0), *itR, borrow);
		}
		else {
			for (; itL < derL.end(); itL++, itR++, itRes++)
				std::tie(*itRes, borrow) = xtd::math::subWithBorrow(*itL, *itR, borrow);
		}
		return std::pair<BiggerNum<Left, Right>, bool>(result, borrow);
	}

	template<typename Left, std::integral Right>
	constexpr std::pair<Left, bool> add(const StaticBigNumBase<Left>& l, Right r, bool carry = 0) {
		Left result;
		using ValueType = typename Left::ValueType;
		auto& derL = l.derived();

		auto itL = derL.begin();
		auto itRes = result.begin();

		std::tie(*itRes, carry) = xtd::math::addWithCarry(*itL, r, carry);
		++itL;
		++itRes;
		for(;carry && itL < derL.end(); ++itL, ++itRes) {
			std::tie(*itRes, carry) = xtd::math::addWithCarry(*itL, ValueType(0), carry);
		}
		for(;itL < derL.end(); ++itL, ++itRes) *itRes = *itL;
		return std::pair<Left, bool>(result, carry);
	}

	template<typename Left, std::integral Right>
	constexpr std::pair<Left, bool> subtract(const StaticBigNumBase<Left>& l, Right r, bool borrow = 0) {
		Left result;
		using ValueType = typename Left::ValueType;
		auto& derL = l.derived();

		auto itL = derL.begin();
		auto itRes = result.begin();

		std::tie(*itRes, borrow) = xtd::math::subWithBorrow(*itL, r, borrow);
		++itL;
		++itRes;
		for(;borrow && itL < derL.end(); ++itL, ++itRes) {
			std::tie(*itRes, borrow) = xtd::math::subWithBorrow(*itL, ValueType(0), borrow);
		}
		for(;itL < derL.end(); ++itL, ++itRes) *itRes = *itL;
		return std::pair<Left, bool>(result, borrow);
	}

	template<typename Left, typename Right>
	requires std::same_as<typename Left::ValueType, typename Right::ValueType>
	constexpr std::pair<BiggerNum<Left, Right>, typename Left::ValueType> multiply(const StaticBigNumBase<Left>& l, const StaticBigNumBase<Right>& r) {
		BiggerNum<Left, Right> result = {};
		auto& derL = l.derived();
		auto& derR = r.derived();

		auto itR = derR.begin();
		auto itL = derL.begin();
		auto itRes = result.begin();
		typename Left::ValueType carry = 0;

		if constexpr (Left::size() > Right::size()) {
			for (; itR < derR.end(); ++itR, ++itRes)
			{
				carry = 0;
				itL = derL.begin();
				for (; itL < derL.end(); ++itL, ++itRes)
				{
					auto [res, carryInternal] = xtd::math::mult(*itL, *itR);
					std::tie(*itRes, carry) = xtd::math::addWithBigCarry(*itRes, res, carry);
					carry += carryInternal;
				}
			}
		}
		else {
			for (; itL < derL.end(); ++itL, ++itRes)
			{
				carry = 0;
				itR = derR.begin();
				for (; itR < derR.end(); ++itR, ++itRes)
				{
					auto [res, carryInternal] = xtd::math::mult(*itL, *itR);
					std::tie(*itRes, carry) = xtd::math::addWithBigCarry(*itRes, res, carry);
					carry += carryInternal;
				}
			}
		}
		return std::pair<BiggerNum<Left, Right>, typename Left::ValueType>(result, carry);
	}

	template<typename Left, std::integral Right>
	constexpr std::pair<Left, typename Left::ValueType> multiply(const StaticBigNumBase<Left>& l, Right r) {
		Left result = {};
		auto& derL = l.derived();

		auto itL = derL.begin();
		auto itRes = result.begin();
		typename Left::ValueType carry = 0;

		for (; itL < derL.end(); ++itL, ++itRes)
		{
			auto [res, carryInternal] = xtd::math::mult(*itL, r);
			std::tie(*itRes, carry) = xtd::math::addWithBigCarry(*itRes, res, carry);
			carry += carryInternal;
		}
		return std::pair<Left, typename Left::ValueType>(result, carry);
	}

	template<typename Left, typename Right>
	requires std::same_as<typename Left::ValueType, typename Right::ValueType>
	struct DivModResult {
		BiggerNum<Left, Right> remainder;
		BiggerNum<Left, Right> quotient;
	};

	template<typename Left, typename Right>
	requires std::same_as<typename Left::ValueType, typename Right::ValueType>
	constexpr DivModResult<Left, Right> divMod(const BigUintBase<Left>& l, const BigUintBase<Right>& r)
	{
		using ValueType = typename Left::ValueType;

		constexpr size_t N = Right::size();

		DivModResult<Left, Right> res = {};

		size_t n = N;

		while (n > 0 && r[n - 1] == 0)
			--n;

		if (n == 0) throw std::runtime_error("division by zero");

		// normalization
		ValueType shift = countLeadingZeroes(r[n - 1]);

		BiggerNum<Left, Right> u = l;
		BiggerNum<Left, Right> v = r;

		shiftLeft(u, shift);
		shiftLeft(v, shift);

		xtd::containers::Array<ValueType, BiggerNum<Left, Right>::size() + 1> un{};

		for (size_t i = 0; i < u.size(); ++i)
			un[i] = u[i];

		un.back() = 0;

		size_t m = u.size() - n;

		for (size_t j = m; j != std::numeric_limits<size_t>::max(); --j)
		{
			//-----------------------------------------
			// D3: estimate quotient
			//-----------------------------------------

			auto [ qhat, rhat ] = xtd::math::divMod(un[j+n], un[j+n-1], v[n-1]);

			if (qhat == std::numeric_limits<uint64_t>::max())
				qhat--;

			if (n > 1)
			{
				while (true)
				{
					auto [plo, phi] = xtd::math::mult(qhat, v[n-2]);					
					if (!xtd::math::compareGreater(phi, plo, rhat, un[j+n-2]))
						break;

					qhat--;

					auto [sum, carry] = xtd::math::addWithCarry(rhat, v[n-1], 0);
					rhat = sum;

					if (carry) break;
				}
			}

			//-----------------------------------------
			// D4: multiply and subtract
			//-----------------------------------------

			ValueType borrow = 0;

			for (size_t i = 0; i < n; ++i)
			{
				auto [plo, phi] = xtd::math::mult(qhat, v[i]);

				auto s1 = xtd::math::subWithBigBorrow(un[i+j], plo, borrow);
				auto s2 = xtd::math::subWithBorrow(s1.diff, phi, 0);

				un[i+j] = s1.first;

				borrow = s1.second + s2.second;
			}

			bool negative;
			std::tie(un[j+n], negative) = xtd::math::subWithBorrow(un[j+n], borrow, 0);

			//-----------------------------------------
			// D5: correction
			//-----------------------------------------

			if (negative)
			{
				qhat--;

				ValueType carry = 0;

				for (size_t i = 0; i < n; ++i) {
					std::tie(un[i+j], carry) = xtd::math::addWithBigCarry(un[i+j], v[i], carry);
				}

				std::tie(un[j+n], std::ignore) = xtd::math::addWithCarry(un[j+n], carry, 0);
			}

			res.quotient[j] = qhat;
		}

		//-----------------------------------------
		// remainder
		//-----------------------------------------

		for (size_t i = 0; i < n; ++i) res.remainder[i] = un[i];

		shiftRight(res.remainder, shift);

		return res;
	}
}