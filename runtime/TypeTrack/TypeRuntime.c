#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>

#define SIZE ((size_t)(sizeof(unsigned int)) * (size_t)(4294967296))

unsigned int *shadow_begin;

/**
 * Initialize the shadow memory which records the 1:1 mapping of addresses to types.
 */
void shadowInit() {
	shadow_begin = (unsigned int *)mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_NORESERVE, -1, 0);

	if (shadow_begin == MAP_FAILED) {
		fprintf(stderr, "Failed to map the shadow memory!");
		fflush(stderr);
		assert(0 && "MAP_FAILED");
	}
}

/**
 * Unmap the shadow memory which records the 1:1 mapping of addresses to types.
 */
void shadowUnmap() {
	if (munmap(shadow_begin, SIZE) == -1) {
		fprintf(stderr, "Failed to unmap the shadow memory!");
		fflush(stderr);
	}
}

/**
 * Check the loaded type against the type recorded in the shadow memory.
 *
 * Note: currently does not handle GEPs.
 */
int trackLoadInst(void *ptr, unsigned int typeNumber) {
	uintptr_t p = (uintptr_t)ptr;
	p &= 0xFFFFFFFF;
	printf("Load: %p, %p = %u | expecting %u\n", ptr, (void *)p, typeNumber, shadow_begin[p]);

	return 0;
}

/**
 * Record the stored type and address in the shadow memory.
 *
 * Note: currently does not handle GEPs.
 */
int trackStoreInst(void *ptr, unsigned int typeNumber) {
	uintptr_t p = (uintptr_t)ptr;
	p &= 0xFFFFFFFF;
	shadow_begin[p] = typeNumber;
#if 0
	printf("Store: %p, %p = %u\n", ptr, (void *)p, typeNumber);
#endif

	return 0;
}
