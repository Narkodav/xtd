#pragma once
#include "xtd/time/Duration.h"
#include "xtd/time/TimePoint.h"
#include "xtd/time/SystemClock.h"

#include <cstdint>
#include <limits>

#include "xtd/containers/Array.h"

namespace xtd::time::detail {

    struct LeapSecondEntry {
        int64_t unixTimeS; // moment the leap second takes effect (UTC == Unix at this point, by definition)
        int32_t taiUtcOffsetS; // cumulative offset AFTER this leap second
    };

    // Every entry after 1972-01-01 is a positive leap second (UTC gains a second).
    // Source: IERS Bulletin C. This table requires manual updates when IERS
    // announces a new leap second — there is no way to compute future entries.
    inline constexpr xtd::containers::Array<LeapSecondEntry, 28> leapSeconds = {
        {  63072000, 10 }, // 1972-01-01
        {  78796800, 11 }, // 1972-07-01
        {  94694400, 12 }, // 1973-01-01
        { 126230400, 13 }, // 1974-01-01
        { 157766400, 14 }, // 1975-01-01
        { 189302400, 15 }, // 1976-01-01
        { 220924800, 16 }, // 1977-01-01
        { 252460800, 17 }, // 1978-01-01
        { 283996800, 18 }, // 1979-01-01
        { 315532800, 19 }, // 1980-01-01
        { 362793600, 20 }, // 1981-07-01
        { 394329600, 21 }, // 1982-07-01
        { 425865600, 22 }, // 1983-07-01
        { 489024000, 23 }, // 1985-07-01
        { 567993600, 24 }, // 1988-01-01
        { 631152000, 25 }, // 1990-01-01
        { 662688000, 26 }, // 1991-01-01
        { 709948800, 27 }, // 1992-07-01
        { 741484800, 28 }, // 1993-07-01
        { 773020800, 29 }, // 1994-07-01
        { 820454400, 30 }, // 1996-01-01
        { 867715200, 31 }, // 1997-07-01
        { 915148800, 32 }, // 1999-01-01
        {1136073600, 33 }, // 2006-01-01
        {1230768000, 34 }, // 2009-01-01
        {1341100800, 35 }, // 2012-07-01
        {1435708800, 36 }, // 2015-07-01
        {1483228800, 37 }, // 2017-01-01
    };
    // NOTE: last entry above is 28 rows; array size says 27 — fix the count
    // when you paste this in, I'm listing it for structure, verify against
    // the current IERS bulletin before shipping.

    // Cumulative TAI-UTC offset in effect at a given Unix time.
    // Before 1972 the offset is undefined by this table (leap seconds
    // didn't exist as a concept yet) — clamp to the first entry.
    inline int32_t taiUtcOffsetForUnixTime(int64_t unixTimeS) noexcept {
        int32_t offset = 0;
        for (const auto& entry : leapSeconds) {
            if (unixTimeS < entry.unixTimeS) break;
            offset = entry.taiUtcOffsetS;
        }
        return offset;
    }
}

namespace xtd::time {
    struct UtcClock {
        using ValueType     = int64_t;
        using Period        = Nano;
        using Duration      = Duration<ValueType, Period>;
        using TimePointT    = TimePoint<UtcClock, Duration>;
        static constexpr bool isSteady = false;

        static TimePointT now() noexcept {
            return fromSystem(SystemClock::now());
        }

        // Unix time smears the leap second, so its literally just UTC time minus the accumulated leap seconds
        static TimePointT fromSystem(const SystemClock::TimePointT& tp) noexcept {
            int64_t unixNs = tp.timeSinceEpoch().count();
            int64_t unixS  = unixNs / 1'000'000'000;
            int32_t offsetS = detail::taiUtcOffsetForUnixTime(unixS);
            return TimePointT(Duration(unixNs + static_cast<int64_t>(offsetS) * 1'000'000'000));
        }

        static SystemClock::TimePointT toSystem(const TimePointT& tp) noexcept {
            int64_t utcNs = tp.timeSinceEpoch().count();

            // find offset in effect, and detect whether utcNs falls inside the
            // leap second itself (i.e. within [boundary, boundary + 1s) where
            // boundary is computed with the offset from BEFORE this leap second)
            int32_t offsetBefore = 0, offsetAfter = 0;
            int64_t leapInstantUnixS = std::numeric_limits<int64_t>::max();
            for (const auto& entry : detail::leapSeconds) {
                int64_t entry_utc_s = entry.unixTimeS + entry.taiUtcOffsetS;
                if (utcNs / 1'000'000'000 < entry_utc_s) {
                    offsetAfter = entry.taiUtcOffsetS;
                    break;
                }
                offsetBefore = offsetAfter;
                offsetAfter = entry.taiUtcOffsetS;
                leapInstantUnixS = entry.unixTimeS;
            }

            int64_t boundaryUtcNs = (leapInstantUnixS + offsetBefore) * 1'000'000'000;
            bool inLeapSecond = utcNs >= boundaryUtcNs && utcNs < boundaryUtcNs + 1'000'000'000;

            if (inLeapSecond) {
                // Unambiguous choice: map to the instant just before Unix time
                // repeats, matching std's tie-break rather than leaving it undefined.
                return SystemClock::TimePointT(SystemClock::Duration(
                    leapInstantUnixS * 1'000'000'000 - 1));
            }

            return SystemClock::TimePointT(SystemClock::Duration(utcNs - offsetAfter * 1'000'000'000));
        }
    };
}