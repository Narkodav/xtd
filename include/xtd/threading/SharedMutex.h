#pragma once
#include <cstdint>
#include <stdint.h>
#include <atomic>

#include "xtd/debug/Checks.h"
#include "xtd/platform/Utilities.h"
#include "xtd/time/Duration.h"
#include "xtd/time/SteadyClock.h"
#include "xtd/threading/ScopedLock.h"
#include "xtd/threading/Mutex.h"

namespace xtd::threading {
    class SharedMutex {
    public:
        enum class State : uint32_t {
            Free = 0,
            Locked = 1
        };
        static constexpr uint32_t _busyWaitTime = 200; // In nanoseconds
    private:
        std::atomic<uint32_t> _writeLocked{static_cast<uint32_t>(State::Free)};
        std::atomic<uint32_t> _refcount{0};

    public:
        SharedMutex() noexcept = default;
        ~SharedMutex() {
            XTD_DEBUG_VERIFY(_refcount.load(std::memory_order_relaxed) == 0 && _writeLocked.load(std::memory_order_relaxed) == static_cast<uint32_t>(State::Free), 
            "SharedMutex destroyed while locked or contended");
        }
        SharedMutex(const SharedMutex&) = delete;
        SharedMutex& operator=(const SharedMutex&) = delete;
        SharedMutex(SharedMutex&&) = delete;
        SharedMutex& operator=(SharedMutex&&) = delete;

        [[nodiscard]] auto getLock() noexcept { return UniqueLock<SharedMutex>(*this); }
        template<typename F, typename... Args>
        auto scopedLock(F&& f, Args&&... args) { 
            auto lock = getLock();
            return f(std::forward<Args>(args)...);
        }
        [[nodiscard]] auto getLockShared() noexcept { return SharedLock<SharedMutex>(*this); }
        template<typename F, typename... Args>
        auto scopedLockShared(F&& f, Args&&... args) { 
            auto lock = getLockShared();
            return f(std::forward<Args>(args)...);
        }

        // ---- exclusive ----
        void lock() noexcept {
            if (tryLockAndDrain()) return;
            if(busyWaitLoop(xtd::time::SteadyClock::now() + xtd::time::Duration<int64_t, xtd::time::Nano>(_busyWaitTime))) return;
            uint32_t expected = static_cast<uint32_t>(State::Locked);
            while (_writeLocked.exchange(static_cast<uint32_t>(State::Locked), std::memory_order_acquire) != static_cast<uint32_t>(State::Free)) {
                xtd::platform::waitOnAddress(_writeLocked, expected);
            }
            // writer flag held; drain existing readers before proceeding
            drainReaders();
        }

        void unlock() noexcept {
            _writeLocked.store(static_cast<uint32_t>(State::Free), std::memory_order_release);
            // wakes both blocked readers and any other blocked writer —
            // both wait on this same word.
            xtd::platform::wakeByAddressAll(_writeLocked);
        }

        bool tryLock() noexcept {
            uint32_t expected = static_cast<uint32_t>(State::Free);
            if (!_writeLocked.compare_exchange_strong(expected, static_cast<uint32_t>(State::Locked),
                    std::memory_order_acquire, std::memory_order_relaxed))
                return false;
            if (_refcount.load(std::memory_order_acquire) != 0) {
                // readers active; give up the writer flag, don't block
                _writeLocked.store(static_cast<uint32_t>(State::Free), std::memory_order_release);
                xtd::platform::wakeByAddressAll(_writeLocked);
                return false;
            }
            return true;
        }

        bool tryLockAndDrain() noexcept {
            uint32_t expected = static_cast<uint32_t>(State::Free);
            if (!_writeLocked.compare_exchange_strong(expected, static_cast<uint32_t>(State::Locked),
                    std::memory_order_seq_cst, std::memory_order_relaxed))
                return false;
            drainReaders();
            return true;
        }

        // ---- shared ----
        void lockShared() noexcept {
            for (;;) {
                if (tryLockShared()) return;
                uint32_t w = _writeLocked.load(std::memory_order_seq_cst);
                if (w != static_cast<uint32_t>(State::Free)) xtd::platform::waitOnAddress(_writeLocked, w);
            }
        }

        void unlockShared() noexcept {
            if (_refcount.fetch_sub(1, std::memory_order_seq_cst) == 1) {
                // we were the last reader out — wake a writer that may be
                // parked draining readers.
                xtd::platform::wakeByAddressAll(_refcount);
            }
        }

        bool tryLockShared() noexcept {
            _refcount.fetch_add(1, std::memory_order_seq_cst);
            if (_writeLocked.load(std::memory_order_seq_cst) == static_cast<uint32_t>(State::Free)) {
                return true; // no writer — we're safely counted as a reader
            }
            // a writer holds or is acquiring; back off
            if (_refcount.fetch_sub(1, std::memory_order_seq_cst) == 1) {
                xtd::platform::wakeByAddressAll(_refcount);
            }
            return false;
        }
    private:
        bool busyWaitLoop(xtd::time::SteadyClock::TimePointT deadline) noexcept {
            while(true) {
                xtd::threading::Thread::pause();
                if (tryLockAndDrain()) return true;
                auto remaining = deadline - xtd::time::SteadyClock::now();
                if (remaining <= decltype(remaining)::zero()) return false;
            }
        }

        void drainReaders() {
            for (;;) {
                uint32_t r = _refcount.load(std::memory_order_seq_cst);
                if (r == 0) return;
                xtd::platform::waitOnAddress(_refcount, r);
            }
        }
    };
}