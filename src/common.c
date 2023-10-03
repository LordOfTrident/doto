#include "common.h"

static void (*cleanup_fn)(void);

void cleanup(void) {
	if (cleanup_fn != NULL)
		cleanup_fn();
}

void set_cleanup(void (*fn)(void)) {
	cleanup_fn = fn;
}

void *xalloc(size_t bytes) {
	assert(bytes > 0);

	void *ptr = malloc(bytes);
	if (ptr == NULL)
		DIE("Memory allocation failed (exiting)");

	return ptr;
}

void *zalloc(size_t bytes) {
	void *ptr = xalloc(bytes);
	memset(ptr, 0, bytes);
	return ptr;
}

void *xrealloc(void *ptr, size_t bytes) {
	assert(ptr != NULL);
	assert(bytes > 0);

	ptr = realloc(ptr, bytes);
	if (ptr == NULL)
		DIE("Memory reallocation failed (exiting)");

	return ptr;
}

void xfree(void *ptr) {
	assert(ptr != NULL);

	free(ptr);
}
