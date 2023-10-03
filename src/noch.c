#include "common.h"

#define NOCH_ALLOC(SIZE)        xalloc  (SIZE)
#define NOCH_REALLOC(PTR, SIZE) xrealloc(PTR, SIZE)
#define NOCH_FREE(PTR)          xfree   (PTR)

#include <noch/args.c>
#include <noch/json.c>
#include <noch/utf8.c>
#include <noch/common.c>
