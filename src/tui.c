#include "tui.h"

static struct {
	/* Original terminal attributes */
	struct termios tinfo;

	/* Screen buffer and size */
	cell_t *scr, *scr_prev;
	int     scr_w, scr_h, scr_size;

	/* Current attribute to use when drawing */
	int fg, bg, attr;

	/* Drawing cursor */
	int cur_x, cur_y;

	/* Drawing viewport */
	int  vp_x, vp_y, vp_w, vp_h, vp_size;
	bool vp;

	bool resized, is_init, clear_scrollback_on_resize;

	/* Terminal cursor visibility and position */
	bool caret;
	int  caret_x, caret_y;
} state;

static int tui_update_size(void) {
	struct winsize size;
	fflush(stdout);
	if (ioctl(0, TIOCGWINSZ, &size) == -1)
		return -1;

	state.scr_w    = size.ws_col;
	state.scr_h    = size.ws_row;
	state.scr_size = state.scr_w * state.scr_h;

	tui_reset_viewport();
	return 0;
}

static void tui_sigwinch(int sig) {
	UNUSED(sig);

	state.resized = true;
}

void tui_init(void) {
	assert(!state.is_init);
	state.is_init = true;

	set_cleanup(tui_deinit);

	/* I noticed that after resizing in xterm, a scrollback buffer is created and you can
	   scroll it. This prevents that */
	const char *term = getenv("TERM");
	if (term != NULL)
		state.clear_scrollback_on_resize = strcmp(term, "xterm") == 0;

	setlocale(LC_CTYPE, "");
	tcgetattr(STDIN_FILENO, &state.tinfo);

	printf("\x1b[?1049h"); /* Alt screen buffer */
	printf("\x1b[?25l");   /* Cursor off */
	fflush(stdout);

	/* Echo off */
	struct termios tinfo;
	tcgetattr(STDIN_FILENO, &tinfo);
	tinfo.c_lflag    &= ~(ECHO | ICANON | IEXTEN | ISIG);
	tinfo.c_iflag    &= ~(IXON | ICRNL);
	tinfo.c_cc[VMIN]  = 0;
	tinfo.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &tinfo);

	state.vp = false;
	if (tui_update_size() == -1)
		DIE("Failed to find terminal size");

	state.scr      = (cell_t*)xalloc(state.scr_size * sizeof(cell_t));
	state.scr_prev = (cell_t*)xalloc(state.scr_size * sizeof(cell_t));

	for (int i = 0; i < state.scr_size; ++ i) {
		state.scr[i].rune = (rune_t)' ';
		state.scr[i].fg   = COLOR_DEFAULT;
		state.scr[i].bg   = COLOR_DEFAULT;
		state.scr[i].attr = ATTR_NONE;

		ZERO_STRUCT(&state.scr_prev[i]);
	}

    struct sigaction sa = {0};
    sa.sa_handler = tui_sigwinch;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGWINCH);
    sigaction(SIGWINCH, &sa, NULL);

	state.fg = COLOR_DEFAULT;
	state.bg = COLOR_DEFAULT;
}

void tui_deinit(void) {
	assert(state.is_init);
	state.is_init = false;

	set_cleanup(NULL);

	printf("\x1b[?1049l");
	printf("\x1b[?25h");
	printf("\x1b[2 q");
	fflush(stdout);

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &state.tinfo);

	xfree(state.scr);
	xfree(state.scr_prev);
}

int tui_get_w(void) {
	return state.scr_w;
}

int tui_get_h(void) {
	return state.scr_h;
}

int tui_get_fg(void) {
	return state.fg;
}

int tui_get_bg(void) {
	return state.bg;
}

void tui_set_fg(int fg) {
	state.fg = fg;
}

void tui_set_bg(int bg) {
	state.bg = bg;
}

void tui_set_color(int fg, int bg) {
	state.fg = fg;
	state.bg = bg;
}

int tui_get_attr(void) {
	return state.attr;
}

void tui_set_attr(int attr) {
	state.attr = attr;
}

void tui_add_attr(int attr) {
	state.attr |= attr;
}

void tui_remove_attr(int attr) {
	state.attr &= ~attr;
}

static void tui_set_rune(rune_t r) {
	if (state.cur_x >= state.vp_w || state.cur_y >= state.vp_h ||
	    state.cur_x <  0          || state.cur_y <  0)
		return;

	int idx = ((state.cur_y + state.vp_y) * state.scr_w) + state.cur_x + state.vp_x;
	state.scr[idx].rune = r;
	state.scr[idx].fg   = state.fg;
	state.scr[idx].bg   = state.bg;
	state.scr[idx].attr = state.attr;
}

void tui_clear(rune_t r) {
	if (!state.vp) {
		for (int i = 0; i < state.scr_size; ++ i) {
			state.scr[i].rune = r;
			state.scr[i].fg   = state.fg;
			state.scr[i].bg   = state.bg;
			state.scr[i].attr = state.attr;
		}
	} else {
		for (state.cur_y = 0; state.cur_y < state.vp_h; ++ state.cur_y) {
			for (state.cur_x = 0; state.cur_x < state.vp_w; ++ state.cur_x)
				tui_set_rune(r);
		}
	}

	state.cur_x = 0;
	state.cur_y = 0;
}

static void print_rune(rune_t r) {
	char buf[sizeof(rune_t) + 1] = {0};
	rune_encode_u8(r, buf);
	fputs(buf, stdout);
}

void tui_render(void) {
	if (state.caret)
		printf("\x1b[?25l");

	printf("\x1b[0m");
	if (state.vp)
		tui_reset_viewport();

	int prev_attr = ATTR_NONE;
	int prev_fg   = COLOR_DEFAULT, prev_bg = COLOR_DEFAULT;

	bool force_jump = true;

	int x = 1, y = 1;
	for (int i = 0; i < state.scr_size; ++ i) {
		if (!state.resized) {
			bool rune_changed = state.scr[i].rune != state.scr_prev[i].rune;
			bool fg_changed   = state.scr[i].fg   != state.scr_prev[i].fg;
			bool bg_changed   = state.scr[i].bg   != state.scr_prev[i].bg;
			bool attr_changed = state.scr[i].attr != state.scr_prev[i].attr;

			if (!rune_changed && !fg_changed && !bg_changed && !attr_changed) {
				force_jump = true;
				goto skip;
			}
		}

		if (state.scr[i].attr != prev_attr ||
		    state.scr[i].fg   != prev_fg   ||
		    state.scr[i].bg   != prev_bg) {
			printf("\x1b[0;%i;%im", (int)state.scr[i].fg + 30, (int)state.scr[i].bg + 40);

			if (state.scr[i].attr != ATTR_NONE) {
				if (state.scr[i].attr & ATTR_BOLD)          printf("\x1b[1m");
				if (state.scr[i].attr & ATTR_DIM)           printf("\x1b[2m");
				if (state.scr[i].attr & ATTR_ITALICS)       printf("\x1b[3m");
				if (state.scr[i].attr & ATTR_UNDERLINE)     printf("\x1b[4m");
				if (state.scr[i].attr & ATTR_BLINK)         printf("\x1b[5m");
				if (state.scr[i].attr & ATTR_STRIKETHROUGH) printf("\x1b[9m");
			}

			prev_attr = state.scr[i].attr;
			prev_fg   = state.scr[i].fg;
			prev_bg   = state.scr[i].bg;
		}

		if (force_jump) {
			printf("\x1b[%i;%iH", y, x);
			force_jump = false;
		}

		print_rune(state.scr[i].rune);

		/* Rendering breaks with unicode characters that take up 2 terminal screen characters.
		   Jumping prevents that, but also slows down the rendering, so we only jump when the
		   previous character was unicode, or if it was not rendered. */
		if (!rune_is_ascii(state.scr[i].rune))
			force_jump = true;

	skip:
		++ x;
		if ((x - 1) % state.scr_w == 0) {
			x = 1;
			++ y;
		}
	}

	if (state.caret) {
		printf("\x1b[%i;%iH", state.caret_y + 1, state.caret_x + 1);
		printf("\x1b[?25h");
	}

	fflush(stdout);

	if (state.resized)
		state.resized = false;

	state.cur_x = 0;
	state.cur_y = 0;
	memcpy(state.scr_prev, state.scr, state.scr_size * sizeof(cell_t));
}

void tui_up(int n) {
	state.cur_y -= n;
}

void tui_down(int n) {
	state.cur_y += n;
}

void tui_left(int n) {
	state.cur_x -= n;
}

void tui_right(int n) {
	state.cur_x += n;
}

void tui_goto(int x, int y) {
	state.cur_x = x;
	state.cur_y = y;
}

void tui_viewport(int x, int y, int w, int h) {
	assert(x >= 0 && y >= 0);
	assert(w >  0 && h >  0);

	assert(x + w <= state.scr_w);
	assert(y + h <= state.scr_h);

	state.vp = true;

	state.vp_x    = x;
	state.vp_y    = y;
	state.vp_w    = w;
	state.vp_h    = h;
	state.vp_size = w * h;

	state.cur_x = 0;
	state.cur_y = 0;
}

void tui_reset_viewport(void) {
	state.vp = false;

	state.vp_x    = 0;
	state.vp_y    = 0;
	state.vp_w    = state.scr_w;
	state.vp_h    = state.scr_h;
	state.vp_size = state.scr_size;
}

void tui_add_rune(rune_t r) {
	tui_set_rune(r);
	++ state.cur_x;
}

void tui_radd_rune(rune_t r) {
	tui_set_rune(r);
	-- state.cur_x;
}

void tui_add_hline(rune_t r, int n) {
	if (n == 0)
		return;

	int move = n > 0? 1 : -1;
	for (int i = 0; i != n; i += move) {
		tui_set_rune(r);
		state.cur_x += move;
		if (state.cur_x >= state.vp_w || state.cur_x < 0)
			break;
	}
}

void tui_add_vline(rune_t r, int n) {
	if (n == 0)
		return;

	int move = n > 0? 1 : -1;
	for (int i = 0; i != n; i += move) {
		tui_set_rune(r);
		state.cur_y += move;
		if (state.cur_y >= state.vp_h || state.cur_y < 0)
			break;
	}
}

void tui_add_str(const char *str) {
	while (*str != '\0') {
		size_t size;
		rune_t r = rune_decode_u8(str, &size);
		tui_add_rune(r);
		str += size;

		if (state.cur_x >= state.scr_w)
			break;
	}
}

void tui_radd_str(const char *str) {
	const char *it = str + u8_str_bytes(str);
	while ((it = u8_str_prev(it, str)) != NULL) {
		rune_t r = rune_decode_u8(it, NULL);
		tui_radd_rune(r);

		if (state.cur_x < 0)
			break;
	}
}

void tui_add_strf(const char *fmt, ...) {
	char    str[1024];
	va_list args;

	va_start(args, fmt);
	vsnprintf(str, sizeof(str), fmt, args);
	va_end(args);

	tui_add_str(str);
}

void tui_radd_strf(const char *fmt, ...) {
	char    str[1024];
	va_list args;

	va_start(args, fmt);
	vsnprintf(str, sizeof(str), fmt, args);
	va_end(args);

	tui_radd_str(str);
}

void msleep(float msec) {
	struct timeval tv;
	tv.tv_sec  = msec / 1000;
	tv.tv_usec = msec * 1000;
	select(0, NULL, NULL, NULL, &tv);
}

static int tui_resize(void) {
	int prev = state.scr_size;
	if (tui_update_size() != 0)
		return -1;

	if (prev != state.scr_size) {
		state.scr      = (cell_t*)xrealloc(state.scr,      state.scr_size * sizeof(cell_t));
		state.scr_prev = (cell_t*)xrealloc(state.scr_prev, state.scr_size * sizeof(cell_t));
	}

	if (state.clear_scrollback_on_resize)
		printf("\x1b[3J");

	return 0;
}

void tui_caret_pipe(bool on) {
	printf(on? "\x1b[6 q" : "\x1b[2 q");
}

void tui_caret(bool on) {
	printf((state.caret = on)? "\x1b[?25h" : "\x1b[?25l");
}

void tui_caret_goto(int x, int y) {
	state.caret_x = state.vp_x + x;
	state.caret_y = state.vp_y + y;
}

const char *key_seqs[] = {
	[KEY_ARROW_UP]    = "[A",
	[KEY_ARROW_DOWN]  = "[B",
	[KEY_ARROW_RIGHT] = "[C",
	[KEY_ARROW_LEFT]  = "[D",

	[KEY_PAGE_UP]   = "[5~",
	[KEY_PAGE_DOWN] = "[6~",

	[KEY_CTRL_ARROW_UP]    = "[1;5A",
	[KEY_CTRL_ARROW_DOWN]  = "[1;5B",
	[KEY_CTRL_ARROW_RIGHT] = "[1;5C",
	[KEY_CTRL_ARROW_LEFT]  = "[1;5D",

	[KEY_SHIFT_ARROW_UP]    = "[1;2A",
	[KEY_SHIFT_ARROW_DOWN]  = "[1;2B",
	[KEY_SHIFT_ARROW_RIGHT] = "[1;2C",
	[KEY_SHIFT_ARROW_LEFT]  = "[1;2D",

	[KEY_CTRL_SHIFT_ARROW_UP]    = "[1;6A",
	[KEY_CTRL_SHIFT_ARROW_DOWN]  = "[1;6B",
	[KEY_CTRL_SHIFT_ARROW_RIGHT] = "[1;6C",
	[KEY_CTRL_SHIFT_ARROW_LEFT]  = "[1;6D",
};

event_t tui_event(void) {
	/* I found out that the read call does not write anything into the passed buffer if there
	   was no key pressed and the IO is non-blocking. So the input has to be initialized to 0
	   to signal no key pressed. */
	event_t evt = {0};

	char in = 0;
	do {
		if (state.resized) {
			if (tui_resize() != -1) {
				evt.type = EVENT_RESIZED;
				return evt;
			}
		}

		if (read(0, &in, 1) == -1) {
			evt.type = EVENT_ERR;
			return evt;
		}

		msleep(1000 / FPS);
	} while (in == 0);

	evt.type = EVENT_KEYPRESS;

	switch (in) {
	case 27: {
#define MAX_SEQ_LEN 16

		char   seq[MAX_SEQ_LEN] = {0}; /* This HAS to be initialized to 0. See the comment above. */
		size_t len = 0;
		do {
			if (read(0, seq + len ++, 1) == -1) {
				evt.type = EVENT_ERR;
				return evt;
			}
		} while (seq[len - 1] != 0 && len < MAX_SEQ_LEN);
		-- len;

#undef MAX_SEQ_LEN

		if (len == 0)
			evt.key = KEY_ESC;
		else if (len == 1)
			evt.key = KEY_ALT(seq[0]);
		else {
			for (size_t i = 0; i < ARRAY_LEN(key_seqs); ++ i) {
				if (key_seqs[i] == NULL)
					continue;

				if (len != strlen(key_seqs[i]))
					continue;

				if (strncmp(key_seqs[i], seq, len) == 0) {
					evt.key = i;
					break;
				}
			}
		}

		if (!evt.key) {
			evt.type = EVENT_UNKNOWN;
			return evt;
		}
	} break;

	case 13: evt.key = KEY_ENTER;     break;
	case 8:  evt.key = KEY_BACKSPACE; break;

	default: evt.key = in;
	}

	return evt;
}
