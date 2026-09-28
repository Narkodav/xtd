#pragma once
#include <memory>
#include <type_traits>
#include <utility>

#include "xtd/allocators/NewAllocators.h"
#include "xtd/allocators/Traits.h"

namespace xtd::containers {

    // This is the std::function equivalent, convenient to use, but complete type erasure with vtable lookup can affect performance
    // Try using Callback as an alternative if your goal is toregister a callback
    template <typename Signature>
    class Function;

    template <typename R, typename... Args>
    class Function<R(Args...)> {
    private:
        struct Callable {
            virtual R invoke(Args... args) = 0;
            virtual ~Callable() = default;
            virtual void destroySelf() = 0;
            size_t requiredSize() = 0;
        };

        template <typename F, xtd::allocators::TypedDeallocatorT D = xtd::allocators::TypedDeallocatorNew>
        struct CallableImpl final : Callable {
            F _f;
            [[no_unique_address]] D _dealloc;

            CallableImpl() = default;
    
            R invoke(Args... args) override final { return f(std::forward<Args>(args)...); }

            size_t requiredSize() { return sizeof(CallableImpl); }

            virtual void destroySelf() { _dealloc.deallocate(this); }
        };

        Callable* _ptr = nullptr;
    public:
        Function() = default;
        ~Function() { reset(); }

        template <typename F, xtd::allocators::AllocatorT A = xtd::allocators::TypedAllocatorNew>
        requires (std::is_same_v<std::decay_t<F>, Function> && std::is_default_constructible_v<A>)
        Function(F& f) {
            auto alloc = A();
            assignImpl(f, alloc);
        }

        template <typename F, xtd::allocators::AllocatorT A = xtd::allocators::TypedAllocatorNew>
        requires (std::is_same_v<std::decay_t<F>, Function>)
        Function(F& f, A& alloc) {
            assignImpl(f, alloc);
        }

        //Copying is explicitly disallowed because it would require hidden polymorphic allocator machinery for non trivial cases
        Function(const Function& other) = delete;
        Function& operator=(const Function& other) = delete;

        Function(Function&& other) noexcept : _ptr(std::exchange(other._ptr, nullptr)) {}
        Function& operator=(Function&& other) noexcept {
            if(this == &other) return *this;
            _ptr = std::exchange(other._ptr, nullptr);
            return *this;
        }

        void reset() { if(_ptr != nullptr) _ptr->destroySelf(); }

        R operator()(Args... args) const { return _ptr->invoke(std::forward<Args>(args)...); }

        explicit operator bool() const noexcept { return _ptr != nullptr; }

    private:
        template<typename F, xtd::allocators::AllocatorT A = xtd::allocators::TypedAllocatorNew>
        requires std::is_same_v<std::decay_t<F>, Function>
        void assignImpl(F f, A& alloc) {
            auto [dealloc, callable] = alloc.template allocate<CallableImpl>();
            callable->_f = std::move(f);
            callable->_dealloc = std::move(dealloc);
            _ptr = callable;
        }
    };
}