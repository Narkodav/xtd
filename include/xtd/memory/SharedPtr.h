#pragma once
#include "xtd/allocators/NewAllocators.h"
#include "xtd/allocators/Traits.h"
#include "xtd/debug/Checks.h"
#include "xtd/containers/Iterators.h"
#include "xtd/containers/Span.h"

#include "xtd/memory/Traits.h"

#include <concepts>
#include <cstddef>
#include <utility>

namespace xtd::memory {
    template<typename T, xtd::allocators::DeallocatorT D = xtd::allocators::DeallocatorNew, ControlBlockCounterSharedT C = SharedControlBlockAtomicCounter>
    class WeakPtr;

    template<typename T, xtd::allocators::DeallocatorT D = xtd::allocators::DeallocatorNew, ControlBlockCounterSharedT C = SharedControlBlockAtomicCounter>
    class SharedPtr {
        friend class WeakPtr<T, D, C>;
    public:
        using ValueType = T;
        using SizeType = size_t;
        using PointerType = ValueType*;
        using DeallocatorType = D;
        using AllocatorType = typename DeallocatorType::Allocator;
        using ControlBlockType = ControlBlock<ValueType, C, DeallocatorType>;
        using ControlBlockPointerType = ControlBlockType*;
        using Self = SharedPtr<ValueType, D, C>;

    private:
        ControlBlockPointerType _cb;

        SharedPtr(ControlBlockPointerType cb) : _cb(cb) {};

    public:
        SharedPtr() : _cb(nullptr) {}

        SharedPtr(std::nullptr_t) : _cb(nullptr) {}
        SharedPtr& operator=(std::nullptr_t) {
            reset();
            _cb = nullptr;
            return *this;
        }

        SharedPtr(const SharedPtr& other) : _cb(other._cb) { _cb->counter().incStrong(); }
        SharedPtr& operator=(const SharedPtr& other) {
            if(this == &other) return *this;

            reset();
            _cb = other._cb;
            _cb->counter().incStrong();

            return *this;
        }

        SharedPtr(SharedPtr&& other) : _cb(std::exchange(other._cb, nullptr)) {}

        SharedPtr& operator=(SharedPtr&& other) {
            if(this == &other) return *this;

            reset();
            _cb = std::exchange(other._cb, nullptr);

            return *this;
        }

        ~SharedPtr() { reset(); }

        inline PointerType data() { return _cb->data(); }
        inline const PointerType data() const { return _cb->data(); }

        ControlBlockType& controlBlock() { return *_cb; }
        const ControlBlockType& controlBlock() const { return *_cb; }
    
        void reset() {
            if(_cb == nullptr) return;
            clearData();
            _cb = nullptr;
        }

        ValueType& operator*() {
            XTD_DEBUG_VERIFY(_cb != nullptr, "Dereferencing a nullptr");
            return *data();
        }
        const ValueType& operator*() const  {
            XTD_DEBUG_VERIFY(_cb != nullptr, "Dereferencing a nullptr");
            return *data();
        }

        ValueType* operator->() {
            XTD_DEBUG_VERIFY(_cb != nullptr, "Dereferencing a nullptr");
            return *data();
        }
        const ValueType* operator->() const  {
            XTD_DEBUG_VERIFY(_cb != nullptr, "Dereferencing a nullptr");
            return *data();
        }

        template<typename... Args>
        static Self make(Args&&... args) requires std::is_default_constructible_v<AllocatorType> {
            AllocatorType alloc{};
            return make(alloc, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static Self make(AllocatorType& alloc, Args&&... args) {
            auto* cb = reinterpret_cast<ControlBlockType*>(alloc.allocate(sizeof(ControlBlockType), alignof(ControlBlockType)));
            ::new (cb) ControlBlockType(alloc.deallocator(), std::forward<Args>(args)...);
            return Self(cb);
        }

        size_t strongCount() const {
            XTD_DEBUG_VERIFY(_cb != nullptr, "Dereferencing a nullptr");
            return _cb->counter().strongCount(); 
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

        std::strong_ordering operator<=>(std::nullptr_t) const { return _cb <=> nullptr; }
        bool operator==(std::nullptr_t) const { return _cb == nullptr; }

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

    private:
        inline void clearData() { 
            auto& counter = _cb->counter();
            if(counter.decStrong() == 0) {
                _cb->destroy();
                if(counter.decWeak() == 0) _cb->dealloc();
            }
        }
    };


    template<typename T, xtd::allocators::DeallocatorT D, ControlBlockCounterSharedT C>
    class SharedPtr<T[], D, C> : public xtd::containers::SpanFunctions<SharedPtr<T[], D, C>, T, size_t, xtd::containers::ContiguousIteratorBase<const T>>  {
        friend class WeakPtr<T[], D, C>;
    public:
        using ValueType = T;
        using SizeType = size_t;
        using PointerType = ValueType*;
        using DeallocatorType = D;
        using AllocatorType = typename DeallocatorType::Allocator;
        using ControlBlockType = ArrayControlBlock<ValueType, C, DeallocatorType>;
        using ControlBlockPointerType = ControlBlockType*;
        using Self = SharedPtr<ValueType, DeallocatorType, C>;

        using IteratorType = xtd::containers::ContiguousIteratorBase<ValueType>;
        using ConstIteratorType = xtd::containers::ContiguousIteratorBase<const ValueType>;

    private:
        ControlBlockPointerType _cb;

        SharedPtr(ControlBlockPointerType cb) : _cb(cb) {};

    public:
        SharedPtr() : _cb(nullptr) {}

        SharedPtr(std::nullptr_t) : _cb(nullptr) {}
        SharedPtr& operator=(std::nullptr_t) {
            reset();
            _cb = nullptr;
            return *this;
        }

        SharedPtr(const SharedPtr& other) : _cb(other._cb) { _cb->counter().incStrong(); }
        SharedPtr& operator=(const SharedPtr& other) {
            if(this == &other) return *this;

            reset();
            _cb = other._cb;
            _cb->counter().incStrong();

            return *this;
        }

        SharedPtr(SharedPtr&& other) : _cb(std::exchange(other._cb, nullptr)) {}

        SharedPtr& operator=(SharedPtr&& other) {
            if(this == &other) return *this;

            reset();
            _cb = std::exchange(other._cb, nullptr);

            return *this;
        }

        ~SharedPtr() { reset(); }

        inline PointerType data() { return _cb->begin(); }
        inline const PointerType data() const { return _cb->begin(); }

        inline PointerType begin() { return _cb->begin(); }
        inline const PointerType begin() const { return _cb->begin(); }

        inline PointerType end() { return _cb->end(); }
        inline const PointerType end() const { return _cb->end(); }

        inline const PointerType cbegin() const { return _cb->begin(); }
        inline const PointerType cend() const { return _cb->end(); }
    
        void reset() {
            if(_cb == nullptr) return;
            clearData();
            _cb = nullptr;
        }

        ValueType& operator[](SizeType i) {
            XTD_DEBUG_VERIFY(_cb != nullptr, "Dereferencing a nullptr");
            XTD_DEBUG_VERIFY(begin() + i < end(), "Index out of scope");
            return begin()[i];
        }
        const ValueType& operator[](SizeType i) const  {
            XTD_DEBUG_VERIFY(_cb != nullptr, "Dereferencing a nullptr");
            XTD_DEBUG_VERIFY(begin() + i < end(), "Index out of scope");
            return begin()[i];
        }

        static Self make(std::initializer_list<ValueType> vals) requires std::is_default_constructible_v<AllocatorType> {
            auto alloc = AllocatorType();
            return make(alloc, vals);
        }

        static Self make(AllocatorType& alloc, std::initializer_list<ValueType> vals) {
            auto* cb = ControlBlockType::create(alloc, vals.size());
            auto* it = cb->begin();
            auto* val = vals.begin();
            for(; it < cb->end(); ++it, ++val) ::new (it) ValueType(*val);
            return Self(cb);
        }

        static Self make(size_t size) requires std::is_default_constructible_v<AllocatorType> && std::is_default_constructible_v<ValueType> {
            auto alloc = AllocatorType();
            auto val = ValueType();
            return make(alloc, size, val);
        }

        static Self make(size_t size, ValueType val) requires std::is_default_constructible_v<AllocatorType> {
            auto alloc = AllocatorType();
            return make(alloc, size, val);
        }

        static Self make(AllocatorType& alloc, size_t size) requires std::is_default_constructible_v<ValueType> {
            auto val = ValueType();
            return make(alloc, size, val);
        }

        static Self make(AllocatorType& alloc, size_t size, ValueType val) {
            auto* cb = ControlBlockType::create(alloc, size);
            auto* begin = cb->begin();
            auto* end = cb->end();
            for(auto* it = begin; it < end; ++it) ::new (it) ValueType(val);
            return Self(cb);
        }

        SizeType size() const {
            XTD_DEBUG_VERIFY(_cb != nullptr, "Dereferencing a nullptr");
            return _cb->size(); 
        }

        size_t strongCount() const {
            XTD_DEBUG_VERIFY(_cb != nullptr, "Dereferencing a nullptr");
            return _cb->counter().strongCount(); 
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

        std::strong_ordering operator<=>(std::nullptr_t) const { return _cb <=> nullptr; }
        bool operator==(std::nullptr_t) const { return _cb == nullptr; }

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

    private:
        inline void clearData() { 
            auto& counter = _cb->counter();
            if(counter.decStrong() == 0) {
                _cb->destroy();
                if(counter.decWeak() == 0) _cb->dealloc();
            }
        }
    };
}


