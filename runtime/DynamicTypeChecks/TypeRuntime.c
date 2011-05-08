#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#define DEBUG (0)
#define SIZE ((size_t)(70368744177664))

uint8_t *shadow_begin;

uintptr_t maskAddress(void *ptr) {
  uintptr_t p = (uintptr_t)ptr;

  if ((p & 0x600000000000) == 0x600000000000) {
    return (p & 0x3FFFFFFFFFFF);
  } else if ((p | 0x1FFFFFFFFFFF) == 0x1FFFFFFFFFFF) {
    return p;
  } else {
    fprintf(stderr, "Address out of range!\n");
    fflush(stderr);
    assert(0 && "MAP_FAILED");
  }
}

/**
 * Initialize the shadow memory which records the 1:1 mapping of addresses to types.
 */
void shadowInit() {
  shadow_begin = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

  if (shadow_begin == MAP_FAILED) {
    fprintf(stderr, "Failed to map the shadow memory!\n");
    fflush(stderr);
    assert(0 && "MAP_FAILED");
  }
}

/**
 * Unmap the shadow memory which records the 1:1 mapping of addresses to types.
 */
void shadowUnmap() {
  if (munmap(shadow_begin, SIZE) == -1) {
    fprintf(stderr, "Failed to unmap the shadow memory!\n");
    fflush(stderr);
  }
}

/**
 * Record the global type and address in the shadow memory.
 */
void trackGlobal(void *ptr, uint8_t typeNumber, uint64_t size, uint32_t tag) {
  uintptr_t p = maskAddress(ptr);
  shadow_begin[p] = typeNumber;
  memset(&shadow_begin[p + 1], 0, size - 1);
#if DEBUG
  printf("Global: %p, %p = %u | %" PRIu64 " bytes\n", ptr, (void *)p, typeNumber, size);
#endif
}
/**
 * Record the type stored at ptr(of size size) and replicate it
 */
void trackGlobalArray(void *ptr, uint64_t size, uint64_t count, uint32_t tag) {
  uintptr_t p = maskAddress(ptr);
  uintptr_t p1 = maskAddress(ptr);
  uint64_t i;

  for (i = 1; i < count; ++i) {
    p += size;
    memcpy(&shadow_begin[p], &shadow_begin[p1], size);
  }
}

/**
 * Record the stored type and address in the shadow memory.
 */
void trackStoreInst(void *ptr, uint8_t typeNumber, uint64_t size, uint32_t tag) {
  uintptr_t p = maskAddress(ptr);
  shadow_begin[p] = typeNumber;
  memset(&shadow_begin[p + 1], 0, size - 1);
#if DEBUG
  printf("Store: %p, %p = %u | %" PRIu64 " bytes | %d\n", ptr, (void *)p, typeNumber, size, tag);
#endif
}

/**
 * Check the loaded type against the type recorded in the shadow memory.
 */
void trackLoadInst(void *ptr, uint8_t typeNumber, uint64_t size, uint32_t tag) {
  uint8_t i = 1;
  uintptr_t p = maskAddress(ptr);

  /* Check if this an initialized but untyped memory.*/
  if (shadow_begin[p] != 0xFF) {
    if (typeNumber != shadow_begin[p]) {
      printf("Type mismatch: detecting %p %u, expecting %u! %u \n", ptr, typeNumber, shadow_begin[p], tag);
      i = size;
    }

    for (; i < size; ++i) {
      if (0 != shadow_begin[p + i]) {
        printf("Type mismatch: detecting %u, expecting %u (0 != %u)!\n", typeNumber, shadow_begin[p], shadow_begin[p + i]);
        break;
      }
    }
  } else {
    /* If so, set type to the type being read.
        Check that none of the bytes are typed.*/
    for (; i < size; ++i) {
      if (0xFF != shadow_begin[p + i]) {
        printf("Type mismatch: detecting %u, expecting %u (0 != %u)! %u\n", typeNumber, shadow_begin[p+i], shadow_begin[p + i], tag);
        break;
      }
    }
    trackStoreInst(ptr, typeNumber, size, tag);
  }

#if DEBUG
  printf("Load: %p, %p = actual: %u, expect: %u | %" PRIu64 " bytes\n", ptr, (void *)p, typeNumber, shadow_begin[p], size);
#endif
}

/**
 *  For memset type instructions, that set values. 
 *  0xFF type indicates that any type can be read, 
 */
void trackInitInst(void *ptr, uint64_t size, uint32_t tag) {
  uintptr_t p = maskAddress(ptr);
  memset(&shadow_begin[p], 0xFF, size);
}
void trackUnInitInst(void *ptr, uint64_t size, uint32_t tag) {
  uintptr_t p = maskAddress(ptr);
  memset(&shadow_begin[p], 0x00, size);
}

/**
 * Copy size bits of metadata from src ptr to dest ptr.
 */
void copyTypeInfo(void *dstptr, void *srcptr, uint64_t size, uint32_t tag) {
  uintptr_t d = maskAddress(dstptr);
  uintptr_t s = maskAddress(srcptr);
  memcpy(&shadow_begin[d], &shadow_begin[s], size);
#if DEBUG
  printf("Copy: %p, %p = %u | %" PRIu64 " bytes | %d\n", dstptr, (void *)d, shadow_begin[s], size, tag);
#endif
}
