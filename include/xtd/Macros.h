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