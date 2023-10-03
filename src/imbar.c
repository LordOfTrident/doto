#include "imbar.h"

static struct {
	int w;
	int fg, bg, sep;

	int  side;
	bool sections, section;
	int  sections_scr_len, sections_count;
	int  prev_bg;
} state;

static void (*tui_xadd_rune)(rune_t);
static void (*tui_xadd_str) (const char*);

void imbar_begin(int x, int y, int w, int fg, int bg, int sep) {
	assert(w > 0);

	ZERO_STRUCT(&state);
	state.w   = w;
	state.fg  = fg;
	state.bg  = bg;
	state.sep = sep;

	tui_viewport(x, y, state.w, 1);

	tui_set_bg(state.bg);
	tui_clear(' ');
}

void imbar_end(void) {
	tui_reset_viewport();
}

void imbar_sections_begin(int side) {
	assert(!state.sections);
	assert(side == IMBAR_RIGHT || side == IMBAR_LEFT);

	state.sections         = true;
	state.side             = side;
	state.sections_count   = 0;
	state.sections_scr_len = 0;

	tui_goto(state.side == IMBAR_RIGHT? state.w - 1: 0, 0);

	tui_xadd_rune = state.side == IMBAR_RIGHT? tui_radd_rune : tui_add_rune;
	tui_xadd_str  = state.side == IMBAR_RIGHT? tui_radd_str  : tui_add_str;
}

static int imbar_section_add_sep(int fg, int bg) {
	if (fg == -1)
		fg = state.fg;

	if (bg == -1)
		bg = state.bg;

	if (state.prev_bg == bg) {
		tui_set_fg(bg == state.bg? state.sep : fg);
		if (state.side == IMBAR_RIGHT)
			tui_radd_str(flags[F_POWERLINE]? " \uE0B3 " : " | ");
		else
			tui_add_str(flags[F_POWERLINE]? " \uE0B1 " : " | ");

		return 3;
	} else {
		tui_set_bg(state.prev_bg);
		tui_xadd_rune(' ');

		if (flags[F_POWERLINE]) {
			tui_set_color(state.prev_bg, bg);
			tui_xadd_rune(state.side == IMBAR_RIGHT? L'\uE0B2' : L'\uE0B0');
		}

		tui_set_bg(bg);
		tui_xadd_rune(' ');
		return 2 + flags[F_POWERLINE];
	}
}

int imbar_sections_end(void) {
	assert(state.sections);

	state.sections = false;

	if (state.sections_count > 0 && state.prev_bg != state.bg)
		state.sections_scr_len += imbar_section_add_sep(0, state.bg);

	return state.sections_scr_len;
}

void imbar_section_begin(int fg, int bg) {
	assert(state.sections);
	assert(!state.section);

	if (fg == -1)
		fg = state.fg;

	if (bg == -1)
		bg = state.bg;

	if (state.sections_count > 0)
		state.sections_scr_len += imbar_section_add_sep(fg, bg);

	state.section = true;
	state.prev_bg = bg;

	tui_set_color(fg, bg);

	if (state.sections_count ++ == 0)
		tui_xadd_rune(' ');
}

void imbar_section_end(void) {
	assert(state.sections);
	assert(state.section);

	state.section = false;
}

void imbar_section(int fg, int bg, const char *str) {
	imbar_section_begin(fg, bg);
	imbar_section_add_str(str);
	imbar_section_end();
}

void imbar_sectionf(int fg, int bg, const char *fmt, ...) {
	char    str[1024];
	va_list args;

	va_start(args, fmt);
	vsnprintf(str, sizeof(str), fmt, args);
	va_end(args);

	imbar_section_begin(fg, bg);
	imbar_section_add_str(str);
	imbar_section_end();
}

void imbar_section_add_str(const char *str) {
	assert(state.sections);
	assert(state.section);

	state.sections_scr_len += strlen(str);
	tui_xadd_str(str);
}

void imbar_section_add_strf(const char *fmt, ...) {
	assert(state.sections);
	assert(state.section);

	char    str[1024];
	va_list args;

	va_start(args, fmt);
	vsnprintf(str, sizeof(str), fmt, args);
	va_end(args);

	state.sections_scr_len += strlen(str);
	tui_xadd_str(str);
}
