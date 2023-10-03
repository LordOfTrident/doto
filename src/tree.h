#ifndef TREE_H_HEADER_GUARD
#define TREE_H_HEADER_GUARD

#include <string.h>  /* strcmp */
#include <assert.h>  /* assert */
#include <stdbool.h> /* bool, true, false */
#include <stdio.h>   /* fprintf, stderr, FILE, fopen, fclose */
#include <stdlib.h>  /* exit, EXIT_FAILURE */

#include <noch/json.h>

#include "common.h"
#include "conf.h"

enum {
	NODE_TASK = 0,
	NODE_GROUP,

	NODE_TYPES_COUNT,
};

typedef struct node  node_t;
typedef struct task  task_t;
typedef struct group group_t;
typedef struct tree  tree_t;

struct node {
	int type;

	node_t  *prev, *next;
	tree_t  *tree;
	group_t *group;
};

#define AS_TASK(NODE)  ((task_t*)NODE)
#define AS_GROUP(NODE) ((group_t*)NODE)

struct task {
	node_t _;

	char *title, *desc;
	bool  done;
};

struct group {
	node_t _;

	char *title;
	float prog;
	bool  open;

	node_t *children;
};

struct tree {
	node_t *root;

	float       prog;
	const char *path;
};

task_t  *task_new (const char *title, const char *desc, bool done);
group_t *group_new(const char *title, bool open);
node_t  *node_new (int type);

void task_toggle_done (task_t  *task);
void group_toggle_open(group_t *group);

void tree_set_root(tree_t *tree, node_t *node);

void node_add_sibl  (node_t  *node,  node_t *sibl);
void group_add_child(group_t *group, node_t *child);

void node_remove(node_t *node);

void tree_load (tree_t *tree, const char *path);
void tree_clean(tree_t *tree);

void tree_save   (tree_t *tree);
void tree_emit_md(tree_t *tree, const char *path);

#endif
