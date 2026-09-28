#pragma once
#include "xtd/Macros.h"
#include "xtd/platform/Macros.h"

#include <atomic>

#include "xtd/time/Duration.h"

namespace xtd::platform {
    enum class WaitResult {
        Woken,     // got a notify (or spurious wake — caller re-checks the value regardless)
        TimedOut,  // timeout elapsed with no notify
        NotEqual,  // *addr != expected at the moment of the check — nothing to wait for
    };
    XTD_API void waitOnAddress(std::atomic<uint32_t>& addr, uint32_t& expected) noexcept;

#if defined(XTD_PLATFORM_WINDOWS)
    XTD_API WaitResult waitOnAddress(std::atomic<uint32_t>& addr, uint32_t& expected, xtd::time::MilliSecond<uint32_t> timeout) noexcept;
#elif defined(XTD_PLATFORM_LINUX)
    WaitResult waitOnAddress(std::atomic<uint32_t>& addr, uint32_t& expected, xtd::time::NanoSecond<uint64_t> timeout) noexcept {
        auto count = timeout.count();
        waitOnAddress(addr, expected, count % 1'000'000'000, count / 1'000'000'000);
    }
    XTD_API WaitResult waitOnAddress(std::atomic<uint32_t>& addr, uint32_t& expected, xtd::time::NanoSecond<uint32_t> timeoutNS, xtd::time::Second<uint32_t> timeoutS) noexcept;
#elif defined(XTD_PLATFORM_MACOS)
    XTD_API WaitResult waitOnAddress(std::atomic<uint32_t>& addr, uint32_t& expected, xtd::time::MicroSecond<uint32_t> timeout) noexcept;
#endif
    
    XTD_API void wakeByAddressSingle(std::atomic<uint32_t>& addr) noexcept;
    XTD_API void wakeByAddressAll(std::atomic<uint32_t>& addr) noexcept;
}