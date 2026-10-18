#pragma once
#include <cstdint>
#include <string>
#include <stdexcept>

#include "xtd/containers/BidirectionalList.h"

#include "Containers/Maps/UnorderedMap.h"

// allocators that can accept any type castable to uintptr_t as a pool pointer
namespace xtd::allocators::external {

	class ListAllocatorNaiveBase {
	private:
		struct MemoryRegion {
			enum class Type : uint8_t {
				Free,
				Allocated
			};
			Type type;
			uintptr_t start;
			size_t size;
		};

		uintptr_t _data;
		size_t _size;
		size_t _freeSize;

		xtd::containers::BidirectionalList<MemoryRegion> _regions;
		
	public:
		ListAllocatorNaiveBase() = default;
		~ListAllocatorNaiveBase() = default;

		ListAllocatorNaiveBase(const ListAllocatorNaiveBase&) = delete;
		ListAllocatorNaiveBase& operator=(const ListAllocatorNaiveBase&) = delete;

		ListAllocatorNaiveBase(ListAllocatorNaiveBase&&) = default;
		ListAllocatorNaiveBase& operator=(ListAllocatorNaiveBase&&) = default;

		void assign(uintptr_t data, size_t size) {
			if (size <= sizeof(MemoryRegion))
				throw std::runtime_error("Size must be more than sizeof(MemoryRegion) bytes");
			_data = data;
			_size = size;
			_freeSize = _size;
			_regions.pushBack(MemoryRegion{MemoryRegion::Type::Free, _data, _size});
		}

        uintptr_t allocate(size_t size) {
            return allocateImpl(size);
        }

        void deallocate(uintptr_t ptr) {
            deallocateImpl(ptr);
        }

	private:

		uintptr_t allocateImpl(size_t size) {
			auto it = _regions.begin();
			while(true) {				
				if(it->type == MemoryRegion::Type::Free && it->size >= size) break;
				++it;
				if(it == _regions.end()) return std::numeric_limits<uintptr_t>::max();
			}
			
			if(it->size == size) {
				it->type = MemoryRegion::Type::Allocated;
				return it->start;
			}

			auto allocated = _regions.insert(it);

			allocated->size = size;
			allocated->start = it->start;
			allocated->type = MemoryRegion::Type::Allocated;

			it->size -= size;
			it->start += size;

			return allocated->start;
		}

		void deallocateImpl(uintptr_t ptr) {
			auto it = _regions.begin();
			while(true) {				
				if(it->start == ptr) break;
				++it;
				if(it == _regions.end()) return;
			}
			auto prev = it.prev();
			auto next = it.next();

			if(prev != _regions.end() && prev->type == MemoryRegion::Type::Free) {
				it->start = prev->start;
				it->size += prev->size;
				_regions.erase(prev);
			}
			if(next != _regions.end() && next->type == MemoryRegion::Type::Free) {
				it->size += next->size;
				_regions.erase(next);
			}
			it->type = MemoryRegion::Type::Free;
		}
	};

	class ListAllocatorBase {
	private:
		struct MemoryRegion {
			enum class Type : uint8_t {
				Free,
				Allocated
			};
			Type type;
			uintptr_t start;
			size_t size;

			using Iterator = Containers::BidirectionalList<typename Containers::BidirectionalList<MemoryRegion>::Iterator>::Iterator;

			Iterator storageInterator;
		};

		uintptr_t _data;
		size_t _size;
		size_t _freeSize;

		Containers::BidirectionalList<MemoryRegion> _regions;
		Containers::BidirectionalList<typename Containers::BidirectionalList<MemoryRegion>::Iterator> _freeRegions;
		Containers::BidirectionalList<typename Containers::BidirectionalList<MemoryRegion>::Iterator> _allocatedRegions;
		Containers::UnorderedMap<uintptr_t, typename Containers::BidirectionalList<MemoryRegion>::Iterator> _pointerToRegionMap;
		
	public:
		ListAllocatorBase() = default;
		~ListAllocatorBase() = default;

		ListAllocatorBase(const ListAllocatorBase&) = delete;
		ListAllocatorBase& operator=(const ListAllocatorBase&) = delete;

		ListAllocatorBase(ListAllocatorBase&&) = default;
		ListAllocatorBase& operator=(ListAllocatorBase&&) = default;

		void assign(uintptr_t data, size_t size) {
			if (size <= sizeof(MemoryRegion))
				throw std::runtime_error("Size must be more than sizeof(MemoryRegion) bytes");
			_data = data;
			_size = size;
			_freeSize = _size;
			_regions.pushBack(MemoryRegion::Type::Free, _data, _size, _freeRegions.end());
			_freeRegions.pushBack(_regions.begin());
			_regions.back().storageInterator = _freeRegions.begin();
		}

        uintptr_t allocate(size_t size) {
            return allocateImpl(size);
        }

        void deallocate(uintptr_t ptr) {
            deallocateImpl(ptr);
        }

	private:

		uintptr_t allocateImpl(size_t size) {
			for(auto it = _freeRegions.begin(); it != _freeRegions.end(); ++it) {
				if((*it)->size > size) {
					auto current = *it;
					size_t offset = current->size - size;
					auto allocation = _regions.insert(current);
					allocation->type = MemoryRegion::Type::Allocated;
					allocation->size = size;
					allocation->start = current->start;
					allocation->storageInterator = _allocatedRegions.insert(_allocatedRegions.end(), allocation);
					_pointerToRegionMap.insert(allocation->start, allocation);

					current->size = offset;
					current->start += offset;
					_freeSize -= size;
					return allocation->start;
				}
				else if((*it)->size == size) {
					auto current = *it;
					_freeRegions.erase(it);
					current->storageInterator = _allocatedRegions.insert(_allocatedRegions.end(), current);
					current->type = MemoryRegion::Type::Allocated;
					_pointerToRegionMap.insert(current->start, current);
					_freeSize -= size;
					return current->start;
				}
			}
			return std::numeric_limits<uintptr_t>::max();
		}

		void deallocateImpl(uintptr_t ptr) {
			auto mapIt = _pointerToRegionMap.find(ptr);
			if(mapIt == _pointerToRegionMap.end()) throw std::runtime_error("Trying to deallocate invalid pointer");
			auto allocation = mapIt->second;
			_pointerToRegionMap.erase(mapIt);
			
			auto next = allocation.next();
			auto prev = allocation.prev();
			_allocatedRegions.erase(allocation->storageInterator);
			_freeSize += allocation->size;
			
			if(next != _regions.end() && next->type == MemoryRegion::Type::Free) {
				if(prev != _regions.end() && prev->type == MemoryRegion::Type::Free) {
					prev->size += allocation->size + next->size;
					_freeRegions.erase(next->storageInterator);
					_regions.erase(next);					
					_regions.erase(allocation);
				}
				else {
					next->start -= allocation->size;
					next->size += allocation->size;
					_regions.erase(allocation);
				}
			}
			else if(prev != _regions.end() && prev->type == MemoryRegion::Type::Free) {
				prev->size += allocation->size;
				_regions.erase(allocation);
			}
			else {
				allocation->storageInterator = _freeRegions.insert(_freeRegions.end(), allocation);
				allocation->type = MemoryRegion::Type::Free;
			}			
		}
	};

}

