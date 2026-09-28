#pragma once
#include <cstddef>
#include <stdint.h>
#include <thread>

#include "xtd/Macros.h"
#include "xtd/Platform/Macros.h"
#include "xtd/time/Duration.h"

namespace xtd::threading {

    // A very thin wrapper around native threads, uses no heap allocation and has only basic functionality
    // The functions do not checks state, the user must manually query joinable before creating joining or detaching or implicitly know object state
    // One use case is taking advantage of POSIX specific void* return values
    class XTD_API NativeThread {
    public:
        //__stdcall is technically pointless, xtd doesn't support 32 bit
        #if defined(XTD_PLATFORM_WINDOWS)
            using FunctionType = unsigned (__stdcall*)(void *);
            using ReturnType = unsigned;
        #else
            using ReturnType = void*;
            using FunctionType = void* (*)(void *);
        #endif

        using HandleType = uintptr_t;
        using IdType = uintptr_t;

    private:
        alignas(void*) std::byte _storage[16]{}; //Aligned storage for OS specific handles
    public:

        NativeThread() = default;
        ~NativeThread() = default;

        NativeThread(NativeThread&&) = default;
        NativeThread& operator=(NativeThread&&) = default;

        NativeThread(const NativeThread&) = delete;
        NativeThread& operator=(const NativeThread&) = delete;

        // To acquire an OS handle do:
        // #if defined(XTD_PLATFORM_WINDOWS)
        //     HANDLE h = reinterpret_cast<HANDLE>(t.handle());
        //     SetThreadPriority(h, THREAD_PRIORITY_HIGHEST);
        // #else
        //     pthread_t h = static_cast<pthread_t>(t.handle()); // or reinterpret_cast if pthread_t is a pointer on your platform
        //     pthread_setaffinity_np(h, sizeof(cpuset), &cpuset);
        // #endif
        inline HandleType handle() const noexcept;
        inline IdType id() const noexcept;

        bool create(FunctionType func, void* data);
#if defined(XTD_PLATFORM_WINDOWS)
        inline unsigned join();
#else
        inline void* join();
#endif
        inline bool joinable() const;
        inline void detach();

        // returns the id of this thread
        static inline IdType thisId();
        // returns the handle to the current thread
        static inline HandleType thisHandle();
        static inline void yield();
        // unlike yield this doesn't give up a time slice to another thread, its a pure CPU hint, use for something like wait loops
        static inline void pause() { _mm_pause(); } 

        //Terminates the thread, note that this doesn't unwind the stack, so destructors will not be ran
        static inline void terminate(ReturnType ret);

#if defined(XTD_PLATFORM_WINDOWS)
        static inline void sleep(xtd::time::MilliSecond<unsigned long> time);
#else
        // Nano must be between 0 and 999'999'999
        static inline void sleep(xtd::time::Second<> sec, xtd::time::NanoSecond<> nano);
#endif
    };
}