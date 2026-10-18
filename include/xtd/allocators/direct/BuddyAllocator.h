#pragma once
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <algorithm>

// allocators that expect visible coherent accessible memory (heap pointers, mapped files, mapped GPU memory, etc.)
namespace xtd::allocators::direct {

    class BuddyAllocatorBase {
    public:
        struct FreeNode {
            FreeNode* next;
            FreeNode* prev;
        };

    private:
        static constexpr size_t s_minAlign = sizeof(FreeNode);

        uintptr_t _data = 0;
        size_t    _size = 0;
        size_t    _minBlock = 0;
        size_t    _maxLevel = 0;

        FreeNode** _freeLists = nullptr;
        uint8_t* _allocLevel = nullptr;   // per min block: 0=free, otherwise level+1
        size_t   _metadataSize = 0;

    public:
        BuddyAllocatorBase() = default;
        ~BuddyAllocatorBase() = default;

        BuddyAllocatorBase(const BuddyAllocatorBase&) = delete;
        BuddyAllocatorBase& operator=(const BuddyAllocatorBase&) = delete;

        BuddyAllocatorBase(BuddyAllocatorBase&&) = default;
        BuddyAllocatorBase& operator=(BuddyAllocatorBase&&) = default;

        void assign(void* metadata, void* memory, size_t heapSize, size_t minBlock = s_minAlign) {
            if (!memory || heapSize < 4096)
                throw std::runtime_error("Invalid pool");

            minBlock = std::max(minBlock, s_minAlign);
            minBlock = alignPow2(minBlock);

            size_t minBlocks = heapSize / minBlock;
            auto levels = log2Floor(minBlocks);

            size_t freeListBytes = levels * sizeof(FreeNode*);
            size_t allocBytes = minBlocks * sizeof(uint8_t);
            size_t meta = align8(freeListBytes + allocBytes);

            _data = reinterpret_cast<uintptr_t>(memory);
            _size = heapSize;
            _minBlock = minBlock;
            _maxLevel = levels - 1;
            _metadataSize = meta;

            _freeLists = reinterpret_cast<FreeNode**>(metadata);
            _allocLevel = reinterpret_cast<uint8_t*>(metadata) + freeListBytes;

            memset(metadata, 0, _metadataSize);

            auto* root = (FreeNode*)_data;
            root->next = root->prev = nullptr;
            _freeLists[0] = root;
            return;
        }

        static size_t computeMetadataSize(size_t totalSize, size_t minBlockSize = s_minAlign) {
            size_t minBlocks = totalSize / minBlockSize;
            int levels = (int)log2Floor(minBlocks) + 1;

            size_t freeListBytes = levels * sizeof(FreeNode*);
            size_t allocBytes = minBlocks * sizeof(uint8_t);
            size_t meta = align8(freeListBytes + allocBytes);
            return meta;
        }

        size_t getUsableSize() const { return _size; }

        template<typename T = void>
        T* allocate(size_t count = 1) {
            return reinterpret_cast<T*>(allocateImpl(sizeof(T) * count));
        }

        template<typename T>
        void deallocate(T* ptr) {
            XTD_DEBUG_VERIFY(ptr < _data || ptr > getEndPointer() || ptr == nullptr, "Invalid pointer");
            deallocateImpl(ptr);
        }

    private:
        static size_t alignPow2(size_t x) {
            x--; x |= x >> 1; x |= x >> 2; x |= x >> 4; x |= x >> 8; x |= x >> 16; x |= x >> 32; return x + 1;
        }
        static size_t align8(size_t x) { return (x + 7) & ~7ULL; }
        static size_t log2Floor(size_t x) { size_t r = 0; while (x >>= 1) r++; return r; }

        size_t levelFor(size_t block) const {
            return log2Floor(_size / block);
        }

        size_t minIndex(void* p) const {
            return ((uintptr_t)p - _data) / _minBlock;
        }

        void* getBuddy(void* p, size_t level) const {
            return (void*)((uintptr_t)p ^ (_size >> level));
        }

        void add(size_t lvl, FreeNode* n) {
            n->prev = nullptr;
            n->next = _freeLists[lvl];
            if (n->next) n->next->prev = n;
            _freeLists[lvl] = n;
        }

        void remove(size_t lvl, FreeNode* n) {
            if (n->prev) n->prev->next = n->next;
            else _freeLists[lvl] = n->next;
            if (n->next) n->next->prev = n->prev;
        }

        bool inFreeList(size_t lvl, void* p) {
            FreeNode* n = _freeLists[lvl];
            while (n) {
                if ((void*)n == p) return true;
                n = n->next;
            }
            return false;
        }

        void* allocateImpl(size_t size) {
            if (!size || size > _size) return nullptr;

            size = std::max(size, _minBlock);
            size = alignPow2(size);

            int target = levelFor(size);
            if (target < 0) return nullptr;

            for (size_t lvl = target; lvl != std::numeric_limits<size_t>::max(); --lvl) {
                if (!_freeLists[lvl]) continue;

                FreeNode* block = _freeLists[lvl];
                remove(lvl, block);

                while (lvl < target) {
                    lvl++;
                    size_t half = _size >> lvl;
                    void* buddy = (void*)((uintptr_t)block + half);
                    add(lvl, (FreeNode*)buddy);
                }

                _allocLevel[minIndex(block)] = uint8_t(target + 1);
                return block;
            }
            return nullptr;
        }

        void deallocateImpl(void* ptr) {
            if (!ptr) return;

            size_t idx = minIndex(ptr);
            uint8_t lvlp = _allocLevel[idx];
            if (!lvlp) throw std::runtime_error("Double free");

            int level = lvlp - 1;
            _allocLevel[idx] = 0;

            void* block = ptr;

            while (level > 0) {
                void* buddy = getBuddy(block, level);
                if (!inFreeList(level, buddy)) break;

                remove(level, (FreeNode*)buddy);
                block = std::min(block, buddy);
                level--;
            }
            add(level, (FreeNode*)block);
        }
    };

}
