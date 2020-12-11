// stdafx.h : includefile for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <limits.h>
#if defined(_MSC_VER)
#include <Windows.h>
#include <intrin.h>
#undef max
#undef min
#else
#include <stddef.h>
#define __forceinline inline
#define _byteswap_ushort(x) __builtin_bswap16((uint16)(x))
#define _byteswap_ulong(x) __builtin_bswap32((uint32)(x))
#define _byteswap_uint64(x) __builtin_bswap64((uint64)(x))
#define _BitScanForward(dst, x) (*(dst) = __builtin_ctz(x))
#define _BitScanReverse(dst, x) (*(dst) = (__builtin_clz(x) ^ 31))

static inline uint32_t _rotl(uint32_t x, int n) {
  return (((x) << (n)) | ((x) >> (32-(n))));
}

#include <xmmintrin.h>
#endif

// Windows has this enabled implicitly, gcc requires passing an additional flag, so check for that.
#ifdef __AVX__
#include <xmmintrin.h>
#endif

// GCC doesn't support 'pragma warning' and in response prints a warning.
#ifdef _MSC_VER
#pragma warning (disable: 4244)
#pragma warning (disable: 4530) // c++ exception handler used without unwind semantics
#pragma warning (disable: 4018) // signed/unsigned mismatch
#endif

// Get GCC to stop complaining about unused parameters in stub functions.
#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

// TODO: reference additional headers your program requires here
typedef uint8_t byte;
typedef uint8_t uint8;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int64_t int64;
typedef int32_t int32;
typedef uint16_t uint16;
typedef int16_t int16;
typedef unsigned int uint;
