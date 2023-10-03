#include "history.h"

void history_init(history_t *this) {
	ZERO_STRUCT(this);

	this->cap = HISTORY_CHUNK_SIZE;
	this->buf = (char**)xalloc(this->cap * sizeof(char*));
}

void history_deinit(history_t *this) {
	for (size_t i = 0; i < this->size; ++ i)
		xfree(this->buf[i]);

	xfree(this->buf);
}

void history_push(history_t *this, const char *str) {
	if (this->size >= this->cap) {
		this->cap *= 2;
		this->buf  = (char**)xrealloc(this->buf, this->cap * sizeof(char*));
	}

	this->buf[this->size ++] = xstrdup(str);
	this->pos = this->size;
}

const char *history_up(history_t *this) {
	if (this->size == 0)
		return NULL;

	if (this->pos > 0)
		-- this->pos;

	return this->buf[this->pos];
}

const char *history_down(history_t *this) {
	if (this->size == 0 || this->pos >= this->size)
		return NULL;

	return ++ this->pos >= this->size? NULL : this->buf[this->pos];
}
