#pragma once
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <stdint.h>
#include <atomic>
#include <new>
#include <algorithm>
#include <utility>

#include "xtd/allocators/Traits.h"

namespace xtd::memory {
    template<typename C>
    concept ControlBlockCounterSharedT = requires(C c) {
        { c.incStrong() } -> std::convertible_to<size_t>;
        { c.incWeak() } -> std::convertible_to<size_t>;
        { c.decStrong() } -> std::convertible_to<size_t>;
        { c.decWeak() } -> std::convertible_to<size_t>;
        { c.incStrongIfNotZero() } -> std::convertible_to<size_t>;

        { c.strongCount() } -> std::convertible_to<size_t>;
        { c.weakCount() } -> std::convertible_to<size_t>;
    };

	class SharedControlBlockCounter {
	private:
		size_t _strongCount = 1;
		size_t _weakCount = 1;
	public:
		inline size_t incStrong() noexcept {
			return ++_strongCount;
		}
		inline size_t decStrong() noexcept {
			return --_strongCount;
		}
		inline size_t incWeak() noexcept {
			return ++_weakCount;
		}
		inline size_t decWeak() noexcept {
			return --_weakCount;
		}
		inline size_t strongCount() const noexcept {
			return _strongCount;
		}
		inline size_t weakCount() const noexcept {
			return _weakCount;
		}

		bool incStrongIfNotZero() noexcept {
			if (_strongCount == 0) {
				return false;
			}
			++_strongCount;
			return true;
		}
	};

	class SharedControlBlockAtomicCounter {
	private:
		std::atomic<size_t> _strongCount = 1;
		std::atomic<size_t> _weakCount = 1;
	public:
		inline size_t incStrong() noexcept {
			return _strongCount.fetch_add(1, std::memory_order_relaxed) + 1;
		}
		inline size_t decStrong() noexcept {
			return _strongCount.fetch_sub(1, std::memory_order_relaxed) - 1;
		}
		inline size_t incWeak() noexcept {
			return _weakCount.fetch_add(1, std::memory_order_relaxed) + 1;
		}
		inline size_t decWeak() noexcept {
			return _weakCount.fetch_sub(1, std::memory_order_relaxed) - 1;
		}
		inline size_t strongCount() const noexcept {
			return _strongCount.load(std::memory_order_relaxed);
		}
		inline size_t weakCount() const noexcept {
			return _strongCount.load(std::memory_order_relaxed);
		}

		bool incStrongIfNotZero() noexcept {
			size_t current = _strongCount.load(std::memory_order_relaxed);
			while (current != 0) {
				if (_strongCount.compare_exchange_weak(current, current + 1, std::memory_order_relaxed)) {
					return true;
				}
			}
			return false;
		}
	};

    template<typename C>
    concept ControlBlockCounterT = requires(C c) {
        { c.inc() } -> std::convertible_to<size_t>;
        { c.dec() } -> std::convertible_to<size_t>;
        { c.count() } -> std::convertible_to<size_t>;
    };

	class ControlBlockCounter {
	private:
		size_t _count = 1;
	public:
		inline size_t inc() noexcept { return ++_count; }
		inline size_t dec() noexcept { return --_count; }
		inline size_t count() const noexcept { return _count; }
	};

	class ControlBlockAtomicCounter {
	private:
		std::atomic<size_t> _count = 1;
	public:
		inline size_t inc() noexcept {
			return _count.fetch_add(1, std::memory_order_relaxed) + 1;
		}
		inline size_t dec() noexcept {
			return _count.fetch_sub(1, std::memory_order_relaxed) - 1;
		}
		inline size_t count() const noexcept {
			return _count.load(std::memory_order_relaxed);
		}
	};

    template<typename T, typename C, xtd::allocators::TypedDeallocatorT DD, xtd::allocators::DeallocatorT DC>
    class ControlBlock {
    public:
        using DataDeallocatorType = DD;
        using ControlBlockDeallocatorType = DC;
        using ControlBlockCounterType = C;
        using ValueType = T;
        using PointerType = ValueType*;
        using Self = ControlBlock<ValueType, ControlBlockCounterType, DataDeallocatorType, ControlBlockDeallocatorType>;
    private:
        ControlBlockCounterType _counter;
        [[no_unique_address]] DataDeallocatorType _dataDealloc;
        [[no_unique_address]] ControlBlockDeallocatorType _controlDealloc;

    public:
        template<typename... Args>
        ControlBlock(DataDeallocatorType&& dataDealloc, ControlBlockDeallocatorType&& controlDealloc) 
            : _counter(), _dataDealloc(std::move(dataDealloc)), _controlDealloc(std::move(controlDealloc)) {}

        auto& counter() { return _counter; }

        void destroy(PointerType data) { _dataDealloc.deallocate(data); }
        void dealloc() {
            auto dealloc = std::move(_controlDealloc);
            this->~Self();
            dealloc.deallocate(this, sizeof(Self), alignof(Self));
        }
    };

    template<typename T, typename C, xtd::allocators::TypedDeallocatorT DD, xtd::allocators::DeallocatorT DC>
    class ArrayControlBlock {
    public:
        using DataDeallocatorType = DD;
        using ControlBlockDeallocatorType = DC;
        using ControlBlockCounterType = C;
        using ValueType = T;
        using PointerType = ValueType*;
        using Self = ArrayControlBlock<ValueType, ControlBlockCounterType, DataDeallocatorType, ControlBlockDeallocatorType>;

        static constexpr size_t _offset = sizeof(ArrayControlBlock);
    private:
        ControlBlockCounterType _counter;
        [[no_unique_address]] DataDeallocatorType _dataDealloc;
        [[no_unique_address]] ControlBlockDeallocatorType _controlDealloc;

    public:
        ArrayControlBlock(DataDeallocatorType&& dataDealloc, ControlBlockDeallocatorType&& controlDealloc) 
            : _counter(), _dataDealloc(std::move(dataDealloc)), _controlDealloc(std::move(controlDealloc)) {}

        void destroy(PointerType data) { _dataDealloc.deallocate(data); }
        void dealloc() {
            auto dealloc = std::move(_controlDealloc);
            this->~Self();
            dealloc.deallocate(this, sizeof(Self), alignof(Self));
        }
    };
}