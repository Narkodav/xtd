#pragma once
#include "xtd/allocators/NewAllocators.h"
#include "xtd/allocators/Traits.h"
#include "xtd/debug/Checks.h"

#include "xtd/memory/SharedPtr.h"
#include "xtd/memory/Traits.h"

#include <concepts>
#include <cstddef>
#include <utility>

namespace xtd::memory {
    template<typename T, xtd::allocators::DeallocatorT D, ControlBlockCounterSharedT C>
    class WeakPtr {
    public:
        using ValueType = T;
        using SizeType = size_t;
        using PointerType = ValueType*;
        using DeallocatorType = D;
        using AllocatorType = typename DeallocatorType::Allocator;
        using ControlBlockType = ControlBlock<ValueType, C, D>;
        using ControlBlockPointerType = ControlBlockType*;
        using Self = WeakPtr<ValueType, D, C>;

    private:
        ControlBlockPointerType _cb;

    public:
        WeakPtr() : _cb(nullptr) {}

        WeakPtr(std::nullptr_t) : _cb(nullptr) {}
        WeakPtr& operator=(std::nullptr_t) {
            reset();
            _cb = nullptr;
            return *this;
        }

        WeakPtr(const WeakPtr& other) : _cb(other._cb) { _cb->counter().incWeak(); }
        WeakPtr& operator=(const WeakPtr& other) {
            if(this == &other) return *this;

            reset();
            _cb = other._cb;
            _cb->counter().incWeak();

            return *this;
        }

        WeakPtr(WeakPtr&& other) : _cb(std::exchange(other._cb, nullptr)) {}

        WeakPtr& operator=(WeakPtr&& other) {
            if(this == &other) return *this;

            reset();
            _cb = std::exchange(other._cb, nullptr);

            return *this;
        }

        ~WeakPtr() { reset(); }

        WeakPtr(const SharedPtr<ValueType, D, C>& other) : _cb(other._cb) { _cb->counter().incWeak(); }
        WeakPtr& operator=(const SharedPtr<ValueType, D, C>& other) {
            reset();
            _cb = other._cb;
            _cb->counter().incWeak();

            return *this;
        }

        operator SharedPtr<ValueType, D, C>() { return lock(); }
        operator SharedPtr<const ValueType, D, C>() const { return lock(); }

		SharedPtr<ValueType, D, C> lock() noexcept {
			if (_cb->counter().incStrongIfNotZero())
				return SharedPtr<ValueType, D, C>(_cb);
			return nullptr;
		}
		SharedPtr<const ValueType, D, C> lock() const noexcept {
			if (_cb->counter().incStrongIfNotZero())
				return SharedPtr<const ValueType, D, C>(_cb);
			return nullptr;
		}

        inline PointerType data() { return _cb->data(); }
        inline const PointerType data() const { return _cb->data(); }

        ControlBlockType& controlBlock() { return *_cb; }
        const ControlBlockType& controlBlock() const { return *_cb; }
    
        void reset() {
            if(_cb == nullptr) return;
            clearData();
            _cb = nullptr;
        }

        size_t weakCount() const {
            XTD_DEBUG_VERIFY(_cb != nullptr, "Dereferencing a nullptr");
            return _cb->counter().weakCount(); 
        }

        template<typename U>
        std::strong_ordering operator<=>(const SharedPtr<U, D, C>& other) const { return _cb <=> other._cb; }
        template<typename U>
        bool operator==(const SharedPtr<U, D, C>& other) const { return _cb == other._cb; }

        template<typename U>
        std::strong_ordering operator<=>(const WeakPtr<U, D, C>& other) const { return _cb <=> other._cb; }
        template<typename U>
        bool operator==(const WeakPtr<U, D, C>& other) const { return _cb == other._cb; }

        template<typename U>
        std::strong_ordering operator<=>(const U* other) const {
            XTD_DEBUG_VERIFY(_cb != nullptr, "Dereferencing a nullptr");
            return _cb->data() <=> other; 
        }
        template<typename U>
        bool operator==(const U* other) const {
            XTD_DEBUG_VERIFY(_cb != nullptr, "Dereferencing a nullptr");
            return _cb->data() == other; 
        }

        std::strong_ordering operator<=>(std::nullptr_t) const { return _cb <=> nullptr; }
        bool operator==(std::nullptr_t) const { return _cb == nullptr; }

    private:
        inline void clearData() { 
            auto& counter = _cb->counter();
            if(counter.decWeak() == 0) _cb->dealloc();
        }
    };

    template<typename T, xtd::allocators::DeallocatorT D, ControlBlockCounterT C>
    class WeakPtr<T[], D, C> : public xtd::containers::SpanFunctions<WeakPtr<T[], D, C>, T, size_t, xtd::containers::ContiguousIteratorBase<const T>>  {
    public:
        using ValueType = T;
        using SizeType = size_t;
        using PointerType = ValueType*;
        using DeallocatorType = D;
        using AllocatorType = typename DeallocatorType::Allocator;
        using ControlBlockType = ArrayControlBlock<ValueType, C, DeallocatorType>;
        using ControlBlockPointerType = ControlBlockType*;
        using Self = WeakPtr<ValueType[], DeallocatorType, C>;

        using IteratorType = xtd::containers::ContiguousIteratorBase<ValueType>;
        using ConstIteratorType = xtd::containers::ContiguousIteratorBase<const ValueType>;

    private:
        ControlBlockPointerType _cb;

        WeakPtr(ControlBlockPointerType cb) : _cb(cb) {};
    public:
        WeakPtr() : _cb(nullptr) {}

        WeakPtr(std::nullptr_t) : _cb(nullptr) {}
        WeakPtr& operator=(std::nullptr_t) {
            reset();
            _cb = nullptr;
            return *this;
        }

        WeakPtr(const WeakPtr& other) : _cb(other._cb) { _cb->counter().incWeak(); }
        WeakPtr& operator=(const WeakPtr& other) {
            if(this == &other) return *this;

            reset();
            _cb = other._cb;
            _cb->counter().incWeak();

            return *this;
        }

        WeakPtr(WeakPtr&& other) : _cb(std::exchange(other._cb, nullptr)) {}

        WeakPtr& operator=(WeakPtr&& other) {
            if(this == &other) return *this;

            reset();
            _cb = std::exchange(other._cb, nullptr);

            return *this;
        }

        ~WeakPtr() { reset(); }

        WeakPtr(const SharedPtr<ValueType[], D, C>& other) : _cb(other._cb) { _cb->counter().incWeak(); }
        WeakPtr& operator=(const SharedPtr<ValueType[], D, C>& other) {
            reset();
            _cb = other._cb;
            _cb->counter().incWeak();

            return *this;
        }

        operator SharedPtr<ValueType[], D, C>() { return lock(); }
        operator SharedPtr<const ValueType[], D, C>() const { return lock(); }

		SharedPtr<ValueType[], D, C> lock() noexcept {
			if (_cb->counter().incStrongIfNotZero())
				return SharedPtr<ValueType[], D, C>(_cb);
			return nullptr;
		}
		SharedPtr<const ValueType[], D, C> lock() const noexcept {
			if (_cb->counter().incStrongIfNotZero())
				return SharedPtr<const ValueType[], D, C>(_cb);
			return nullptr;
		}

        inline PointerType data() { return _cb->data(); }
        inline const PointerType data() const { return _cb->data(); }

        ControlBlockType& controlBlock() { return *_cb; }
        const ControlBlockType& controlBlock() const { return *_cb; }
    
        void reset() {
            if(_cb == nullptr) return;
            clearData();
            _cb = nullptr;
        }

        size_t weakCount() const {
            XTD_DEBUG_VERIFY(_cb != nullptr, "Dereferencing a nullptr");
            return _cb->counter().weakCount(); 
        }

        template<typename U>
        std::strong_ordering operator<=>(const SharedPtr<U[], D, C>& other) const { return _cb <=> other._cb; }
        template<typename U>
        bool operator==(const SharedPtr<U[], D, C>& other) const { return _cb == other._cb; }

        template<typename U>
        std::strong_ordering operator<=>(const WeakPtr<U[], D, C>& other) const { return _cb <=> other._cb; }
        template<typename U>
        bool operator==(const WeakPtr<U[], D, C>& other) const { return _cb == other._cb; }

        template<typename U>
        std::strong_ordering operator<=>(const U* other) const {
            XTD_DEBUG_VERIFY(_cb != nullptr, "Dereferencing a nullptr");
            return _cb->data() <=> other; 
        }
        template<typename U>
        bool operator==(const U* other) const {
            XTD_DEBUG_VERIFY(_cb != nullptr, "Dereferencing a nullptr");
            return _cb->data() == other; 
        }

        std::strong_ordering operator<=>(std::nullptr_t) const { return _cb <=> nullptr; }
        bool operator==(std::nullptr_t) const { return _cb == nullptr; }

    private:
        inline void clearData() { 
            auto& counter = _cb->counter();
            if(counter.decWeak() == 0) _cb->dealloc();
        }
    };
}


