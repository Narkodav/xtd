#pragma once
#include "xtd/Macros.h"
#include "xtd/simd/Support.h"

#include <immintrin.h>
#if defined(XTD_COMPILER_MSVC)
#include <intrin.h>
#endif

#include <cstdint>

// Defines `T op T` and `T& op= T` for a binary intrinsic.
#define XTD_DETAIL_SIMD_OP(T, OP, INTRIN)                                                  \
    XTD_FORCE_INLINE T operator OP(T a, T b) { return T(INTRIN(a.v, b.v)); }        \
    XTD_FORCE_INLINE T& operator OP##=(T & a, T b) { a = T(INTRIN(a.v, b.v)); return a; }

namespace xtd::simd {
    // ===========================================================================
    // SSE: F32x4
    // ===========================================================================
    #if defined(XTD_SSE)
        struct F32x4 {
            using Scalar = float;
            static constexpr int lanes = 4;
            __m128 v;
    
            XTD_FORCE_INLINE F32x4() = default;
            XTD_FORCE_INLINE explicit F32x4(__m128 x) : v(x) {}
            XTD_FORCE_INLINE explicit F32x4(float s) : v(_mm_set1_ps(s)) {}
            XTD_FORCE_INLINE F32x4(float a, float b, float c, float d) : v(_mm_setr_ps(a, b, c, d)) {} // a = lane 0
    
            XTD_FORCE_INLINE static F32x4 zero() { return F32x4(_mm_setzero_ps()); }
            XTD_FORCE_INLINE static F32x4 load(const float* p) { return F32x4(_mm_load_ps(p)); }   // 16-byte aligned
            XTD_FORCE_INLINE static F32x4 loadu(const float* p) { return F32x4(_mm_loadu_ps(p)); }
            XTD_FORCE_INLINE void store(float* p) const { _mm_store_ps(p, v); }                      // 16-byte aligned
            XTD_FORCE_INLINE void storeu(float* p) const { _mm_storeu_ps(p, v); }
            XTD_FORCE_INLINE float get(int i) const { alignas(16) float t[4]; _mm_store_ps(t, v); return t[i]; }
            XTD_FORCE_INLINE float getFirst() const { float result; _mm_store_ss(&result, v);return result; }
        };
    
        XTD_DETAIL_SIMD_OP(F32x4, +, _mm_add_ps)
        XTD_DETAIL_SIMD_OP(F32x4, -, _mm_sub_ps)
        XTD_DETAIL_SIMD_OP(F32x4, *, _mm_mul_ps)
        XTD_DETAIL_SIMD_OP(F32x4, /, _mm_div_ps)
        XTD_DETAIL_SIMD_OP(F32x4, &, _mm_and_ps)
        XTD_DETAIL_SIMD_OP(F32x4, |, _mm_or_ps)
        XTD_DETAIL_SIMD_OP(F32x4, ^, _mm_xor_ps)
    
        XTD_FORCE_INLINE F32x4 operator-(F32x4 a) { return F32x4(_mm_xor_ps(a.v, _mm_set1_ps(-0.0f))); }
        XTD_FORCE_INLINE F32x4 andnot(F32x4 a, F32x4 b) { return F32x4(_mm_andnot_ps(a.v, b.v)); } // ~a & b
    
        XTD_FORCE_INLINE F32x4 min(F32x4 a, F32x4 b) { return F32x4(_mm_min_ps(a.v, b.v)); }
        XTD_FORCE_INLINE F32x4 max(F32x4 a, F32x4 b) { return F32x4(_mm_max_ps(a.v, b.v)); }
        XTD_FORCE_INLINE F32x4 sqrt(F32x4 a) { return F32x4(_mm_sqrt_ps(a.v)); }
        XTD_FORCE_INLINE F32x4 rsqrt(F32x4 a) { return F32x4(_mm_rsqrt_ps(a.v)); } // ~12-bit approximation
        XTD_FORCE_INLINE F32x4 rcp(F32x4 a) { return F32x4(_mm_rcp_ps(a.v)); }     // ~12-bit approximation
        XTD_FORCE_INLINE F32x4 abs(F32x4 a) { return F32x4(_mm_andnot_ps(_mm_set1_ps(-0.0f), a.v)); }
    
        // a * b + c (fused if FMA is enabled at compile time)
        XTD_FORCE_INLINE F32x4 fma(F32x4 a, F32x4 b, F32x4 c) {
        #if defined(XTD_FMA)
            return F32x4(_mm_fmadd_ps(a.v, b.v, c.v));
        #else
            return a * b + c;
        #endif
        }
    
        // Comparisons produce masks (all-ones / all-zeros per lane)
        XTD_FORCE_INLINE F32x4 cmpeq(F32x4 a, F32x4 b) { return F32x4(_mm_cmpeq_ps(a.v, b.v)); }
        XTD_FORCE_INLINE F32x4 cmpne(F32x4 a, F32x4 b) { return F32x4(_mm_cmpneq_ps(a.v, b.v)); }
        XTD_FORCE_INLINE F32x4 cmplt(F32x4 a, F32x4 b) { return F32x4(_mm_cmplt_ps(a.v, b.v)); }
        XTD_FORCE_INLINE F32x4 cmple(F32x4 a, F32x4 b) { return F32x4(_mm_cmple_ps(a.v, b.v)); }
        XTD_FORCE_INLINE F32x4 cmpgt(F32x4 a, F32x4 b) { return F32x4(_mm_cmpgt_ps(a.v, b.v)); }
        XTD_FORCE_INLINE F32x4 cmpge(F32x4 a, F32x4 b) { return F32x4(_mm_cmpge_ps(a.v, b.v)); }
    
        // mask ? a : b
        XTD_FORCE_INLINE F32x4 select(F32x4 m, F32x4 a, F32x4 b) {
            return F32x4(_mm_or_ps(_mm_and_ps(m.v, a.v), _mm_andnot_ps(m.v, b.v)));
        }
        XTD_FORCE_INLINE int movemask(F32x4 m) { return _mm_movemask_ps(m.v); } // 1 bit per lane
    
        XTD_FORCE_INLINE float hsum(F32x4 a) {
            __m128 hi = _mm_movehl_ps(a.v, a.v);
            __m128 s = _mm_add_ps(a.v, hi);
            s = _mm_add_ss(s, _mm_shuffle_ps(s, s, 1));
            return _mm_cvtss_f32(s);
        }
    
        template <int Lane>
        XTD_FORCE_INLINE F32x4 broadcast(F32x4 a) {
            return F32x4(_mm_shuffle_ps(a.v, a.v, _MM_SHUFFLE(Lane, Lane, Lane, Lane)));
        }
    
    #endif // XTD_SSE
    
    // ===========================================================================
    // SSE2: F64x2, I32x4, conversions
    // ===========================================================================
    #if defined(XTD_SSE2)
    
        struct F64x2 {
            using Scalar = double;
            static constexpr int lanes = 2;
            __m128d v;
    
            F64x2() = default;
            explicit F64x2(__m128d x) : v(x) {}
            explicit F64x2(double s) : v(_mm_set1_pd(s)) {}
            F64x2(double a, double b) : v(_mm_setr_pd(a, b)) {}
    
            static F64x2 zero() { return F64x2(_mm_setzero_pd()); }
            static F64x2 load(const double* p) { return F64x2(_mm_load_pd(p)); }
            static F64x2 loadu(const double* p) { return F64x2(_mm_loadu_pd(p)); }
            void store(double* p) const { _mm_store_pd(p, v); }
            void storeu(double* p) const { _mm_storeu_pd(p, v); }
            double get(int i) const { alignas(16) double t[2]; _mm_store_pd(t, v); return t[i]; }
        };
    
        XTD_DETAIL_SIMD_OP(F64x2, +, _mm_add_pd)
        XTD_DETAIL_SIMD_OP(F64x2, -, _mm_sub_pd)
        XTD_DETAIL_SIMD_OP(F64x2, *, _mm_mul_pd)
        XTD_DETAIL_SIMD_OP(F64x2, /, _mm_div_pd)
        XTD_DETAIL_SIMD_OP(F64x2, &, _mm_and_pd)
        XTD_DETAIL_SIMD_OP(F64x2, |, _mm_or_pd)
        XTD_DETAIL_SIMD_OP(F64x2, ^, _mm_xor_pd)
    
        XTD_FORCE_INLINE F64x2 operator-(F64x2 a) { return F64x2(_mm_xor_pd(a.v, _mm_set1_pd(-0.0))); }
        XTD_FORCE_INLINE F64x2 andnot(F64x2 a, F64x2 b) { return F64x2(_mm_andnot_pd(a.v, b.v)); }
        XTD_FORCE_INLINE F64x2 min(F64x2 a, F64x2 b) { return F64x2(_mm_min_pd(a.v, b.v)); }
        XTD_FORCE_INLINE F64x2 max(F64x2 a, F64x2 b) { return F64x2(_mm_max_pd(a.v, b.v)); }
        XTD_FORCE_INLINE F64x2 sqrt(F64x2 a) { return F64x2(_mm_sqrt_pd(a.v)); }
        XTD_FORCE_INLINE F64x2 abs(F64x2 a) { return F64x2(_mm_andnot_pd(_mm_set1_pd(-0.0), a.v)); }
    
        XTD_FORCE_INLINE F64x2 fma(F64x2 a, F64x2 b, F64x2 c) {
        #if defined(XTD_FMA)
            return F64x2(_mm_fmadd_pd(a.v, b.v, c.v));
        #else
            return a * b + c;
        #endif
        }
    
        XTD_FORCE_INLINE F64x2 cmpeq(F64x2 a, F64x2 b) { return F64x2(_mm_cmpeq_pd(a.v, b.v)); }
        XTD_FORCE_INLINE F64x2 cmpne(F64x2 a, F64x2 b) { return F64x2(_mm_cmpneq_pd(a.v, b.v)); }
        XTD_FORCE_INLINE F64x2 cmplt(F64x2 a, F64x2 b) { return F64x2(_mm_cmplt_pd(a.v, b.v)); }
        XTD_FORCE_INLINE F64x2 cmple(F64x2 a, F64x2 b) { return F64x2(_mm_cmple_pd(a.v, b.v)); }
        XTD_FORCE_INLINE F64x2 cmpgt(F64x2 a, F64x2 b) { return F64x2(_mm_cmpgt_pd(a.v, b.v)); }
        XTD_FORCE_INLINE F64x2 cmpge(F64x2 a, F64x2 b) { return F64x2(_mm_cmpge_pd(a.v, b.v)); }
    
        XTD_FORCE_INLINE F64x2 select(F64x2 m, F64x2 a, F64x2 b) {
            return F64x2(_mm_or_pd(_mm_and_pd(m.v, a.v), _mm_andnot_pd(m.v, b.v)));
        }
        XTD_FORCE_INLINE int movemask(F64x2 m) { return _mm_movemask_pd(m.v); }
        XTD_FORCE_INLINE double hsum(F64x2 a) { return _mm_cvtsd_f64(_mm_add_sd(a.v, _mm_unpackhi_pd(a.v, a.v))); }
    
        // -----------------------------------------------------------------------
    
        struct I32x4 {
            using Scalar = int32_t;
            static constexpr int lanes = 4;
            __m128i v;
    
            constexpr I32x4() = default;
            constexpr explicit I32x4(__m128i x) : v(x) {}
            constexpr explicit I32x4(int32_t s) : v(_mm_set1_epi32(s)) {}
            constexpr I32x4(int32_t a, int32_t b, int32_t c, int32_t d) : v(_mm_setr_epi32(a, b, c, d)) {}

            constexpr static I32x4 zero() { return I32x4(_mm_setzero_si128()); }
            static I32x4 load(const int* p) { return I32x4(_mm_load_si128(reinterpret_cast<const __m128i*>(p))); }
            static I32x4 loadu(const int* p) { return I32x4(_mm_loadu_si128(reinterpret_cast<const __m128i*>(p))); }
            void store(int* p) const { _mm_store_si128(reinterpret_cast<__m128i*>(p), v); }
            void storeu(int* p) const { _mm_storeu_si128(reinterpret_cast<__m128i*>(p), v); }
            constexpr int get(int i) const { alignas(16) int t[4]; store(t); return t[i]; }
        };
    
        namespace detail {
            // _mm_mullo_epi32 is SSE4.1; this emulates it with SSE2 only.
            XTD_FORCE_INLINE __m128i mullo_epi32(__m128i a, __m128i b) {
            #if defined(__SSE4_1__)
                return _mm_mullo_epi32(a, b);
            #else
                __m128i even = _mm_mul_epu32(a, b);
                __m128i odd = _mm_mul_epu32(_mm_srli_si128(a, 4), _mm_srli_si128(b, 4));
                return _mm_unpacklo_epi32(_mm_shuffle_epi32(even, _MM_SHUFFLE(0, 0, 2, 0)),
                                        _mm_shuffle_epi32(odd, _MM_SHUFFLE(0, 0, 2, 0)));
            #endif
            }
        } // namespace detail
    
        XTD_DETAIL_SIMD_OP(I32x4, +, _mm_add_epi32)
        XTD_DETAIL_SIMD_OP(I32x4, -, _mm_sub_epi32)
        XTD_DETAIL_SIMD_OP(I32x4, *, detail::mullo_epi32)
        XTD_DETAIL_SIMD_OP(I32x4, &, _mm_and_si128)
        XTD_DETAIL_SIMD_OP(I32x4, |, _mm_or_si128)
        XTD_DETAIL_SIMD_OP(I32x4, ^, _mm_xor_si128)
    
        XTD_FORCE_INLINE I32x4 operator-(I32x4 a) { return I32x4(_mm_sub_epi32(_mm_setzero_si128(), a.v)); }
        XTD_FORCE_INLINE I32x4 operator~(I32x4 a) { return I32x4(_mm_xor_si128(a.v, _mm_set1_epi32(-1))); }
        XTD_FORCE_INLINE I32x4 andnot(I32x4 a, I32x4 b) { return I32x4(_mm_andnot_si128(a.v, b.v)); }
    
        XTD_FORCE_INLINE I32x4 shl(I32x4 a, int n) { return I32x4(_mm_slli_epi32(a.v, n)); } // logical
        XTD_FORCE_INLINE I32x4 shr(I32x4 a, int n) { return I32x4(_mm_srli_epi32(a.v, n)); } // logical
        XTD_FORCE_INLINE I32x4 sar(I32x4 a, int n) { return I32x4(_mm_srai_epi32(a.v, n)); } // arithmetic
    
        XTD_FORCE_INLINE I32x4 cmpeq(I32x4 a, I32x4 b) { return I32x4(_mm_cmpeq_epi32(a.v, b.v)); }
        XTD_FORCE_INLINE I32x4 cmpgt(I32x4 a, I32x4 b) { return I32x4(_mm_cmpgt_epi32(a.v, b.v)); }
        XTD_FORCE_INLINE I32x4 cmplt(I32x4 a, I32x4 b) { return I32x4(_mm_cmplt_epi32(a.v, b.v)); }
    
        XTD_FORCE_INLINE I32x4 select(I32x4 m, I32x4 a, I32x4 b) {
            return I32x4(_mm_or_si128(_mm_and_si128(m.v, a.v), _mm_andnot_si128(m.v, b.v)));
        }
        XTD_FORCE_INLINE I32x4 min(I32x4 a, I32x4 b) { return select(cmpgt(a, b), b, a); }
        XTD_FORCE_INLINE I32x4 max(I32x4 a, I32x4 b) { return select(cmpgt(a, b), a, b); }
        XTD_FORCE_INLINE I32x4 abs(I32x4 a) {
            __m128i sign = _mm_srai_epi32(a.v, 31);
            return I32x4(_mm_sub_epi32(_mm_xor_si128(a.v, sign), sign));
        }
        XTD_FORCE_INLINE int movemask(I32x4 m) { return _mm_movemask_ps(_mm_castsi128_ps(m.v)); }
        XTD_FORCE_INLINE int hsum(I32x4 a) {
            __m128i s = _mm_add_epi32(a.v, _mm_shuffle_epi32(a.v, _MM_SHUFFLE(1, 0, 3, 2)));
            s = _mm_add_epi32(s, _mm_shuffle_epi32(s, _MM_SHUFFLE(2, 3, 0, 1)));
            return _mm_cvtsi128_si32(s);
        }

        // Conversions (F32x4 <-> I32x4)
        XTD_FORCE_INLINE F32x4 toFloat(I32x4 a) { return F32x4(_mm_cvtepi32_ps(a.v)); }
        XTD_FORCE_INLINE I32x4 toIntRound(F32x4 a) { return I32x4(_mm_cvtps_epi32(a.v)); }   // current rounding mode (nearest-even)
        XTD_FORCE_INLINE I32x4 toIntTrunc(F32x4 a) { return I32x4(_mm_cvttps_epi32(a.v)); }  // toward zero
        // Reinterpret bits without conversion
        XTD_FORCE_INLINE F32x4 bitcastToFloat(I32x4 a) { return F32x4(_mm_castsi128_ps(a.v)); }
        XTD_FORCE_INLINE I32x4 bitcastToInt(F32x4 a) { return I32x4(_mm_castps_si128(a.v)); }
    
        // -----------------------------------------------------------------------
    
        struct I8x16 {
            using Scalar = int8_t;
            static constexpr int lanes = 16;
            __m128i v;
    
            constexpr I8x16() = default;
            constexpr explicit I8x16(__m128i x) : v(x) {}
            constexpr explicit I8x16(int8_t s) : v(_mm_set1_epi8(s)) {}

            constexpr I8x16& operator=(int8_t s) { v = _mm_set1_epi8(s); return *this; }

            constexpr I8x16(int8_t a, int8_t b, int8_t c, int8_t d, int8_t e, int8_t f, int8_t g, int8_t h,
                int8_t i, int8_t j, int8_t k, int8_t l, int8_t m, int8_t n, int8_t o, int8_t p) 
                : v(_mm_setr_epi8(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p)) {}

            constexpr static I8x16 zero() { return I8x16(_mm_setzero_si128()); }
            static I8x16 load(const int8_t* p) { return I8x16(_mm_load_si128(reinterpret_cast<const __m128i*>(p))); }
            static I8x16 loadu(const int8_t* p) { return I8x16(_mm_loadu_si128(reinterpret_cast<const __m128i*>(p))); }
            void store(int8_t* p) const { _mm_store_si128(reinterpret_cast<__m128i*>(p), v); }
            void storeu(int8_t* p) const { _mm_storeu_si128(reinterpret_cast<__m128i*>(p), v); }
            constexpr int get(int8_t i) const { alignas(16) int8_t t[16]; store(t); return t[i]; }
        };
    
        XTD_DETAIL_SIMD_OP(I8x16, +, _mm_add_epi8)
        XTD_DETAIL_SIMD_OP(I8x16, -, _mm_sub_epi8)
        XTD_DETAIL_SIMD_OP(I8x16, &, _mm_and_si128)
        XTD_DETAIL_SIMD_OP(I8x16, |, _mm_or_si128)
        XTD_DETAIL_SIMD_OP(I8x16, ^, _mm_xor_si128)
    
        XTD_FORCE_INLINE I8x16 operator-(I8x16 a) { return I8x16(_mm_sub_epi8(_mm_setzero_si128(), a.v)); }
        XTD_FORCE_INLINE I8x16 operator~(I8x16 a) { return I8x16(_mm_xor_si128(a.v, _mm_set1_epi8(-1))); }
        XTD_FORCE_INLINE I8x16 andnot(I8x16 a, I8x16 b) { return I8x16(_mm_andnot_si128(a.v, b.v)); }
    
        XTD_FORCE_INLINE I8x16 cmpeq(I8x16 a, I8x16 b) { return I8x16(_mm_cmpeq_epi8(a.v, b.v)); }
        XTD_FORCE_INLINE I8x16 cmpgt(I8x16 a, I8x16 b) { return I8x16(_mm_cmpgt_epi8(a.v, b.v)); }
        XTD_FORCE_INLINE I8x16 cmplt(I8x16 a, I8x16 b) { return I8x16(_mm_cmplt_epi8(a.v, b.v)); }

        XTD_FORCE_INLINE uint32_t movemask(I8x16 v) { return _mm_movemask_epi8(v.v); }
    
        XTD_FORCE_INLINE I8x16 select(I8x16 m, I8x16 a, I8x16 b) {
            return I8x16(_mm_or_si128(_mm_and_si128(m.v, a.v), _mm_andnot_si128(m.v, b.v)));
        }
        XTD_FORCE_INLINE I8x16 min(I8x16 a, I8x16 b) { return select(cmpgt(a, b), b, a); }
        XTD_FORCE_INLINE I8x16 max(I8x16 a, I8x16 b) { return select(cmpgt(a, b), a, b); }
    
    #endif // XTD_SSE2
    
    // ===========================================================================
    // SSE3: horizontal ops and addsub
    // ===========================================================================
    #if defined(XTD_SSE3) && defined(XTD_SSE2)
        // hadd(a, b) = { a0+a1, a2+a3, b0+b1, b2+b3 }
        XTD_FORCE_INLINE F32x4 hadd(F32x4 a, F32x4 b) { return F32x4(_mm_hadd_ps(a.v, b.v)); }
        XTD_FORCE_INLINE F32x4 hsub(F32x4 a, F32x4 b) { return F32x4(_mm_hsub_ps(a.v, b.v)); }
        XTD_FORCE_INLINE F64x2 hadd(F64x2 a, F64x2 b) { return F64x2(_mm_hadd_pd(a.v, b.v)); }
        XTD_FORCE_INLINE F64x2 hsub(F64x2 a, F64x2 b) { return F64x2(_mm_hsub_pd(a.v, b.v)); }
        // addsub(a, b) = { a0-b0, a1+b1, a2-b2, a3+b3 } (handy for complex multiplication)
        XTD_FORCE_INLINE F32x4 addsub(F32x4 a, F32x4 b) { return F32x4(_mm_addsub_ps(a.v, b.v)); }
        XTD_FORCE_INLINE F64x2 addsub(F64x2 a, F64x2 b) { return F64x2(_mm_addsub_pd(a.v, b.v)); }
    #endif // XTD_SSE3
    
    // ===========================================================================
    // AVX: F32x8, F64x4
    // ===========================================================================
    #if defined(XTD_AVX)
        struct F32x8 {
            using Scalar = float;
            static constexpr int lanes = 8;
            __m256 v;
    
            F32x8() = default;
            explicit F32x8(__m256 x) : v(x) {}
            explicit F32x8(float s) : v(_mm256_set1_ps(s)) {}
            F32x8(float a, float b, float c, float d, float e, float f, float g, float h)
                : v(_mm256_setr_ps(a, b, c, d, e, f, g, h)) {}
            F32x8(F32x4 lo, F32x4 hi) : v(_mm256_insertf128_ps(_mm256_castps128_ps256(lo.v), hi.v, 1)) {}
    
            static F32x8 zero() { return F32x8(_mm256_setzero_ps()); }
            static F32x8 load(const float* p) { return F32x8(_mm256_load_ps(p)); }   // 32-byte aligned
            static F32x8 loadu(const float* p) { return F32x8(_mm256_loadu_ps(p)); }
            void store(float* p) const { _mm256_store_ps(p, v); }                      // 32-byte aligned
            void storeu(float* p) const { _mm256_storeu_ps(p, v); }
            float get(int i) const { alignas(32) float t[8]; _mm256_store_ps(t, v); return t[i]; }
            F32x4 low() const { return F32x4(_mm256_castps256_ps128(v)); }
            F32x4 high() const { return F32x4(_mm256_extractf128_ps(v, 1)); }
        };
    
        XTD_DETAIL_SIMD_OP(F32x8, +, _mm256_add_ps)
        XTD_DETAIL_SIMD_OP(F32x8, -, _mm256_sub_ps)
        XTD_DETAIL_SIMD_OP(F32x8, *, _mm256_mul_ps)
        XTD_DETAIL_SIMD_OP(F32x8, /, _mm256_div_ps)
        XTD_DETAIL_SIMD_OP(F32x8, &, _mm256_and_ps)
        XTD_DETAIL_SIMD_OP(F32x8, |, _mm256_or_ps)
        XTD_DETAIL_SIMD_OP(F32x8, ^, _mm256_xor_ps)
    
        XTD_FORCE_INLINE F32x8 operator-(F32x8 a) { return F32x8(_mm256_xor_ps(a.v, _mm256_set1_ps(-0.0f))); }
        XTD_FORCE_INLINE F32x8 andnot(F32x8 a, F32x8 b) { return F32x8(_mm256_andnot_ps(a.v, b.v)); }
        XTD_FORCE_INLINE F32x8 min(F32x8 a, F32x8 b) { return F32x8(_mm256_min_ps(a.v, b.v)); }
        XTD_FORCE_INLINE F32x8 max(F32x8 a, F32x8 b) { return F32x8(_mm256_max_ps(a.v, b.v)); }
        XTD_FORCE_INLINE F32x8 sqrt(F32x8 a) { return F32x8(_mm256_sqrt_ps(a.v)); }
        XTD_FORCE_INLINE F32x8 rsqrt(F32x8 a) { return F32x8(_mm256_rsqrt_ps(a.v)); }
        XTD_FORCE_INLINE F32x8 rcp(F32x8 a) { return F32x8(_mm256_rcp_ps(a.v)); }
        XTD_FORCE_INLINE F32x8 abs(F32x8 a) { return F32x8(_mm256_andnot_ps(_mm256_set1_ps(-0.0f), a.v)); }
        XTD_FORCE_INLINE F32x8 floor(F32x8 a) { return F32x8(_mm256_floor_ps(a.v)); }
        XTD_FORCE_INLINE F32x8 ceil(F32x8 a) { return F32x8(_mm256_ceil_ps(a.v)); }
    
        XTD_FORCE_INLINE F32x8 fma(F32x8 a, F32x8 b, F32x8 c) {
        #if defined(XTD_FMA)
            return F32x8(_mm256_fmadd_ps(a.v, b.v, c.v));
        #else
            return a * b + c;
        #endif
        }
    
        XTD_FORCE_INLINE F32x8 cmpeq(F32x8 a, F32x8 b) { return F32x8(_mm256_cmp_ps(a.v, b.v, _CMP_EQ_OQ)); }
        XTD_FORCE_INLINE F32x8 cmpne(F32x8 a, F32x8 b) { return F32x8(_mm256_cmp_ps(a.v, b.v, _CMP_NEQ_UQ)); }
        XTD_FORCE_INLINE F32x8 cmplt(F32x8 a, F32x8 b) { return F32x8(_mm256_cmp_ps(a.v, b.v, _CMP_LT_OQ)); }
        XTD_FORCE_INLINE F32x8 cmple(F32x8 a, F32x8 b) { return F32x8(_mm256_cmp_ps(a.v, b.v, _CMP_LE_OQ)); }
        XTD_FORCE_INLINE F32x8 cmpgt(F32x8 a, F32x8 b) { return F32x8(_mm256_cmp_ps(a.v, b.v, _CMP_GT_OQ)); }
        XTD_FORCE_INLINE F32x8 cmpge(F32x8 a, F32x8 b) { return F32x8(_mm256_cmp_ps(a.v, b.v, _CMP_GE_OQ)); }
    
        XTD_FORCE_INLINE F32x8 select(F32x8 m, F32x8 a, F32x8 b) { return F32x8(_mm256_blendv_ps(b.v, a.v, m.v)); }
        XTD_FORCE_INLINE int movemask(F32x8 m) { return _mm256_movemask_ps(m.v); }
        XTD_FORCE_INLINE float hsum(F32x8 a) { return hsum(a.low() + a.high()); }
    
        // -----------------------------------------------------------------------
    
        struct F64x4 {
            using Scalar = double;
            static constexpr int lanes = 4;
            __m256d v;
    
            F64x4() = default;
            explicit F64x4(__m256d x) : v(x) {}
            explicit F64x4(double s) : v(_mm256_set1_pd(s)) {}
            F64x4(double a, double b, double c, double d) : v(_mm256_setr_pd(a, b, c, d)) {}
    
            static F64x4 zero() { return F64x4(_mm256_setzero_pd()); }
            static F64x4 load(const double* p) { return F64x4(_mm256_load_pd(p)); }
            static F64x4 loadu(const double* p) { return F64x4(_mm256_loadu_pd(p)); }
            void store(double* p) const { _mm256_store_pd(p, v); }
            void storeu(double* p) const { _mm256_storeu_pd(p, v); }
            double get(int i) const { alignas(32) double t[4]; _mm256_store_pd(t, v); return t[i]; }
            F64x2 low() const { return F64x2(_mm256_castpd256_pd128(v)); }
            F64x2 high() const { return F64x2(_mm256_extractf128_pd(v, 1)); }
        };
    
        XTD_DETAIL_SIMD_OP(F64x4, +, _mm256_add_pd)
        XTD_DETAIL_SIMD_OP(F64x4, -, _mm256_sub_pd)
        XTD_DETAIL_SIMD_OP(F64x4, *, _mm256_mul_pd)
        XTD_DETAIL_SIMD_OP(F64x4, /, _mm256_div_pd)
        XTD_DETAIL_SIMD_OP(F64x4, &, _mm256_and_pd)
        XTD_DETAIL_SIMD_OP(F64x4, |, _mm256_or_pd)
        XTD_DETAIL_SIMD_OP(F64x4, ^, _mm256_xor_pd)
    
        XTD_FORCE_INLINE F64x4 operator-(F64x4 a) { return F64x4(_mm256_xor_pd(a.v, _mm256_set1_pd(-0.0))); }
        XTD_FORCE_INLINE F64x4 andnot(F64x4 a, F64x4 b) { return F64x4(_mm256_andnot_pd(a.v, b.v)); }
        XTD_FORCE_INLINE F64x4 min(F64x4 a, F64x4 b) { return F64x4(_mm256_min_pd(a.v, b.v)); }
        XTD_FORCE_INLINE F64x4 max(F64x4 a, F64x4 b) { return F64x4(_mm256_max_pd(a.v, b.v)); }
        XTD_FORCE_INLINE F64x4 sqrt(F64x4 a) { return F64x4(_mm256_sqrt_pd(a.v)); }
        XTD_FORCE_INLINE F64x4 abs(F64x4 a) { return F64x4(_mm256_andnot_pd(_mm256_set1_pd(-0.0), a.v)); }
        XTD_FORCE_INLINE F64x4 floor(F64x4 a) { return F64x4(_mm256_floor_pd(a.v)); }
        XTD_FORCE_INLINE F64x4 ceil(F64x4 a) { return F64x4(_mm256_ceil_pd(a.v)); }
    
        XTD_FORCE_INLINE F64x4 fma(F64x4 a, F64x4 b, F64x4 c) {
        #if defined(XTD_FMA)
            return F64x4(_mm256_fmadd_pd(a.v, b.v, c.v));
        #else
            return a * b + c;
        #endif
        }
    
        XTD_FORCE_INLINE F64x4 cmpeq(F64x4 a, F64x4 b) { return F64x4(_mm256_cmp_pd(a.v, b.v, _CMP_EQ_OQ)); }
        XTD_FORCE_INLINE F64x4 cmpne(F64x4 a, F64x4 b) { return F64x4(_mm256_cmp_pd(a.v, b.v, _CMP_NEQ_UQ)); }
        XTD_FORCE_INLINE F64x4 cmplt(F64x4 a, F64x4 b) { return F64x4(_mm256_cmp_pd(a.v, b.v, _CMP_LT_OQ)); }
        XTD_FORCE_INLINE F64x4 cmple(F64x4 a, F64x4 b) { return F64x4(_mm256_cmp_pd(a.v, b.v, _CMP_LE_OQ)); }
        XTD_FORCE_INLINE F64x4 cmpgt(F64x4 a, F64x4 b) { return F64x4(_mm256_cmp_pd(a.v, b.v, _CMP_GT_OQ)); }
        XTD_FORCE_INLINE F64x4 cmpge(F64x4 a, F64x4 b) { return F64x4(_mm256_cmp_pd(a.v, b.v, _CMP_GE_OQ)); }
    
        XTD_FORCE_INLINE F64x4 select(F64x4 m, F64x4 a, F64x4 b) { return F64x4(_mm256_blendv_pd(b.v, a.v, m.v)); }
        XTD_FORCE_INLINE int movemask(F64x4 m) { return _mm256_movemask_pd(m.v); }
        XTD_FORCE_INLINE double hsum(F64x4 a) { return hsum(a.low() + a.high()); }
    
    #endif // XTD_AVX
    
    // ===========================================================================
    // AVX2: I32x8, conversions, gather
    // ===========================================================================
    #if defined(XTD_AVX2)
    
        struct I32x8 {
            using Scalar = int;
            static constexpr int lanes = 8;
            __m256i v;
    
            I32x8() = default;
            explicit I32x8(__m256i x) : v(x) {}
            explicit I32x8(int s) : v(_mm256_set1_epi32(s)) {}
            I32x8(int a, int b, int c, int d, int e, int f, int g, int h)
                : v(_mm256_setr_epi32(a, b, c, d, e, f, g, h)) {}
            I32x8(I32x4 lo, I32x4 hi) : v(_mm256_inserti128_si256(_mm256_castsi128_si256(lo.v), hi.v, 1)) {}
    
            static I32x8 zero() { return I32x8(_mm256_setzero_si256()); }
            static I32x8 load(const int* p) { return I32x8(_mm256_load_si256(reinterpret_cast<const __m256i*>(p))); }
            static I32x8 loadu(const int* p) { return I32x8(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(p))); }
            void store(int* p) const { _mm256_store_si256(reinterpret_cast<__m256i*>(p), v); }
            void storeu(int* p) const { _mm256_storeu_si256(reinterpret_cast<__m256i*>(p), v); }
            int get(int i) const { alignas(32) int t[8]; store(t); return t[i]; }
            I32x4 low() const { return I32x4(_mm256_castsi256_si128(v)); }
            I32x4 high() const { return I32x4(_mm256_extracti128_si256(v, 1)); }
        };
    
        XTD_DETAIL_SIMD_OP(I32x8, +, _mm256_add_epi32)
        XTD_DETAIL_SIMD_OP(I32x8, -, _mm256_sub_epi32)
        XTD_DETAIL_SIMD_OP(I32x8, *, _mm256_mullo_epi32)
        XTD_DETAIL_SIMD_OP(I32x8, &, _mm256_and_si256)
        XTD_DETAIL_SIMD_OP(I32x8, |, _mm256_or_si256)
        XTD_DETAIL_SIMD_OP(I32x8, ^, _mm256_xor_si256)
    
        XTD_FORCE_INLINE I32x8 operator-(I32x8 a) { return I32x8(_mm256_sub_epi32(_mm256_setzero_si256(), a.v)); }
        XTD_FORCE_INLINE I32x8 operator~(I32x8 a) { return I32x8(_mm256_xor_si256(a.v, _mm256_set1_epi32(-1))); }
        XTD_FORCE_INLINE I32x8 andnot(I32x8 a, I32x8 b) { return I32x8(_mm256_andnot_si256(a.v, b.v)); }
    
        XTD_FORCE_INLINE I32x8 shl(I32x8 a, int n) { return I32x8(_mm256_slli_epi32(a.v, n)); }
        XTD_FORCE_INLINE I32x8 shr(I32x8 a, int n) { return I32x8(_mm256_srli_epi32(a.v, n)); }
        XTD_FORCE_INLINE I32x8 sar(I32x8 a, int n) { return I32x8(_mm256_srai_epi32(a.v, n)); }
        // Per-lane variable shifts (AVX2)
        XTD_FORCE_INLINE I32x8 shl(I32x8 a, I32x8 n) { return I32x8(_mm256_sllv_epi32(a.v, n.v)); }
        XTD_FORCE_INLINE I32x8 shr(I32x8 a, I32x8 n) { return I32x8(_mm256_srlv_epi32(a.v, n.v)); }
        XTD_FORCE_INLINE I32x8 sar(I32x8 a, I32x8 n) { return I32x8(_mm256_srav_epi32(a.v, n.v)); }
    
        XTD_FORCE_INLINE I32x8 cmpeq(I32x8 a, I32x8 b) { return I32x8(_mm256_cmpeq_epi32(a.v, b.v)); }
        XTD_FORCE_INLINE I32x8 cmpgt(I32x8 a, I32x8 b) { return I32x8(_mm256_cmpgt_epi32(a.v, b.v)); }
        XTD_FORCE_INLINE I32x8 cmplt(I32x8 a, I32x8 b) { return I32x8(_mm256_cmpgt_epi32(b.v, a.v)); }
    
        XTD_FORCE_INLINE I32x8 select(I32x8 m, I32x8 a, I32x8 b) { return I32x8(_mm256_blendv_epi8(b.v, a.v, m.v)); }
        XTD_FORCE_INLINE I32x8 min(I32x8 a, I32x8 b) { return I32x8(_mm256_min_epi32(a.v, b.v)); }
        XTD_FORCE_INLINE I32x8 max(I32x8 a, I32x8 b) { return I32x8(_mm256_max_epi32(a.v, b.v)); }
        XTD_FORCE_INLINE I32x8 abs(I32x8 a) { return I32x8(_mm256_abs_epi32(a.v)); }
        XTD_FORCE_INLINE int movemask(I32x8 m) { return _mm256_movemask_ps(_mm256_castsi256_ps(m.v)); }
        XTD_FORCE_INLINE int hsum(I32x8 a) { return hsum(a.low() + a.high()); }
    
        // Conversions (F32x8 <-> I32x8)
        XTD_FORCE_INLINE F32x8 toFloat(I32x8 a) { return F32x8(_mm256_cvtepi32_ps(a.v)); }
        XTD_FORCE_INLINE I32x8 toIntRound(F32x8 a) { return I32x8(_mm256_cvtps_epi32(a.v)); }
        XTD_FORCE_INLINE I32x8 toIntTrunc(F32x8 a) { return I32x8(_mm256_cvttps_epi32(a.v)); }
        XTD_FORCE_INLINE F32x8 bitcastToFloat(I32x8 a) { return F32x8(_mm256_castsi256_ps(a.v)); }
        XTD_FORCE_INLINE I32x8 bitcastToInt(F32x8 a) { return I32x8(_mm256_castps_si256(a.v)); }
    
        // Gather: result[i] = base[idx[i]]  (indices are in elements, not bytes)
        XTD_FORCE_INLINE F32x8 gather(const float* base, I32x8 idx) { return F32x8(_mm256_i32gather_ps(base, idx.v, 4)); }
        XTD_FORCE_INLINE I32x8 gather(const int* base, I32x8 idx) { return I32x8(_mm256_i32gather_epi32(base, idx.v, 4)); }
    
    #endif // XTD_AVX2
} // namespace xtd::simd
#undef XTD_DETAIL_SIMD_OP