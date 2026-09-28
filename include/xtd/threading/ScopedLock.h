#pragma once
#include <stdint.h>

namespace xtd::threading {
    template<typename MutexT, auto lockMethod, auto unlockMethod>
    class ScopedLock {
        MutexT* _mutex;
    public:
        explicit ScopedLock(MutexT& m) noexcept : _mutex(&m) { (_mutex->*lockMethod)(); }
        ~ScopedLock() noexcept { if (_mutex) (_mutex->*unlockMethod)(); }

        ScopedLock(const ScopedLock&) = delete;
        ScopedLock& operator=(const ScopedLock&) = delete;

        // move support — lets you e.g. return a ScopedLock from a function
        ScopedLock(ScopedLock&& other) noexcept : _mutex(other._mutex) {
            other._mutex = nullptr;
        }
        ScopedLock& operator=(ScopedLock&& other) noexcept {
            if (this != &other) {
                if (_mutex) (_mutex->*unlockMethod)();
                _mutex = other._mutex;
                other._mutex = nullptr;
            }
            return *this;
        }
    };

    template<typename Mutex>
    using UniqueLock = ScopedLock<Mutex, &Mutex::lock, &Mutex::unlock>;
    template<typename Mutex>
    using SharedLock = ScopedLock<Mutex, &Mutex::lockShared, &Mutex::unlockShared>;
}