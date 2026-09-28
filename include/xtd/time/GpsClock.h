#pragma once
#include "xtd/time/Duration.h"
#include "xtd/time/TimePoint.h"
#include "xtd/time/UtcClock.h"

#include <cstdint>
#include <limits>

namespace xtd::time {
    struct GpsClock {
        using ValueType     = int64_t;
        using Period        = Nano;
        using Duration      = Duration<ValueType, Period>;
        using TimePointT    = TimePoint<GpsClock, Duration>;
        static constexpr bool isSteady = false;
        static constexpr int64_t gpsEpochAdjustS = -315'964'809;

        static TimePointT now() noexcept { return fromUtc(UtcClock::now()); }

        static UtcClock::TimePointT toUtc(const TimePointT& tp) noexcept {
            return UtcClock::TimePointT(UtcClock::Duration(
                tp.timeSinceEpoch().count() - gpsEpochAdjustS * 1'000'000'000));
        }
        static TimePointT fromUtc(const UtcClock::TimePointT& tp) noexcept {
            return TimePointT(Duration(
                tp.timeSinceEpoch().count() + gpsEpochAdjustS * 1'000'000'000));
        }
    };
}