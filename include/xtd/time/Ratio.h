#pragma once
#include <cstdint>

namespace xtd::time {

    // Might move this to general algorithms
    static inline constexpr intmax_t gcd(intmax_t A, intmax_t B) {
        if(B == 0) return A < 0 ? -A : A;
        return gcd(B, A % B);
    }

    // Tells how many seconds are in one tick
    template<intmax_t Num, intmax_t Denom = 1>
    struct Ratio {
        static_assert(Denom != 0, "Denominator cannot be zero");
    private:
        // pull sign onto numerator only, then reduce
        static constexpr intmax_t sign = (Num < 0) != (Denom < 0) ? -1 : 1;
        static constexpr intmax_t absNum = Num < 0 ? -Num : Num;
        static constexpr intmax_t absDen = Denom < 0 ? -Denom : Denom;
        static constexpr intmax_t gcdVal = gcd(absNum, absDen);

    public:
        static constexpr intmax_t num = sign * (absNum / (gcdVal == 0 ? 1 : gcdVal));
        static constexpr intmax_t den = absDen / (gcdVal == 0 ? 1 : gcdVal);

        using Self = Ratio<num, den>;

        template<intmax_t otherNum, intmax_t otherDenom>
        constexpr auto operator+(const Ratio<otherNum, otherDenom>&) const {
            using Other = Ratio<otherNum, otherDenom>;
            constexpr intmax_t num = Self::num * Other::den + Other::num * Self::den;
            constexpr intmax_t den = Self::den * Other::den;
            return Ratio<num, den>();
        }

        template<intmax_t otherNum, intmax_t otherDenom>
        constexpr auto operator-(const Ratio<otherNum, otherDenom>&) const {
            using Other = Ratio<otherNum, otherDenom>;
            constexpr intmax_t num = Self::num * Other::den - Other::num * Self::den;
            constexpr intmax_t den = Self::den * Other::den;
            return Ratio<num, den>();
        }

        template<intmax_t otherNum, intmax_t otherDenom>
        constexpr auto operator*(const Ratio<otherNum, otherDenom>&) const {
            using Other = Ratio<otherNum, otherDenom>;
            constexpr intmax_t num = Self::num * Other::num;
            constexpr intmax_t den = Self::den * Other::den;
            return Ratio<num, den>();
        }

        template<intmax_t otherNum, intmax_t otherDenom>
        constexpr auto operator/(const Ratio<otherNum, otherDenom>&) const {
            using Other = Ratio<otherNum, otherDenom>;
            constexpr intmax_t num = Self::num * Other::den;
            constexpr intmax_t den = Self::den * Other::num;
            return Ratio<num, den>();
        }

        template<intmax_t otherNum, intmax_t otherDenom>
        constexpr auto operator==(const Ratio<otherNum, otherDenom>&) const {
            using Other = Ratio<otherNum, otherDenom>;
            constexpr bool result = Other::num == Self::num && Other::den == Self::den;
            return result;
        }

        template<intmax_t otherNum, intmax_t otherDenom>
        constexpr auto operator!=(const Ratio<otherNum, otherDenom>& other) const { return !(*this == other); }

        template<intmax_t otherNum, intmax_t otherDenom>
        constexpr auto operator<(const Ratio<otherNum, otherDenom>&) const {
            using Other = Ratio<otherNum, otherDenom>;
            constexpr bool result = (Self::num * Other::den) < (Other::num * Self::den);
            return result;
        }

        template<intmax_t otherNum, intmax_t otherDenom>
        constexpr auto operator>(const Ratio<otherNum, otherDenom>&) const {
            using Other = Ratio<otherNum, otherDenom>;
            constexpr bool result = (Self::num * Other::den) > (Other::num * Self::den);
            return result;
        }
    };

    using Nano  = Ratio<1, 1'000'000'000>;
    using Micro = Ratio<1, 1'000'000>;
    using Milli = Ratio<1, 1'000>;
    using Centi = Ratio<1, 100>;
    using Deci  = Ratio<1, 10>;
    using Deca  = Ratio<10, 1>;
    using Becto = Ratio<100, 1>;
    using Kilo  = Ratio<1'000, 1>;

}