#ifndef CONF_H_HEADER_GUARD
#define CONF_H_HEADER_GUARD

#include <stdio.h>  /* fopen, fclose, FILE, fprintf */
#include <stdlib.h> /* getenv */
#include <assert.h> /* assert */
#include <ctype.h>  /* isprint, isalpha, islower */
#include <string.h> /* strlen, strcpy, strcat, strcmp, strncmp */
#include <unistd.h> /* access */

#include <noch/json.h>

#include "common.h"
#include "tui.h"

#define CONF_PATH "~/.config/doto.json"

enum {
	F_POWERLINE = 0,
	F_STRIKETHROUGH,
	F_CARET_PIPE,
	F_OPEN_GROUPS_ON_START,
	F_TREE_ARROW,
	F_NO_XCLIP,
	F_CIRCLE_MARK,

	FLAGS_COUNT,
};

enum {
	C_FG = 0,
	C_BG,

	C_ACTIVE_FG,
	C_ACTIVE_BG,

	C_RULER_INACTIVE,
	C_RULER_ACTIVE,

	C_SCROLLBAR_FG,
	C_SCROLLBAR_BG,

	C_DONE_INACTIVE,
	C_DONE_ACTIVE,
	C_UNDONE_INACTIVE,
	C_UNDONE_ACTIVE,

	C_OPEN_INACTIVE,
	C_OPEN_ACTIVE,
	C_CLOSE_INACTIVE,
	C_CLOSE_ACTIVE,

	C_MATCH_INACTIVE_FG,
	C_MATCH_INACTIVE_BG,
	C_MATCH_ACTIVE_BG,
	C_MATCH_ACTIVE_FG,

	C_LOW,
	C_MID,
	C_HIGH,
	C_PERC,

	C_BAR_FG,
	C_BAR_BG,
	C_SEP,

	C_PROMPT_FG,
	C_PROMPT_BG,
	C_PROMPT_ERR,

	C_MATCHES_FG,
	C_MATCHES_BG,
	C_NO_MATCHES_FG,
	C_NO_MATCHES_BG,

	C_SELECT_FG,
	C_SELECT_BG,

	C_PATH_FG,
	C_PATH_BG,
	C_NORM_FG,
	C_NORM_BG,
	C_HELP_FG,
	C_HELP_BG,
	C_JUMP_FG,
	C_JUMP_BG,
	C_FIND_FG,
	C_FIND_BG,
	C_CS_FG,
	C_CS_BG,
	C_ADD_FG,
	C_ADD_BG,
	C_Y_FG,
	C_Y_BG,
	C_N_FG,
	C_N_BG,
	C_CANCEL_FG,
	C_CANCEL_BG,
	C_TITLE_FG,
	C_TITLE_BG,

	COLORSCHEME_COUNT,
};

enum {
	K_QUIT = 0,
	K_NORM,
	K_HELP,
	K_RELOAD,
	K_FINDCI,
	K_FINDCS,
	K_JUMP,

	K_DONE,

	K_UP,
	K_PGUP,
	K_TOP,
	K_DOWN,
	K_PGDOWN,
	K_BOTTOM,
	K_LEFT,
	K_FULL_LEFT,
	K_RIGHT,

	K_TASK_S,
	K_TASK_C,
	K_GROUP_S,
	K_GROUP_C,
	K_EDIT,

	K_REMOVE,
	K_TOGGLE,

	K_HIST_UP,
	K_HIST_DOWN,

	K_NEXT,
	K_PREV,

	KEYBINDS_COUNT,
};

extern bool flags[FLAGS_COUNT];
extern int  colorscheme[COLORSCHEME_COUNT];
extern int  keybinds[KEYBINDS_COUNT];
extern char keybinds_str[KEYBINDS_COUNT][16];

void conf_load(void);

#endif
