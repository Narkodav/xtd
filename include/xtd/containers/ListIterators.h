#pragma once
#include <type_traits>
#include <utility>
#include <concepts>
#include <stdexcept>

#include "xtd/containers/Traits.h"

namespace xtd::containers {
    template<typename T>
    class ListForwardNode;

    template<typename T, typename... Bases>
    class ListForwardSentinelNode : public Bases...
    {
    protected:
        ListForwardNode<T> *_next = nullptr;
    public:
        ListForwardSentinelNode() = default;
        ~ListForwardSentinelNode() = default;
        ListForwardSentinelNode(const ListForwardSentinelNode&) = delete;
        ListForwardSentinelNode& operator=(const ListForwardSentinelNode&) = delete;
        ListForwardSentinelNode(ListForwardSentinelNode&&) = delete;
        ListForwardSentinelNode& operator=(ListForwardSentinelNode&&) = delete;

        ListForwardNode<T>* next() noexcept { return _next; }
        const ListForwardNode<T>* next() const noexcept { return _next; }
        void setNext(ListForwardNode<T>* next) noexcept { _next = next; }
    };
    
    template<typename T>
    class ListBidirectionalNode;

    template<typename T, typename... Bases>
    class ListBidirectionalSentinelNode : public Bases... {
    protected:
        ListBidirectionalNode<T>* _next = nullptr;
        ListBidirectionalNode<T>* _prev = nullptr;
    public:
        ListBidirectionalSentinelNode() = default;
        ~ListBidirectionalSentinelNode() = default;
        ListBidirectionalSentinelNode(const ListBidirectionalSentinelNode&) = delete;
        ListBidirectionalSentinelNode& operator=(const ListBidirectionalSentinelNode&) = delete;
        ListBidirectionalSentinelNode(ListBidirectionalSentinelNode&&) = delete;
        ListBidirectionalSentinelNode& operator=(ListBidirectionalSentinelNode&&) = delete;

        ListBidirectionalNode<T>* prev() noexcept { return _prev; }
        const ListBidirectionalNode<T>* prev() const noexcept { return _prev; }
        void setPrev(ListBidirectionalNode<T>* prev) noexcept { _prev = prev; }

        ListBidirectionalNode<T>* next() noexcept { return _next; }
        const ListBidirectionalNode<T>* next() const noexcept { return _next; }
        void setNext(ListBidirectionalNode<T>* next) noexcept { _next = next; }
    };

    template<typename T, typename... Bases>
    class ListNodeDataStorageBase : public Bases... {
    protected:
        T* _data;
    public:
        ListNodeDataStorageBase() = default;
        ~ListNodeDataStorageBase() = default;
        ListNodeDataStorageBase(const ListNodeDataStorageBase&) = delete;
        ListNodeDataStorageBase& operator=(const ListNodeDataStorageBase&) = delete;
        ListNodeDataStorageBase(ListNodeDataStorageBase&&) = delete;
        ListNodeDataStorageBase& operator=(ListNodeDataStorageBase&&) = delete;

        T* data() noexcept { return _data; }
        const T* data() const noexcept { return _data; }
        T& value() noexcept { return *_data; }
        const T& value() const noexcept { return *_data; }
    };

    template<typename T>
    class ListForwardNode : public ListForwardSentinelNode<T, ListNodeDataStorageBase<T>> {
    public:
        using ListForwardSentinelNode<T>::ListForwardSentinelNode;
        using ListNodeDataStorageBase<T>::ListNodeDataStorageBase;
    };

    template<typename T>
    class ListBidirectionalNode : public ListBidirectionalSentinelNode<T, ListNodeDataStorageBase<T>>{
    public:
        using ListBidirectionalSentinelNode<T>::ListBidirectionalSentinelNode;
        using ListNodeDataStorageBase<T>::ListNodeDataStorageBase;
    };

    template <typename T, typename Node>
    class ForwardListIteratorBase {
    protected:
        Node *_node = nullptr;

    public:
        using ValueType = T;
        using PointerType = ValueType*;
        using ReferenceType = ValueType&;

        ForwardListIteratorBase() = default;
        ForwardListIteratorBase(Node *node) : _node(node) {};

        template <typename U>
        ForwardListIteratorBase(U *node) requires std::convertible_to<const U *, Node *> : _node(node) {};

        ForwardListIteratorBase(const ForwardListIteratorBase&) = default;
        ForwardListIteratorBase& operator=(const ForwardListIteratorBase&) = default;
        ForwardListIteratorBase(ForwardListIteratorBase&&) = default;
        ForwardListIteratorBase& operator=(ForwardListIteratorBase&&) = default;

		template <typename U, typename N>
        requires std::convertible_to<const U*, T*> && std::convertible_to<const N*, Node*>
		constexpr ForwardListIteratorBase(ForwardListIteratorBase<U, N> it) noexcept : _node(it.node()){};

		template <typename U, typename N>
        requires std::convertible_to<const U*, T*> && std::convertible_to<const N*, Node*>
		constexpr ForwardListIteratorBase operator=(ForwardListIteratorBase<U, N> it) noexcept { _node = it.node(); };

        ReferenceType operator*() { return _node->value(); };
        PointerType operator->()  { return _node->data(); };
        const ValueType& operator*() const { return _node->value(); };
        const PointerType operator->() const { return _node->data(); };

        // Increment operator (in-order traversal)
        ForwardListIteratorBase &operator++() {
            Node *next = _node->next();
            XTD_DEBUG_VERIFY(next != nullptr, "Iterator cannot be incremented past the end");
            _node = next;
            return *this;
        }
        ForwardListIteratorBase operator++(int) {
            ForwardListIteratorBase tmp = *this;
            Node *next = _node->next();
            XTD_DEBUG_VERIFY(next != nullptr, "Iterator cannot be incremented past the end");
            _node = next;
            return tmp;
        }
        bool operator==(const ForwardListIteratorBase &other) const { return _node == other._node; };
        bool operator!=(const ForwardListIteratorBase &other) const { return _node != other._node; };

        Node* node() { return _node; }
        const Node* node() const { return _node; }
    };

    template <typename T, typename Node>
    class BidirectionalListIteratorBase {
    protected:
        Node *_node = nullptr;

    public:
        using ValueType = T;
        using PointerType = ValueType*;
        using ReferenceType = ValueType&;

        BidirectionalListIteratorBase() = default;
        BidirectionalListIteratorBase(Node *node) : _node(node) {};

        template <typename U>
        BidirectionalListIteratorBase(U *node) requires std::convertible_to<const U *, Node *>
        : _node(node) {};

        BidirectionalListIteratorBase(const BidirectionalListIteratorBase&) = default;
        BidirectionalListIteratorBase& operator=(const BidirectionalListIteratorBase&) = default;
        BidirectionalListIteratorBase(BidirectionalListIteratorBase&&) = default;
        BidirectionalListIteratorBase& operator=(BidirectionalListIteratorBase&&) = default;

		template <typename U, typename N>
        requires std::convertible_to<const U *, T *> && std::convertible_to<const N *, Node *>
		constexpr BidirectionalListIteratorBase(BidirectionalListIteratorBase<U, N> it) noexcept : _node(it.node()){};

		template <typename U, typename N>
        requires std::convertible_to<const U *, T *> && std::convertible_to<const N *, Node *>
		constexpr BidirectionalListIteratorBase operator=(BidirectionalListIteratorBase<U, N> it) noexcept { _node = it.node(); };

        ReferenceType operator*() { return _node->value(); };
        PointerType operator->()  { return _node->data(); };
        const ValueType& operator*() const { return _node->value(); };
        const PointerType operator->() const { return _node->data(); };

        BidirectionalListIteratorBase &operator++() {
            Node *next = _node->next();
            XTD_DEBUG_VERIFY(next != nullptr, "Iterator cannot be incremented past the end");
            _node = next;
            return *this;
        }
        BidirectionalListIteratorBase operator++(int) {
            BidirectionalListIteratorBase tmp = *this;
            Node *next = _node->next();
            XTD_DEBUG_VERIFY(next != nullptr, "Iterator cannot be incremented past the end");
            _node = next;
            return tmp;
        }
        BidirectionalListIteratorBase &operator--() {
            Node *prev = _node->prev();
            XTD_DEBUG_VERIFY(prev != nullptr, "Iterator cannot be incremented past the end");
            _node = prev;
            return *this;
        }
        BidirectionalListIteratorBase operator--(int) {
            BidirectionalListIteratorBase tmp = *this;
            Node *prev = _node->prev();
            XTD_DEBUG_VERIFY(prev != nullptr, "Iterator cannot be incremented past the end");
            _node = prev;
            return tmp;
        }

        bool operator==(const BidirectionalListIteratorBase &other) const { return _node == other._node; };
        bool operator!=(const BidirectionalListIteratorBase &other) const { return _node != other._node; };

        Node* node() { return _node; }
        const Node* node() const { return _node; }

        BidirectionalListIteratorBase next() { return BidirectionalListIteratorBase(_node->next()); }
        BidirectionalListIteratorBase prev() { return BidirectionalListIteratorBase(_node->prev()); }
    };
}