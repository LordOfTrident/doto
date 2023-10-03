#ifndef PROMPT_H_HEADER_GUARD
#define PROMPT_H_HEADER_GUARD

#include <assert.h>  /* assert */
#include <ctype.h>   /* isspace, isalnum */
#include <stdbool.h> /* bool, true, false */
#include <string.h>  /* strlen, strcpy */

#include "common.h"
#include "history.h"
#include "conf.h"
#include "clipboard.h"

#define PROMPT_CHUNK_SIZE 64

typedef struct {
	char  *line;
	size_t len, size, cap, cur;

	bool   sel;
	size_t scroll, sel_pos;

	history_t hist;
	bool      changed;
} prompt_t;

void prompt_init  (prompt_t *this);
void prompt_deinit(prompt_t *this);

void prompt_set_line(prompt_t *this, const char *line);

void prompt_clear(prompt_t *this);

void prompt_render(prompt_t *this, int x, int y, int w, int fg, int bg, int sel_fg, int sel_bg);
void prompt_input(prompt_t *this, int in);

#endif
