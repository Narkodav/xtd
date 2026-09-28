#pragma once
#include <type_traits>
#include <utility>

namespace xtd::containers {
    template <typename Signature>
    struct Callback;

    template <typename R, typename... Args>
    struct Callback<R(Args...)> {
        using Self = Callback<R(Args...)>;
        using FnPtr = R(*)(void*, Args...);
        void* ctx = nullptr;
        FnPtr fn = nullptr;

        R operator()(Args... args) const {
            return fn(ctx, std::forward<Args>(args)...);
        }
        explicit operator bool() const noexcept { return fn != nullptr; }
        
        static Self make(void* ctx, FnPtr f) { return Self{ ctx, f }; }

        template<typename F>
        requires (std::is_invocable_r_v<R, F, void*, Args...>)
        static Self make(F& f) {     
            return Self{
                .ctx = &f,
                .fn = [](void* ctx, Args... args) -> R {
                    return (*static_cast<F*>(ctx))(std::forward<Args>(args)...);
                }
            };
        }
    };

    template <typename C, typename R, typename... Args>
    struct Callback<R (C::*)(Args...)> : public Callback<R(Args...)> {
        using Base = Callback<R(Args...)>;
        using Self = Callback<R (C::*)(Args...)>;

        template<auto MemFn>
        requires (std::is_member_function_pointer_v<decltype(MemFn)> &&
            std::is_invocable_r_v<R, decltype(MemFn), C*, Args...>)
        static Self make(C* obj) {     
            return Self{ Base{
                .ctx = obj,
                .fn = [](void* ctx, Args... args) -> R {
                    return (static_cast<C*>(ctx)->*MemFn)(std::forward<Args>(args)...);
                } }
            };
        }

        template<auto StaticF>
        requires (!std::is_member_function_pointer_v<decltype(StaticF)> &&
            std::is_invocable_r_v<R, decltype(StaticF), C*, Args...>)
        static Self make(C* obj) {     
            return Self{ Base{
                .ctx = obj,
                .fn = [](void* ctx, Args... args) -> R {
                    return StaticF(static_cast<C*>(ctx), std::forward<Args>(args)...);
                } }
            };
        }
    };
}

