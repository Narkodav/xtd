#pragma once
#include "xtd/Macros.h"

#include <immintrin.h>

#if defined(XTD_COMPILER_MSVC)
#include <intrin.h>
#endif

#if defined(__SSE__)
#define XTD_SSE
#endif
#if defined(__SSE2__) // This is mandatory for x86_64 so true in practice for essentially all modern machines and all machines xtd supports
#define XTD_SSE2
#endif
#if defined(__SSE3__)
#define XTD_SSE3
#endif
#if defined(__AVX__)
#define XTD_AVX
#endif
#if defined(__AVX2__)
#define XTD_AVX2
#endif

// Feature macro fallbacks for MSVC (it doesn't define __SSE__/__SSE2__/__SSE3__)
#if defined(XTD_COMPILER_MSVC) && !defined(XTD_COMPILER_CLANG)
    #if !defined(XTD_SSE) && (defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1))
        #define XTD_SSE
    #endif
    #if !defined(XTD_SSE2) && (defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2))
        #define XTD_SSE2
    #endif
    #if !defined(XTD_SSE3) && defined(__AVX__)
        #define XTD_SSE3
    #endif
#endif

// FMA3 is a separate CPUID flag from AVX2, but every AVX2 CPU shipped so far has it.
#if defined(__FMA__) || (defined(XTD_COMPILER_MSVC) && !defined(XTD_COMPILER_CLANG) && defined(__AVX2__))
    #define XTD_FMA
#endif

namespace xtd::simd {
#if defined(XTD_COMPILER_CLANG) || defined(XTD_COMPILER_GCC)

    // GCC/Clang's builtin checks both the CPUID bit and OS support (XGETBV) for AVX.
    inline bool supportsSSE()  { __builtin_cpu_init(); return __builtin_cpu_supports("sse"); }
    inline bool supportsSSE2() { __builtin_cpu_init(); return __builtin_cpu_supports("sse2"); }
    inline bool supportsSSE3() { __builtin_cpu_init(); return __builtin_cpu_supports("sse3"); }
    inline bool supportsAVX()  { __builtin_cpu_init(); return __builtin_cpu_supports("avx"); }
    inline bool supportsAVX2() { __builtin_cpu_init(); return __builtin_cpu_supports("avx2"); }

#elif defined(XTD_COMPILER_MSVC)
    namespace detail {
        struct CpuFeatures {
            bool sse = false;
            bool sse2 = false;
            bool sse3 = false;
            bool avx = false;
            bool avx2 = false;
        };

        inline CpuFeatures detect() {
            CpuFeatures f;

            int regs[4]; // eax, ebx, ecx, edx
            __cpuid(regs, 0);
            const int maxLeaf = regs[0];
            if (maxLeaf < 1) return f;

            __cpuid(regs, 1);
            const unsigned ecx = static_cast<unsigned>(regs[2]);
            const unsigned edx = static_cast<unsigned>(regs[3]);

            f.sse  = (edx & (1u << 25)) != 0;
            f.sse2 = (edx & (1u << 26)) != 0;
            f.sse3 = (ecx & (1u << 0))  != 0;

            // AVX needs: CPU support, OSXSAVE enabled, and the OS saving XMM+YMM state.
            const bool osxsave  = (ecx & (1u << 27)) != 0;
            const bool avxCpu   = (ecx & (1u << 28)) != 0;
            bool osSavesYmm = false;
            if (osxsave && avxCpu) {
                const unsigned long long xcr0 = _xgetbv(0);
                osSavesYmm = (xcr0 & 0x6) == 0x6; // bit 1 = SSE state, bit 2 = AVX state
            }
            f.avx = avxCpu && osSavesYmm;

            if (f.avx && maxLeaf >= 7) {
                int ext[4];
                __cpuidex(ext, 7, 0);
                f.avx2 = (static_cast<unsigned>(ext[1]) & (1u << 5)) != 0;
            }

            return f;
        }

        inline const CpuFeatures& features() {
            static const CpuFeatures f = detect(); // thread-safe singleton
            return f;
        }
    } // namespace detail

    inline bool supportsSSE()  { return detail::features().sse; }
    inline bool supportsSSE2() { return detail::features().sse2; }
    inline bool supportsSSE3() { return detail::features().sse3; }
    inline bool supportsAVX()  { return detail::features().avx; }
    inline bool supportsAVX2() { return detail::features().avx2; }

#else

    // Unknown compiler / non-x86: report no support.
    inline bool supportsSSE()  { return false; }
    inline bool supportsSSE2() { return false; }
    inline bool supportsSSE3() { return false; }
    inline bool supportsAVX()  { return false; }
    inline bool supportsAVX2() { return false; }

#endif
}