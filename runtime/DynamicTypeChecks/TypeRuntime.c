#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#define DEBUG (0)

#define SIZE ((size_t)(4294967296))

#if 0
/* 2^47 bits */
#define SIZE ((size_t)(140737488355328))
#endif

uint8_t *shadow_begin;

/**
 * Initialize the shadow memory which records the 1:1 mapping of addresses to types.
 */
void shadowInit() {
	shadow_begin = (uint8_t *)mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_NORESERVE, -1, 0);

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
void trackGlobal(void *ptr, uint8_t typeNumber, uint8_t size) {
	uintptr_t p = (uintptr_t)ptr;
	p &= 0xFFFFFFFF;
	shadow_begin[p] = typeNumber;
	memset(&shadow_begin[p + 1], 0, size - 1);

#if DEBUG
	printf("Global: %p, %p = %u | %u bytes\n", ptr, (void *)p, typeNumber, size);
#endif
}
/**
 * Record the type stored at ptr(of size size) and replicate it
 */
void trackGlobalArray(void *ptr, uint32_t size, uint32_t count) {
  unsigned i;
  uintptr_t p = (uintptr_t)ptr;
  uint8_t typeNumber = shadow_begin[p & 0xFFFFFFFF];
  for(i =1; i<count;i++) {
    p += size;
    shadow_begin[p & 0xFFFFFFFF] = typeNumber;
  }
}

/**
 * Check the loaded type against the type recorded in the shadow memory.
 */
void trackLoadInst(void *ptr, uint8_t typeNumber, uint8_t size) {
	uint8_t i = 1;
	uintptr_t p = (uintptr_t)ptr;
	p &= 0xFFFFFFFF;

	if (typeNumber != shadow_begin[p]) {
		printf("Type mismatch: detecting %u, expecting %u!\n", typeNumber, shadow_begin[p]);
		i = size;
	}

	for (; i < size; ++i) {
		if (0 != shadow_begin[p + i]) {
			printf("Type mismatch: detecting %u, expecting %u (0 != %u)!\n", typeNumber, shadow_begin[p], shadow_begin[p + i]);
			break;
		}
	}

#if DEBUG
	printf("Load: %p, %p = actual: %u, expect: %u | %u bytes\n", ptr, (void *)p, typeNumber, shadow_begin[p], size);
#endif
}

/**
 * Record the stored type and address in the shadow memory.
 */
void trackStoreInst(void *ptr, uint8_t typeNumber, uint8_t size) {
	uintptr_t p = (uintptr_t)ptr;
	p &= 0xFFFFFFFF;
	shadow_begin[p] = typeNumber;
	memset(&shadow_begin[p + 1], 0, size - 1);

#if DEBUG
	printf("Store: %p, %p = %u | %u bytes\n", ptr, (void *)p, typeNumber, size);
#endif
}

/**
 * Copy size bits of metadata from src ptr to dest ptr.
 */
void copyTypeInfo(void *dstptr, void *srcptr, uint8_t size) {
	uintptr_t d = (uintptr_t)dstptr;
	uintptr_t s = (uintptr_t)srcptr;
	d &= 0xFFFFFFFF;
	s &= 0xFFFFFFFF;
	memcpy(&shadow_begin[d], &shadow_begin[s], size);
}
