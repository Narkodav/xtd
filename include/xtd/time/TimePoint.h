#pragma once
#include "xtd/time/Duration.h"

namespace xtd::time {

    template<typename C, typename Dur = typename C::Duration>
    class TimePoint {
    public:
        using Clock     = C;
        using Duration  = Dur;
        using ValueType = typename Duration::ValueType;
        using Period    = typename Duration::Period;

    private:
        Duration _sinceEpoch;

    public:
        constexpr TimePoint() = default;
        constexpr explicit TimePoint(const Dur& d) : _sinceEpoch(d) {}

        // widening conversion from another TimePoint of the SAME clock,
        // gated the same way Duration's converting ctor is
        template<typename Dur2>
        requires(std::is_convertible_v<Dur2, Duration>)
        constexpr TimePoint(const TimePoint<Clock, Dur2>& other) : _sinceEpoch(other.timeSinceEpoch()) {}

        constexpr Duration timeSinceEpoch() const { return _sinceEpoch; }

        // cast-like unary transform -> member
        template<typename ToTimePoint>
        constexpr ToTimePoint to() const {
            static_assert(std::is_same_v<typename ToTimePoint::Clock, Clock>,
                "to<>() only converts durations within the same clock; use clock_cast for cross-clock conversion");
            return ToTimePoint(_sinceEpoch.template to<typename ToTimePoint::duration>());
        }

        constexpr TimePoint& operator+=(const Dur& d) { _sinceEpoch += d; return *this; }
        constexpr TimePoint& operator-=(const Dur& d) { _sinceEpoch -= d; return *this; }

        static constexpr TimePoint min() { return TimePoint(Dur::min()); }
        static constexpr TimePoint max() { return TimePoint(Dur::max()); }
    };

    template<typename Clock, typename Dur1, typename Dur2>
    constexpr auto operator-(const TimePoint<Clock, Dur1>& lhs, const TimePoint<Clock, Dur2>& rhs) {
        return lhs.timeSinceEpoch() - rhs.timeSinceEpoch();
    }

    template<typename Clock, typename Dur, typename Rep2, typename Period2>
    constexpr auto operator+(const TimePoint<Clock, Dur>& tp, const Duration<Rep2, Period2>& d) {
        return TimePoint<Clock, Dur>(tp.timeSinceEpoch() + d);
    }

    template<typename Clock, typename Dur, typename Rep2, typename Period2>
    constexpr auto operator+(const Duration<Rep2, Period2>& d, const TimePoint<Clock, Dur>& tp) {
        return TimePoint<Clock, Dur>(tp + d);
    }

    template<typename Clock, typename Dur, typename Rep2, typename Period2>
    constexpr auto operator-(const TimePoint<Clock, Dur>& tp, const Duration<Rep2, Period2>& d) {
        return TimePoint<Clock, Dur>(tp.timeSinceEpoch() - d);
    }
}