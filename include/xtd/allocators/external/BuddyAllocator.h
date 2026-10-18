#pragma once
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <algorithm>

#include "xtd/math/Utilities.h"
#include "xtd/containers/Vector.h"

// allocators that can accept any type castable to uintptr_t as a pool pointer
namespace xtd::allocators::external {

    namespace Detail {
        template<std::integral T>
        struct BroadcastPattern {};

        template<>
        struct BroadcastPattern<uint64_t> {
            static inline const uint64_t s_pattern = 0x5555555555555555ull;
        };

        template<>
        struct BroadcastPattern<uint32_t> {
            static inline const uint32_t s_pattern = 0x55555555u;
        };

        // UB at x = 0
        inline size_t alignNextPow2(size_t x) {
            x--;
            x |= x >> 1; x |= x >> 2; x |= x >> 4; x |= x >> 8; x |= x >> 16;
            if constexpr (sizeof(size_t) == 8) {
                x |= x >> 32;
            }
            return x + 1;
        }
        inline size_t log2Floor(size_t x) { size_t r = 0; while (x >>= 1) r++; return r; }
    }

    class BuddyBlockAllocator {
    public:

        template<std::integral Limb>
        class Tree {
        public:
            enum class NodeState : uint8_t {
                Free = 0,
                Used = 1,
                Split = 2,
                Unused = 3,
            };

        private:
            xtd::containers::Vector<Limb> _tree;

        public:
            Tree() = default;
            ~Tree() = default;

            Tree(const Tree&) = default;
            Tree& operator=(const Tree&) = default;

            Tree(Tree&&) = default;
            Tree& operator=(Tree&&) = default;

            void init(size_t levelCount) {
                size_t bitCount = getNodeLevelOffset(levelCount + 1) * 2; // two bits per block
                size_t byteCount = (bitCount - 1) / 8 + 1;
                size_t limbCount = (byteCount - 1) / sizeof(Limb) + 1;
                _tree.resize(limbCount, std::numeric_limits<Limb>::max());
                _tree[0] = (_tree[0] >> 2) << 2;
            }

            NodeState get(size_t lvl, size_t index) const {
                uint8_t state = 0;
                auto [bitIndex, limbIndex] = getInternalIndex(lvl, index);
                state = (_tree[limbIndex] >> bitIndex) & 0b11;
                return static_cast<NodeState>(state);
            }

            void set(size_t lvl, size_t index, NodeState state) {
                auto [bitIndex, limbIndex] = getInternalIndex(lvl, index);
                auto slot = static_cast<Limb>(0b11) << bitIndex;
                auto mask = static_cast<Limb>(state) << bitIndex;
                _tree[limbIndex] = (_tree[limbIndex] & ~slot) | mask;
            }

            size_t findFreeAtLevel(size_t lvl) const {
                auto [startBitIndex, startLimbIndex] = getInternalIndex(lvl, 0);
                auto [endBitIndex, limbEnd] = getInternalIndex(lvl + 1, 0);
                size_t startNodeIndex = startBitIndex >> 1;
                size_t endNodeIndex = endBitIndex >> 1;
                size_t limbIndex = startLimbIndex;
                for(;limbIndex < limbEnd; ++limbIndex) {
                    auto index = findPattern(_tree[limbIndex] >> startBitIndex, static_cast<uint8_t>(NodeState::Free));
                    if(index < sizeof(Limb) * 4 - startNodeIndex) return limbIndex * sizeof(Limb) * 4 + index + startNodeIndex - getNodeLevelOffset(lvl);
                    startNodeIndex = 0;
                    startBitIndex = 0;
                }
                auto index = findPattern(_tree[limbIndex] >> startBitIndex, static_cast<uint8_t>(NodeState::Free));
                if(index < endNodeIndex - startNodeIndex) return limbIndex * sizeof(Limb) * 4 + index + startNodeIndex - getNodeLevelOffset(lvl);
                return std::numeric_limits<size_t>::max();
            }

            static size_t getCountAtLevel(size_t lvl) {
                return 1 << lvl;
            }

        private:
            std::pair<size_t, size_t> getInternalIndex(size_t lvl, size_t index) const {
                size_t nodeIndex = getNodeLevelOffset(lvl) + index;
                size_t globalBitIndex = nodeIndex * 2;
                size_t bitIndex = globalBitIndex % (sizeof(Limb) * 8);
                size_t limbIndex = globalBitIndex / (sizeof(Limb) * 8);
                return { bitIndex, limbIndex };
            }

            static size_t findPattern(Limb limb, uint8_t pattern) {
                Limb broadcast = Detail::BroadcastPattern<Limb>::s_pattern * pattern;

                Limb y = limb ^ broadcast;

                Limb lo = y & Detail::BroadcastPattern<Limb>::s_pattern;
                Limb hi = (y >> 1) & Detail::BroadcastPattern<Limb>::s_pattern;

                Limb matches = ~(lo | hi) & Detail::BroadcastPattern<Limb>::s_pattern;

                if (!matches) return std::numeric_limits<size_t>::max();

                return Detail::countTrailingZeroes(matches) >> 1;
            }

            static size_t getNodeLevelOffset(size_t level) {
                return (size_t{1} << level) - 1;
            }
        };

    private:
        size_t _blockCount;
        size_t _levelCount;
        Containers::Vector<uint64_t> _freeNodeCounts = {};
        Tree<uint64_t> _blockStates;

    public:

        BuddyBlockAllocator() = default;
        ~BuddyBlockAllocator() = default;

        BuddyBlockAllocator(const BuddyBlockAllocator&) = default;
        BuddyBlockAllocator& operator=(const BuddyBlockAllocator&) = default;

        BuddyBlockAllocator(BuddyBlockAllocator&&) = default;
        BuddyBlockAllocator& operator=(BuddyBlockAllocator&&) = default;

        BuddyBlockAllocator(size_t blockCount) {
            assign(blockCount);
        }

        void assign(size_t blockCount) {
            if (!blockCount) throw std::runtime_error("Cannot assign 0 blocks");
            _blockCount = Detail::alignNextPow2(blockCount);
            _levelCount = Detail::log2Floor(_blockCount) + 1;

            // size_t freeListsBitCount = 0;
            // for(size_t i = 0; i < _levelCount; ++i) freeListsBitCount += 1 << i;

           //_freeLists.resize((freeListsBitCount - 1) / (sizeof(uint64_t) * 8) + 1, 0);
            _freeNodeCounts.resize(_levelCount, 0);
            _blockStates.init(_levelCount);
            _freeNodeCounts[0] = 1;
        }

        size_t allocate(size_t blockCount) {
            if (!blockCount || blockCount > _blockCount) return std::numeric_limits<size_t>::max();

            blockCount = Detail::alignNextPow2(blockCount);

            size_t targetLevel = _levelCount - Detail::log2Floor(blockCount) - 1;
            size_t lvl = targetLevel;
            for(;;--lvl) {
                if (_freeNodeCounts[lvl] != 0) break;
                if (lvl == 0) return std::numeric_limits<size_t>::max();
            }

            size_t i = _blockStates.findFreeAtLevel(lvl);
            if(i == std::numeric_limits<size_t>::max()) return std::numeric_limits<size_t>::max();

            size_t block = i * (_blockCount >> lvl);
            --_freeNodeCounts[lvl];

            if(lvl == targetLevel) {
                _blockStates.set(lvl, i, Tree<uint64_t>::NodeState::Used);
            }
            else {
                _blockStates.set(lvl, i, Tree<uint64_t>::NodeState::Split);
                for(++lvl, i <<= 1; lvl < targetLevel; ++lvl, i <<= 1) {
                    ++_freeNodeCounts[lvl];
                    _blockStates.set(lvl, i, Tree<uint64_t>::NodeState::Split);
                    _blockStates.set(lvl, i ^ 1, Tree<uint64_t>::NodeState::Free);
                }
                ++_freeNodeCounts[lvl];
                _blockStates.set(lvl, i, Tree<uint64_t>::NodeState::Used);
                _blockStates.set(lvl, i ^ 1, Tree<uint64_t>::NodeState::Free);
            }
            return block;
        }

        void deallocate(size_t block) {
            if (block == std::numeric_limits<size_t>::max()) throw std::runtime_error("Deallocating an invalid block");

            size_t lvl = minLevelFromBlock(block);
            size_t i;
            while(true) {                
                i = block / (_blockCount >> lvl);
                if(_blockStates.get(lvl, i) == Tree<uint64_t>::NodeState::Used) break;
                ++lvl;
                if(lvl >= _levelCount) throw std::runtime_error("Deallocating an invalid block");
            }

            while (lvl > 0) {
                size_t buddy = i ^ 1;
                if (_blockStates.get(lvl, buddy) != Tree<uint64_t>::NodeState::Free)
                    break;
                _blockStates.set(lvl, i, Tree<uint64_t>::NodeState::Unused);
                _blockStates.set(lvl, buddy, Tree<uint64_t>::NodeState::Unused);
                --_freeNodeCounts[lvl];
                --lvl;
                i >>= 1;
            }

            ++_freeNodeCounts[lvl];
            _blockStates.set(lvl, i, Tree<uint64_t>::NodeState::Free);
        }

        void debugPrint() const {
            std::cout << "BuddyBlockAllocator state\n";
            std::cout << "Total blocks: " << _blockCount
                    << ", Levels: " << _levelCount << "\n\n";

            for (size_t lvl = 0; lvl < _levelCount; ++lvl) {
                size_t nodeCount = _blockStates.getCountAtLevel(lvl);
                size_t blockSize = _blockCount >> lvl;

                // Indentation to visualize tree structure
                size_t indent = (1ull << (_levelCount - lvl - 1)) - 1;
                std::cout << "L" << lvl
                        << " (blockSize=" << std::setw(4) << blockSize << ") "
                        << "[free=" << _freeNodeCounts[lvl] << "] : ";
                for (size_t s = 0; s < indent; ++s) std::cout << "  ";

                for (size_t i = 0; i < nodeCount; ++i) {
                    char c = '?';
                    switch (_blockStates.get(lvl, i)) {
                        case Tree<uint64_t>::NodeState::Free:   c = 'F'; break;
                        case Tree<uint64_t>::NodeState::Used:   c = 'U'; break;
                        case Tree<uint64_t>::NodeState::Split:  c = 'S'; break;
                        case Tree<uint64_t>::NodeState::Unused: c = 'N'; break;
                    }

                    std::cout << c;

                    // spacing between siblings for readability
                    if (i + 1 < nodeCount) {
                        size_t spacing = (1ull << (_levelCount - lvl)) - 1;
                        for (size_t s = 0; s < spacing; ++s) std::cout << ' ';
                    }
                }

                std::cout << "\n";
            }

            std::cout << std::endl;
        }

        bool isFree() const {
            return _freeNodeCounts.empty() || (_freeNodeCounts[0] == 1 && _blockStates.get(0, 0) == Tree<uint64_t>::NodeState::Free);
        }

    private:

        inline size_t minLevelFromBlock(size_t block)
        {
            if (block == 0)
                return 0;
            return (_levelCount - 1) - Detail::countTrailingZeroes(block);
        }
    };

    class BuddyAllocatorBase {
    private:
        BuddyBlockAllocator _impl;
        size_t _minBlockSize;
        uintptr_t _heap;
        size_t _size;
        static inline size_t s_defaultMinBlockSize = 64;

    public:
        BuddyAllocatorBase() = default;
        ~BuddyAllocatorBase() noexcept = default;

        BuddyAllocatorBase(const BuddyAllocatorBase&) = delete;
        BuddyAllocatorBase& operator=(const BuddyAllocatorBase&) = delete;

        BuddyAllocatorBase(BuddyAllocatorBase&&) = default;
        BuddyAllocatorBase& operator=(BuddyAllocatorBase&&) = default;

        void assign(uintptr_t heap, size_t size, size_t minBlockSize = s_defaultMinBlockSize) {
            if(heap % minBlockSize != 0) throw std::runtime_error("Heap must be minBlockSize aligned");
            //if(Detail::alignNextPow2(minBlockSize) != minBlockSize) throw std::runtime_error("Minimal block size must be power of 2");
            size_t blockCount = size / minBlockSize;
            if(Detail::alignNextPow2(blockCount) != blockCount) throw std::runtime_error("Block count must be power of 2");
            _impl.assign(blockCount);            
            _minBlockSize = minBlockSize;
            _heap = heap;
            _size = size;
        }

        static size_t alignHeapSize(size_t size, size_t minBlockSize = s_defaultMinBlockSize) {
            if(size == 0) return minBlockSize;
            size_t blockCount = (size - 1) / minBlockSize + 1;
            blockCount = Detail::alignNextPow2(blockCount);
            return blockCount * minBlockSize;
        }

        uintptr_t allocate(size_t size) {
            if(size == 0) return 0;
            size_t blockCount = (size - 1) / _minBlockSize + 1;
            size_t allocation = _impl.allocate(blockCount);
            if(allocation == std::numeric_limits<size_t>::max()) return std::numeric_limits<uintptr_t>::max();
            return _heap + allocation * _minBlockSize;
        }

        void deallocate(uintptr_t ptr) {
            auto offset = ptr - _heap;
            if(offset % _minBlockSize != 0) throw std::runtime_error("Unaligned pointer");
            if(offset >= _size) throw std::runtime_error("Invalid pointer");
            auto index = offset / _minBlockSize;
            _impl.deallocate(index);
        }

        void debugPrint() const { _impl.debugPrint(); }
    };
}
