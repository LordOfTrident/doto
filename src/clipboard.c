#include "clipboard.h"

static struct {
	char *buf, *xclip_buf;

	bool xclip;
} state;

void clipboard_init() {
	state.buf       = NULL;
	state.xclip_buf = NULL;
	state.xclip     = !flags[F_NO_XCLIP];
}

void clipboard_deinit(void) {
	if (state.buf != NULL)
		xfree(state.buf);

	if (state.xclip_buf != NULL)
		xfree(state.xclip_buf);
}

void clipboard_set(const char *str, size_t len) {
	char *copy = (char*)xalloc(len + 1);
	memcpy(copy, str, len);
	copy[len] = '\0';

	if (state.xclip) {
		FILE *file = popen("xclip -i -sel clip 2> /dev/null", "w");
		if (file == NULL)
			goto no_xclip;

		fputs(copy, file);

		int ret = pclose(file);
		if (ret == 127 || ret == 32512)
			goto no_xclip;

		xfree(copy);
		return;
	}

/* TODO: Make this not a label */
no_xclip:
	if (state.xclip)
		state.xclip = false;

	if (state.buf != NULL)
		xfree(state.buf);

	state.buf = copy;
}

const char *clipboard_get(void) {
	if (state.xclip) {
		FILE *file = popen("xclip -o -sel clip 2> /dev/null", "r");
		if (file == NULL)
			goto no_xclip;

		if (state.xclip_buf != NULL)
			xfree(state.xclip_buf);

		size_t cap      = 64, size = 0;
		state.xclip_buf = (char*)xalloc(cap);

		int ch;
		while ((ch = fgetc(file)) != EOF) {
			if (size + 1 >= cap) {
				cap *= 2;
				state.xclip_buf = (char*)xrealloc(state.xclip_buf, cap);
			}

			state.xclip_buf[size ++] = ch;
		}
		state.xclip_buf[size] = '\0';

		int ret = pclose(file);
		if (ret == 127 || ret == 32512)
			goto no_xclip;

		return state.xclip_buf;
	}

no_xclip:
	if (state.xclip)
		state.xclip = false;

	return state.buf;
}
