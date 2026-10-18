#include "xtd/containers/hashtables/FlatIteratorBase.h"
#include "xtd/containers/hashtables/RobinHoodTable.h"
#include "xtd/containers/hashtables/SwissTable.h"

namespace xtd::containers {
    template<typename HashTable>
    class UnorderedSetImpl {
    public:
        using ValueType = typename HashTable::ValueType;
        using SizeType = typename HashTable::SizeType;
        using PointerType = typename HashTable::PointerType;
		using BucketType = typename HashTable::BucketType;
        
        using HasherType = typename HashTable::HasherType;
		using ComparatorType = typename HashTable::ComparatorType;
		using AllocatorType = typename HashTable::AllocatorType;
		using GrowthType = typename HashTable::GrowthType;

        using IteratorType = FlatIteratorBase<BucketType>;
        using ConstIteratorType = FlatIteratorBase<const BucketType>;

        using Self = UnorderedSetImpl<HashTable>;
    private:
        HashTable _impl;

    public:
		template<typename U>
        IteratorType find(const U& key) { return IteratorType(_impl.find(key)); }

		template<typename U>
        IteratorType find(SizeType hash, const U& key) { return IteratorType(_impl.find(hash, key)); }

		template<typename U>
        ConstIteratorType find(const U& key) const { return ConstIteratorType(_impl.find(key)); }

		template<typename U>
        ConstIteratorType find(SizeType hash, const U& key) const { return ConstIteratorType(_impl.find(hash, key)); }

        template<typename... Args>
        IteratorType insert(Args&&... args) { return IteratorType(_impl.insert(std::forward<Args>(args)...)); }

        template<typename... Args>
        IteratorType insert(SizeType hash, Args&&... args) { return IteratorType(_impl.insert(hash, std::forward<Args>(args)...)); }

        IteratorType erase(IteratorType& it) { return IteratorType(_impl.erase(it.bucket())); }

        template<typename U>
        IteratorType erase(const U& key) { return IteratorType(_impl.erase(key)); }

        template<typename U>
        IteratorType erase(SizeType hash, const U& key) { return IteratorType(_impl.erase(hash, key)); }

		void clear() { _impl.clear(); }

		constexpr void rehash(size_t newCapacity) { _impl.rehash(newCapacity); }

        constexpr bool empty() const { return _impl.empty(); }
        constexpr SizeType size() const { return _impl.size(); }
		constexpr SizeType capacity() const { return _impl.capacity(); }

        constexpr IteratorType begin() { return IteratorType(_impl.begin()); }
        constexpr IteratorType end() { return IteratorType(_impl.end()); }
        constexpr ConstIteratorType begin() const { return ConstIteratorType(_impl.begin()); }
        constexpr ConstIteratorType end() const { return ConstIteratorType(_impl.end()); }
        constexpr ConstIteratorType cbegin() const { return ConstIteratorType(_impl.begin()); }
        constexpr ConstIteratorType cend() const { return ConstIteratorType(_impl.end()); }

		constexpr float loadFactor() const { return _impl.loadFactor(); }
    };

    template<typename T, typename Policy>
    using UnorderedSet = UnorderedSetImpl<RobinHoodTable<T, Policy>>;
    template<typename T, typename Policy>
    using FlatUnorderedSet = UnorderedSetImpl<SwissTable<T, Policy>>;
}