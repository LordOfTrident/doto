#ifndef COMMON_H_HEADER_GUARD
#define COMMON_H_HEADER_GUARD

#include <stdlib.h> /* abort, malloc, realloc, free, exit, EXIT_FAILURE */
#include <stdio.h>  /* fprintf, stderr */
#include <assert.h> /* assert */
#include <string.h> /* memset */
#include <ctype.h>  /* tolower */

#include <noch/common.h>

#define DIE(...)                   \
	(cleanup(),                    \
	 fprintf(stderr, __VA_ARGS__), \
	 fprintf(stderr, "\n"),        \
	 exit(EXIT_FAILURE))

#define NEW(T) (T*)zalloc(sizeof(T))

#define MATCH(T, X, ...) \
	do {                 \
		T _to_match = X; \
		                 \
		if (false) {}    \
		__VA_ARGS__      \
	} while (0)

#define TO(X) else if (_to_match == (X))

void cleanup(void);
void set_cleanup(void (*fn)(void));

void *xalloc  (size_t bytes);
void *zalloc  (size_t bytes);
void *xrealloc(void *ptr, size_t bytes);
void  xfree   (void *ptr);

#endif
