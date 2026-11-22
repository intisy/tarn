#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *require(void *block) {
    if (block == NULL) {
        fputs("tarn-check: out of memory\n", stderr);
        abort();
    }
    return block;
}

void *memory_allocate(size_t size) {
    return require(malloc(size == 0 ? 1 : size));
}

void *memory_resize(void *block, size_t size) {
    return require(realloc(block, size == 0 ? 1 : size));
}

char *memory_copy_bytes(const char *bytes, size_t length) {
    char *copy = memory_allocate(length + 1);
    memcpy(copy, bytes, length);
    copy[length] = '\0';
    return copy;
}

char *memory_copy_string(const char *text) {
    return memory_copy_bytes(text, strlen(text));
}
