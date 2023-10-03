#ifndef TUI_H_HEADER_GUARD
#define TUI_H_HEADER_GUARD

#include <stdio.h>   /* fputs, printf, fflush, fputwc */
#include <stdint.h>  /* uint8_t */
#include <string.h>  /* memcpy, strcmp */
#include <stdbool.h> /* bool, true, false */
#include <wchar.h>   /* wchar_t, wcslen */
#include <stdarg.h>  /* va_list, va_start, va_end, vsnprintf */
#include <assert.h>  /* assert */
#include <locale.h>  /* setlocale, LC_TYPE */
#include <stdlib.h>  /* getenv */

#include <signal.h>  /* struct sigaction, sigaction, sigemptyset, sigaddset, SIGWINCH */

#include <termios.h>    /* tcgetattr, tcsetattr, struct termios */
#include <unistd.h>     /* read, STDIN_FILENO */
#include <sys/ioctl.h>  /* ioctl */
#include <sys/select.h> /* select, struct timeval */

#include <noch/utf8.h>

#include "common.h"

#define FPS 60

enum {
	COLOR_BLACK = 0,
	COLOR_RED,
	COLOR_GREEN,
	COLOR_YELLOW,
	COLOR_BLUE,
	COLOR_MAGENTA,
	COLOR_CYAN,
	COLOR_WHITE,

	COLOR_GREY = 60,
	COLOR_BRED,
	COLOR_BGREEN,
	COLOR_BYELLOW,
	COLOR_BBLUE,
	COLOR_BMAGENTA,
	COLOR_BCYAN,
	COLOR_BWHITE,

	COLOR_DEFAULT,
};

enum {
	ATTR_NONE          = 0,
	ATTR_BOLD          = 1 << 0,
	ATTR_DIM           = 1 << 1,
	ATTR_ITALICS       = 1 << 2,
	ATTR_UNDERLINE     = 1 << 3,
	ATTR_BLINK         = 1 << 4,
	ATTR_STRIKETHROUGH = 1 << 5,
};

enum {
	EVENT_ERR  = -1,
	EVENT_NONE = 0,
	EVENT_RESIZED,
	EVENT_KEYPRESS,

	EVENT_UNKNOWN,
};

#define KEY_CTRL(KEY) (KEY & 31)
#define KEY_ALT(KEY)  (KEY + KEY_ALT_START)

enum {
	KEY_SPACE     = 32,
	KEY_ENTER     = 10,
	KEY_TAB       = 9,
	KEY_ESC       = 27,
	KEY_BACKSPACE = 127,
	KEY_RESIZE,

	KEY_PAGE_UP,
	KEY_PAGE_DOWN,

	KEY_ARROW_UP,
	KEY_ARROW_DOWN,
	KEY_ARROW_RIGHT,
	KEY_ARROW_LEFT,

	KEY_CTRL_ARROW_UP,
	KEY_CTRL_ARROW_DOWN,
	KEY_CTRL_ARROW_RIGHT,
	KEY_CTRL_ARROW_LEFT,

	KEY_SHIFT_ARROW_UP,
	KEY_SHIFT_ARROW_DOWN,
	KEY_SHIFT_ARROW_RIGHT,
	KEY_SHIFT_ARROW_LEFT,

	KEY_CTRL_SHIFT_ARROW_UP,
	KEY_CTRL_SHIFT_ARROW_DOWN,
	KEY_CTRL_SHIFT_ARROW_RIGHT,
	KEY_CTRL_SHIFT_ARROW_LEFT,

	KEY_ALT_START,
};

typedef struct {
	int type, key;
} event_t;

typedef struct {
	rune_t  rune;
	uint8_t fg, bg, attr;
} cell_t;

void tui_init(void);
void tui_deinit(void);

int tui_get_w(void);
int tui_get_h(void);

int tui_get_fg(void);
int tui_get_bg(void);

void tui_set_fg(int fg);
void tui_set_bg(int bg);
void tui_set_color(int fg, int bg);

int tui_get_attr(void);

void tui_set_attr   (int attr);
void tui_add_attr   (int attr);
void tui_remove_attr(int attr);

void tui_clear(rune_t r);
void tui_render(void);

void tui_up   (int n);
void tui_down (int n);
void tui_left (int n);
void tui_right(int n);
void tui_goto (int x, int y);

void tui_viewport(int x, int y, int w, int h);
void tui_reset_viewport(void);

void tui_add_rune (rune_t r);
void tui_radd_rune(rune_t r);

void tui_add_hline(rune_t r, int n);
void tui_add_vline(rune_t r, int n);

void tui_add_str  (const char *str);
void tui_radd_str (const char *str);
void tui_add_strf (const char *fmt, ...);
void tui_radd_strf(const char *fmt, ...);

void tui_caret_pipe(bool on);
void tui_caret(bool on);
void tui_caret_goto(int x, int y);

event_t tui_event(void);

#endif
