#include "xtd/platform/Utilities.h"
#include "xtd/time/Duration.h"

#include <climits>
#include <cstdint>
#include <ctime>

static_assert(sizeof(std::atomic<uint32_t>) == sizeof(uint32_t));
static_assert(std::atomic<uint32_t>::is_always_lock_free);

#if defined(XTD_PLATFORM_WINDOWS)
    #include <windows.h>
    #include <synchapi.h>
#elif defined(XTD_PLATFORM_LINUX)
    #include <linux/futex.h>
    #include <sys/syscall.h>
    #include <unistd.h>
    #include <cerrno>
#elif defined(XTD_PLATFORM_MACOS)
    #include <cerrno>
    // Private, undocumented — no public header declares these pre-14.4.
    // This is the actual mechanism libc++'s std::atomic::wait uses on macOS
    // (see llvm-project libcxx/src/atomic.cpp), and what Rust's std used
    // before switching to the public os_sync_wait_on_address (macOS 14.4+).
    extern "C" {
        int __ulock_wait(uint32_t operation, void* addr, uint64_t value,
                          uint32_t timeout_us); // 0 == infinite
        int __ulock_wake(uint32_t operation, void* addr, uint64_t wake_value);
    }
    #define XTD_UL_COMPARE_AND_WAIT 1
    #define XTD_ULF_WAKE_ALL        0x00000100
    #define XTD_ULF_NO_ERRNO        0x01000000
#endif

namespace xtd::platform {
    // Sleeps while *addr == expected, exactly like WaitOnAddress's contract.
    // Only 32-bit words supported here — same restriction the futex syscall
    // itself imposes (FUTEX_WAIT is a 32-bit-value op); WaitOnAddress and
    // os_sync_wait_on_address support other widths, but 32-bit covers every
    // mutex/lock-word use case and keeps the three backends symmetric.
    // expected is a reference to match WaitOnAddress
    void waitOnAddress(std::atomic<uint32_t>& addr, uint32_t& expected) noexcept {
    #if defined(XTD_PLATFORM_WINDOWS)
        // WaitOnAddress takes CompareAddress by pointer, not by value — it does
        // a byte-wise compare of *Address vs *CompareAddress under an internal
        // lock, atomically w.r.t. concurrent WakeByAddress* calls.
        ::WaitOnAddress(&addr, &expected, sizeof(expected), INFINITE);

    #elif defined(XTD_PLATFORM_LINUX)
        // _PRIVATE variants skip the kernel's cross-process futex bookkeeping
        // (no hashing into the global futex table by vma/inode) since this
        // address is only ever waited on from within this process.
        for (;;) {
            long rc = ::syscall(SYS_futex, &addr, FUTEX_WAIT_PRIVATE, expected, nullptr, nullptr, 0);
            if (rc == 0) return;                 // woken
            if (errno == EAGAIN) return;         // value already changed
            if (errno == EINTR) continue;        // spurious signal, retry
            return; // any other error: treat as spurious wake, caller re-checks
        }

    #elif defined(XTD_PLATFORM_MACOS)
        for (;;) {
            int rc = __ulock_wait(XTD_UL_COMPARE_AND_WAIT, &addr,
                                static_cast<uint64_t>(expected), 0 /* infinite */);
            if (rc >= 0) return;
            if (errno == EINTR) continue;
            return;
        }
    #endif
    }

#if defined(XTD_PLATFORM_WINDOWS)
    WaitResult waitOnAddress(std::atomic<uint32_t>& addr, uint32_t& expected, xtd::time::MilliSecond<uint32_t> timeout) noexcept {
        DWORD ms = timeout.count();
        if(ms > DWORD(MAXDWORD - 1)) ms = DWORD(MAXDWORD - 1);

        if (::WaitOnAddress(&addr, &expected, sizeof(expected), ms))
            return WaitResult::Woken; // note: also returns TRUE on spurious wake — same as futex
        // GetLastError() == ERROR_TIMEOUT distinguishes real timeout from other failures;
        // we fold "other failure" into TimedOut since caller should just re-check + retry logic anyway.
        return WaitResult::TimedOut;
    }
#elif defined(XTD_PLATFORM_LINUX)
    WaitResult waitOnAddress(std::atomic<uint32_t>& addr, uint32_t& expected, xtd::time::NanoSecond<uint32_t> timeoutNS, xtd::time::Second<uint32_t> timeoutS) noexcept {
        struct timespec ts;
        ts.tv_sec  = static_cast<time_t>(timeoutS.count());
        ts.tv_nsec = static_cast<long>(timeoutNS.count());

        // FUTEX_WAIT_PRIVATE's timeout is RELATIVE (unlike FUTEX_WAIT_BITSET,
        // which takes an absolute CLOCK_MONOTONIC deadline) — matches
        // WaitOnAddress/ulock's relative-duration contract, so no conversion
        // headache between the three backends.
        long rc = ::syscall(SYS_futex, &addr, FUTEX_WAIT_PRIVATE, expected, &ts, nullptr, 0);
        if (rc == 0) return WaitResult::Woken;
        if (errno == EAGAIN)  return WaitResult::NotEqual;
        if (errno == ETIMEDOUT) return WaitResult::TimedOut;
        // EINTR or anything else: treat as a spurious wake, caller re-checks.
        return WaitResult::Woken;
    }
#elif defined(XTD_PLATFORM_MACOS)
    WaitResult waitOnAddress(std::atomic<uint32_t>& addr, uint32_t& expected, xtd::time::MicroSecond<uint32_t> timeout) noexcept {
        const auto& us = timeout.count();
        // timeout_us is a 32-bit field — clamp rather than silently overflow/wrap
        // for very long durations (~71 minutes max in one call).
        uint32_t ulockUs = us == 0 ? 1 // 0 means "infinite" to ulock, so floor at 1us
                        : (us > 0xFFFFFFFFu) ? 0xFFFFFFFFu
                        : static_cast<uint32_t>(us);

        int rc = __ulock_wait(XTD_UL_COMPARE_AND_WAIT, &addr,
                            static_cast<uint64_t>(expected), ulockUs);
        if (rc >= 0) return WaitResult::Woken;
        if (errno == ETIMEDOUT) return WaitResult::TimedOut;
        if (errno == EINTR)     return WaitResult::Woken; // spurious, caller re-checks
        return WaitResult::NotEqual; // treat other errors (e.g. value mismatch paths) this way
    }
#endif

    void wakeByAddressSingle(std::atomic<uint32_t>& addr) noexcept {
    #if defined(XTD_PLATFORM_WINDOWS)
        ::WakeByAddressSingle(&addr);

    #elif defined(XTD_PLATFORM_LINUX)
        ::syscall(SYS_futex, &addr, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);

    #elif defined(XTD_PLATFORM_MACOS)
        int rc = __ulock_wake(XTD_UL_COMPARE_AND_WAIT | XTD_ULF_NO_ERRNO, &addr, 0);
        (void)rc; // ENOENT-equivalent (no waiters) is expected/benign, same as
                // futex waking 0 threads on an uncontended unlock
    #endif
    }

    void wakeByAddressAll(std::atomic<uint32_t>& addr) noexcept {
    #if defined(XTD_PLATFORM_WINDOWS)
        ::WakeByAddressAll(&addr);

    #elif defined(XTD_PLATFORM_LINUX)
        ::syscall(SYS_futex, &addr, FUTEX_WAKE_PRIVATE, INT_MAX, nullptr, nullptr, 0);

    #elif defined(XTD_PLATFORM_MACOS)
        int rc = __ulock_wake(XTD_UL_COMPARE_AND_WAIT | XTD_ULF_WAKE_ALL | XTD_ULF_NO_ERRNO, &addr, 0);
        (void)rc;
    #endif
    }

} // namespace xtd