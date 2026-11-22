#ifndef TARN_README_H
#define TARN_README_H

#include "context.h"
#include "text.h"

#include <stddef.h>

typedef struct {
    char *name;
    int is_folder;
    int is_source_file;
    int listed;
} readme_child;

/** What a README must list: its folder's children and, in a source mirror, the code files of the mirrored folder. */
typedef struct {
    readme_child *items;
    size_t count;
} readme_children;

typedef struct {
    const char *readme_path;
    const char *folder;
    /** The mirrored source folder of a `source/` README, or NULL for any other README. */
    const char *source_folder;
} readme_place;

void readme_children_add(readme_children *children, const char *name, int is_folder, int is_source_file);
void readme_children_free(readme_children *children);

/**
 * Checks the README's form and its child lines against children, and sets child_lines[i] for every line that is a
 * child line, so the link check leaves those lines to this one.
 */
void readme_check(const check_context *context, const readme_place *place, const text_document *document,
                  readme_children *children, unsigned char *child_lines);

#endif
