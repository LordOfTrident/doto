#include "conf.h"

bool flags[FLAGS_COUNT];
int  colorscheme[COLORSCHEME_COUNT];
int  keybinds[KEYBINDS_COUNT];
char keybinds_str[KEYBINDS_COUNT][16];

static const char *conf[] = {
	"/*",
	"\t          ██████╗  ██████╗ ████████╗ ██████╗",
	"\t⬤ ━━━━━   ██╔══██╗██╔═══██╗╚══██╔══╝██╔═══██╗",
	"\t⬤ ━━━━━   ██║  ██║██║   ██║   ██║   ██║   ██║",
	"\t◯ ━━━━━   ██████╔╝╚██████╔╝   ██║   ╚██████╔╝",
	"\t          ╚═════╝  ╚═════╝    ╚═╝    ╚═════╝",
	"\t          Config file",
	"*/",
	"",
	"{",
	"\t\"flags\": {",
	"\t\t\"powerline\":            false, /* Bar powerline separators */",
	"\t\t\"strikethrough\":        true,  /* Task done strikethrough attribute */",
	"\t\t\"caret-pipe\":           false, /* Pipe caret shape */",
	"\t\t\"open-groups-on-start\": false,",
	"\t\t\"tree-arrow\":           true,  /* Arrow cursor on the selected tree line */",
	"\t\t\"no-xclip\":             false, /* Use an internal clipboard instead of xclip */",
	"\t\t\"circle-mark\":          false, /* Use a circle shape for done/undone task symbol */",
	"\t},",
	"",
	"\t\"colorscheme\": {",
	"\t\t\"fg\": \"default\",",
	"\t\t\"bg\": \"default\",",
	"",
	"\t\t\"active-fg\": \"white\",",
	"\t\t\"active-bg\": \"grey\",",
	"",
	"\t\t\"ruler-inactive\": \"grey\",",
	"\t\t\"ruler-active\":   \"white\",",
	"",
	"\t\t\"scrollbar-fg\": \"grey\",",
	"\t\t\"scrollbar-bg\": \"black\",",
	"",
	"\t\t\"done-inactive\":   \"green\",",
	"\t\t\"undone-inactive\": \"grey\",",
	"\t\t\"done-active\":     \"green\",",
	"\t\t\"undone-active\":   \"black\",",
	"",
	"\t\t\"open-inactive\":  \"cyan\",",
	"\t\t\"close-inactive\": \"magenta\",",
	"\t\t\"open-active\":    \"cyan\",",
	"\t\t\"close-active\":   \"magenta\",",
	"",
	"\t\t\"match-inactive-fg\": \"bright white\",",
	"\t\t\"match-inactive-bg\": \"magenta\",",
	"\t\t\"match-active-fg\":   \"black\",",
	"\t\t\"match-active-bg\":   \"bright white\",",
	"",
	"\t\t\"low\":  \"red\",",
	"\t\t\"mid\":  \"yellow\",",
	"\t\t\"high\": \"green\",",
	"\t\t\"perc\": \"black\",",
	"",
	"\t\t\"bar-fg\": \"bright white\",",
	"\t\t\"bar-bg\": \"grey\",",
	"\t\t\"sep\":    \"black\",",
	"",
	"\t\t\"prompt-fg\":  \"white\",",
	"\t\t\"prompt-bg\":  \"black\",",
	"\t\t\"prompt-err\": \"bright red\",",
	"",
	"\t\t\"matches-fg\":      \"bright white\",",
	"\t\t\"matches-bg\":      \"grey\",",
	"\t\t\"no-matches-fg\": \"bright white\",",
	"\t\t\"no-matches-bg\": \"red\",",
	"",
	"\t\t\"select-fg\": \"black\",",
	"\t\t\"select-bg\": \"white\",",
	"",
	"\t\t\"path-fg\": \"bright white\",",
	"\t\t\"path-bg\": \"blue\",",
	"\t\t\"norm-fg\": \"black\",",
	"\t\t\"norm-bg\": \"white\",",
	"\t\t\"help-fg\": \"black\",",
	"\t\t\"help-bg\": \"cyan\",",
	"\t\t\"jump-fg\": \"bright white\",",
	"\t\t\"jump-bg\": \"magenta\",",
	"\t\t\"find-fg\": \"black\",",
	"\t\t\"find-bg\": \"green\",",
	"\t\t\"cs-fg\":   \"bright white\",",
	"\t\t\"cs-bg\":   \"grey\",",
	"\t\t\"add-fg\":  \"black\",",
	"\t\t\"add-bg\":  \"green\",",
	"",
	"\t\t\"title-fg\":  \"white\",",
	"\t\t\"title-bg\":  \"black\",",
	"\t\t\"y-fg\":      \"black\",",
	"\t\t\"y-bg\":      \"green\",",
	"\t\t\"n-fg\":      \"black\",",
	"\t\t\"n-bg\":      \"red\",",
	"\t\t\"cancel-fg\": \"black\",",
	"\t\t\"cancel-bg\": \"cyan\",",
	"\t},",
	"",
	"\t\"keybinds\": {",
	"\t\t/* Normal mode */",
	"\t\t\"quit\":          \"q\",",
	"\t\t\"reload-config\": \"ctrl+r\",",
	"\t\t\"norm\":          \"escape\",",
	"\t\t\"help\":          \"?\",",
	"\t\t\"find-ci\":       \"f\",",
	"\t\t\"find-cs\":       \"shift+f\",",
	"\t\t\"jump\":          \"j\",",
	"",
	"\t\t\"done\": \"enter\",",
	"",
	"\t\t\"up\":        \"arrow up\",",
	"\t\t\"page-up\":   \"page up\",",
	"\t\t\"top\":       \"ctrl+arrow up\",",
	"\t\t\"down\":      \"arrow down\",",
	"\t\t\"page-down\": \"page down\",",
	"\t\t\"bottom\":    \"ctrl+arrow down\",",
	"\t\t\"left\":      \"arrow left\",",
	"\t\t\"full-left\": \"ctrl+arrow left\",",
	"\t\t\"right\":     \"arrow right\",",
	"",
	"\t\t\"task-sibling\": \"t\",",
	"\t\t\"task-child\":   \"shift+t\",",
	"",
	"\t\t\"group-sibling\": \"g\",",
	"\t\t\"group-child\":   \"shift+g\",",
	"",
	"\t\t\"edit-title\":   \"e\",",
	"",
	"\t\t\"remove\": \"r\",",
	"\t\t\"toggle\": \"space\", /* Mark a task, open/close a group */",
	"",
	"\t\t\"history-up\":   \"arrow up\",",
	"\t\t\"history-down\": \"arrow down\",",
	"",
	"\t\t\"next-match\": \"ctrl+n\",",
	"\t\t\"prev-match\": \"ctrl+p\",",
	"\t}",
	"}",
};

static const char *flags_keys[COLORSCHEME_COUNT] = {
	[F_POWERLINE]            = "powerline",
	[F_STRIKETHROUGH]        = "strikethrough",
	[F_CARET_PIPE]           = "caret-pipe",
	[F_OPEN_GROUPS_ON_START] = "open-groups-on-start",
	[F_TREE_ARROW]           = "tree-arrow",
	[F_NO_XCLIP]             = "no-xclip",
	[F_CIRCLE_MARK]          = "circle-mark",
};

static const char *colorscheme_keys[COLORSCHEME_COUNT] = {
	[C_FG] = "fg",
	[C_BG] = "bg",

	[C_ACTIVE_FG] = "active-fg",
	[C_ACTIVE_BG] = "active-bg",

	[C_RULER_INACTIVE] = "ruler-inactive",
	[C_RULER_ACTIVE]   = "ruler-active",

	[C_SCROLLBAR_FG] = "scrollbar-fg",
	[C_SCROLLBAR_BG] = "scrollbar-bg",

	[C_DONE_INACTIVE]   = "done-inactive",
	[C_DONE_ACTIVE]     = "done-active",
	[C_UNDONE_INACTIVE] = "undone-inactive",
	[C_UNDONE_ACTIVE]   = "undone-active",

	[C_OPEN_INACTIVE]  = "open-inactive",
	[C_OPEN_ACTIVE]    = "open-active",
	[C_CLOSE_INACTIVE] = "close-inactive",
	[C_CLOSE_ACTIVE]   = "close-active",

	[C_MATCH_INACTIVE_FG] = "match-inactive-fg",
	[C_MATCH_INACTIVE_BG] = "match-inactive-bg",
	[C_MATCH_ACTIVE_FG]   = "match-active-fg",
	[C_MATCH_ACTIVE_BG]   = "match-active-bg",

	[C_LOW]  = "low",
	[C_MID]  = "mid",
	[C_HIGH] = "high",
	[C_PERC] = "perc",

	[C_BAR_FG] = "bar-fg",
	[C_BAR_BG] = "bar-bg",
	[C_SEP]    = "sep",

	[C_PROMPT_FG]  = "prompt-fg",
	[C_PROMPT_BG]  = "prompt-bg",
	[C_PROMPT_ERR] = "prompt-err",

	[C_MATCHES_FG]    = "matches-fg",
	[C_MATCHES_BG]    = "matches-bg",
	[C_NO_MATCHES_FG] = "no-matches-fg",
	[C_NO_MATCHES_BG] = "no-matches-bg",

	[C_SELECT_FG] = "select-fg",
	[C_SELECT_BG] = "select-bg",

	[C_PATH_FG]   = "path-fg",
	[C_PATH_BG]   = "path-bg",
	[C_NORM_FG]   = "norm-fg",
	[C_NORM_BG]   = "norm-bg",
	[C_HELP_FG]   = "help-fg",
	[C_HELP_BG]   = "help-bg",
	[C_JUMP_FG]   = "jump-fg",
	[C_JUMP_BG]   = "jump-bg",
	[C_FIND_FG]   = "find-fg",
	[C_FIND_BG]   = "find-bg",
	[C_CS_FG]     = "cs-fg",
	[C_CS_BG]     = "cs-bg",
	[C_ADD_FG]    = "add-fg",
	[C_ADD_BG]    = "add-bg",
	[C_Y_FG]      = "y-fg",
	[C_Y_BG]      = "y-bg",
	[C_N_FG]      = "n-fg",
	[C_N_BG]      = "n-bg",
	[C_CANCEL_FG] = "cancel-fg",
	[C_CANCEL_BG] = "cancel-bg",
	[C_TITLE_FG]  = "title-fg",
	[C_TITLE_BG]  = "title-bg",
};

static const char *keybinds_keys[KEYBINDS_COUNT] = {
	[K_QUIT]   = "quit",
	[K_NORM]   = "norm",
	[K_HELP]   = "help",
	[K_RELOAD] = "reload-config",
	[K_FINDCI] = "find-ci",
	[K_FINDCS] = "find-cs",
	[K_JUMP]   = "jump",

	[K_DONE] = "done",

	[K_UP]        = "up",
	[K_PGUP]      = "page-up",
	[K_TOP]       = "top",
	[K_DOWN]      = "down",
	[K_PGDOWN]    = "page-down",
	[K_BOTTOM]    = "bottom",
	[K_LEFT]      = "left",
	[K_FULL_LEFT] = "full-left",
	[K_RIGHT]     = "right",

	[K_TASK_S]  = "task-sibling",
	[K_TASK_C]  = "task-child",
	[K_GROUP_S] = "group-sibling",
	[K_GROUP_C] = "group-child",
	[K_EDIT]    = "edit-title",

	[K_REMOVE] = "remove",
	[K_TOGGLE] = "toggle",

	[K_HIST_UP]   = "history-up",
	[K_HIST_DOWN] = "history-down",

	[K_NEXT] = "next-match",
	[K_PREV] = "prev-match",
};

static const char *color_str[] = {
	[COLOR_BLACK]   = "black",
	[COLOR_RED]     = "red",
	[COLOR_GREEN]   = "green",
	[COLOR_YELLOW]  = "yellow",
	[COLOR_BLUE]    = "blue",
	[COLOR_MAGENTA] = "magenta",
	[COLOR_CYAN]    = "cyan",
	[COLOR_WHITE]   = "white",

	[COLOR_GREY]     = "grey",
	[COLOR_BRED]     = "bright red",
	[COLOR_BGREEN]   = "bright green",
	[COLOR_BYELLOW]  = "bright yellow",
	[COLOR_BBLUE]    = "bright blue",
	[COLOR_BMAGENTA] = "bright magenta",
	[COLOR_BCYAN]    = "bright cyan",
	[COLOR_BWHITE]   = "bright white",

	[COLOR_DEFAULT] = "default",
};

static const char *key_str[] = {
	[KEY_SPACE]     = "space",
	[KEY_ENTER]     = "enter",
	[KEY_TAB]       = "tab",
	[KEY_BACKSPACE] = "backspace",
	[KEY_ESC]       = "escape",

	[KEY_PAGE_UP]   = "page up",
	[KEY_PAGE_DOWN] = "page down",

	[KEY_ARROW_UP]    = "arrow up",
	[KEY_ARROW_DOWN]  = "arrow down",
	[KEY_ARROW_RIGHT] = "arrow right",
	[KEY_ARROW_LEFT]  = "arrow left",

	[KEY_CTRL_ARROW_UP]    = "ctrl+arrow up",
	[KEY_CTRL_ARROW_DOWN]  = "ctrl+arrow down",
	[KEY_CTRL_ARROW_RIGHT] = "ctrl+arrow right",
	[KEY_CTRL_ARROW_LEFT]  = "ctrl+arrow left",

	[KEY_SHIFT_ARROW_UP]    = "shift+arrow up",
	[KEY_SHIFT_ARROW_DOWN]  = "shift+arrow down",
	[KEY_SHIFT_ARROW_RIGHT] = "shift+arrow right",
	[KEY_SHIFT_ARROW_LEFT]  = "shift+arrow left",
};

static int str_to_color(const char *str) {
	for (size_t i = 0; i < ARRAY_LEN(color_str); ++ i) {
		if (color_str[i] == NULL)
			continue;

		if (strcmp(color_str[i], str) == 0)
			return i;
	}

	return -1;
}

typedef struct {
	const char *prefix;
	size_t      len;

	int (*func)(char);
} key_modif_t;

#define MODIF_FUNC(NAME, CH, EXPR) \
	static int NAME(char CH) {     \
		return EXPR;               \
	}

MODIF_FUNC(modif_ctrl,  ch, KEY_CTRL(ch))
MODIF_FUNC(modif_shift, ch, toupper (ch))
MODIF_FUNC(modif_alt,   ch, KEY_ALT (ch))

static key_modif_t key_modifs[] = {
	{.prefix = "ctrl",  .len = 4, .func = modif_ctrl},
	{.prefix = "shift", .len = 5, .func = modif_shift},
	{.prefix = "alt",   .len = 3, .func = modif_alt},
};

static int str_to_key(const char *str) {
	for (size_t i = 0; i < ARRAY_LEN(key_str); ++ i) {
		if (key_str[i] == NULL)
			continue;

		if (strcmp(key_str[i], str) == 0)
			return i;
	}

	if (strlen(str) == 1 && isprint(*str) && *str != ' ')
		return *str;

	for (size_t i = 0; i < ARRAY_LEN(key_modifs); ++ i) {
		key_modif_t *modif = &key_modifs[i];
		if (strlen(str) != modif->len + 2)
			continue;

		if (str[modif->len] != '+')
			continue;

		char ch = str[modif->len + 1];
		if (!isalpha(ch) || !islower(ch))
			continue;

		return modif->func(ch);
	}

	return -1;
}

static json_t *conf_load_group(json_t *j, const char *path, const char *key) {
	json_t *group = json_obj_at(j, key);
	if (group == NULL)
		DIE("%s: Section \"%s\" is missing", path, key);
	else if (group->type != JSON_OBJ)
		DIE("%s: Section \"%s\" is not an object", path, key);

	return group;
}

static void conf_load_flags(json_t *j, const char *path) {
	json_t *group = conf_load_group(j, path, "flags");

	for (int i = 0; i < FLAGS_COUNT; ++ i) {
		assert(flags_keys[i] != NULL);

		json_t *val = json_obj_at(group, flags_keys[i]);
		if (val == NULL)
			DIE("%s: Flag \"%s\" is missing", path, flags_keys[i]);
		else if (val->type != JSON_BOOL)
			DIE("%s: Flag \"%s\" expected to be a 'true' or 'false' boolean", path, flags_keys[i]);

		flags[i] = val->as.bool_;
	}
}

static void conf_load_colorscheme(json_t *j, const char *path) {
	json_t *group = conf_load_group(j, path, "colorscheme");

	for (int i = 0; i < COLORSCHEME_COUNT; ++ i) {
		assert(colorscheme_keys[i] != NULL);

		json_t *val = json_obj_at(group, colorscheme_keys[i]);
		if (val == NULL)
			DIE("%s: Color \"%s\" is missing", path, colorscheme_keys[i]);
		else if (val->type != JSON_STR)
			DIE("%s: Color \"%s\" expected to be a color", path, colorscheme_keys[i]);

		int color = str_to_color(val->as.str.buf);
		if (color == -1)
			DIE("%s: Color \"%s\" got an unknown color \"%s\"",
			    path, colorscheme_keys[i], val->as.str.buf);

		colorscheme[i] = color;
	}
}

static void conf_load_keybinds(json_t *j, const char *path) {
	json_t *group = conf_load_group(j, path, "keybinds");

	for (int i = 0; i < KEYBINDS_COUNT; ++ i) {
		assert(colorscheme_keys[i] != NULL);

		json_t *val = json_obj_at(group, keybinds_keys[i]);
		if (val == NULL)
			DIE("%s: Keybind \"%s\" is missing", path, keybinds_keys[i]);
		else if (val->type != JSON_STR && val->type != JSON_NULL)
			DIE("%s: Keybind \"%s\" expected to be a key or null", path, keybinds_keys[i]);

		if (val->type == JSON_NULL) {
			if (i == K_QUIT)
				DIE("%s: Cannot set keybind \"%s\" to null", path, keybinds_keys[i]);

			keybinds[i] = -1;
			strcpy(keybinds_str[i], "none");
		} else {
			int key = str_to_key(val->as.str.buf);
			if (key == -1)
				DIE("%s: Keybind \"%s\" got an unknown key \"%s\"",
				    path, keybinds_keys[i], val->as.str.buf);

			keybinds[i] = key;
			strcpy(keybinds_str[i], val->as.str.buf);
		}
	}
}

void conf_load(void) {
	assert(CONF_PATH[0] == '~');

	const char *home = getenv("HOME");
	if (home == NULL)
		DIE("Failed to read environment variable \"HOME\"");

#define CONF_PATH_CAP 1024

	assert(strlen(home) + strlen(CONF_PATH) <= CONF_PATH_CAP);

	char path[CONF_PATH_CAP];
	strcpy(path, home);
	strcat(path, &CONF_PATH[1]);

#undef CONF_PATH_CAP

	if (access(path, F_OK) != 0) {
		FILE *file = fopen(path, "w");
		if (file == NULL)
			DIE("Failed to create config file \"%s\"", path);

		for (size_t i = 0; i < ARRAY_LEN(conf); ++ i)
			fprintf(file, "%s\n", conf[i]);

		fclose(file);
	}

	size_t  row, col;
	json_t *j = json_from_file(path, &row, &col);
	if (j == NULL) {
		assert(noch_get_err() != NOCH_ERR_FOPEN);

		if (noch_get_err() == NOCH_ERR_PARSER)
			DIE("%s:%zu:%zu: %s", path, row, col, noch_get_err_msg());
		else
			DIE("%s: %s", path, noch_get_err_msg());
	}

	conf_load_flags(j, path);
	conf_load_colorscheme(j, path);
	conf_load_keybinds(j, path);

	json_destroy(j);
}
