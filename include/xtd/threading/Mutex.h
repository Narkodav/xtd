#pragma once
#include <cstdint>
#include <stdint.h>
#include <atomic>

#include "xtd/debug/Checks.h"
#include "xtd/platform/Utilities.h"
#include "xtd/time/Duration.h"
#include "xtd/time/SteadyClock.h"
#include "xtd/threading/ScopedLock.h"
#include "xtd/threading/Thread.h"

namespace xtd::threading {
    template<typename Mutex>
    using UniqueLock = ScopedLock<Mutex, &Mutex::lock, &Mutex::unlock>;

    class Mutex {
    private:
        enum class State : uint32_t {
            Free = 0,
            Locked = 1
        };
    
        static constexpr uint32_t _busyWaitTime = 200; // In nanoseconds

        std::atomic<uint32_t> _state{ static_cast<uint32_t>(State::Free) };
        std::atomic<uint32_t> _waiterCount{0};

    public:
        Mutex() noexcept = default;
        ~Mutex() {
            XTD_DEBUG_VERIFY(_state.load(std::memory_order_relaxed) == static_cast<uint32_t>(State::Free) && 
                _waiterCount.load(std::memory_order_relaxed) == 0, 
                "Mutex destroyed while locked or contended");
        }
        Mutex(const Mutex&) = delete;
        Mutex& operator=(const Mutex&) = delete;
        Mutex(Mutex&&) = delete;
        Mutex& operator=(Mutex&&) = delete;

        // Discarding is not unsafe but it will immediately unlock and any other operations on the data this lock is supposed to protect will be unsafe
        // An advantage over scoping is that it can be passed as an object to signify access
        [[nodiscard]] auto getLock() noexcept { return UniqueLock<Mutex>(*this); }
        template<typename F, typename... Args>
        auto scopedLock(F&& f, Args&&... args) { 
            auto lock = getLock();
            return f(std::forward<Args>(args)...);
        }

        void lock() noexcept {
            if (tryLock()) return;
            if(busyWaitLoop(xtd::time::SteadyClock::now() + xtd::time::Duration<int64_t, xtd::time::Nano>(_busyWaitTime))) return;
            _waiterCount.fetch_add(1, std::memory_order_relaxed);
            uint32_t expected = static_cast<uint32_t>(State::Free);
            while (!_state.compare_exchange_strong(expected, static_cast<uint32_t>(State::Locked),
            std::memory_order_acquire, std::memory_order_relaxed)) {
                xtd::platform::waitOnAddress(_state, expected);
                expected = static_cast<uint32_t>(State::Free);
            }
            _waiterCount.fetch_sub(1, std::memory_order_relaxed);
        }

        void unlock() noexcept {
            _state.store(static_cast<uint32_t>(State::Free), std::memory_order_release);
            if(_waiterCount.load(std::memory_order_relaxed) != 0) {
                xtd::platform::wakeByAddressSingle(_state);
            }
        }

        bool tryLock() noexcept {
            uint32_t expected = static_cast<uint32_t>(State::Free);
            return _state.compare_exchange_strong(expected, static_cast<uint32_t>(State::Locked),
                std::memory_order_acquire,
                std::memory_order_relaxed);
        }

        size_t waiterCount() const { return _waiterCount.load(std::memory_order_relaxed); }

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
                _waiterCount.fetch_add(1, std::memory_order_relaxed);
                uint32_t expected;
                while(true) {
                    expected = static_cast<uint32_t>(State::Free);
                    if (_state.compare_exchange_strong(expected, static_cast<uint32_t>(State::Locked),
                std::memory_order_acquire, std::memory_order_relaxed)) {
                        _waiterCount.fetch_sub(1, std::memory_order_relaxed);
                        return true;
                    }
                    auto remaining = deadline - xtd::time::SteadyClock::now();
                    if (remaining <= xtd::time::Duration<Rep, Period>::zero()) {
                        _waiterCount.fetch_sub(1, std::memory_order_relaxed);
                        return false;
                    }
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