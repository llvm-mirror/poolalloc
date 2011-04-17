#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
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
 * Check the loaded type against the type recorded in the shadow memory.
 */
void trackLoadInst(void *ptr, uint8_t typeNumber) {
	uintptr_t p = (uintptr_t)ptr;
	p &= 0xFFFFFFFF;

	if (typeNumber != shadow_begin[p]) {
		printf("Type mismatch: detecting %u, expecting %u!\n", typeNumber, shadow_begin[p]);
	}

#if DEBUG
	printf("Load: %p, %p = %u | expecting %u\n", ptr, (void *)p, typeNumber, shadow_begin[p]);
#endif
}

/**
 * Record the stored type and address in the shadow memory.
 */
void trackStoreInst(void *ptr, uint8_t typeNumber) {
	uintptr_t p = (uintptr_t)ptr;
	p &= 0xFFFFFFFF;
	shadow_begin[p] = typeNumber;
#if DEBUG
	printf("Store: %p, %p = %u\n", ptr, (void *)p, typeNumber);
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
