#ifndef TARN_DIRECTORY_H
#define TARN_DIRECTORY_H

#include <stddef.h>

typedef struct {
    char *name;
    int is_directory;
} directory_entry;

/**
 * A folder's entries without `.` and `..`, sorted by byte order of name, names exactly as stored on disk.
 * @implNote A link to a folder (a symlink, or a junction on Windows) is listed as a non-folder, so no walk follows
 * it into a cycle.
 */
typedef struct {
    directory_entry *entries;
    size_t count;
} directory_listing;

int directory_list(const char *path, directory_listing *out);
void directory_listing_free(directory_listing *listing);
const directory_entry *directory_find(const directory_listing *listing, const char *name);
int directory_is_directory(const char *path);
int directory_read_file(const char *path, char **content, size_t *length);

#endif
