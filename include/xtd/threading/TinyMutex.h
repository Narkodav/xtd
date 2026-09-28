#pragma once
#include <stdint.h>
#include <atomic>

#include "xtd/debug/Checks.h"
#include "xtd/platform/Utilities.h"
#include "xtd/time/Duration.h"
#include "xtd/time/SteadyClock.h"
#include "xtd/threading/ScopedLock.h"
#include "xtd/threading/Thread.h"

namespace xtd::threading {


    // This mutex doesn't use a counter to check if a wakeByAddressSingle is necessary, only a state flag, 
    // meaning a syscall with no threads actually waiting is possible if a thread that was sleep waiting acquires the lock and then unlocks it with no waiting threads
    // The upside is the mutex is 32 bits and the logic is significantly simpler, but unnecessary wakeByAddressSingle create syscalls that waste CPU time even with no threads waiting
    // Additionally cannot provide waiter count metadata
    // Whether Mutex or TinyMutex is better is generally platform and usage dependent, benchmark for your specific use case
    class TinyMutex {
    public:
        enum class State : uint32_t {
            Free = 0,
            Locked = 1,
            LockedWithWaiters = 2
        };

    private:
        static constexpr uint32_t _busyWaitTime = 200; // In nanoseconds

        std::atomic<uint32_t> _state{ static_cast<uint32_t>(State::Free)};

    public:
        TinyMutex() noexcept = default;
        ~TinyMutex() {
            XTD_DEBUG_VERIFY(_state.load(std::memory_order_relaxed) == static_cast<uint32_t>(State::Free), "Mutex destroyed while locked or contended");
        }
        TinyMutex(const TinyMutex&) = delete;
        TinyMutex& operator=(const TinyMutex&) = delete;
        TinyMutex(TinyMutex&&) = delete;
        TinyMutex& operator=(TinyMutex&&) = delete;

        // Discarding is not unsafe but it will immediately unlock and any other operations on the data this lock is supposed to protect will be unsafe
        [[nodiscard]] auto getLock() noexcept { return UniqueLock<TinyMutex>(*this); }
        template<typename F, typename... Args>
        auto scopedLock(F&& f, Args&&... args) { 
            auto lock = getLock();
            return f(std::forward<Args>(args)...);
        }

        void lock() noexcept {
            if (tryLock()) return;
            if(busyWaitLoop(xtd::time::SteadyClock::now() + xtd::time::Duration<int64_t, xtd::time::Nano>(_busyWaitTime))) return;
            uint32_t expected = static_cast<uint32_t>(TinyMutex::State::LockedWithWaiters);
            while (_state.exchange(static_cast<uint32_t>(TinyMutex::State::LockedWithWaiters), std::memory_order_acquire) 
                    != static_cast<uint32_t>(State::Free)) {
                xtd::platform::waitOnAddress(_state, expected);
            }
            // We cannot change to Locked here without creating a race so when the last waiting thread acquires the lock and 
            // then unlocks with no waiters there will be a needless wakeByAddressSingle call, which is harmless but wasteful
        }

        void unlock() noexcept {
            if(_state.exchange(static_cast<uint32_t>(State::Free), std::memory_order_release) == static_cast<uint32_t>(TinyMutex::State::LockedWithWaiters)) {
                xtd::platform::wakeByAddressSingle(_state);
            }
        }

        bool tryLock() noexcept {
            uint32_t expected = static_cast<uint32_t>(State::Free);
            return _state.compare_exchange_strong(expected, static_cast<uint32_t>(TinyMutex::State::Locked),
                std::memory_order_acquire,
                std::memory_order_relaxed);
        }

        // Timeout granularity is platform-dependent:
        //   Windows: milliseconds (DWORD)
        //   Linux:   nanoseconds (native futex timespec)
        //   macOS:   microseconds (ulock)
        // Sub-granularity remainders are truncated toward zero, meaning a very
        // short requested timeout may resolve to an immediate single check
        // rather than any actual sleep. Callers requiring a guaranteed minimum
        // sleep duration should round up before calling.
        template <class Rep, class Period>
        bool tryLock(xtd::time::Duration<Rep, Period> timeout) noexcept {
            if (tryLock()) return true;
            if(timeout > xtd::time::Duration<Rep, xtd::time::Nano>(_busyWaitTime)) {
                auto start = xtd::time::SteadyClock::now();
                if(busyWaitLoop(start + xtd::time::Duration<Rep, xtd::time::Nano>(_busyWaitTime))) return true;
                auto deadline = start + timeout;
                uint32_t expected = static_cast<uint32_t>(TinyMutex::State::LockedWithWaiters);
                while(true) {
                    if (_state.exchange(static_cast<uint32_t>(TinyMutex::State::LockedWithWaiters), std::memory_order_acquire) 
                        == static_cast<uint32_t>(State::Free)) return true;
                    auto remaining = deadline - xtd::time::SteadyClock::now();
                    if (remaining <= xtd::time::Duration<Rep, Period>::zero()) return false;
#if defined(XTD_PLATFORM_WINDOWS)
                    xtd::platform::waitOnAddress(_state, expected, remaining.template to<xtd::time::MilliSecond<uint32_t>>());
#elif defined(XTD_PLATFORM_LINUX)
                    xtd::platform::waitOnAddress(_state, expected, remaining.template to<xtd::time::NanoSecond<uint64_t>>());
#elif defined(XTD_PLATFORM_MACOS)
                    xtd::platform::waitOnAddress(_state, expected, remaining.template to<xtd::time::MicroSecond<uint32_t>>());
#endif
                }
            }
            else {
                return busyWaitLoop(xtd::time::SteadyClock::now() + timeout);
            }
        }
    private:
        bool busyWaitLoop(xtd::time::SteadyClock::TimePointT deadline) noexcept {
            while(true) {
                xtd::threading::Thread::pause();
                if (tryLock()) return true;
                auto remaining = deadline - xtd::time::SteadyClock::now();
                if (remaining <= decltype(remaining)::zero()) return false;
            }
        }
    };
}