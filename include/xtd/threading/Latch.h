#pragma once
#include <atomic>

// #if defined(_MSC_VER)
//     #include <intrin.h>
//     #define CPU_PAUSE() _mm_pause()
// #else
//     #include <immintrin.h>
//     #define CPU_PAUSE() __builtin_ia32_pause()
// #endif

namespace xtd::threading {

    // Single use semaphore, no syscalls or pauses, busy waits
    class Latch {
    private:
        std::atomic<bool> _lock = false;

    public:
        Latch() = default;
        ~Latch() = default;

        Latch(const Latch&) = delete;
        Latch& operator=(const Latch&) = delete;
        Latch(Latch&&) = delete;
        Latch& operator=(Latch&&) = delete;

        void acquire() { while (!_lock.load(std::memory_order_acquire)) {} }
        void release() { _lock.store(true, std::memory_order_release); }
        bool done() const { return _lock.load(); }
    };
}