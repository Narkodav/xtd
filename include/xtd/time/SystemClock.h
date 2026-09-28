#pragma once
#include "xtd/time/Duration.h"
#include "xtd/time/TimePoint.h"
#include "xtd/time/Detail.h"

namespace xtd::time {

    // Models POSIX/Unix time: clock_gettime(CLOCK_REALTIME) / equivalent.
    // Always 86400 seconds/day by definition — leap seconds are absorbed by
    // repeating or smearing, never counted. This is NOT a linear count of
    // real elapsed seconds across a leap-second boundary.
    struct SystemClock {
        using ValueType     = int64_t;
        using Period        = Nano;
        using Duration      = Duration<ValueType, Period>;
        using TimePointT    = TimePoint<SystemClock, Duration>;
        static constexpr bool isSteady = false;

        static TimePointT now() noexcept {
            return TimePointT(Duration(detail::osRealtimeNs()));
        }

        
    };
}