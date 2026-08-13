#pragma once
#include <concepts>

// A wrapper around std concepts and traits
namespace xtd::concepts {
    template<typename T>
    concept StandardLayoutT = std::is_standard_layout_v<T>;
    
}