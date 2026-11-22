#ifndef TARN_SOURCE_CHILDREN_H
#define TARN_SOURCE_CHILDREN_H

#include "context.h"
#include "string_list.h"

/**
 * The child lines of the `source/` README that mirrors the source folder `mirrored`, sorted by byte order of the
 * name as written (so `a.c` sorts before `a/`). A line's text is kept from the line of current_lines with the same
 * name, and is the placeholder `to write` for a name that has none.
 */
void source_children_lines(const check_context *context, const char *mirrored, const string_list *current_lines,
                           string_list *out);
/** Whether the line is a child line whose text is still the placeholder `to write`. */
int source_children_is_unwritten(const char *line);

#endif
