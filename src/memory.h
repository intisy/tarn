#ifndef TARN_MEMORY_H
#define TARN_MEMORY_H

#include <stddef.h>

/** Every allocation in tarn-check goes through these; each aborts rather than returning NULL. */
void *memory_allocate(size_t size);
void *memory_resize(void *block, size_t size);
char *memory_copy_string(const char *text);
char *memory_copy_bytes(const char *bytes, size_t length);

#endif
