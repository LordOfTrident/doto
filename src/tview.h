#ifndef TVIEW_H_HEADER_GUARD
#define TVIEW_H_HEADER_GUARD

#include <assert.h>  /* assert */
#include <string.h>  /* strstr */
#include <stdbool.h> /* bool, true, false */

#include <noch/utf8.h>

#include "common.h"
#include "tui.h"
#include "tree.h"
#include "conf.h"

typedef struct {
	node_t *node;
	size_t  indent;
} tview_line_t;

#define TVIEW_CHUNK_SIZE   64
#define MATCHES_CHUNK_SIZE 64

typedef struct {
	tree_t *tree;

	tview_line_t *lines;
	size_t        size, cap, cur;

	size_t scroll_y, scroll_x;
	int    page_h;

	size_t prev_cur, prev_scroll_y;

	const char *search;
	size_t     *matches, matches_count, matches_cap, match_pos;

	const char *(*str_find)(const char*, const char*, size_t*);

	bool        prompted;
	const char *hooked_text;
} tview_t;

void tview_populate(tview_t *this, tree_t *tree);
void tview_clean(tview_t *this);

void tview_search        (tview_t *this, const char *search, bool cs);
void tview_search_end    (tview_t *this);
void tview_search_discard(tview_t *this);

void tview_jump(tview_t *this, size_t to);

void tview_render(tview_t *this, int x, int y, int w, int h);
void tview_input (tview_t *this, int in);

#endif
