#pragma once
#include "xtd/time/Duration.h"
#include "xtd/time/TimePoint.h"
#include "xtd/time/UtcClock.h"

#include <cstdint>
#include <limits>

namespace xtd::time {
    
    struct TaiClock {
        using ValueType     = int64_t;
        using Period        = Nano;
        using Duration      = Duration<ValueType, Period>;
        using TimePointT    = TimePoint<TaiClock, Duration>;
        static constexpr bool isSteady = false;
        static constexpr int64_t taiEpochAdjustS = 378'691'210;

        static TimePointT now() noexcept { return fromUtc(UtcClock::now()); }

        static UtcClock::TimePointT toUtc(const TimePointT& tp) noexcept {
            return UtcClock::TimePointT(UtcClock::Duration(
                tp.timeSinceEpoch().count() - taiEpochAdjustS * 1'000'000'000));
        }
        static TimePointT fromUtc(const UtcClock::TimePointT& tp) noexcept {
            return TimePointT(Duration(
                tp.timeSinceEpoch().count() + taiEpochAdjustS * 1'000'000'000));
        }
    };
}