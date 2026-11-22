#include "source_children.h"

#include "directory.h"
#include "memory.h"
#include "path.h"
#include "rules.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char placeholder_text[] = "to write";

typedef struct {
    char *name;
    char *target;
} wanted_child;

typedef struct {
    wanted_child *items;
    size_t count;
} wanted_children;

typedef struct {
    const char *name;
    size_t name_length;
    const char *text;
} child_text;

static int parse_child_text(const char *line, child_text *out) {
    const char *name_end;
    const char *target_end;
    if (strncmp(line, "- [", 3) != 0) {
        return 0;
    }
    out->name = line + 3;
    name_end = strstr(out->name, "](");
    target_end = name_end == NULL ? NULL : strchr(name_end + 2, ')');
    if (target_end == NULL || strncmp(target_end + 1, " - ", 3) != 0) {
        return 0;
    }
    out->name_length = (size_t)(name_end - out->name);
    out->text = target_end + 4;
    return 1;
}

int source_children_is_unwritten(const char *line) {
    child_text parsed;
    return parse_child_text(line, &parsed) && strcmp(parsed.text, placeholder_text) == 0;
}

static size_t component_count(const char *relative) {
    size_t count = relative[0] == '\0' ? 0 : 1;
    for (; *relative != '\0'; relative++) {
        count += *relative == '/';
    }
    return count;
}

static char *path_up_to_repo_root(const char *mirrored) {
    size_t levels = 2 + component_count(mirrored);
    char *prefix = memory_allocate(levels * 3 + 1);
    size_t level;
    for (level = 0; level < levels; level++) {
        memcpy(prefix + level * 3, "../", 3);
    }
    prefix[levels * 3] = '\0';
    return prefix;
}

static char *join_text(const char *first, const char *second) {
    size_t first_length = strlen(first);
    size_t second_length = strlen(second);
    char *joined = memory_allocate(first_length + second_length + 1);
    memcpy(joined, first, first_length);
    memcpy(joined + first_length, second, second_length + 1);
    return joined;
}

static void add_wanted(wanted_children *wanted, char *name, char *target) {
    wanted->items = memory_resize(wanted->items, (wanted->count + 1) * sizeof *wanted->items);
    wanted->items[wanted->count].name = name;
    wanted->items[wanted->count].target = target;
    wanted->count++;
}

static void add_folder_child(const check_context *context, const char *relative, const directory_entry *entry,
                             wanted_children *wanted) {
    if (string_list_contains(&context->code_folders, relative)) {
        add_wanted(wanted, join_text(entry->name, "/"), join_text(entry->name, "/"));
    }
}

static void add_file_child(const char *relative, const directory_entry *entry, const char *prefix,
                           wanted_children *wanted) {
    if (rules_is_code_file(entry->name)) {
        add_wanted(wanted, memory_copy_string(entry->name), join_text(prefix, relative));
    }
}

static void collect_wanted(const check_context *context, const char *mirrored, wanted_children *wanted) {
    char *on_disk = path_on_disk(context->repo_root, mirrored);
    char *prefix = path_up_to_repo_root(mirrored);
    directory_listing listing;
    size_t index;
    if (directory_list(on_disk, &listing) == 0) {
        for (index = 0; index < listing.count; index++) {
            const directory_entry *entry = &listing.entries[index];
            char *relative = path_join(mirrored, entry->name);
            if (!ignore_matches(&context->ignore, relative, entry->is_directory)) {
                if (entry->is_directory) {
                    add_folder_child(context, relative, entry, wanted);
                } else {
                    add_file_child(relative, entry, prefix, wanted);
                }
            }
            free(relative);
        }
        directory_listing_free(&listing);
    }
    free(prefix);
    free(on_disk);
}

static int compare_wanted(const void *left, const void *right) {
    return strcmp(((const wanted_child *)left)->name, ((const wanted_child *)right)->name);
}

static const char *kept_text(const string_list *current_lines, const char *name) {
    size_t index;
    for (index = 0; index < current_lines->count; index++) {
        child_text parsed;
        if (parse_child_text(current_lines->items[index], &parsed) && parsed.name_length == strlen(name) &&
            strncmp(parsed.name, name, parsed.name_length) == 0) {
            return parsed.text;
        }
    }
    return placeholder_text;
}

static char *format_line(const wanted_child *child, const char *text) {
    int length = snprintf(NULL, 0, "- [%s](%s) - %s", child->name, child->target, text);
    char *line;
    if (length < 0) {
        abort();
    }
    line = memory_allocate((size_t)length + 1);
    if (snprintf(line, (size_t)length + 1, "- [%s](%s) - %s", child->name, child->target, text) != length) {
        abort();
    }
    return line;
}

static void free_wanted(wanted_children *wanted) {
    size_t index;
    for (index = 0; index < wanted->count; index++) {
        free(wanted->items[index].name);
        free(wanted->items[index].target);
    }
    free(wanted->items);
}

void source_children_lines(const check_context *context, const char *mirrored, const string_list *current_lines,
                           string_list *out) {
    wanted_children wanted = {NULL, 0};
    size_t index;
    collect_wanted(context, mirrored, &wanted);
    if (wanted.count > 1) {
        qsort(wanted.items, wanted.count, sizeof *wanted.items, compare_wanted);
    }
    for (index = 0; index < wanted.count; index++) {
        char *line = format_line(&wanted.items[index], kept_text(current_lines, wanted.items[index].name));
        string_list_push(out, line);
        free(line);
    }
    free_wanted(&wanted);
}
