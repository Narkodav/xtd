#pragma once
#include "xtd/time/Ratio.h"
#include <compare>
#include <cstdint>
#include <type_traits>
#include <limits>

namespace xtd::time {
    template<typename Rep, typename Per = Ratio<1>>
    class Duration {
    public:
        using ValueType = Rep;
        using Period = typename Per::Self;

    private:
        ValueType _count;

        template<typename Period2>
        using Factor = decltype(Period() / Period2());

    public:
        constexpr Duration() = default;
        constexpr Duration(const Duration&) = default;

        template<typename Rep2>
        requires (std::is_convertible_v<Rep2, ValueType> && !(std::is_floating_point_v<Rep2> && std::is_integral_v<ValueType>))
        constexpr explicit Duration(const Rep2& count) : _count(static_cast<ValueType>(count)) {}

        template<typename Rep2, typename Period2>
        requires std::is_floating_point_v<ValueType> || (Factor<Period2>::den == 1 && !std::is_floating_point_v<Rep2>)
        constexpr Duration(const Duration<Rep2, Period2>& other)
            : _count(static_cast<ValueType>(other.count() * Factor<Period2>::num / Factor<Period2>::den)) {}

        constexpr explicit operator ValueType() const { return _count; }

        constexpr ValueType count() const { return _count; }

        template<typename ToDuration>
        constexpr ToDuration to() const {
            using Period2 = typename ToDuration::Period;
            using Repetition2 = typename ToDuration::ValueType;
            using Factor = Factor<Period2>;
            using CommonRep = std::common_type_t<Repetition2, ValueType, intmax_t>;
            if constexpr (Factor::num == 1 && Factor::den == 1) {
                return ToDuration(static_cast<Repetition2>(_count));
            } else if constexpr (Factor::den == 1) {
                return ToDuration(static_cast<Repetition2>(
                    static_cast<CommonRep>(_count) * static_cast<CommonRep>(Factor::num)));
            } else if constexpr (Factor::num == 1) {
                return ToDuration(static_cast<Repetition2>(
                    static_cast<CommonRep>(_count) / static_cast<CommonRep>(Factor::den)));
            } else {
                return ToDuration(static_cast<Repetition2>(
                    static_cast<CommonRep>(_count) * static_cast<CommonRep>(Factor::num)
                    / static_cast<CommonRep>(Factor::den)));
            }
        }

        // arithmetic
        constexpr Duration operator+() const { return *this; }
        constexpr Duration operator-() const { return Duration(-_count); }

        constexpr Duration& operator++() { ++_count; return *this; }
        constexpr Duration operator++(int) { return Duration(_count++); }
        constexpr Duration& operator--() { --_count; return *this; }
        constexpr Duration operator--(int) { return Duration(_count--); }

        constexpr Duration& operator+=(const Duration& d) { _count += d.count(); return *this; }
        constexpr Duration& operator-=(const Duration& d) { _count -= d.count(); return *this; }
        constexpr Duration& operator*=(const ValueType& rhs) { _count *= rhs; return *this; }
        constexpr Duration& operator/=(const ValueType& rhs) { _count /= rhs; return *this; }
        constexpr Duration& operator%=(const ValueType& rhs) { _count %= rhs; return *this; }

        static constexpr Duration zero() { return Duration(ValueType(0)); }
        static constexpr Duration min()  { return Duration(std::numeric_limits<ValueType>::lowest()); }
        static constexpr Duration max()  { return Duration(std::numeric_limits<ValueType>::max()); }
    };

    template<typename D1, typename D2>
    struct DurationCommonType;
    
    template<typename Rep1, typename Period1, typename Rep2, typename Period2>
    struct DurationCommonType<Duration<Rep1, Period1>, Duration<Rep2, Period2>> {
        static constexpr intmax_t num = gcd(Period1::num, Period2::num);
        static constexpr intmax_t den = (Period1::den / gcd(Period1::den, Period2::den)) * Period2::den;

        using Type = Duration<std::common_type_t<Rep1, Rep2>, Ratio<num, den>>;
    };

    template<typename D1, typename D2>
    using DurationCommonTypeT = typename DurationCommonType<D1, D2>::Type;

    template<typename Rep1, typename Period1, typename Rep2, typename Period2>
    constexpr auto operator+(const Duration<Rep1, Period1>& lhs, const Duration<Rep2, Period2>& rhs) {
        using CD = DurationCommonTypeT<Duration<Rep1, Period1>, Duration<Rep2, Period2>>;
        return CD(CD(lhs).count() + CD(rhs).count());
    }

    template<typename Rep1, typename Period1, typename Rep2, typename Period2>
    constexpr auto operator-(const Duration<Rep1, Period1>& lhs, const Duration<Rep2, Period2>& rhs) {
        using CD = DurationCommonTypeT<Duration<Rep1, Period1>, Duration<Rep2, Period2>>;
        return CD(CD(lhs).count() - CD(rhs).count());
    }

    template<typename Rep1, typename Period1, typename Rep2, typename Period2>
    constexpr auto operator*(const Duration<Rep1, Period1>& lhs, const Duration<Rep2, Period2>& rhs) {
        using CD = DurationCommonTypeT<Duration<Rep1, Period1>, Duration<Rep2, Period2>>;
        return CD(CD(lhs).count() * CD(rhs).count());
    }

    template<typename Rep1, typename Period1, typename Rep2, typename Period2>
    constexpr auto operator/(const Duration<Rep1, Period1>& lhs, const Duration<Rep2, Period2>& rhs) {
        using CD = DurationCommonTypeT<Duration<Rep1, Period1>, Duration<Rep2, Period2>>;
        return CD(CD(lhs).count() / CD(rhs).count());
    }

    template<typename Rep1, typename Period1, typename Rep2, typename Period2>
    constexpr std::strong_ordering operator<=>(const Duration<Rep1, Period1>& lhs, const Duration<Rep2, Period2>& rhs) {
        using CD = DurationCommonTypeT<Duration<Rep1, Period1>, Duration<Rep2, Period2>>;
        return CD(lhs).count() <=> CD(rhs).count();
    }

    template<typename Rep1, typename Period1, typename Rep2, typename Period2>
    constexpr bool operator==(const Duration<Rep1, Period1>& lhs, const Duration<Rep2, Period2>& rhs) {
        using CD = DurationCommonTypeT<Duration<Rep1, Period1>, Duration<Rep2, Period2>>;
        return CD(lhs).count() == CD(rhs).count();
    }

    template<typename Rep1, typename Period1, typename Rep2, typename Period2>
    constexpr bool operator!=(const Duration<Rep1, Period1>& lhs, const Duration<Rep2, Period2>& rhs) {
        using CD = DurationCommonTypeT<Duration<Rep1, Period1>, Duration<Rep2, Period2>>;
        return CD(lhs).count() != CD(rhs).count();
    }

    using Nano  = Ratio<1, 1'000'000'000>;
    using Micro = Ratio<1, 1'000'000>;
    using Milli = Ratio<1, 1'000>;
    using Centi = Ratio<1, 100>;
    using Deci  = Ratio<1, 10>;
    using Deca  = Ratio<10, 1>;
    using Becto = Ratio<100, 1>;
    using Kilo  = Ratio<1'000, 1>;

    template<typename T = int64_t>
    using NanoSecond    = Duration<T, Nano>;
    template<typename T = int64_t>
    using MicroSecond   = Duration<T, Micro>;
    template<typename T = int64_t>
    using MilliSecond   = Duration<T, Milli>;
    template<typename T = int64_t>
    using CentiSecond   = Duration<T, Centi>;
    template<typename T = int64_t>
    using Second        = Duration<T, Ratio<1>>;
    template<typename T = int64_t>
    using DeciSecond    = Duration<T, Deci>;
    template<typename T = int64_t>
    using DecaSecond    = Duration<T, Deca>;
    template<typename T = int64_t>
    using BectoSecond   = Duration<T, Becto>;
    template<typename T = int64_t>
    using KiloSecond    = Duration<T, Kilo>;
    
}