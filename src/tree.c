#include "tree.h"

static const char *node_type_to_str_map[NODE_TYPES_COUNT] = {
	[NODE_TASK]  = "task",
	[NODE_GROUP] = "group",
};

static const char *node_type_to_str(int type) {
	assert(type < NODE_TYPES_COUNT && type >= 0);
	return node_type_to_str_map[type];
}

task_t *task_new(const char *title, const char *desc, bool done) {
	assert(title != NULL);

	task_t *task = NEW(task_t);
	task->_.type = NODE_TASK;

	task->title = xstrdup(title);
	task->desc  = desc == NULL? NULL : xstrdup(desc);
	task->done  = done;
	return task;
}

group_t *group_new(const char *title, bool open) {
	assert(title != NULL);

	group_t *group = NEW(group_t);
	group->_.type  = NODE_GROUP;

	group->title = xstrdup(title);
	group->open  = open;
	group->prog  = -1;
	return group;
}

node_t *node_new(int type) {
	switch (type) {
	case NODE_TASK:  return (node_t*)task_new("", NULL, false);
	case NODE_GROUP: return (node_t*)group_new("", true);

	default: UNREACHABLE();
	}
}

static float children_calc_prog(node_t *child) {
	size_t count = 0;
	float  prog  = 0;
	for (; child != NULL; child = child->next) {
		switch (child->type) {
		case NODE_TASK:
			prog += AS_TASK(child)->done;
			++ count;
			break;

		case NODE_GROUP:
			if (AS_GROUP(child)->prog == -1)
				break;

			prog += AS_GROUP(child)->prog;
			++ count;
			break;

		default: UNREACHABLE();
		}
	}

	return count == 0? -1 : prog / count;
}

static void node_update_parents(node_t *node) {
	for (group_t *group = node->group; group != NULL; group = group->_.group)
		group->prog = children_calc_prog(group->children);

	node->tree->prog = children_calc_prog(node->tree->root);
}

void task_toggle_done(task_t *task) {
	assert(task != NULL);

	task->done = !task->done;
	node_update_parents((node_t*)task);
}

void group_toggle_open(group_t *group) {
	assert(group != NULL);

	group->open = !group->open;
}

void tree_set_root(tree_t *tree, node_t *node) {
	assert(node != NULL);

	node->tree = tree;
	tree->root = node;
}

void node_add_sibl(node_t *node, node_t *sibl) {
	assert(node != NULL);
	assert(sibl != NULL);

	sibl->group = node->group;
	sibl->tree  = node->tree;

	if (node->next == NULL) {
		node->next = sibl;
		sibl->prev = node;
	} else {
		sibl->next       = node->next;
		sibl->prev       = node;
		node->next->prev = sibl;
		node->next       = sibl;
	}

	node_update_parents(node);
}

void group_add_child(group_t *group, node_t *child) {
	assert(group != NULL);
	assert(child != NULL);

	if (group->children == NULL)
		group->children = child;
	else {
		node_t *last_child = group->children;
		while (last_child->next != NULL)
			last_child = last_child->next;

		last_child->next = child;
		child->prev      = last_child;
	}

	child->group = group;
	child->tree  = group->_.tree;

	node_update_parents(child);
}

static void node_destroy(node_t *node);

static void children_destroy(node_t *child) {
	for (node_t *next = NULL; child != NULL; child = next) {
		next = child->next;
		node_destroy(child);
	}
}

static void node_destroy(node_t *node) {
	switch (node->type) {
	case NODE_TASK: {
		task_t *task = AS_TASK(node);

		if (task->desc != NULL)
			xfree(task->desc);

		xfree(task->title);
	} break;

	case NODE_GROUP: {
		group_t *group = AS_GROUP(node);

		if (group->children != NULL)
			children_destroy(group->children);

		xfree(group->title);
	} break;

	default: UNREACHABLE();
	}

	xfree(node);
}

void node_remove(node_t *node) {
	assert(node != NULL);

	if (node->prev != NULL)
		node->prev->next = node->next;
	else if (node->group != NULL)
		node->group->children = node->next;
	else
		node->tree->root = node->next;

	if (node->next != NULL)
		node->next->prev = node->prev;

	node_update_parents(node);
	node_destroy(node);
}

static node_t *json_list_to_node(json_list_t *list, tree_t *tree, group_t *group);

static group_t *json_obj_to_group(json_obj_t *obj, tree_t *tree) {
	json_t *title    = json_obj_at(obj, "title");
	json_t *children = json_obj_at(obj, "children");

	if (title == NULL || children == NULL)
		DIE("%s: Data corrupted (Missing group fields)", tree->path);

	if (title->type != JSON_STR || children->type != JSON_LIST)
		DIE("%s: Data corrupted (Incorrect group field type)", tree->path);

	group_t *group  = group_new(JSON_STR(title)->buf, flags[F_OPEN_GROUPS_ON_START]);
	group->children = json_list_to_node(JSON_LIST(children), tree, group);

	return group;
}

static task_t *json_obj_to_task(json_obj_t *obj, tree_t *tree) {
	json_t *title = json_obj_at(obj, "title");
	json_t *desc  = json_obj_at(obj, "desc");
	json_t *done  = json_obj_at(obj, "done");

	if (title == NULL || desc == NULL || done == NULL)
		DIE("%s: Data corrupted (Missing task fields)", tree->path);

	if (title->type != JSON_STR || done->type != JSON_BOOL ||
	    (desc->type != JSON_STR && desc->type != JSON_NULL))
		DIE("%s: Data corrupted (Incorrect task field type)", tree->path);

	const char *desc_str = desc == json_null()? NULL : JSON_STR(desc)->buf;
	return task_new(JSON_STR(title)->buf, desc_str, JSON_BOOL(done)->val);
}

static node_t *json_list_to_node(json_list_t *list, tree_t *tree, group_t *group) {
	node_t *ret  = NULL, *last = NULL;

	size_t count = 0;
	float  prog  = 0;
	for (size_t i = 0; i < list->size; ++ i) {
		json_obj_t *data;
		JSON_EXPECT_OBJ(data, json_list_at(list, i), {
			DIE("%s: Data corrupted (Expected an object)", tree->path);
		});

		json_str_t *type;
		JSON_EXPECT_STR(type, json_obj_at(data, "type"), {
			DIE("%s: Data corrupted (Type not a string)", tree->path);
		});

		if (type == NULL)
			DIE("%s: Data corrupted (Type not found)", tree->path);

		node_t *node;
		if (strcmp(type->buf, node_type_to_str(NODE_GROUP)) == 0) {
			group_t *group = json_obj_to_group(data, tree);
			if (group->prog != -1) {
				prog += group->prog;
				++ count;
			}

			node = (node_t*)group;
		} else if (strcmp(type->buf, node_type_to_str(NODE_TASK)) == 0) {
			task_t *task = json_obj_to_task(data, tree);
			prog += task->done;
			++ count;

			node = (node_t*)task;
		} else
			DIE("%s: Data corrupted (Unknown type \"%s\")", tree->path, type->buf);

		node->tree  = tree;
		node->group = group;

		if (ret == NULL) {
			ret  = node;
			last = node;
		} else {
			last->next = node;
			node->prev = last;
			last = node;
		}
	}

	prog = count > 0? prog / count : -1;

	if (group != NULL)
		group->prog = prog;
	else
		tree->prog = prog;

	return ret;
}

void tree_load(tree_t *tree, const char *path) {
	assert(path != NULL);

	ZERO_STRUCT(tree);
	tree->path = path;

	size_t row, col;
	json_list_t *json;
	JSON_EXPECT_LIST(json, json_from_file(tree->path, &row, &col), {
		DIE("%s: Expected data to be a list", path);
	});

	if (json == NULL) {
		if (noch_get_err() == NOCH_ERR_FOPEN)
			return;
		else if (noch_get_err() == NOCH_ERR_PARSER)
			DIE("%s:%zu:%zu: Data corrupted: %s", path, row, col, noch_get_err_msg());
		else
			DIE("%s: %s", path, noch_get_err_msg());
	}

	tree->root = json_list_to_node(json, tree, NULL);

	JSON_DESTROY(json);
}

void tree_clean(tree_t *tree) {
	if (tree->root != NULL)
		children_destroy(tree->root);
}

static json_t *node_to_json(node_t *node) {
	json_list_t *list = json_new_list();
	for (; node != NULL; node = node->next) {
		json_obj_t *data = json_new_obj();
		JSON_OBJ_ADD(data, "type", json_new_str(node_type_to_str(node->type)));

		switch (node->type) {
		case NODE_TASK: {
			task_t *task = (task_t*)node;
			json_t *desc = task->desc == NULL? json_null() : (json_t*)json_new_str(task->desc);

			JSON_OBJ_ADD(data, "title", json_new_str(task->title));
			JSON_OBJ_ADD(data, "desc",  desc);
			JSON_OBJ_ADD(data, "done",  json_new_bool(task->done));
		} break;

		case NODE_GROUP: {
			group_t *group = (group_t*)node;

			JSON_OBJ_ADD(data, "title",    json_new_str(group->title));
			JSON_OBJ_ADD(data, "children", node_to_json(group->children));
		} break;

		default: UNREACHABLE();
		}

		JSON_LIST_ADD(list, data);
	}

	return (json_t*)list;
}

void tree_save(tree_t *tree) {
	assert(tree->path != NULL);

	json_t *json = node_to_json(tree->root);

	FILE *file = fopen(tree->path, "w");
	if (file == NULL)
		DIE("Failed to write TODO file \"%s\"", tree->path);

	JSON_FPRINT(json, file);
	JSON_DESTROY(json);

	fclose(file);
}

static void findent(FILE *file, size_t indent) {
	for (size_t i = 0; i < indent; ++ i)
		fputc('\t', file);
}

static void node_to_md(node_t *node, FILE *file, size_t nest) {
	for (; node != NULL; node = node->next) {
		findent(file, nest);

		switch (node->type) {
		case NODE_TASK: {
			task_t *task = AS_TASK(node);

			fprintf(file, "- [%c] %s\n", task->done? 'X' : ' ', task->title);

			if (task->desc != NULL) {
				findent(file, nest);
				fprintf(file, "> ");

				size_t len = strlen(task->desc);
				for (size_t i = 0; i < len; ++ i) {
					if (task->desc[i] != '\n') {
						fputc(task->desc[i], file);
						continue;
					}

					if (i + 1 == len)
						break;

					fprintf(file, "<br>\n");
					findent(file, nest);
					fprintf(file, "> ");
				}

				fprintf(file, "\n");
			}
		} break;

		case NODE_GROUP: {
			group_t *group = (group_t*)node;

			if      (group->prog == 1)  fprintf(file, "- [X] ");
			else if (group->prog == -1) fprintf(file, "- (empty) ");
			else                        fprintf(file, "- (`%i%%`) ", (int)(group->prog * 100));

			fprintf(file, "**%s**\n", group->title);

			node_to_md(group->children, file, nest + 1);
		} break;

		default: UNREACHABLE();
		}
	}
}

void tree_emit_md(tree_t *tree, const char *path) {
	assert(path != NULL);

	FILE *file = fopen(path, "w");
	if (file == NULL)
		DIE("Failed to write markdown file \"%s\"", path);

	fprintf(file, "# TODO");
	if      (tree->prog == 1)  fprintf(file, " (done)");
	else if (tree->prog == -1) fprintf(file, " (empty)");
	else                       fprintf(file, " (%i%% done)", (int)(tree->prog * 100));

	fprintf(file, "\n");
	node_to_md(tree->root, file, 0);

	fclose(file);
}
