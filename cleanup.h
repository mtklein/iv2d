#pragma once

#include <stdlib.h>

static void free_cleanup(void *p) {
    free(*(void**)p);
}

