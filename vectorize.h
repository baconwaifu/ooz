#pragma once

/*
 * Provides some useful vector primitives. Relies on GNU C compilers to auto-vectorize. (MS C is more conservative with vector instructions, so manually handle those)
 * Note that inlined functions get optimized based on the parameters of their 'parent' function.
 */

// Provide a macro for telling GCC to bloat the code for performance. (optimize several copies for different targets, selected at runtime; folds if resulting code is identical)
#ifdef __GNUC__

#ifdef OODLE_ENABLE_AVX2
//Only generate code-bloat and optimized versions when vectorization hasn't been explicitly disabled.
#define VECTORIZED __attribute__((target_clones("default","avx","avx2","arch=znver1","arch=skylake")))
#else
#define VECTORIZED
#endif //OODLE_ENABLE_AVX2
// allow use of MS syntax for forceinline in GNU C.
#define __forceinline inline __attribute__((always_inline))
#else
//MS C doesn't support target_clones *or* target
#define VECTORIZED
// MS C doesn't support __builtin_memcpy, so fall back to stdlib if AVX is disabled
#define __builtin_memcpy memcpy
#define __builtin_memmove memmove
// Prototype this here so we don't have to include a whole header just for these...
void memcpy(void* dst, const void* src, size_t len);
void memmove(void* dst, void* src, size_t len);
#endif
 

#ifdef _MSC_VER
#include <intrin.h>
#endif

#ifdef __GNUC__
#if defined(__AVX__) || defined(__AVX2__)
#include <xmmintrin.h>
#endif
#endif // __GNUC__

// 64-Bit vector add
static __forceinline void COPY_64_ADD(char* dst, const char* srca, const char* srcb);
static __forceinline void COPY_64_ADD(byte* dst, const byte* srca, const byte* srcb);

//64-byte copy
static __forceinline void COPY_64_BYTES(byte* dst, const byte* src);

//32-byte forward-move
static __forceinline void MOVE32(int32* src, int32* dst);

// No AVX support. generate fallback. (_MSC_VER implies AVX if it's not explicitly disabled in the config)
#if !(defined(__AVX__) || (defined(_MSC_VER) && defined(OODLE_ENABLE_AVX2)))
static __forceinline void COPY64_ADD(char* dst, const char* srca, const char* srcb){
    *dst++ = *srca++ + *srcb++;
    *dst++ = *srca++ + *srcb++;
    *dst++ = *srca++ + *srcb++;
    *dst++ = *srca++ + *srcb++;
    *dst++ = *srca++ + *srcb++;
    *dst++ = *srca++ + *srcb++;
    *dst++ = *srca++ + *srcb++;
    *dst++ = *srca++ + *srcb++;
}
static __forceinline void COPY64_ADD(byte* dst, const byte* srca, const byte* srcb){
    *dst++ = *srca++ + *srcb++;
    *dst++ = *srca++ + *srcb++;
    *dst++ = *srca++ + *srcb++;
    *dst++ = *srca++ + *srcb++;
    *dst++ = *srca++ + *srcb++;
    *dst++ = *srca++ + *srcb++;
    *dst++ = *srca++ + *srcb++;
    *dst++ = *srca++ + *srcb++;
}
#else
// We have AVX intrinsics; use them.
static __forceinline void COPY_64_ADD(char* dst, const char* srca, const char* srcb){
    __m128i* d = (__m128i*) dst;
    __m128i* a = (__m128i*) srca;
    __m128i* b = (__m128i*) srcb;
    _mm_storel_epi64(d, _mm_add_epi8(_mm_loadl_epi64(a), _mm_loadl_epi64(b)));
}
static __forceinline void COPY_64_ADD(byte* dst, const byte* srca, const byte* srcb){
    __m128i* d = (__m128i*) dst;
    __m128i* a = (__m128i*) srca;
    __m128i* b = (__m128i*) srcb;
    _mm_storel_epi64(d, _mm_add_epi8(_mm_loadl_epi64(a), _mm_loadl_epi64(b)));
}
#endif

#if __GNUC__ || !OODLE_ENABLE_AVX2
//Auto vectorization and unrolling in GCC/Clang, fallback for Win32 if AVX is disabled.
static __forceinline void COPY_64_BYTES(byte* dst, const byte* src){
  __builtin_memcpy(dst, src, 64);
}
#else
// Force AVX in MS C
static __forceinline void COPY_64_BYTES(byte* dst, const byte* src){
  __m128i* d = (__m128i*)dst;
  __m128i* s = (__m128i*)src;
  _mm_storel_epi128(d++,_mm_loadl_epi128(s++));
  _mm_storel_epi128(d++,_mm_loadl_epi128(s++));
  _mm_storel_epi128(d++,_mm_loadl_epi128(s++));
  _mm_storel_epi128(d++,_mm_loadl_epi128(s++));
  
}
#endif

#if defined(__GNUC__) || !defined(OODLE_ENABLE_AVX2)
//Auto-vectorization or explicitly disabled AVX, on GCC/clang and MSVC respectively.
static __forceinline void MOVE32(int32* dst, int32* src){
  __builtin_memmove(dst, src, 32);
}
#else
//MSVC needs explicit AVX.
static __forceinline void MOVE32(int32* dst, int32* src){
  __m128i* s = (__m128i*)(src + 4); //we go backwards, so bump our pointers forwards before casting.
  __m128i* d = (__m128i*)(dst + 4); //note that pointer arithmetic scales the operand to the size, so this is 16 bytes (1 m128i)
  _mm_storel_epi128(d--, _mm_loadl_epi128(src--));
  _mm_storel_epi128(d--, _mm_loadl_epi128(src--));
}
#endif

