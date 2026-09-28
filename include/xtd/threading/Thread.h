#pragma once
#include <stdexcept>
#include <stdint.h>
#include <tuple>
#include <type_traits>

#include "xtd/threading/NativeThread.h"

#include "xtd/threading/Latch.h"

namespace xtd::threading {

    class Thread {
    public:
        using Id = NativeThread::IdType;
        using Handle = NativeThread::HandleType;

        struct ExitSignal {}; // Sentinel type, throw this to safely exit the thread. Note that it requires the whole stack frame to rethrow this error

    private:
        NativeThread _impl;

        template<typename F, typename... Args>
        struct Ctx {
            std::tuple<std::decay_t<Args>...> argTuple;
            F f;
            xtd::threading::Latch ready;
        };

        template<typename F>
        struct Ctx<F> {
            F f;
            xtd::threading::Latch ready;
        };

        template<typename F, typename Except, typename... Args>
        struct CtxExcept {
            std::tuple<std::decay_t<Args>...> argTuple;
            F f;
            Except e;
            xtd::threading::Latch ready;
        };

        template<typename F, typename Except>
        struct CtxExcept<F, Except> {
            F f;
            Except e;
            xtd::threading::Latch ready;
        };

        template<typename F, typename... Args>
#if defined(XTD_PLATFORM_WINDOWS)
        static unsigned __stdcall trampoline(void* data) noexcept {
#else
        static void* trampoline(void* data) noexcept {
#endif
            using C = Ctx<F, Args...>;
            auto* ctx = reinterpret_cast<C*>(data);
            if constexpr (sizeof...(Args) != 0) {
                auto argCopy = ctx->argTuple;
                auto fCopy = ctx->f;
                ctx->ready.release();
                std::apply([&fCopy](auto&&... args) {
                    try {
                        fCopy(args...);
                    }
                    catch(ExitSignal) {
                        return 0;
                    }
                    catch(...) {
                        std::terminate();
                    }
                }, argCopy);
            } else {
                auto fCopy = ctx->f;
                ctx->ready.release();
                try {
                    fCopy();
                }
                catch(ExitSignal) {
                    return 0;
                }
                catch(...) {
                    std::terminate();
                }
            }
            return 0;
        }

        template<typename F, typename... Args>
#if defined(XTD_PLATFORM_WINDOWS)
        static unsigned __stdcall exceptTrampoline(void* data) noexcept {
#else
        static void* exceptTrampoline(void* data) noexcept {
#endif
            using C = Ctx<F, Args...>;
            auto* ctx = reinterpret_cast<C*>(data);
            if constexpr (sizeof...(Args) != 0) {
                auto argCopy = ctx->argTuple;
                auto fCopy = ctx->f;
                auto eCopy = ctx->e;
                ctx->ready.release();
                std::apply([&fCopy, &eCopy](auto&&... args) {
                    try {
                        fCopy(args...);
                    }
                    catch(ExitSignal) {
                        return 0;
                    }
                    catch(const std::exception& e) {
                        eCopy(e);
                    }
                    catch(...) {
                        std::terminate();
                    }
                }, argCopy);
            } else {
                auto fCopy = ctx->f;
                auto eCopy = ctx->e;
                ctx->ready.release();
                try {
                    fCopy();
                }
                catch(ExitSignal) {
                    return 0;
                }
                catch(const std::exception& e) {
                    eCopy(e);
                }
                catch(...) {
                    std::terminate();
                }
            }
            return 0;
        }

    public:
        Thread() noexcept = default;
        ~Thread() noexcept { join(); }

        Thread(Thread&&) = default;
        Thread& operator=(Thread&&) = default;

        Thread(const Thread&) = delete;
        Thread& operator=(const Thread&) = delete;

        // To acquire an OS handle do:
        // #if defined(XTD_PLATFORM_WINDOWS)
        //     HANDLE h = reinterpret_cast<HANDLE>(t.handle());
        //     SetThreadPriority(h, THREAD_PRIORITY_HIGHEST);
        // #else
        //     pthread_t h = static_cast<pthread_t>(t.handle()); // or reinterpret_cast if pthread_t is a pointer on your platform
        //     pthread_setaffinity_np(h, sizeof(cpuset), &cpuset);
        // #endif
        inline Handle handle() const noexcept { return _impl.handle(); }
        inline Id id() const noexcept { return _impl.id(); }

        template<typename F, typename... Args>
        requires std::is_invocable_v<F, Args...>
        bool create(F&& f, Args&&... args) noexcept {
            join();
            using C = Ctx<F, Args...>;
            if constexpr (sizeof...(Args) != 0) {
                C ctx{
                        .argTuple = { std::forward<Args>(args)... },
                        .f = std::forward<F>(f),
                        .ready = xtd::threading::Latch()
                };
                auto result = _impl.create(trampoline<F, Args...>, &ctx);
                if(!result) return result;
                ctx.ready.acquire();
            } else {
                C ctx{ 
                    .f = std::forward<F>(f),
                    .ready = xtd::threading::Latch()
                };
                auto result = _impl.create(trampoline<F, Args...>, &ctx);
                if(!result) return result;
                ctx.ready.acquire();
            }
            return true;
        }

        template<typename F, typename Except, typename... Args>
        requires std::is_invocable_v<F, Args...>
        bool createWithException(F&& f, Except&& e, Args&&... args) noexcept {
            join();
            using C = Ctx<F, Args...>;
            if constexpr (sizeof...(Args) != 0) {
                C ctx{
                        .argTuple = { std::forward<Args>(args)... },
                        .f = std::forward<F>(f),
                        .e = std::forward<Except>(e),
                        .ready = xtd::threading::Latch()
                };
                auto result = _impl.create(trampoline<F, Args...>, &ctx);
                if(!result) return result;
                ctx.ready.acquire();
            } else {
                C ctx{ 
                    .f = std::forward<F>(f),
                    .e = std::forward<Except>(e),
                    .ready = xtd::threading::Latch()
                };
                auto result = _impl.create(trampoline<F, Args...>, &ctx);
                if(!result) return result;
                ctx.ready.acquire();
            }
            return true;
        }

        inline bool joinable() const noexcept { return _impl.joinable(); }
        inline void join() noexcept { if(joinable()) _impl.join(); }
        inline void detach() noexcept { if(joinable()) _impl.detach(); }

        // returns the id of this thread
        static inline Id thisId() noexcept { return NativeThread::thisId(); }
        // returns the handle to the current thread
        static inline Handle thisHandle() noexcept { return NativeThread::thisHandle(); }
        static inline void yield() noexcept { return NativeThread::yield(); }
        static inline void pause() noexcept { return NativeThread::pause(); }

        //Terminates the thread, note that this doesn't unwind the stack, so destructors will not be ran
        static inline void terminate() noexcept { NativeThread::terminate(0); };

        template<typename Rep, typename Ratio>
        static inline void sleep(xtd::time::Duration<Rep, Ratio> time) noexcept {
#if defined(XTD_PLATFORM_WINDOWS)
            NativeThread::sleep(time.template to<xtd::time::MilliSecond<unsigned long>>());
#else
            auto seconds = time.to<xtd::time::Seconds<int64_t>>();
            auto nano = time.to<xtd::time::Nanoseconds<int64_t>>() - seconds.to<xtd::time::Nanoseconds<int64_t>>();
            NativeThread::sleep(seconds, nano);
#endif
        }
    };
}