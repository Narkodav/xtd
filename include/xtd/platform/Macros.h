#pragma once
#if defined(_WIN32)
    #define XTD_PLATFORM_WINDOWS 1
#elif defined(__APPLE__)
    #define XTD_PLATFORM_MACOS 1
#elif defined(__linux__)
    #define XTD_PLATFORM_LINUX 1
#else
    #error "xtd: unsupported platform"
#endif

#if defined(XTD_PLATFORM_MACOS) || (XTD_PLATFORM_LINUX)
    #define XTD_PLATFORM_POSIX 1
#endif

// todo: Add compiler defines