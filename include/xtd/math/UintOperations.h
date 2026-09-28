#pragma once
#include <cstdlib>
#include <cstdint>
#include <type_traits>
#include <limits>
#include <bit>
#include <utility>
#include <iostream>

#include "xtd/containers/Array.h"
#include "xtd/math/Utilities.h"

// Intrinsic like helpers
namespace xtd::math {
    template<typename Derived>
	class UintBase {
    public:
		Derived& derived() { return *static_cast<Derived*>(this); }
		const Derived& derived() const { return *static_cast<const Derived*>(this); }
	};

	

	// Shifting by a number greater than the bit count is UB, mirroring the c++ convention
	template<typename Num>
	void shiftLeft(UintBase<Num>& val, uint64_t shift)
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
            for(; it >= derVal.begin() + limbShift + 1; --it) {
                auto correspond = it - limbShift;
                *it = *correspond << bitShift;
                *it |= *(correspond - 1) >> (Num::limbBitSize() - bitShift);
            }
            auto correspond = it - limbShift;
            *it = *correspond << bitShift;
            --it;
            for(; it >= derVal.begin(); --it)*it = 0;
        }
	}

	// Shifting by a number greater than the bit count is UB, mirroring the c++ convention
	template<typename Num>
	void shiftRight(UintBase<Num>& val, uint64_t shift)
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
            for(; it < derVal.end() - limbShift - 1; ++it) {
                auto correspond = it + limbShift;
                *it = *correspond >> bitShift;
                *it |= *(correspond + 1) << (Num::limbBitSize() - bitShift);
            }
            auto correspond = it + limbShift;
            *it = *correspond >> bitShift;
            ++it;
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
	CompareResult compare(const UintBase<Left>& l, const UintBase<Right>& r) {
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
	requires (sizeof(typename Left::ValueType) == sizeof(typename Right::ValueType))
	using BiggerNum = std::conditional_t<Left::size() >= Right::size(), Left, Right>;

	template<typename Left, typename Right>
	requires std::same_as<typename Left::ValueType, typename Right::ValueType>
	std::pair<BiggerNum<Left, Right>, bool> add(const UintBase<Left>& l, const UintBase<Right>& r, bool carry = 0) {
		BiggerNum<Left, Right> result;
		using ValueType = typename Left::ValueType;
		auto& derL = l.derived();
		auto& derR = r.derived();

		auto itL = derL.begin();
		auto itR = derR.begin();
		auto itRes = result.begin();

		if constexpr (Left::size() > Right::size()) {
			for (; itR < derR.end(); itL++, itR++, itRes++)
				std::tie(*itRes, carry) = xtd::math::addWithCarry(*itL, *itR, carry);
			for (; itL < derL.end(); itL++)
				std::tie(*itRes, carry) = xtd::math::addWithCarry(*itL, ValueType(0), carry);
		}
		else if constexpr (Left::size() < Right::size()) {
			for (; itL < derL.end(); itL++, itR++, itRes++)
				std::tie(*itRes, carry) = xtd::math::addWithCarry(*itL, *itR, carry);
			for (; itR < derR.end(); itR++)
				std::tie(*itRes, carry) = xtd::math::addWithCarry(ValueType(0),*itR,  carry);
		}
		else {
			for (; itL < derL.end(); itL++, itR++, itRes++)
				std::tie(*itRes, carry) = xtd::math::addWithCarry(*itL, *itR, carry);
		}
		return std::make_pair<BiggerNum<Left, Right>, bool>(result, carry);
	}

	template<typename Left, typename Right>
	requires std::same_as<typename Left::ValueType, typename Right::ValueType>
	std::pair<BiggerNum<Left, Right>, bool> subtract(const UintBase<Left>& l, const UintBase<Right>& r, bool borrow = 0) {
		BiggerNum<Left, Right> result;
		using ValueType = typename Left::ValueType;
		auto& derL = l.derived();
		auto& derR = r.derived();

		auto itL = derL.begin();
		auto itR = derR.begin();
		auto itRes = result.begin();

		if constexpr (Left::size() > Right::size()) {
			for (; itR < derR.end(); itL++, itR++, itRes++)
				std::tie(*itRes, borrow) = xtd::math::subWithBorrow(*itL, *itR, borrow);
			for (; itL < derL.end(); itL++)
				std::tie(*itRes, borrow) = xtd::math::subWithBorrow(*itL, ValueType(0), borrow);
		}
		else if constexpr (Left::size() < Right::size()) {
			for (; itL < derL.end(); itL++, itR++, itRes++)
				std::tie(*itRes, borrow) = xtd::math::subWithBorrow(*itL, *itR, borrow);
			for (; itR < derR.end(); itR++)
				std::tie(*itRes, borrow) = xtd::math::subWithBorrow(ValueType(0), *itR, borrow);
		}
		else {
			for (; itL < derL.end(); itL++, itR++, itRes++)
				std::tie(*itRes, borrow) = xtd::math::subWithBorrow(*itL, *itR, borrow);
		}
		return std::make_pair<BiggerNum<Left, Right>, bool>(result, borrow);
	}

	// template<typename ContainerLeft, typename ContainerRight>
	// requires (std::is_unsigned_v<typename ContainerLeft::ValueType> 
	// 	&& std::is_unsigned_v<typename ContainerRight::ValueType> &&
	// 	sizeof(typename ContainerLeft::ValueType) == sizeof(typename ContainerRight::ValueType))
	// BiggerArray<ContainerLeft, ContainerRight> multiply(const ContainerLeft& x, const ContainerRight& y)
	// {
	// 	BiggerArray<ContainerLeft, ContainerRight> result;
	// 	using ValueType = typename ContainerLeft::ValueType;
	// 	if constexpr (ContainerLeft::size() > ContainerRight::size()) {
	// 		for (size_t i = 0; i < ContainerRight::size(); ++i)
	// 		{
	// 			ValueType carry = 0;
	// 			for (size_t j = 0; j < ContainerLeft::size() - i; ++j)
	// 			{
	// 				auto [res, carryInternal] = xtd::math::mult(y[i], x[j]);
	// 				std::tie(result[i + j], carry) = xtd::math::addWithBigCarry(result[i + j], res, carry);
	// 				carry += carryInternal;
	// 			}
	// 		}
	// 	}
	// 	else {
	// 		for (size_t i = 0; i < ContainerLeft::size(); ++i)
	// 		{
	// 			ValueType carry = 0;
	// 			for (size_t j = 0; j < ContainerRight::size() - i; ++j)
	// 			{
	// 				auto [res, carryInternal] = xtd::math::mult(x[i], y[j]);
	// 				std::tie(result[i + j], carry) = xtd::math::addWithBigCarry(result[i + j], res, carry);
	// 				carry += carryInternal;
	// 			}
	// 		}
	// 	}
	// 	return result;
	// }

	// template<typename ContainerLeft, typename ContainerRight>
	// requires (std::is_unsigned_v<typename ContainerLeft::ValueType> 
	// 	&& std::is_unsigned_v<typename ContainerRight::ValueType> &&
	// 	sizeof(typename ContainerLeft::ValueType) == sizeof(typename ContainerRight::ValueType))
	// struct DivModResult {
	// 	BiggerArray<ContainerLeft, ContainerRight> remainder;
	// 	BiggerArray<ContainerLeft, ContainerRight> quotient;
	// };

	// template<typename ContainerLeft, typename ContainerRight>
	// requires (std::is_unsigned_v<typename ContainerLeft::ValueType> &&
	// 	std::is_unsigned_v<typename ContainerRight::ValueType> &&
	// 	sizeof(typename ContainerLeft::ValueType) ==
	// 	sizeof(typename ContainerRight::ValueType))
	// DivModResult<ContainerLeft, ContainerRight> divMod(const ContainerLeft& x, const ContainerRight& y)
	// {
	// 	using ValueType = typename ContainerLeft::ValueType;

	// 	constexpr size_t N = ContainerRight::size();

	// 	DivModResult<ContainerLeft, ContainerRight> res{};

	// 	size_t n = N;

	// 	while (n > 0 && y[n - 1] == 0)
	// 		--n;

	// 	if (n == 0)
	// 		throw std::runtime_error("division by zero");

	// 	// normalization
	// 	ValueType shift = countLeadingZeroes(y[n - 1]);

	// 	BiggerArray<ContainerLeft, ContainerRight> u = x;
	// 	BiggerArray<ContainerLeft, ContainerRight> v = y;

	// 	shiftThisLeftOneLimb(u, shift);
	// 	shiftThisLeftOneLimb(v, shift);

	// 	xtd::containers::Array<ValueType,
	// 		BiggerArray<ContainerLeft, ContainerRight>::size() + 1> un{};

	// 	for (size_t i = 0; i < u.size(); ++i)
	// 		un[i] = u[i];

	// 	un.back() = 0;

	// 	size_t m = u.size() - n;

	// 	for (size_t j = m; j != std::numeric_limits<size_t>::max(); --j)
	// 	{
	// 		//-----------------------------------------
	// 		// D3: estimate quotient
	// 		//-----------------------------------------

	// 		auto [ qhat, rhat ] = xtd::math::divMod(un[j+n], un[j+n-1], v[n-1]);

	// 		if (qhat == std::numeric_limits<uint64_t>::max())
	// 			qhat--;

	// 		if (n > 1)
	// 		{
	// 			while (true)
	// 			{
	// 				auto [plo, phi] = xtd::math::mult(qhat, v[n-2]);					
	// 				if (!xtd::math::compareGreater(phi, plo, rhat, un[j+n-2]))
	// 					break;

	// 				qhat--;

	// 				auto [sum, carry] = xtd::math::addWithCarry(rhat, v[n-1], 0);
	// 				rhat = sum;

	// 				if (carry)
	// 					break;
	// 			}
	// 		}

	// 		//-----------------------------------------
	// 		// D4: multiply and subtract
	// 		//-----------------------------------------

	// 		ValueType borrow = 0;

	// 		for (size_t i = 0; i < n; ++i)
	// 		{
	// 			auto [plo, phi] = xtd::math::mult(qhat, v[i]);

	// 			auto s1 = xtd::math::subWithBigBorrow(un[i+j], plo, borrow);
	// 			auto s2 = xtd::math::subWithBorrow(s1.diff, phi, 0);

	// 			un[i+j] = s1.first;

	// 			borrow = s1.second + s2.second;
	// 		}

	// 		bool negative;
	// 		std::tie(un[j+n], negative) = xtd::math::subWithBorrow(un[j+n], borrow, 0);

	// 		//-----------------------------------------
	// 		// D5: correction
	// 		//-----------------------------------------

	// 		if (negative)
	// 		{
	// 			qhat--;

	// 			ValueType carry = 0;

	// 			for (size_t i = 0; i < n; ++i) {
	// 				std::tie(un[i+j], carry) = xtd::math::addWithBigCarry(un[i+j], v[i], carry);
	// 			}

	// 			std::tie(un[j+n], std::ignore) = xtd::math::addWithCarry(un[j+n], carry, 0);
	// 		}

	// 		res.quotient[j] = qhat;
	// 	}

	// 	//-----------------------------------------
	// 	// remainder
	// 	//-----------------------------------------

	// 	for (size_t i = 0; i < n; ++i)
	// 		res.remainder[i] = un[i];

	// 	shiftThisRightOneLimb(res.remainder, shift);

	// 	return res;
	// }


	// template<typename LeftNum, typename RightNum>
	// inline constexpr auto operator+(const UintBase<LeftNum>& left, const UintBase<RightNum>& right) noexcept {
    //     auto& leftDer = left.derived();
    //     auto& rightDer = right.derived();

	// 	return 1;
	// }
}