#include "editor.h"
#include "tview.h"

static void tview_add_line(tview_t *this, tview_line_t line) {
	if (this->size >= this->cap) {
		this->cap  *= 2;
		this->lines = (tview_line_t*)xrealloc(this->lines, this->cap * sizeof(tview_line_t));
	}

	this->lines[this->size ++] = line;
}

static void tview_add_children(tview_t *this, size_t indent, node_t *child) {
	for (; child != NULL; child = child->next) {
		tview_line_t line;
		line.node   = child;
		line.indent = indent;
		tview_add_line(this, line);

		if (child->type == NODE_GROUP) {
			if (AS_GROUP(child)->open)
				tview_add_children(this, indent + 1, AS_GROUP(child)->children);
		}
	}
}

void tview_populate(tview_t *this, tree_t *tree) {
	ZERO_STRUCT(this);
	this->str_find = u8_str_find_str;
	this->tree     = tree;

	this->cap   = TVIEW_CHUNK_SIZE;
	this->lines = (tview_line_t*)xalloc(this->cap * sizeof(tview_line_t));

	this->matches_cap = MATCHES_CHUNK_SIZE;
	this->matches     = (size_t*)xalloc(this->matches_cap * sizeof(size_t));

	tview_add_children(this, 0, tree->root);
}

void tview_clean(tview_t *this) {
	xfree(this->lines);
	xfree(this->matches);
}

void tview_search(tview_t *this, const char *search, bool cs) {
	if (this->search == NULL) {
		this->prev_cur      = this->cur;
		this->prev_scroll_y = this->scroll_y;
	}

	this->str_find      = cs? u8_str_find_str : u8_str_find_str_ci;
	this->search        = search;
	this->matches_count = 0;

	bool   was_below_cur = true;
	size_t len = u8_str_len(this->search);
	if (len == 0)
		return;

	size_t match_below_cur = -1, match_above_cur = -1;
	for (size_t i = 0; i < this->size; ++ i) {
		const char *title;

		switch (this->lines[i].node->type) {
		case NODE_TASK:  title = AS_TASK (this->lines[i].node)->title; break;
		case NODE_GROUP: title = AS_GROUP(this->lines[i].node)->title; break;

		default: UNREACHABLE();
		}

		if (i == this->cur)
			this->match_pos = i;

		if (this->str_find(title, this->search, NULL) == NULL)
			continue;

		if (this->cur > i) {
			was_below_cur   = true;
			match_below_cur = this->matches_count;
		} else if (was_below_cur) {
			was_below_cur   = false;
			match_above_cur = this->matches_count;
		}

		if (this->matches_count >= this->matches_cap) {
			this->matches_cap *= 2;
			this->matches = (size_t*)xrealloc(this->matches, this->matches_cap * sizeof(size_t));
		}

		this->matches[this->matches_count ++] = i;
	}

	if (this->matches_count > 0) {
		this->match_pos = match_above_cur == (size_t)-1? match_below_cur : match_above_cur;
		this->cur       = this->matches[this->match_pos];
	}
}

void tview_search_end(tview_t *this) {
	this->search = NULL;
	this->matches_count = 0;

	if (this->matches_cap > MATCHES_CHUNK_SIZE) {
		this->matches_cap = MATCHES_CHUNK_SIZE;
		this->matches     = (size_t*)xrealloc(this->matches, this->matches_cap * sizeof(size_t));
	}
}

void tview_search_discard(tview_t *this) {
	tview_search_end(this);

	this->cur      = this->prev_cur;
	this->scroll_y = this->prev_scroll_y;
}

void tview_jump(tview_t *this, size_t to) {
	this->cur = this->scroll_y + to;

	if (this->cur >= this->size)
		this->cur = this->size - 1;
}

static void tview_render_title(tview_t *this, const char *title, bool active) {
	if (active && this->prompted) {
		tui_set_color(colorscheme[C_SELECT_FG], colorscheme[C_SELECT_BG]);
		tui_add_str(this->hooked_text);
		tui_set_color(colorscheme[C_ACTIVE_FG], colorscheme[C_ACTIVE_BG]);
		return;
	}

	int fg, bg, match_fg, match_bg;

	if (active) {
		fg       = colorscheme[C_ACTIVE_FG];
		bg       = colorscheme[C_ACTIVE_BG];
		match_fg = colorscheme[C_MATCH_ACTIVE_FG];
		match_bg = colorscheme[C_MATCH_ACTIVE_BG];
	} else {
		fg       = colorscheme[C_FG];
		bg       = colorscheme[C_BG];
		match_fg = colorscheme[C_MATCH_INACTIVE_FG];
		match_bg = colorscheme[C_MATCH_INACTIVE_BG];
	}

	tui_set_color(fg, bg);

	size_t      len  = this->search == NULL? 0 : u8_str_len(this->search);
	const char *next = this->search == NULL || len == 0?
	                   NULL : this->str_find(title, this->search, NULL);

	for (const char *it = title; *it != '\0'; it += u8_rune_size(*it)) {
		if (it == next) {
			int prev = tui_get_attr();
			tui_add_attr(ATTR_BOLD);
			tui_set_color(match_fg, match_bg);

			for (size_t i = 0; i < len; ++ i) {
				size_t size;
				rune_t r = rune_decode_u8(it, &size);
				tui_add_rune(r);

				if (i + 1 < len)
					it += size;
			}

			tui_set_attr(prev);
			tui_set_color(fg, bg);

			next = this->str_find(it + u8_rune_size(*it), this->search, NULL);
		} else
			tui_add_rune(rune_decode_u8(it, NULL));
	}

	tui_set_color(fg, bg);
}

static void tview_render_task(tview_t *this, task_t *task, bool active) {
	tui_right(3);
	tui_set_attr(ATTR_NONE);

	if (active)
		tui_set_fg(task->done? colorscheme[C_DONE_ACTIVE] : colorscheme[C_UNDONE_ACTIVE]);
	else
		tui_set_fg(task->done? colorscheme[C_DONE_INACTIVE] : colorscheme[C_UNDONE_INACTIVE]);

	tui_add_rune(flags[F_CIRCLE_MARK]? L'\u2B24' : L'\u25FC');

	if (flags[F_STRIKETHROUGH] && task->done)
		tui_add_attr(ATTR_STRIKETHROUGH);

	tui_right(1);
	tview_render_title(this, task->title, active);
}

static void tview_render_group(tview_t *this, group_t *group, bool active) {
	tui_set_attr(ATTR_BOLD);

	if (active)
		tui_set_fg(group->open? colorscheme[C_CLOSE_ACTIVE] : colorscheme[C_OPEN_ACTIVE]);
	else
		tui_set_fg(group->open? colorscheme[C_CLOSE_INACTIVE] : colorscheme[C_OPEN_INACTIVE]);

	tui_add_rune(group->open? '-' : '+');
	tui_right(1);

	if (group->prog == 0)
		tui_set_fg(colorscheme[C_LOW]);
	else if (group->prog == 1)
		tui_set_fg(colorscheme[C_HIGH]);
	else
		tui_set_fg(colorscheme[C_MID]);

	if (group->prog == -1)
		tui_add_str(" - %");
	else
		tui_add_strf("%3i%%", (int)(group->prog * 100));

	if (flags[F_STRIKETHROUGH] && group->prog == 1)
		tui_add_attr(ATTR_STRIKETHROUGH);

	tui_right(1);
	tview_render_title(this, group->title, active);
}

static void tview_render_tree(tview_t *this, int w, int h) {
	tui_set_attr(ATTR_NONE);
	tui_set_color(colorscheme[C_FG], colorscheme[C_BG]);
	tui_clear(' ');

	if (this->cur <= this->scroll_y)
		this->scroll_y = this->cur == 0? 0 : this->cur - 1;
	else if (this->cur + 3 >= this->scroll_y + h)
		this->scroll_y = this->cur == this->size - 1? this->size - h + 2 : this->cur - h + 4;

	if (this->scroll_y + h > this->size + 2 && this->scroll_y > 0)
		this->scroll_y = this->size < (size_t)h? 0 : this->size - h + 2;

#define INDENT_SIZE 4
#define TREE_START  3
#define TASK_START  (TREE_START + 3)
#define ARROW_START (TREE_START / 2)

	int x           = -(int)this->scroll_x;
	int tree_start  = x + TREE_START;
	int task_start  = x + TASK_START;
	int arrow_start = x + ARROW_START;

	if (this->scroll_y >= 1) {
		tui_goto(x + 7 + this->lines[this->scroll_y].indent * INDENT_SIZE, 0);
		tui_add_str("...");
	}

	for (size_t i = this->scroll_y; i < this->size; ++ i) {
		int  y      = (int)i + 1 - this->scroll_y;
		bool active = this->cur == i;

		tui_set_attr(ATTR_NONE);
		tui_set_color(colorscheme[C_FG], colorscheme[C_BG]);

		if (y + 1 >= h) {
			int indent = this->lines[this->scroll_y + h - 3].indent * INDENT_SIZE;
			tui_goto(task_start + indent, h - 1);
			tui_add_str("...");
			break;
		}

		if (active) {
			tui_set_color(colorscheme[C_ACTIVE_FG],
			              colorscheme[C_ACTIVE_BG]);
			tui_goto(0, y);
			tui_add_hline(' ', w);

			if (flags[F_TREE_ARROW]) {
				tui_goto(arrow_start, y);
				tui_set_attr(ATTR_BOLD);
				tui_add_rune('>');
			}
		}

		tui_goto(tree_start + this->lines[i].indent * INDENT_SIZE, y);

#undef INDENT_SIZE

		switch (this->lines[i].node->type) {
		case NODE_TASK:  tview_render_task (this, AS_TASK (this->lines[i].node), active); break;
		case NODE_GROUP: tview_render_group(this, AS_GROUP(this->lines[i].node), active); break;

		default: UNREACHABLE();
		}
	}
}

static void tview_render_ruler(tview_t *this, int w, int h) {
	tui_set_attr(ATTR_NONE);
	tui_set_color(colorscheme[C_FG], colorscheme[C_BG]);
	tui_clear(' ');

	for (size_t i = this->scroll_y; i < this->size; ++ i) {
		int  y      = (int)i + 1 - this->scroll_y;
		bool active = this->cur == i;

		if (y + 1 >= h)
			break;

		if (active)
			tui_set_color(colorscheme[C_RULER_ACTIVE], colorscheme[C_ACTIVE_BG]);
		else
			tui_set_color(colorscheme[C_RULER_INACTIVE], colorscheme[C_BG]);

		tui_goto(0, y);

		if (active) {
			tui_add_hline(' ', w);
			tui_left(w);
		}

		int ruler_n = y - 1;
		int size    = ruler_n == 0;
		for (int n = ruler_n; n > 0; n /= 10)
			++ size;

		for (int i = w - size; i --> 1;)
			tui_add_rune(' ');

		tui_add_strf("%i", ruler_n);
	}
}

static void tview_render_scrollbar(tview_t *this, int h) {
	tui_set_attr(ATTR_NONE);
	tui_set_bg(colorscheme[C_SCROLLBAR_BG]);
	tui_clear(' ');

	float pos  = (float)this->scroll_y / this->size * h;
	float size = (float)h / this->size * h;

	tui_goto(0, pos);
	tui_set_bg(colorscheme[C_SCROLLBAR_FG]);
	tui_add_vline(' ', size);
}

void tview_render(tview_t *this, int x, int y, int w, int h) {
	this->page_h = h - 2;

	if (this->size == 0) {
		tui_viewport(x, y, w, h);

		tui_set_attr(ATTR_NONE);
		tui_set_color(colorscheme[C_FG], colorscheme[C_BG]);
		tui_clear(' ');

		const char *str = "This TODO is empty";

		tui_goto(w / 2 - strlen(str) / 2, h / 2);
		tui_add_str(str);
	} else {
		int ruler_w = 1;
		for (int n = h - 2; n > 0; n /= 10)
			++ ruler_w;

		int tree_w = w - 1 - ruler_w;

		tui_viewport(x + ruler_w, y, tree_w, h);
		tview_render_tree(this, tree_w, h);

		tui_viewport(x, y, ruler_w, h);
		tview_render_ruler(this, ruler_w, h);

		tui_viewport(x + w - 1, y, 1, h);
		tview_render_scrollbar(this, h);
	}

	tui_reset_viewport();
}

static void tview_up(tview_t *this) {
	if (this->cur > 0)
		-- this->cur;
}

static void tview_down(tview_t *this) {
	if (this->cur + 1 < this->size)
		++ this->cur;
}

static void tview_page_up(tview_t *this) {
	if ((size_t)this->page_h > this->cur)
		this->cur = 0;
	else {
		this->cur -= this->page_h;
		this->scroll_y -= this->page_h;
	}
}

static void tview_page_down(tview_t *this) {
	this->cur += this->page_h;
	this->scroll_y += this->page_h;

	if (this->cur >= this->size)
		this->cur = this->size - 1;
}

static void tview_top(tview_t *this) {
	this->cur = 0;
}

static void tview_bottom(tview_t *this) {
	this->cur = this->size - 1;
}

static void tview_left(tview_t *this) {
	if (this->scroll_x > 0)
		-- this->scroll_x;
}

static void tview_right(tview_t *this) {
	++ this->scroll_x;
}

static void tview_full_left(tview_t *this) {
	this->scroll_x = 0;
}

static void tview_next_match(tview_t *this) {
	if (this->matches_count == 0)
		return;

	if (++ this->match_pos >= this->matches_count)
		this->match_pos = 0;

	this->cur = this->matches[this->match_pos];
}

static void tview_prev_match(tview_t *this) {
	if (this->matches_count == 0)
		return;

	if (this->match_pos == 0)
		this->match_pos = this->matches_count - 1;
	else
		-- this->match_pos;

	this->cur = this->matches[this->match_pos];
}

static void tview_expand_group(tview_t *this, size_t pos) {
	assert(this->lines[pos].node->type == NODE_GROUP);

	group_t *group = AS_GROUP(this->lines[pos].node);
	if (group->children == NULL)
		return;

	size_t        size = this->size - pos - 1;
	tview_line_t *tmp  = NULL;
	if (size != 0) {
		tmp = (tview_line_t*)xalloc(size * sizeof(tview_line_t));
		memcpy(tmp, this->lines + pos + 1, size * sizeof(tview_line_t));
		this->size = pos + 1;
	}

	tview_add_children(this, this->lines[pos].indent + 1, group->children);

	if (size != 0) {
		size_t prev = this->size;
		this->size += size;
		while (this->size > this->cap)
			this->cap *= 2;

		this->lines = (tview_line_t*)xrealloc(this->lines, this->cap * sizeof(tview_line_t));
		memcpy(this->lines + prev, tmp, size * sizeof(tview_line_t));

		xfree(tmp);
	}
}

static size_t tview_find_group_size(tview_t *this, size_t pos) {
	assert(this->lines[pos].node->type == NODE_GROUP);

	size_t size = 0, indent = this->lines[pos].indent;
	for (size_t i = pos + 1; i < this->size && this->lines[i].indent > indent; ++ i)
		++ size;

	return size;
}

static void tview_unexpand_group(tview_t *this, size_t pos) {
	size_t size = tview_find_group_size(this, pos);
	if (size == 0)
		return;

	tview_line_t *dest = this->lines + pos + 1;
	tview_line_t *src  = this->lines + pos + 1 + size;
	for (size_t i = 0; i < this->size - pos - 1 - size; ++ i)
		dest[i] = src[i];

	this->size -= size;
}

static void tview_toggle_line(tview_t *this) {
	node_t *node = this->lines[this->cur].node;
	switch (node->type) {
	case NODE_TASK:
		task_toggle_done(AS_TASK(node));
		break;

	case NODE_GROUP: {
		group_toggle_open(AS_GROUP(node));

		if (AS_GROUP(node)->open)
			tview_expand_group(this, this->cur);
		else
			tview_unexpand_group(this, this->cur);
	} break;

	default: UNREACHABLE();
	}
}

static void tview_move_up_from(tview_t *this, size_t pos) {
	if (this->size ++ >= this->cap) {
		this->cap  *= 2;
		this->lines = (tview_line_t*)xrealloc(this->lines, this->cap * sizeof(tview_line_t));
	}

	for (size_t i = this->size - 1; i --> pos;)
		this->lines[i + 1] = this->lines[i];
}

static void tview_prompt_callback(tview_t *this, const char *text) {
	node_t *node = this->lines[this->cur].node;
	switch (node->type) {
	case NODE_TASK:
		xfree(AS_TASK(node)->title);
		AS_TASK(node)->title = xstrdup(text);
		break;

	case NODE_GROUP:
		xfree(AS_GROUP(node)->title);
		AS_GROUP(node)->title = xstrdup(text);
		break;

	default: UNREACHABLE();
	}

	this->prompted = false;
}

static void tview_prompt_hook(tview_t *this, const char *text) {
	this->hooked_text = text;
}

static void tview_prompt_name(tview_t *this) {
	const char *text;

	node_t *node = this->lines[this->cur].node;
	switch (node->type) {
	case NODE_TASK:  text = AS_TASK (node)->title; break;
	case NODE_GROUP: text = AS_GROUP(node)->title; break;

	default: UNREACHABLE();
	}

	this->prompted = true;
	editor_prompt("Enter the title", text, (void*)this,
	              (tcb_t)tview_prompt_callback, (tcb_t)tview_prompt_hook);
}

static void tview_add_sibl(tview_t *this, int type) {
	node_t *new = node_new(type);

	if (this->size == 0) {
		tree_set_root(this->tree, new);

		tview_move_up_from(this, 0);

		this->lines[0].node   = new;
		this->lines[0].indent = 0;
		this->cur = 0;
	} else {
		node_t *node = this->lines[this->cur].node;
		node_add_sibl(node, new);

		size_t pos = this->cur + 1;
		if (node->type == NODE_GROUP)
			pos += tview_find_group_size(this, this->cur);

		tview_move_up_from(this, pos);

		this->lines[pos].node   = new;
		this->lines[pos].indent = this->lines[this->cur].indent;
		this->cur = pos;
	}

	tview_prompt_name(this);
}

static void tview_add_child(tview_t *this, int type) {
	if (this->size == 0) {
		tview_add_sibl(this, type);
		return;
	} else if (this->lines[this->cur].node->type != NODE_GROUP) {
		tview_add_sibl(this, type);
		return;
	}

	node_t  *new   = node_new(type);
	group_t *group = AS_GROUP(this->lines[this->cur].node);
	group_add_child(group, new);

	size_t pos = this->cur + 1;
	tview_move_up_from(this, pos);

	this->lines[pos].node   = new;
	this->lines[pos].indent = this->lines[this->cur].indent + 1;
	this->cur = pos;

	tview_prompt_name(this);
}

static void tview_remove(tview_t *this) {
	if (this->lines[this->cur].node->type == NODE_GROUP)
		tview_unexpand_group(this, this->cur);

	node_remove(this->lines[this->cur].node);

	-- this->size;
	for (size_t i = this->cur; i < this->size; ++ i)
		this->lines[i] = this->lines[i + 1];

	if (this->cur >= this->size && this->size > 0)
		-- this->cur;
}

static void tview_remove_callback(tview_t *this, int answer) {
	if (answer == YES)
		tview_remove(this);
}

static void tview_prompt_remove(tview_t *this) {
	editor_yn("Are you sure you want to remove?", (void*)this, (yncb_t)tview_remove_callback);
}

void tview_input(tview_t *this, int in) {
	if (this->search != NULL) {
		MATCH(int, in,
			TO(keybinds[K_NEXT]) tview_next_match(this);
			TO(keybinds[K_PREV]) tview_prev_match(this);
		);

		return;
	}

	MATCH(int, in,
		TO(keybinds[K_UP])   tview_up(this);
		TO(keybinds[K_PGUP]) tview_page_up(this);
		TO(keybinds[K_TOP])  tview_top    (this);

		TO(keybinds[K_DOWN])   tview_down     (this);
		TO(keybinds[K_PGDOWN]) tview_page_down(this);
		TO(keybinds[K_BOTTOM]) tview_bottom   (this);

		TO(keybinds[K_LEFT])      tview_left     (this);
		TO(keybinds[K_FULL_LEFT]) tview_full_left(this);

		TO(keybinds[K_RIGHT]) tview_right(this);

		TO(keybinds[K_TOGGLE]) tview_toggle_line(this);

		TO(keybinds[K_TASK_S]) tview_add_sibl (this, NODE_TASK);
		TO(keybinds[K_TASK_C]) tview_add_child(this, NODE_TASK);

		TO(keybinds[K_GROUP_S]) tview_add_sibl (this, NODE_GROUP);
		TO(keybinds[K_GROUP_C]) tview_add_child(this, NODE_GROUP);

		TO(keybinds[K_EDIT]) tview_prompt_name(this);

		TO(keybinds[K_REMOVE]) tview_prompt_remove(this);
	);
}
