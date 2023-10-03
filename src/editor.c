#include "editor.h"

static struct {
	bool    tree_loaded;
	tree_t  tree;
	tview_t tview;

	bool cs_find;

	prompt_t prompt;

	bool   prompt_is_uint;
	size_t prompt_uint;

	const char *title;
	void       *data;
	yncb_t      yncb;
	tcb_t       tcb, thook;

	int  mode;
	bool quit;
} state;

static const char *mode_str[] = {
	[MODE_NORM] = "NORM",
	[MODE_FIND] = "FIND",
	[MODE_JUMP] = "JUMP",
	[MODE_HELP] = "HELP",
};

static void editor_autosave(void) {
	if (state.tree_loaded)
		tree_save(&state.tree);
}

static void editor_init(const char *path) {
	conf_load();

	tree_load(&state.tree, path);
	state.tree_loaded = true;

	printf("\x1b]0;" TITLE " - %s\x07", state.tree.path); /* Set the terminal title */
	setlocale(LC_CTYPE, "");

	clipboard_init();
	tui_init();

	tui_caret_pipe(flags[F_CARET_PIPE]);

	tview_populate(&state.tview, &state.tree);

	prompt_init(&state.prompt);
}

static void editor_deinit(void) {
	editor_autosave();

	clipboard_deinit();

	tview_clean(&state.tview);
	prompt_deinit(&state.prompt);

	if (state.tree_loaded) {
		tree_clean(&state.tree);
		state.tree_loaded = false;
	}

	tui_deinit();
}

static int get_progress_color(float prog) {
	if (prog == 0)
		return colorscheme[C_LOW];
	else if (prog == 1)
		return colorscheme[C_HIGH];
	else
		return colorscheme[C_MID];
}

static void editor_render_bar_norm(void) {
	imbar_begin(0, tui_get_h() - 1, tui_get_w(),
	            colorscheme[C_BAR_FG], colorscheme[C_BAR_BG], colorscheme[C_SEP]); {
		imbar_sections_begin(IMBAR_RIGHT); {
			int color = get_progress_color(state.tree.prog);

			if (state.tree.prog == -1)
				imbar_section(colorscheme[C_PERC], color, "-");
			else
				imbar_sectionf(colorscheme[C_PERC], color, "%i%%", (int)(state.tree.prog * 100));

			imbar_sectionf(-1, -1, "help: %s", keybinds_str[K_HELP]);
			imbar_section (-1, -1, "utf8");
			imbar_section (-1, -1, "unix");
		} imbar_sections_end();

		imbar_sections_begin(IMBAR_LEFT); {
			imbar_section(colorscheme[C_NORM_FG], colorscheme[C_NORM_BG], mode_str[state.mode]);
			imbar_section(colorscheme[C_PATH_FG], colorscheme[C_PATH_BG], state.tree.path);
		} imbar_sections_end();
	} imbar_end();
}

static void editor_render_bar_find(void) {
	bool has_matches = state.tview.matches_count > 0;
	int  prompt_fg   = colorscheme[has_matches? C_PROMPT_FG : C_PROMPT_ERR];

	imbar_begin(0, tui_get_h() - 1, tui_get_w(), prompt_fg, colorscheme[C_PROMPT_BG], 0); {
		imbar_sections_begin(IMBAR_RIGHT); {
			int matches_fg = colorscheme[has_matches? C_MATCHES_FG : C_NO_MATCHES_FG];
			int matches_bg = colorscheme[has_matches? C_MATCHES_BG : C_NO_MATCHES_BG];

			if (state.tview.matches_count > 9999)
				imbar_section(matches_fg, matches_bg, "9999+");
			else
				imbar_sectionf(matches_fg, matches_bg, "%5zu", state.tview.matches_count);
		} int len_right = imbar_sections_end();

		imbar_sections_begin(IMBAR_LEFT); {
			imbar_section(colorscheme[C_FIND_FG], colorscheme[C_FIND_BG], mode_str[state.mode]);

			if (state.cs_find)
				imbar_section(colorscheme[C_CS_FG], colorscheme[C_CS_BG], "CS");
		} int len_left = imbar_sections_end();

		int prompt_x = len_left + 1;
		int prompt_w = tui_get_w() - prompt_x - len_right - 1;

		prompt_render(&state.prompt, prompt_x, 0, prompt_w, prompt_fg, colorscheme[C_PROMPT_BG],
		              colorscheme[C_SELECT_FG], colorscheme[C_SELECT_BG]);
	} imbar_end();
}

static void editor_render_bar_yn(void) {
	imbar_begin(0, tui_get_h() - 1, tui_get_w(), colorscheme[C_BAR_FG], colorscheme[C_BAR_BG], 0); {
		imbar_sections_begin(IMBAR_LEFT); {
			imbar_section(colorscheme[C_TITLE_FG],  colorscheme[C_TITLE_BG],  state.title);
			imbar_section(colorscheme[C_Y_FG],      colorscheme[C_Y_BG],      "y");
			imbar_section(colorscheme[C_N_FG],      colorscheme[C_N_BG],      "n");
			imbar_section(colorscheme[C_CANCEL_FG], colorscheme[C_CANCEL_BG], "esc");
		} imbar_sections_end();
	} imbar_end();
}

static void editor_render_bar_prompt(void) {
	imbar_begin(0, tui_get_h() - 1, tui_get_w(),
	            colorscheme[C_PROMPT_FG], colorscheme[C_PROMPT_BG], 0); {
		imbar_sections_begin(IMBAR_LEFT); {
			imbar_section(colorscheme[C_FIND_FG], colorscheme[C_FIND_BG], state.title);
		} int len_left = imbar_sections_end();

		int prompt_x = len_left + 1;
		int prompt_w = tui_get_w() - prompt_x - 1;

		prompt_render(&state.prompt, prompt_x, 0, prompt_w, colorscheme[C_PROMPT_FG],
		              colorscheme[C_PROMPT_BG], colorscheme[C_SELECT_FG], colorscheme[C_SELECT_BG]);
	} imbar_end();
}

static void editor_render_bar_jump(void) {
	int prompt_fg = colorscheme[state.prompt_is_uint? C_PROMPT_FG : C_PROMPT_ERR];

	imbar_begin(0, tui_get_h() - 1, tui_get_w(), prompt_fg, colorscheme[C_PROMPT_BG], 0); {
		imbar_sections_begin(IMBAR_LEFT); {
			imbar_section(colorscheme[C_JUMP_FG], colorscheme[C_JUMP_BG], mode_str[state.mode]);
		} int len = imbar_sections_end();

		int prompt_x = len + 1;
		int prompt_w = tui_get_w() - prompt_x - 1;

		prompt_render(&state.prompt, prompt_x, 0, prompt_w, prompt_fg, colorscheme[C_PROMPT_BG],
		              colorscheme[C_SELECT_FG], colorscheme[C_SELECT_BG]);
	} imbar_end();
}

static void editor_render_bar_help(void) {
	imbar_begin(0, tui_get_h() - 1, tui_get_w(), colorscheme[C_BAR_FG], colorscheme[C_BAR_BG], 0); {
		imbar_sections_begin(IMBAR_LEFT); {
			imbar_section(colorscheme[C_HELP_FG], colorscheme[C_HELP_BG], mode_str[state.mode]);
		} imbar_sections_end();
	} imbar_end();
}

static void editor_render_bar(void) {
	switch (state.mode) {
	case MODE_NORM:   editor_render_bar_norm();   break;
	case MODE_FIND:   editor_render_bar_find();   break;
	case MODE_YN:     editor_render_bar_yn();     break;
	case MODE_PROMPT: editor_render_bar_prompt(); break;
	case MODE_JUMP:   editor_render_bar_jump();   break;
	case MODE_HELP:   editor_render_bar_help();   break;

	default: UNREACHABLE();
	}
}

static void editor_set_mode(int mode) {
	assert(mode != state.mode);

	state.mode = mode;
	switch (state.mode) {
	case MODE_FIND:
		prompt_clear(&state.prompt);
		tview_search(&state.tview, state.prompt.line, state.cs_find);
		break;

	case MODE_PROMPT: prompt_clear(&state.prompt); break;

	case MODE_JUMP:
		state.prompt_is_uint = false;
		state.prompt_uint    = 0;
		prompt_clear(&state.prompt);
		break;

	default: break;
	}
}

static void editor_answer_yn(int answer) {
	state.yncb(state.data, answer);
	editor_set_mode(MODE_NORM);
}

static void editor_answer_prompt(void) {
	state.tcb(state.data, state.prompt.line);
	editor_set_mode(MODE_NORM);
}

static void editor_handle_keypress(int key) {
	switch (state.mode) {
	case MODE_NORM:
		MATCH(int, key,
			TO(keybinds[K_QUIT]) state.quit = true;

			TO(keybinds[K_FINDCS]) {
				state.cs_find = true;
				editor_set_mode(MODE_FIND);
			}

			TO(keybinds[K_FINDCI]) {
				state.cs_find = false;
				editor_set_mode(MODE_FIND);
			}

			TO(keybinds[K_RELOAD]) {
				conf_load();
				tui_caret_pipe(flags[F_CARET_PIPE]);
			}

			TO(keybinds[K_HELP]) editor_set_mode(MODE_HELP);
			TO(keybinds[K_JUMP]) editor_set_mode(MODE_JUMP);

			else
				tview_input(&state.tview, key);
		);
		break;

	case MODE_FIND:
		MATCH(int, key,
			TO(keybinds[K_NORM]) {
				tview_search_discard(&state.tview);
				editor_set_mode(MODE_NORM);
			}

			TO(keybinds[K_DONE]) {
				tview_search_end(&state.tview);
				editor_set_mode(MODE_NORM);
			}

			else {
				tview_input(&state.tview, key);
				prompt_input(&state.prompt, key);

				if (state.prompt.changed)
					tview_search(&state.tview, state.prompt.line, state.cs_find);
			}
		);
		break;

	case MODE_YN:
		switch (key) {
		case 'y': case 'Y': editor_answer_yn(YES);    break;
		case 'n': case 'N': editor_answer_yn(NO);     break;
		case KEY_ESC:       editor_answer_yn(CANCEL); break;
		}
		break;

	case MODE_PROMPT:
		if (key == keybinds[K_DONE])
			editor_answer_prompt();
		else
			prompt_input(&state.prompt, key);
		break;

	case MODE_JUMP:
		MATCH(int, key,
			TO(keybinds[K_NORM]) editor_set_mode(MODE_NORM);

			TO(keybinds[K_DONE]) {
				if (state.prompt_is_uint)
					tview_jump(&state.tview, state.prompt_uint);

				editor_set_mode(MODE_NORM);
			}

			else {
				prompt_input(&state.prompt, key);

				if (state.prompt.changed) {
					char *ptr;
					state.prompt_uint    = (size_t)strtoull(state.prompt.line, &ptr, 10);
					state.prompt_is_uint = *ptr == '\0';
				}
			}
		);
		break;

	case MODE_HELP:
		MATCH(int, key,
			TO(keybinds[K_NORM]) editor_set_mode(MODE_NORM);
			TO(keybinds[K_QUIT]) state.quit = true;
		);
		break;

	default: UNREACHABLE();
	}

	if (state.prompt.changed)
		state.thook(state.data, state.prompt.line);
}

void edit(const char *path) {
	atexit(editor_autosave);
	editor_init(path);

	while (!state.quit) {
		tui_caret(false);

		editor_render_bar();
		tview_render(&state.tview, 0, 0, tui_get_w(), tui_get_h() - 1);

		tui_render();

		event_t evt = tui_event();
		switch (evt.type) {
		case EVENT_ERR: DIE("Encountered an input error");
		case EVENT_KEYPRESS: editor_handle_keypress(evt.key); break;

		default: break;
		}
	}

	editor_deinit();
}

void editor_yn(const char *title, void *data, yncb_t cb) {
	editor_set_mode(MODE_YN);

	state.title = title;
	state.data  = data;

	state.yncb = cb;
}

void editor_prompt(const char *title, const char *text, void *data, tcb_t cb, tcb_t hook) {
	editor_set_mode(MODE_PROMPT);

	state.title = title;
	state.data  = data;

	state.tcb   = cb;
	state.thook = hook;

	prompt_set_line(&state.prompt, text);
	state.thook(state.data, state.prompt.line);
}
