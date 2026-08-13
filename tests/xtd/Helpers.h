#pragma once
#include <compare>

// ---------------------------------------------------------------------
// Helper type: counts constructions/destructions so tests can verify
// that UniquePtr actually manages object lifetime correctly, not just
// memory.
// ---------------------------------------------------------------------
struct LifetimeTracker {
    inline static int constructedCount = 0;
    inline static int destroyedCount = 0;

    int value;
    const bool shouldTrack;

    explicit LifetimeTracker(int v = 0, const bool shouldTrack = true) : value(v), shouldTrack(shouldTrack) { 
        if(shouldTrack) ++constructedCount;
    }
    LifetimeTracker(const LifetimeTracker& other) : value(other.value), shouldTrack(true) { 
        if(shouldTrack) ++constructedCount;
    }
    LifetimeTracker(LifetimeTracker&& other) noexcept : value(other.value), shouldTrack(true) { 
        if(shouldTrack) ++constructedCount;
    }
    ~LifetimeTracker() {
        if(shouldTrack) ++destroyedCount;
    }

    static void resetCounts() {
        constructedCount = 0;
        destroyedCount = 0;
    }

    std::strong_ordering operator<=>(const LifetimeTracker& other) const { return value <=> other.value; }
    bool operator==(const LifetimeTracker& other) const { return value == other.value; }
};