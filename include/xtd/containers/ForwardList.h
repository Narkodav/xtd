#pragma once
#include <type_traits>
#include <utility>
#include <concepts>
#include <stdexcept>

#include "xtd/containers/ListIterators.h"
#include "xtd/allocators/NewAllocators.h"
#include "xtd/memory/Lifetime.h"
#include "xtd/Macros.h"

namespace xtd::containers {
    template <typename T, xtd::allocators::AllocatorT A = xtd::allocators::AllocatorNew>
    class ForwardList {
    public:
        using ValueType = T;
        using SizeType = size_t;
        using PointerType = ValueType*;
        using AllocatorType = A;

        using NodeType = ListForwardNode<T>;
        using SentinelNodeType = ListForwardSentinelNode<T>;
        using IteratorType = ForwardListIteratorBase<ValueType, NodeType>;
        using ConstIteratorType = ForwardListIteratorBase<const ValueType, const NodeType>;

        using Self = ForwardList<ValueType, AllocatorType>;

    private:
        SentinelNodeType _sentinel;
        SizeType _size;
        XTD_NO_UNIQUE_ADDRESS AllocatorType _allocator;

    public:
        ForwardList() : _size(0) { resetSentinel(); }

        ForwardList(const ForwardList &other) : _size(0) {
            resetSentinel();
            NodeType* current = _sentinel.next();
            const NodeType* currentOther = other._sentinel.next();
            addRange(current, getSentinel(), currentOther, other.getSentinel());
            _size = other._size;
        }

        ForwardList &operator=(const ForwardList &other) {
            if (&other != this) return *this;
            
            if(other.size() > size())
            {
                NodeType* current = _sentinel.next();
                const NodeType* currentOther = other._sentinel.next();
                while(current != getSentinel()) {
                    xtd::memory::destroyAt(current->data());
                    xtd::memory::copyTo(current->data(), currentOther->value());
                    current = current->next();
                    currentOther = currentOther->next();
                }
                current = tail();
                addRange(current, getSentinel(), currentOther, getSentinel());
            }
            else
            {
                NodeType* current = _sentinel.next();
                const NodeType* currentOther = other._sentinel.next();
                while(currentOther != other.getSentinel()) {
                    xtd::memory::destroyAt(current->data());
                    xtd::memory::copyTo(current->data(), currentOther->value());
                    current = current->next();
                    currentOther = currentOther->next();
                }
                clearOrphans(current->next(), getSentinel());
                current->setNext(getSentinel());                
            }
            _size = other._size;           
            return *this;
        }

        ForwardList(ForwardList &&other) { 
            _sentinel.setNext(other._sentinel.next());        
            other.resetSentinel();
            _size = std::exchange(other._size, 0);
            _allocator = std::exchange(other._allocator, AllocatorType());
        };

        ForwardList &operator=(ForwardList &&other) {
            if (this != &other)
            {
                clear();
                _sentinel.setNext(other._sentinel.next());
                other.resetSentinel();
                _size = std::exchange(other._size, 0);
                _allocator = std::exchange(other._allocator, AllocatorType());
            }
            return *this;
        }

        ~ForwardList() { clear(); }

        void clear() {
            clearOrphans(head(), getSentinel());   
            resetSentinel();
            _size = 0;
        }

        template <typename... Args>
        void push(Args&&... args) {
            NodeType *newNode = constructNode(std::forward<Args>(args)...);
            NodeType* headNode = head();
            newNode->setNext(headNode);
            _sentinel.setNext(newNode);
            ++_size;
        }

        void pop() {
            XTD_DEBUG_VERIFY(!empty(), "Cannot pop an empty list");
            NodeType* headNode = head();
            _sentinel.setNext(headNode->next());
            deleteNode(headNode);
            --_size;
        }

        IteratorType erase(ConstIteratorType cit) {
            NodeType* node = asMutable(cit).node();
            NodeType* next = node->next();
            node->setNext(next->next());
            deleteNode(next);
            --_size;
            return IteratorType(node->next());
        }

        IteratorType erase(ConstIteratorType cbegin, ConstIteratorType cend) {
            if (cbegin == cend) return IteratorType(asMutable(cend));
            NodeType* begin = asMutable(cbegin).node();
            NodeType* end = asMutable(cend).node();
            NodeType* start = begin->next();
            if(start == end) return IteratorType(end);
            begin->setNext(end);
            _size -= clearOrphansCounted(start, end);
            return IteratorType(end);
        }

        template<typename... Args>
        IteratorType insert(ConstIteratorType cit, Args... args) {            
            NodeType* node = asMutable(cit).node();
            NodeType* newNode = constructNode(std::forward<Args>(args)...);
            newNode->setNext(node->next());
            node->setNext(newNode);
            ++_size;
            return IteratorType(newNode);
        }
        
        // inserting at end will insert to the beggining of the list
        IteratorType insert(ConstIteratorType cit, ConstIteratorType begin, ConstIteratorType end) {
            NodeType* node = asMutable(cit).node();
            _size += addRangeCounted(node, node->next(), begin, end);            
            return IteratorType(node);
        }

        ValueType& front() { return head()->value(); }
        const ValueType& front() const { return head()->value(); }

        bool empty() const { return _size == 0; }
        SizeType size() const { return _size; }

        IteratorType begin() { return IteratorType(head()); }
        IteratorType end() { return IteratorType(getSentinel()); }
        ConstIteratorType begin() const { return ConstIteratorType(head()); }
        ConstIteratorType end() const { return ConstIteratorType(getSentinel()); }
        ConstIteratorType cbegin() const { return begin(); }
        ConstIteratorType cend() const { return end(); }

        inline IteratorType asMutable(ConstIteratorType it) {
			return IteratorType(const_cast<NodeType *>(it.node()));
		}

    private:

        void clearOrphans(NodeType* orphan, const NodeType* end) {
            while(orphan != end) {
                NodeType* next = orphan->next();
                deleteNode(orphan);
                orphan = next;
            }
        }

        SizeType clearOrphansCounted(NodeType* orphan, const NodeType* end) {
            SizeType count = 0;
            while(orphan != end) {
                NodeType* next = orphan->next();
                deleteNode(orphan);
                orphan = next;
                ++count;
            }
            return count;
        }

        void addRange(NodeType* dst, NodeType* dstEnd, const NodeType* src, const NodeType* srcEnd) {
            while(src != srcEnd) {
                NodeType* newNode = constructNode(src->value());
                dst->setNext(newNode);
                dst = newNode;
                src = src->next();
            }
            dst->setNext(dstEnd);
        }

        SizeType addRangeCounted(NodeType*& dst, NodeType* dstEnd, const NodeType* src, const NodeType* srcEnd) {
            SizeType count = 0;
            while(src != srcEnd) {
                NodeType* newNode = constructNode(src->value());
                dst->setNext(newNode);
                dst = newNode;
                src = src->next();
                ++count;
            }
            dst->setNext(dstEnd);
            return count;
        }
        
        template <typename... Args>
        NodeType* constructNode(Args&&... args) {
            NodeType *newNode = _allocator.allocate();
            xtd::memory::constructAt(newNode->data(), std::forward<Args>(args)...);
            return newNode;
        }

        void deleteNode(NodeType *node) {
            xtd::memory::destroyAt(node->data());
            _allocator.deallocate(node);
        }

        NodeType* getSentinel() { return static_cast<NodeType*>(&_sentinel); }
        const NodeType* getSentinel() const { return static_cast<NodeType*>(&_sentinel); }

        void resetSentinel() { _sentinel.setNext(getSentinel()); }

        NodeType* head() { return static_cast<NodeType*>(_sentinel.next()); } //next of sentinel is head
        const NodeType* head() const { return static_cast<NodeType*>(_sentinel.next()); }
    };

}