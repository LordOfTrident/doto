#ifndef CLIPBOARD_H_HEADER_GUARD
#define CLIPBOARD_H_HEADER_GUARD

#include <stdio.h>   /* FILE, popen, pclose, fputs, fgetc, EOF */
#include <stdbool.h> /* bool, true, false */
#include <assert.h>  /* assert */
#include <string.h>  /* memcpy */

#include "common.h"
#include "conf.h"

void clipboard_init(void);
void clipboard_deinit(void);

void        clipboard_set(const char *str, size_t len);
const char *clipboard_get(void);

#endif
