#pragma once
#include "xtd/containers/Traits.h"

namespace xtd::containers {

    template<typename T>
    class ContiguousIteratorBase {
    public:
        using ContiguousIteratorTag = void;
        using ValueType = T;
        using SizeType = size_t;
        using PointerType = ValueType*;
        using Self = ContiguousIteratorBase<ValueType>;

    protected:
        T* _data;
    public:
        constexpr ContiguousIteratorBase() = default;
        constexpr ~ContiguousIteratorBase() noexcept = default;
        constexpr ContiguousIteratorBase(const ContiguousIteratorBase&) noexcept = default;
        constexpr ContiguousIteratorBase(ContiguousIteratorBase&&) noexcept = default;
        constexpr ContiguousIteratorBase& operator=(const ContiguousIteratorBase&) noexcept = default;
        constexpr ContiguousIteratorBase& operator=(ContiguousIteratorBase&&) noexcept = default;

        constexpr ContiguousIteratorBase(T* data) : _data(data) {};
        operator T*() { return _data; }
        operator const T*() const { return _data; }

        PointerType data() { return _data; }
        const PointerType data() const { return _data; }

        ValueType& operator*() { return *_data; }
        const ValueType& operator*() const { return *_data; }

        PointerType operator->() { return _data; }
        const PointerType operator->() const { return _data; }

        ValueType& operator[](SizeType i) { return _data[i]; }
        const ValueType& operator[](SizeType i) const { return _data[i]; }

        ContiguousIteratorBase& operator++() { ++_data; return *this; }
        ContiguousIteratorBase operator++(int) {
            ContiguousIteratorBase copy = _data;
            ++_data; 
            return copy; 
        }

        ContiguousIteratorBase& operator--() { --_data; return *this; }
        ContiguousIteratorBase operator--(int) {
            ContiguousIteratorBase copy = _data;
            --_data; 
            return copy; 
        }

        ContiguousIteratorBase operator+(SizeType val) const { return _data + val; }
        ContiguousIteratorBase operator-(SizeType val) const { return _data - val; }

        ContiguousIteratorBase operator+(const ContiguousIteratorBase& other) const { return _data + other.data(); }
        ContiguousIteratorBase operator-(const ContiguousIteratorBase& other) const { return _data - other.data(); }

        ContiguousIteratorBase& operator+=(SizeType val) const { _data += val; return *this; }
        ContiguousIteratorBase& operator-=(SizeType val) const { _data -= val; return *this; }

        bool operator==(const ContiguousIteratorBase& other) { return _data == other.data(); }
        bool operator!=(const ContiguousIteratorBase& other) { return _data != other.data(); }

        bool operator<(const ContiguousIteratorBase& other) { return _data < other.data(); }
        bool operator>(const ContiguousIteratorBase& other) { return _data > other.data(); }

        bool operator<=(const ContiguousIteratorBase& other) { return _data <= other.data(); }
        bool operator>=(const ContiguousIteratorBase& other) { return _data >= other.data(); }
    };

    static_assert(ContiguousIteratorType<ContiguousIteratorBase<int>>, "ContiguousIteratorBase must satisfy ContiguousContainerType");
}