#pragma once
#include "xtd/time/Detail.h"

#include "xtd/platform/Macros.h"

#if defined(XTD_PLATFORM_WINDOWS)
    #define NOMINMAX
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#elif defined(XTD_PLATFORM_LINUX)
    #include <time.h>
#elif defined(XTD_PLATFORM_MACOS)
    //Support for MACOs may be spotty
    #include <time.h>
    #include <mach/mach_time.h>
#endif

#include "xtd/math/Utilities.h"

namespace xtd::time::detail {
    // Nanoseconds since an unspecified fixed point, monotonic, never adjusted.
    int64_t osMonotonicNs() noexcept {
#if defined(XTD_PLATFORM_WINDOWS)
        static const int64_t freq = [] {
            LARGE_INTEGER f;
            QueryPerformanceFrequency(&f);
            return f.QuadPart;
        }();
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        // scale to ns without overflowing for realistic frequencies (<10GHz)
        auto [lo, hi] = xtd::math::mult(static_cast<uint64_t>(counter.QuadPart), uint64_t(1'000'000'000));
        auto [quotient, remainder] = xtd::math::divMod(hi, lo, static_cast<uint64_t>(freq));
        return quotient;
#elif defined(XTD_PLATFORM_MACOS)
        static const mach_timebase_info_data_t tb = [] {
            mach_timebase_info_data_t info;
            mach_timebase_info(&info);
            return info;
        }();
        uint64_t t = mach_absolute_time();
        auto [lo, hi] = xtd::math::mult(static_cast<uint64_t>(t), static_cast<uint64_t>(tb.numer));
        auto [quotient, remainder] = xtd::math::divMod(hi, lo, static_cast<uint64_t>(tb.denom));
        return quotient;
#elif defined(XTD_PLATFORM_LINUX)
        timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts); // NTP-slewed but never steps backward
        return static_cast<int64_t>(ts.tv_sec) * 1'000'000'000 + ts.tv_nsec;
#endif
    }

    // Nanoseconds since Unix epoch, wall clock, CAN jump (NTP step, user change).
    int64_t osRealtimeMs() noexcept {
#if defined(XTD_PLATFORM_WINDOWS)
        FILETIME ft;
        GetSystemTimePreciseAsFileTime(&ft); // Windows 8+; 100ns ticks since 1601-01-01
        uint64_t ticks = (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
        constexpr uint64_t EPOCH_DIFF_100NS = 116444736000000000ULL; // 1601 -> 1970
        return static_cast<int64_t>((ticks - EPOCH_DIFF_100NS) * 100);
#else
        timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        return static_cast<int64_t>(ts.tv_sec) * 1'000'000'000 + ts.tv_nsec;
#endif
    }

    // Total CPU time consumed by the current process.
    int64_t osProcessCpuNs() noexcept {
#if defined(XTD_PLATFORM_WINDOWS)
        FILETIME creation, exit, kernel, user;
        GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user);
        auto toNs = [](const FILETIME& ft) -> int64_t {
            uint64_t t = (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
            return static_cast<int64_t>(t) * 100;
        };
        return toNs(kernel) + toNs(user);
#else
        timespec ts;
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
        return static_cast<int64_t>(ts.tv_sec) * 1'000'000'000 + ts.tv_nsec;
#endif
    }

    OscillatorInfo osMonotonicOscillator() noexcept {
#if defined(XTD_PLATFORM_WINDOWS)
        // QPC's frequency is queried directly from the OS and is exact —
        // Windows guarantees a fixed frequency for the life of the process.
        static const int64_t freq = [] {
            LARGE_INTEGER f;
            QueryPerformanceFrequency(&f);
            return f.QuadPart;
        }();
        return OscillatorInfo{ freq, 1'000'000'000, freq, true };

#elif defined(XTD_PLATFORM_MACOS)
        // mach_timebase gives an exact rational: 1 mach tick = numer/denom ns.
        // Frequency = 1e9 * denom/numer, which may not be an integer, so we
        // report the period as the exact rational and derive frequency for
        // display purposes only (rounded).
        static const mach_timebase_info_data_t tb = [] {
            mach_timebase_info_data_t info;
            mach_timebase_info(&info);
            return info;
        }();
        auto [lo, hi] = xtd::math::mult(1'000'000'000, static_cast<uint64_t>(tb.denom));
        auto [quotient, remainder] = xtd::math::divMod(hi, lo, static_cast<uint64_t>(tb.numer));
        int64_t freq = quotient;
        return OscillatorInfo{ freq, tb.numer, tb.denom, true };

#elif defined(XTD_PLATFORM_LINUX)
        // clock_getres reports the *advertised* timer granularity, which on
        // most modern kernels is normalized to 1ns regardless of the true
        // underlying TSC frequency — Linux doesn't cleanly expose the real
        // oscillator rate through this API. We report what's available and
        // mark it non-exact rather than pretend it's a hardware fact.
        timespec res;
        clock_getres(CLOCK_MONOTONIC, &res);
        int64_t period_ns = static_cast<int64_t>(res.tv_sec) * 1'000'000'000 + res.tv_nsec;
        int64_t freq = period_ns > 0 ? (1'000'000'000 / period_ns) : 0;
        return OscillatorInfo{ freq, period_ns, 1, false };
#endif
    }
}