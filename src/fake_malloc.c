#include <stdlib.h>
#include "fake_malloc.h"

void *
rte_malloc(const char *name, unsigned size, unsigned alignment) {
        return malloc(size);
}

void
rte_free(void *mem) {
        free(mem);
}