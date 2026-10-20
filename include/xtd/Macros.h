#pragma once
#if defined(_WIN32)
  #if defined(XTD_BUILD_SHARED)
    #define XTD_API __declspec(dllexport)
  #elif defined(XTD_SHARED)
    #define XTD_API __declspec(dllimport)
  #else
    #define XTD_API
  #endif
#else
  #define XTD_API
#endif

#if defined(__GNUC__) 
  #define XTD_COMPILER_GCC
#endif
#if defined(__clang__)
  #define XTD_COMPILER_CLANG
#endif
#if defined(_MSC_VER)
  #define XTD_COMPILER_MSVC
#endif

#if !defined(XTD_COMPILER_GCC) && !defined(XTD_COMPILER_CLANG) && !defined(XTD_COMPILER_MSVC)
  #error "Unsupported compiler, some modules may not compile or produce UB"
#endif

#if defined(XTD_COMPILER_GCC) || defined(XTD_COMPILER_CLANG)
  #define XTD_MAY_ALIAS __attribute__((__may_alias__))
#else
  #define XTD_MAY_ALIAS   // MSVC doesn't perform type based alias analysis
#endif

#if defined(XTD_COMPILER_MSVC) || defined(XTD_COMPILER_GCC) || defined(XTD_COMPILER_CLANG)
  #define XTD_RESTRICT __restrict
#else
  #define XTD_RESTRICT
#endif

#if defined(_MSC_VER)
  #define XTD_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
  #define XTD_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

  #if defined(XTD_COMPILER_MSVC) && !defined(XTD_COMPILER_CLANG)
      #define XTD_FORCE_INLINE __forceinline
  #else
      #define XTD_FORCE_INLINE inline __attribute__((always_inline))
  #endif
