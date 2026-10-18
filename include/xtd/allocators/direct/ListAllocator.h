#pragma once
#include "xtd/debug/Checks.h"
#include <cstdint>
#include <string>
#include <stdexcept>

// allocators that expect visible coherent accessible memory (heap pointers, mapped files, mapped GPU memory, etc.)
namespace xtd::allocators::direct {

	class ListAllocatorBase
	{
	private:
		struct MemoryRegion {
			enum class Type : uint8_t {
				Free,
				Allocated
			};
			MemoryRegion* next;
			MemoryRegion* previous;
			Type type;

			inline void* getData() {
				return reinterpret_cast<uint8_t*>(this) + sizeof(MemoryRegion);
			}

			inline size_t getSize() {
				return reinterpret_cast<uint8_t*>(next) - reinterpret_cast<uint8_t*>(getData());
			}

			inline MemoryRegion* calculateNext(size_t size) {
				return reinterpret_cast<MemoryRegion*>(reinterpret_cast<uint8_t*>(getData()) + size);
			}
		};

		void* _data;
		size_t _size;
		size_t _freeSize;
	public:
		ListAllocatorBase() = default;
		~ListAllocatorBase() = default;

		ListAllocatorBase(const ListAllocatorBase&) = delete;
		ListAllocatorBase& operator=(const ListAllocatorBase&) = delete;

		ListAllocatorBase(ListAllocatorBase&&) = default;
		ListAllocatorBase& operator=(ListAllocatorBase&&) = default;

		void assign(void* data, size_t size) {
			if (size <= sizeof(MemoryRegion))
				throw std::runtime_error("Size must be more than sizeof(MemoryRegion) bytes");
			_data = data;
			_size = size;
			_freeSize = size - sizeof(MemoryRegion);
			MemoryRegion* region = reinterpret_cast<MemoryRegion*>(_data);
			region->type = MemoryRegion::Type::Free;
			region->next = reinterpret_cast<MemoryRegion*>(getEndPointer());
			region->previous = nullptr;
		}

		template<typename T>
		T* allocate(size_t count = 1) {
			if (count > _size - sizeof(MemoryRegion)) return nullptr;
			return reinterpret_cast<T*>(allocateImpl(count * sizeof(T)));
		}

		template<typename T>
		void deallocate(T* ptr) {
			XTD_DEBUG_VERIFY(ptr < _data || ptr > getEndPointer() || ptr == nullptr, "Invalid pointer");
			auto region = getFromPointer(ptr);
			XTD_DEBUG_VERIFY(region->type == MemoryRegion::Type::Free, "Invalid pointer");
			deallocateImpl(reinterpret_cast<void*>(ptr));
		}

		template<typename T>
		T* reallocate(T* ptr, size_t count = 1) {
			XTD_DEBUG_VERIFY(count > _size - sizeof(MemoryRegion), "Invalid size");
			XTD_DEBUG_VERIFY(ptr < _data || ptr > getEndPointer() || ptr == nullptr, "Invalid pointer");
			auto region = getFromPointer(ptr);
			XTD_DEBUG_VERIFY(region->type == MemoryRegion::Type::Free, "Invalid pointer");
			return reinterpret_cast<T*>(reallocateImpl(reinterpret_cast<void*>(ptr), count * sizeof(T)));
		}

		size_t getSize() {
			return _size;
		}

		double getFragmentationRatio() {
			auto region = reinterpret_cast<MemoryRegion*>(_data);
			auto end = getEndPointer();
			size_t biggestFree = 0;
			while(region < end) {
				if (region->type == MemoryRegion::Type::Free && region->getSize() > biggestFree)
					biggestFree = region->getSize();
			}
			return 1. - static_cast<double>(biggestFree) / static_cast<double>(_freeSize);
		}

	private:
		inline void* getEndPointer() {
			return reinterpret_cast<uint8_t*>(_data) + _size;
		}

		inline MemoryRegion* getFromPointer(void* ptr) {
			return reinterpret_cast<MemoryRegion*>(reinterpret_cast<uint8_t*>(ptr) - sizeof(MemoryRegion));
		}

		void* allocateImpl(size_t size) {
			MemoryRegion* region = reinterpret_cast<MemoryRegion*>(_data);
			auto end = getEndPointer();
			while (region->getSize() < size || region->type == MemoryRegion::Type::Allocated) {
				region = region->next;
				if (region == end)
					return nullptr;
			}
			region->type = MemoryRegion::Type::Allocated;
			_freeSize -= region->getSize();
			shrinkToFitRegion(region, size);
			return region->getData();
		}

		void deallocateImpl(void* ptr) {
			auto end = getEndPointer();
			auto region = getFromPointer(ptr);
			_freeSize += region->getSize();
			auto next = region->next;
			auto previous = region->previous;
			if (previous != nullptr && previous->type == MemoryRegion::Type::Free)
			{
				region = previous;
				region->next = next;
				_freeSize += sizeof(MemoryRegion);
			}
			else region->type = MemoryRegion::Type::Free;

			if (next < end)
			{
				if (next->type == MemoryRegion::Type::Free)
				{
					region->next = next->next;
					next = region->next;
					if (next < end)
						next->previous = region;
					_freeSize += sizeof(MemoryRegion);
				}
				else next->previous = region;
			}
		}

		void* reallocateImpl(void* ptr, size_t size) {
			auto region = getFromPointer(ptr);
			if(size < region->getSize())
			{
				auto newNext = region->calculateNext(size);
				if (newNext->getData() < region->next)
				{
					newNext->previous = region;
					newNext->next = region->next;
					newNext->type = MemoryRegion::Type::Free;
					region->next = newNext;
					_freeSize += newNext->getSize();
					if(newNext->next < getEndPointer() && newNext->next->type == MemoryRegion::Type::Free)
					{
						region->next = newNext->next->next;
						_freeSize += sizeof(MemoryRegion);
					}
				}
				return ptr;
			}
			else if (region->next < getEndPointer() && region->next->type == MemoryRegion::Type::Free &&
				region->next->getSize() + sizeof(MemoryRegion) + region->getSize() >= size)
			{
				region->next = region->next->next;
				_freeSize -= region->next->getSize();
				region->type = MemoryRegion::Type::Allocated;
				shrinkToFitRegion(region, size);
				return ptr;
			}
			auto newPtr = allocateImpl(size);
			if (newPtr == nullptr)
				throw std::runtime_error("Not enough memory");
			memcpy(newPtr, ptr, region->getSize());
			deallocateImpl(ptr);
			return newPtr;
		}

		inline void shrinkToFitRegion(MemoryRegion* region, size_t size) {
			auto newNext = region->calculateNext(size);
			if (newNext->getData() < region->next)
			{
				newNext->previous = region;
				newNext->next = region->next;
				newNext->type = MemoryRegion::Type::Free;
				region->next = newNext;
				_freeSize += newNext->getSize();
			}
		}
	};
}

