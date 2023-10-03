#ifndef IMBAR_H_HEADER_GUARD
#define IMBAR_H_HEADER_GUARD

#include <assert.h> /* assert */
#include <string.h> /* string */
#include <stdarg.h> /* va_list, va_start, va_end, vsnprintf */

#include <noch/utf8.h>

#include "common.h"
#include "conf.h"
#include "tui.h"

enum {
	IMBAR_LEFT = 0,
	IMBAR_RIGHT,
};

void imbar_begin(int x, int y, int w, int fg, int bg, int sep);
void imbar_end(void);

void imbar_sections_begin(int side);
int  imbar_sections_end(void);

void imbar_section_begin(int fg, int bg);
void imbar_section_end(void);

void imbar_section (int fg, int bg, const char *str);
void imbar_sectionf(int fg, int bg, const char *fmt, ...);

void imbar_section_add_str (const char *str);
void imbar_section_add_strf(const char *fmt, ...);

#endif
