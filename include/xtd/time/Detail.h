#pragma once
#include <cstdint>

#include "xtd/Macros.h"

namespace xtd::time::detail {
    // Nanoseconds since an unspecified fixed point, monotonic, never adjusted.
    XTD_API int64_t osMonotonicNs() noexcept;

    // Nanoseconds since Unix epoch, wall clock, CAN jump (NTP step, user change).
    XTD_API int64_t osRealtimeNs() noexcept;

    // Total CPU time consumed by the current process.
    XTD_API int64_t osProcessCpuNs() noexcept;

    struct OscillatorInfo {
        int64_t frequencyHz;  // ticks per second — the defined physical quantity
        int64_t periodNumNs;  // period = period_num_ns / period_den_ns, in nanoseconds
        int64_t periodDenNs;  // kept as an exact rational; avoids lossy float rounding
        bool exact;           // false if the platform can't report a true physical value
    };

    XTD_API OscillatorInfo osMonotonicOscillator() noexcept;
}