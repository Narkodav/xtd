#pragma once
#include "xtd/time/Duration.h"
#include "xtd/time/TimePoint.h"
#include "xtd/time/Detail.h"

namespace xtd::time {

    struct ProcessCpuClock {
        using ValueType     = int64_t;
        using Period        = Nano;
        using Duration      = Duration<ValueType, Period>;
        using TimePointT    = TimePoint<ProcessCpuClock, Duration>;
        static constexpr bool isSteady = true;

        static TimePointT now() noexcept {
            return TimePointT(Duration(detail::osProcessCpuNs()));
        }
    };
}