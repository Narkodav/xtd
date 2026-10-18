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
    class BidirectionalList {
    public:
        using ValueType = T;
        using SizeType = size_t;
        using PointerType = ValueType*;
        using AllocatorType = A;

        using NodeType = ListBidirectionalNode<T>;
        using SentinelNodeType = ListBidirectionalSentinelNode<T>;
        using IteratorType = BidirectionalListIteratorBase<ValueType, NodeType>;
        using ConstIteratorType = BidirectionalListIteratorBase<const ValueType, const NodeType>;

        using Self = BidirectionalList<ValueType, AllocatorType>;

    private:
        SentinelNodeType _sentinel;
        SizeType _size = 0;
        XTD_NO_UNIQUE_ADDRESS AllocatorType _allocator;

    public:
        BidirectionalList() : _size(0) {
            resetSentinel();
        }

        BidirectionalList(const BidirectionalList &other) : _size(0)
        {
            resetSentinel();
            NodeType* current = _sentinel.next();
            const NodeType* currentOther = other._sentinel.next();
            addRange(current, getSentinel(), currentOther, other.getSentinel());
            _size = other._size;
        }

        BidirectionalList &operator=(const BidirectionalList &other)
        {
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
                _sentinel.setPrev(current);
                clearOrphans(current->next(), getSentinel());
                current->setNext(getSentinel());                
            }
            _size = other._size;           
            return *this;
        }

        BidirectionalList(BidirectionalList &&other) { 
            _sentinel.setNext(other._sentinel.next());
            _sentinel.setPrev(other._sentinel.prev());            
            other.resetSentinel();
            _size = std::exchange(other._size, 0);
            _allocator = std::exchange(other._allocator, AllocatorType());
        };

        BidirectionalList &operator=(BidirectionalList &&other)
        {
            if (this != &other)
            {
                clear();
                _sentinel.setNext(other._sentinel.next());
                _sentinel.setPrev(other._sentinel.prev());            
                other.resetSentinel();
                _size = std::exchange(other._size, 0);
                _allocator = std::exchange(other._allocator, AllocatorType());
            }
            return *this;
        }

        ~BidirectionalList()
        {
            clear();
        }

        void clear()
        {
            clearOrphans(head(), getSentinel());   
            resetSentinel();
            _size = 0;
        }

        template <typename... Args>
        void pushBack(Args&&... args)
        {
            NodeType *newNode = constructNode(std::forward<Args>(args)...);
            NodeType* tailNode = tail();
            tailNode->setNext(newNode);
            newNode->setPrev(tailNode);
            newNode->setNext(getSentinel());
            _sentinel.setPrev(newNode);
            ++_size;
        }

        template <typename... Args>
        void pushFront(Args&&... args)
        {
            NodeType *newNode = constructNode(std::forward<Args>(args)...);
            NodeType* headNode = head();
            headNode->setPrev(newNode);
            newNode->setNext(headNode);
            newNode->setPrev(getSentinel());
            _sentinel.setNext(newNode);
            ++_size;
        }

        void popBack()
        {
            CONTAINERS_VERIFY(!empty(), "Cannot pop an empty list");
            NodeType* tailNode = tail();
            tailNode->prev()->setNext(getSentinel());
            _sentinel.setPrev(tailNode->prev());
            deleteNode(tailNode);
            --_size;
        }

        void popFront()
        {
            CONTAINERS_VERIFY(!empty(), "Cannot pop an empty list");
            NodeType* headNode = head();
            headNode->next()->setPrev(getSentinel());
            _sentinel.setNext(headNode->next());
            deleteNode(headNode);
            --_size;
        }

        IteratorType erase(ConstIteratorType cit)
        {
            if (cit == end()) return end();
            NodeType* node = asMutable(cit).node();
            NodeType* next = node->next();
            node->prev()->setNext(next);
            next->setPrev(node->prev());
            deleteNode(node);
            --_size;
            return IteratorType(next);
        }

        IteratorType erase(ConstIteratorType cbegin, ConstIteratorType cend)
        {
            if (cbegin == cend) return IteratorType(asMutable(cend));
            NodeType* begin = asMutable(cbegin).node();
            NodeType* end = asMutable(cend).node();
            begin->prev()->setNext(end);
            end->setPrev(begin->prev());
            _size -= clearOrphansCounted(begin, end);
            return IteratorType(end);
        }

        template<typename... Args>
        IteratorType insert(ConstIteratorType cit, Args... args)
        {            
            NodeType* node = asMutable(cit).node();
            NodeType* newNode = constructNode(std::forward<Args>(args)...);            
            newNode->setNext(node);
            newNode->setPrev(node->prev());
            node->prev()->setNext(newNode);
            node->setPrev(newNode);
            ++_size;
            return IteratorType(newNode);
        }
        
        IteratorType insert(ConstIteratorType cit, ConstIteratorType begin, ConstIteratorType end)
        {
            NodeType* node = asMutable(cit).node();
            NodeType* start = node->prev();
            _size += addRangeCounted(start, node, begin, end);            
            return IteratorType(start);
        }

        ValueType& front() { return head()->value(); }
        const ValueType& front() const { return head()->value(); }

        ValueType& back() { return tail()->value(); }
        const ValueType& back() const { return tail()->value(); }

        bool empty() const { return _size == 0; }
        SizeType size() const { return _size; }

        IteratorType begin() { return IteratorType(head()); }
        IteratorType end() { return IteratorType(getSentinel()); }
        ConstIteratorType begin() const { return ConstIteratorType(head()); }
        ConstIteratorType end() const { return ConstIteratorType(getSentinel()); }
        ConstIteratorType cbegin() const { return begin(); }
        ConstIteratorType cend() const { return end(); }

        inline IteratorType asMutable(ConstIteratorType it)
		{
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

        void addRange(NodeType* dst, NodeType* dstEnd, const NodeType* src, const NodeType* srcEnd)
        {
            while(src != srcEnd) {
                NodeType* newNode = constructNode(src->value());
                dst->setNext(newNode);
                newNode->setPrev(dst);
                dst = newNode;
                src = src->next();
            }
            dst->setNext(dstEnd);
            dstEnd->setPrev(dst);
        }

        SizeType addRangeCounted(NodeType* dst, NodeType* dstEnd, const NodeType* src, const NodeType* srcEnd)
        {
            SizeType count = 0;
            while(src != srcEnd) {
                NodeType* newNode = constructNode(src->value());
                dst->setNext(newNode);
                newNode->setPrev(dst);
                dst = newNode;
                src = src->next();
                ++count;
            }
            dst->setNext(dstEnd);
            dstEnd->setPrev(dst);
            return count;
        }
        
        template <typename... Args>
        NodeType* constructNode(Args&&... args)
        {
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

        void resetSentinel() {
            _sentinel.setNext(getSentinel());
            _sentinel.setPrev(getSentinel());
        }

        NodeType* tail() { return static_cast<NodeType*>(_sentinel.prev()); } //prev of sentinel is tail
        const NodeType* tail() const { return static_cast<NodeType*>(_sentinel.prev()); }

        NodeType* head() { return static_cast<NodeType*>(_sentinel.next()); } //next of sentinel is head
        const NodeType* head() const { return static_cast<NodeType*>(_sentinel.next()); }
    };

}