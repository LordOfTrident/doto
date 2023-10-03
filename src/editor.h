#ifndef EDITOR_H_HEADER_GUARD
#define EDITOR_H_HEADER_GUARD

#include "common.h"
#include "tui.h"
#include "clipboard.h"
#include "conf.h"
#include "imbar.h"
#include "tree.h"
#include "tview.h"
#include "prompt.h"

#define TITLE "doto"

enum {
	MODE_NORM = 0,
	MODE_FIND,
	MODE_YN,
	MODE_PROMPT,
	MODE_JUMP,
	MODE_HELP,
};

void edit(const char *path);

enum {
	CANCEL = 0,
	YES,
	NO,
};

typedef void (*yncb_t)(void*, int);
typedef void (*tcb_t) (void*, const char*);

void editor_yn(const char *title, void *data, yncb_t cb);
void editor_prompt(const char *title, const char *text, void *data, tcb_t cb, tcb_t hook);

#endif
