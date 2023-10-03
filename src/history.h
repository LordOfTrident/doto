#ifndef HISTORY_H_HEADER_GUARD
#define HISTORY_H_HEADER_GUARD

#include <assert.h> /* assert */

#include "common.h"

#define HISTORY_CHUNK_SIZE 32

typedef struct {
	char **buf;
	size_t size, cap, pos;
} history_t;

void history_init  (history_t *this);
void history_deinit(history_t *this);

void history_push(history_t *this, const char *str);

const char *history_up  (history_t *this);
const char *history_down(history_t *this);

#endif
