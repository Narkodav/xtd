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

#if defined(__GNUC__) || defined(__clang__)
  #define XTD_MAY_ALIAS __attribute__((__may_alias__))
#else
  #define XTD_MAY_ALIAS   // MSVC doesn't perform type based alias analysis
#endif

#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
  #define XTD_RESTRICT __restrict
#else
  #define XTD_RESTRICT
#endif