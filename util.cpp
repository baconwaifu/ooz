
#include "util.h"
// Allocate memory with a specific alignment
void *MallocAligned(size_t size, size_t alignment) {
  void *x = malloc(size + (alignment - 1) + sizeof(void*)), *x_org = x;
  if (x) {
    x = (void*)(((intptr_t)x + alignment - 1 + sizeof(void*)) & ~(alignment - 1));
    ((void**)x)[-1] = x_org;
  }
  return x;
}

// Free memory allocated through |MallocAligned|
void FreeAligned(void *p) {
  free(((void**)p)[-1]);
}
