#include "xtd/threading/NativeThread.h"
#include "xtd/platform/Macros.h"

#include <cstring>
#include <new>

#if defined(XTD_PLATFORM_WINDOWS)
    #define NOMINMAX
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <process.h>
#elif defined(XTD_PLATFORM_LINUX)
    #include <pthread.h>
    #include <sched.h>
    #include <unistd.h>
#elif defined(XTD_PLATFORM_MACOS)
    //Support for MACOs may be spotty
    #include <pthread.h>
    #include <sched.h>
    #include <unistd.h>
#endif

#if !defined(XTD_PLATFORM_WINDOWS)
    #include "xtd/threading/Latch.h"
#endif

namespace xtd::threading {

#if !defined(XTD_PLATFORM_WINDOWS)
    struct ThreadStartCtx {
        NativeThread::FunctionType func;
        void *data;
        pid_t tid;
        xtd::threading::Latch ready;
    };
    static void* trampoline(void* data) {
        ThreadStartCtx* ctx = reinterpret_cast<ThreadStartCtx*>(data);
        ctx->tid = syscall(SYS_gettid);
        // ctx->tid = gettid();         // Linux-specific; glibc >= 2.30 has it directly,
        ctx->ready.release();           // signal parent the ID is populated
        return ctx->func(ctx->data);
    }
#endif

    bool NativeThread::create(NativeThread::FunctionType func, void* data) {
#if defined(XTD_PLATFORM_WINDOWS)
        auto* hThread = reinterpret_cast<HANDLE*>(_storage);
        auto* threadId = reinterpret_cast<uint32_t*>(_storage + sizeof(HANDLE));
        ::new (hThread) HANDLE();
        ::new (threadId) uint32_t();

        // 2. Create the thread
        *hThread = reinterpret_cast<HANDLE>(_beginthreadex(
            NULL,                   // Default security attributes
            0,                     // Default stack size (1MB)
            func,               // Pointer to the thread function
            data,                    // Parameter passed to the thread
            0,                      // Creation flags (0 = runs immediately)
            threadId                // Pointer to receive thread identifier
        ));

        if (*hThread == NULL) {
            hThread->~HANDLE();
            threadId->~uint32_t();
            return false;
        }
#else
        auto* hThread = reinterpret_cast<pthread_t*>(_storage);
        auto* threadId = reinterpret_cast<pid_t*>(_storage + sizeof(pthread_t));
        ::new (hThread) pthread_t();
        ::new (threadId) pid_t();

        ThreadStartCtx ctx {
            func, data
        };

        auto result = pthread_create(hThread, NULL, trampoline, reinterpret_cast<void*>(&ctx));
        if (result != 0) {
            sem_destroy(&ctx.ready);
            hThread->~pthread_t();
            threadId->~pid_t();
            return false;
        };
        ctx.ready.acquire();
        *threadId = ctx.tid;
#endif
        return true;
    }

#if defined(XTD_PLATFORM_WINDOWS)
    unsigned NativeThread::join() {
        auto* hThread = reinterpret_cast<HANDLE*>(_storage);
        auto* threadId = reinterpret_cast<uint64_t*>(_storage + sizeof(HANDLE));
        WaitForSingleObject(*hThread, INFINITE);

        DWORD exitCode;
        GetExitCodeThread(*hThread, &exitCode);

        CloseHandle(*hThread);
        hThread->~HANDLE();
        threadId->~uint64_t();
        std::memset(_storage, 0, 16);
        return exitCode;
#else
    void* NativeThread::join() {
        auto* hThread = reinterpret_cast<pthread_t*>(_storage);
        auto* threadId = reinterpret_cast<pid_t*>(_storage + sizeof(pthread_t));
        void* result;
        pthread_join(*hThread, &result);
        hThread->~pthread_t();
        threadId->~pid_t();
        std::memset(_storage, 0, 16);
        return result;
#endif
    }

    //Checks if id is not zero
    bool NativeThread::joinable() const {
#if defined(XTD_PLATFORM_WINDOWS)
        return *reinterpret_cast<const HANDLE*>(_storage + sizeof(HANDLE)) != 0;
#else
        return *reinterpret_cast<const pid_t*>(_storage + sizeof(pthread_t)) != 0;
#endif
    }

    NativeThread::IdType NativeThread::id() const noexcept {
#if defined(XTD_PLATFORM_WINDOWS)
    return *reinterpret_cast<const uintptr_t*>(_storage + sizeof(HANDLE));
#else
    static_assert(sizeof(pid_t) <= sizeof(std::uintptr_t), "pthread_t doesn't fit in uintptr_t on this platform");
    return *reinterpret_cast<const uintptr_t*>(_storage + sizeof(pthread_t));
#endif
    }

    NativeThread::HandleType NativeThread::handle() const noexcept {
#if defined(XTD_PLATFORM_WINDOWS)
    return *reinterpret_cast<const uintptr_t*>(_storage);
#else
    static_assert(sizeof(pthread_t) <= sizeof(uintptr_t), "pthread_t doesn't fit in uintptr_t on this platform");
    return *reinterpret_cast<const uintptr_t*>(_storage);
#endif
    }

    void NativeThread::detach() {
#if defined(XTD_PLATFORM_WINDOWS)
        auto* hThread = reinterpret_cast<HANDLE*>(_storage);
        auto* threadId = reinterpret_cast<uint64_t*>(_storage + sizeof(HANDLE));
        CloseHandle(*hThread);
        hThread->~HANDLE();
        threadId->~uint64_t();
        std::memset(_storage, 0, 16);
#else
        auto* hThread = reinterpret_cast<pthread_t*>(_storage);
        auto* threadId = reinterpret_cast<pid_t*>(_storage + sizeof(pthread_t));
        pthread_detach(hThread); 
        hThread->~HANDLE();
        threadId->~uint64_t();
        std::memset(_storage, 0, 16);
#endif
    }

    NativeThread::IdType NativeThread::thisId() {
#if defined(XTD_PLATFORM_WINDOWS)
        return static_cast<NativeThread::IdType>(GetCurrentThreadId());
#else
        return static_cast<NativeThread::IdType>(syscall(SYS_gettid));
#endif
    }

    NativeThread::HandleType NativeThread::thisHandle() {
#if defined(XTD_PLATFORM_WINDOWS)
        return reinterpret_cast<NativeThread::HandleType>(GetCurrentThread());
#else
        if constexpr(std::is_pointer_v<pthread_t>) {
            return reinterpret_cast<NativeThread::HandleType>(pthread_self());
        }
        else {
            return static_cast<NativeThread::HandleType>(pthread_self());
        }
#endif
    }

    void NativeThread::yield() {
#if defined(XTD_PLATFORM_WINDOWS)
        SwitchToThread();
#else
        sched_yield();
#endif
    }

    void NativeThread::terminate(ReturnType ret) {
#if defined(XTD_PLATFORM_WINDOWS)
        _endthreadex(ret);
#else
        pthread_exit(ret);
#endif
    }

#if defined(XTD_PLATFORM_WINDOWS)
    void NativeThread::sleep(xtd::time::MilliSecond<unsigned long> time) {
        Sleep(static_cast<DWORD>(time));
    }
#else
    void NativeThread::sleep(xtd::time::Second<> sec, xtd::time::NanoSecond<> nano)
        XTD_DEBUG_VERIFY(nano >= 0 && nano <= 999'999'999, "nano must be between 0 and 999'999'999");
        struct timespec req;
        req.tv_sec = static_cast<int64_t>(sec);
        req.tv_nsec = static_cast<int64_t>(nano);
        nanosleep(&req, NULL);
    }
#endif
}