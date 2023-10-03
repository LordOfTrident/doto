#include "prompt.h"

void prompt_init(prompt_t *this) {
	ZERO_STRUCT(this);

	this->cap   = PROMPT_CHUNK_SIZE;
	this->line  = (char*)xalloc(this->cap);
	*this->line = '\0';

	history_init(&this->hist);
}

void prompt_deinit(prompt_t *this) {
	assert(this != NULL);

	xfree(this->line);
	history_deinit(&this->hist);
}

void prompt_set_line(prompt_t *this, const char *line) {
	if (line == NULL) {
		*this->line = '\0';
		this->cur   = 0;
		this->size  = 0;
		this->len   = 0;
		return;
	}

	this->size = u8_str_bytes(line);
	this->len  = u8_str_len  (line);
	if (this->size >= this->cap) {
		do
			this->cap *= 2;
		while (this->size >= this->cap);

		this->line = (char*)xrealloc(this->line, this->cap);
	}

	strcpy(this->line, line);
	this->cur = this->len;
	this->sel = false;
}

void prompt_clear(prompt_t *this) {
	assert(this != NULL);

	if (this->len > 0)
		history_push(&this->hist, this->line);
	prompt_set_line(this, NULL);
}

static size_t prompt_sel_start(prompt_t *this) {
	return this->sel_pos > this->cur? this->cur : this->sel_pos;
}

static size_t prompt_sel_end(prompt_t *this) {
	return this->sel_pos > this->cur? this->sel_pos : this->cur;
}

void prompt_render(prompt_t *this, int x, int y, int w, int fg, int bg, int sel_fg, int sel_bg) {
	assert(this != NULL);

	if (this->cur <= this->scroll)
		this->scroll = this->cur == 0? 0 : this->cur - 1;
	else if (this->cur >= this->scroll + w)
		this->scroll = this->cur == this->len? this->len - w : this->cur - w + 1;

	if (this->scroll + w > this->len && this->scroll > 0)
		this->scroll = this->len < (size_t)w? 0 : this->len - w;

	size_t start = prompt_sel_start(this);
	size_t end   = prompt_sel_end(this);

	tui_goto(x, y);
	tui_set_color(fg, bg);

	const char *it = u8_str_idx_to_ptr(this->line, this->scroll);
	for (size_t i = this->scroll; i < this->len; ++ i) {
		int pos = i - this->scroll;
		if (pos >= w)
			break;

		if (this->sel) {
			if (i == start)
				tui_set_color(sel_fg, sel_bg);
			else if (i == end)
				tui_set_color(fg, bg);
		}

		size_t size;
		rune_t r = rune_decode_u8(it, &size);
		tui_add_rune(r);
		it += size;
	}

	tui_caret_goto(x + this->cur - this->scroll, y);
	tui_caret(!this->sel);
}

static void prompt_sel_all(prompt_t *this) {
	this->cur     = 0;
	this->sel_pos = this->len;
	this->sel     = true;
}

static void prompt_remove_sel(prompt_t *this) {
	size_t start_idx = prompt_sel_start(this);
	size_t end_idx   = prompt_sel_end  (this);

	char *start = (char*)u8_str_idx_to_ptr(this->line, start_idx);
	char *end   = (char*)u8_str_idx_to_ptr(this->line, end_idx);

	assert(start != NULL);
	assert(end   != NULL);

	size_t size = this->size - (size_t)(end - this->line);
	for (size_t i = 0; i < size; ++ i)
		start[i] = end[i];

	start[size]  = '\0';
	this->cur  = start_idx;
	this->sel  = false;
	this->size = (size_t)(start - this->line) + size;
	this->len  = u8_str_len(this->line);
}

static void prompt_copy(prompt_t *this) {
	if (!this->sel) {
		clipboard_set(this->line, this->size);
		return;
	}

	char *start = (char*)u8_str_idx_to_ptr(this->line, prompt_sel_start(this));
	char *end   = (char*)u8_str_idx_to_ptr(this->line, prompt_sel_end  (this));

	clipboard_set(start, (size_t)(end - start));
}

static void prompt_paste(prompt_t *this) {
	if (this->sel)
		prompt_remove_sel(this);

	const char *paste = clipboard_get();
	if (paste == NULL)
		return;

	size_t size      = u8_str_bytes(paste);
	size_t prev_size = this->size;
	this->size += size;
	if (this->size >= this->cap) {
		do
			this->cap *= 2;
		while (this->size >= this->cap);

		this->line = (char*)xrealloc(this->line, this->cap);
	}

	char *cur = (char*)u8_str_idx_to_ptr(this->line, this->cur);

	for (char *it = this->line + prev_size + 1; it --> cur;)
		it[size] = *it;

	memcpy(cur, paste, size);

	size_t len = u8_str_len(paste);
	this->cur += len;
	this->len += len;
}

static void prompt_cut(prompt_t *this) {
	if (this->sel) {
		prompt_copy(this);
		prompt_remove_sel(this);
	}
}

static void prompt_insert(prompt_t *this, rune_t r) {
	if (this->sel)
		prompt_remove_sel(this);

	size_t prev_size = this->size;

	char   seq[sizeof(rune_t) + 1] = {0};
	size_t size = rune_encode_u8(r, seq);
	assert(size != (size_t)-1);

	this->size += size;
	if (this->size >= this->cap) {
		this->cap *= 2;
		this->line = (char*)xrealloc(this->line, this->cap);
	}

	if (this->cur == this->len ++) {
		memcpy(this->line + prev_size, seq, size + 1);
	} else {
		char *cur = (char*)u8_str_idx_to_ptr(this->line, this->cur);

		for (char *it = this->line + prev_size + 1; it --> cur;)
			it[size] = *it;

		memcpy(cur, seq, size);
	}

	++ this->cur;
}

static void prompt_delete(prompt_t *this) {
	if (this->sel) {
		prompt_remove_sel(this);
		return;
	}

	if (this->cur == 0)
		return;

	if (this->cur == this->len --) {
		char *prev = (char*)u8_str_prev(this->line + this->size, this->line);
		assert(prev != NULL);

		this->size -= u8_rune_size(*prev);
		*prev = '\0';
	} else {
		char *cur  = (char*)u8_str_idx_to_ptr(this->line, this->cur);
		char *prev = (char*)u8_str_prev(cur, this->line);
		assert(prev != NULL);

		size_t size = u8_rune_size(*prev);
		for (char *it = cur; it <= this->line + this->size; ++ it)
			it[-size] = *it;

		this->size -= size;
		this->line[this->size] = '\0';
	}

	-- this->cur;
}

static void prompt_left(prompt_t *this, bool sel) {
	if (!this->sel && sel) {
		this->sel     = true;
		this->sel_pos = this->cur;
	} else if (this->sel && !sel) {
		this->sel = false;
		this->cur = prompt_sel_start(this);
		return;
	}

	if (this->cur > 0)
		-- this->cur;

	if (this->sel && this->sel_pos == this->cur)
		this->sel = false;
}

static void prompt_right(prompt_t *this, bool sel) {
	if (!this->sel && sel) {
		this->sel     = true;
		this->sel_pos = this->cur;
	} else if (this->sel && !sel) {
		this->sel = false;
		this->cur = prompt_sel_end(this);
		return;
	}

	if (this->cur < this->len)
		++ this->cur;

	if (this->sel && this->sel_pos == this->cur)
		this->sel = false;
}

static bool is_word_ch(char ch) {
	return isalnum(ch) || ch == '_' || u8_rune_size(ch) > 1;
}

/* TODO: This is probably pretty slow, so instead optimize it to store the
         cursor pointer along with the cursor index

         edit: or maybe it doesnt matter anyways */
#define PROMPT_CUR_CH(PROMPT) *u8_str_idx_to_ptr((PROMPT)->line, (PROMPT)->cur)

static void prompt_word_left(prompt_t *this, bool sel) {
	prompt_left(this, sel);

	while (isspace(PROMPT_CUR_CH(this))) {
		if (this->cur == 0)
			return;

		prompt_left(this, sel);
	}

	prompt_left(this, sel);
	while (is_word_ch(PROMPT_CUR_CH(this))) {
		if (this->cur == 0)
			return;

		if (is_word_ch(PROMPT_CUR_CH(this)))
			prompt_left(this, sel);
	}
	prompt_right(this, sel);
}

static void prompt_word_right(prompt_t *this, bool sel) {
	while (isspace(PROMPT_CUR_CH(this))) {
		prompt_right(this, sel);

		if (this->cur >= this->len)
			return;
	}

	do {
		if (this->cur >= this->len) {
			prompt_right(this, sel);
			return;
		}

		prompt_right(this, sel);
	} while (is_word_ch(PROMPT_CUR_CH(this)));
}

static void prompt_up(prompt_t *this) {
	prompt_set_line(this, history_up(&this->hist));
}

static void prompt_down(prompt_t *this) {
	prompt_set_line(this, history_down(&this->hist));
}

void prompt_input(prompt_t *this, int in) {
	MATCH(int, in,
		TO(KEY_ARROW_LEFT)       prompt_left      (this, false);
		TO(KEY_ARROW_RIGHT)      prompt_right     (this, false);
		TO(KEY_CTRL_ARROW_LEFT)  prompt_word_left (this, false);
		TO(KEY_CTRL_ARROW_RIGHT) prompt_word_right(this, false);

		TO(KEY_SHIFT_ARROW_LEFT)       prompt_left      (this, true);
		TO(KEY_SHIFT_ARROW_RIGHT)      prompt_right     (this, true);
		TO(KEY_CTRL_SHIFT_ARROW_LEFT)  prompt_word_left (this, true);
		TO(KEY_CTRL_SHIFT_ARROW_RIGHT) prompt_word_right(this, true);

		TO(KEY_CTRL('a')) prompt_sel_all(this);
		TO(KEY_CTRL('c')) prompt_copy   (this);

		else
			this->changed = true;
	);

	/* Input that modifies the prompt content */
	if (isprint(in))
		prompt_insert(this, in);
	else {
		MATCH(int, in,
			TO(keybinds[K_HIST_UP])   prompt_up    (this);
			TO(keybinds[K_HIST_DOWN]) prompt_down  (this);
			TO(KEY_BACKSPACE)            prompt_delete(this);

			TO(KEY_CTRL('v')) prompt_paste(this);
			TO(KEY_CTRL('x')) prompt_cut  (this);

			else
				this->changed = false;
		);
	}
}
