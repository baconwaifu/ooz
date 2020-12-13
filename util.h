#pragma once

#include "stdafx.h"

void *MallocAligned(size_t size, size_t alignment);
void FreeAligned(void *p);

inline size_t Max(size_t a, size_t b) { return a > b ? a : b; }
inline size_t Min(size_t a, size_t b) { return a < b ? a : b; }

#define ALIGN_POINTER(p, align) ((uint8*)(((uintptr_t)(p) + (align - 1)) & ~(align - 1)))
