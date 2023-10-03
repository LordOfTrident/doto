#include <stdlib.h> /* exit */
#include <string.h> /* strcat */

#include <noch/args.h>

#include "common.h"
#include "editor.h"
#include "tree.h"

#define DESC         "A TODO viewing/editing program"
#define DEFAULT_PATH "./todo.json"

static const char *usages[] = {
	"[FILE]              Open the given TODO file (default '" DEFAULT_PATH "')",
	"[FILE] -md [OUT]    Emit markdown of the given TODO file",
	"[OPTIONS]",
};

static args_t      args;
static const char *argv0;

static bool        f_help;
static const char *f_md;

static void usage(FILE *file) {
	args_usage_fprint(file, argv0, usages, ARRAY_LEN(usages), DESC, true);
}

static void parse_opts(void) {
	size_t where;
	bool   extra;

	if (args_parse_flags(&args, &where, NULL, &extra) != 0)
		DIE("Error: %s: %s", args.v[where], noch_get_err_msg());
	else if (extra)
		DIE("Error: %s: Unexpected argument", args.v[where]);

	if (f_help) {
		usage(stdout);
		exit(0);
	}
}

static const char *parse_args(void) {
	if (args.c == 0)
		return NULL;

	if (arg_is_flag(args.v[0])) {
		parse_opts();
		return NULL;
	}

	const char *path = args_shift(&args);

	if (args.c > 0)
		parse_opts();

	return path;
}

static void emit_md(const char *path) {
	tree_t tree;
	tree_load(&tree, path);

	if (*f_md == '\0') {
		char *out = (char*)xalloc(strlen(path) + 4);
		strcpy(out, path);

		char *last_dot = NULL;
		for (char *it = out; *it != '\0'; ++ it) {
			if (*it == '.')
				last_dot = it;
		}

		if (last_dot != NULL)
			*last_dot = '\0';

		strcat(out, ".md");

		tree_emit_md(&tree, out);
	} else
		tree_emit_md(&tree, f_md);

	tree_clean(&tree);
}

int main(int argc, const char **argv) {
	args  = args_new(argc, argv);
	argv0 = args_shift(&args);

	assert(argv0 != NULL);

	flag_bool("h",  "help",     "Show the usage",                   &f_help);
	flag_str ("md", "markdown", "Generate markdown of a TODO file", &f_md);

	const char *path = parse_args();
	if (path == NULL)
		path = DEFAULT_PATH;

	if (f_md != NULL)
		emit_md(path);
	else
		edit(path);

	return 0;
}
