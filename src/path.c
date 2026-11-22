#include "path.h"

#include "directory.h"
#include "memory.h"
#include "string_list.h"
#include "text.h"

#include <stdlib.h>
#include <string.h>

char *path_join(const char *base, const char *child) {
    size_t base_length = strlen(base);
    size_t child_length = strlen(child);
    char *joined;
    if (base_length == 0) {
        return memory_copy_string(child);
    }
    if (child_length == 0) {
        return memory_copy_string(base);
    }
    joined = memory_allocate(base_length + child_length + 2);
    memcpy(joined, base, base_length);
    joined[base_length] = '/';
    memcpy(joined + base_length + 1, child, child_length + 1);
    return joined;
}

static void split_parts(const char *path, string_list *parts) {
    const char *start = path;
    for (;;) {
        const char *slash = strchr(start, '/');
        size_t length = slash == NULL ? strlen(start) : (size_t)(slash - start);
        char *part = memory_copy_bytes(start, length);
        string_list_push(parts, part);
        free(part);
        if (slash == NULL) {
            return;
        }
        start = slash + 1;
    }
}

static char *join_parts(const string_list *parts, size_t count) {
    char *joined = memory_copy_string("");
    size_t index;
    for (index = 0; index < count; index++) {
        char *next = path_join(joined, parts->items[index]);
        free(joined);
        joined = next;
    }
    return joined;
}

char *path_normalize(const char *path) {
    string_list parts;
    string_list kept;
    size_t index;
    char *normalized = NULL;
    int climbs_out = 0;
    string_list_init(&parts);
    string_list_init(&kept);
    split_parts(path, &parts);
    for (index = 0; index < parts.count && !climbs_out; index++) {
        const char *part = parts.items[index];
        if (part[0] == '\0' || strcmp(part, ".") == 0) {
            continue;
        }
        if (strcmp(part, "..") == 0) {
            if (kept.count == 0) {
                climbs_out = 1;
            } else {
                free(kept.items[--kept.count]);
            }
            continue;
        }
        string_list_push(&kept, part);
    }
    if (!climbs_out) {
        normalized = join_parts(&kept, kept.count);
    }
    string_list_free(&parts);
    string_list_free(&kept);
    return normalized;
}

char *path_parent(const char *path) {
    const char *slash = strrchr(path, '/');
    if (slash == NULL) {
        return memory_copy_string("");
    }
    return memory_copy_bytes(path, (size_t)(slash - path));
}

const char *path_base_name(const char *path) {
    const char *slash = strrchr(path, '/');
    return slash == NULL ? path : slash + 1;
}

char *path_on_disk(const char *repo_root, const char *relative) {
    return path_join(repo_root, relative);
}

static const directory_entry *find_ignoring_case(const directory_listing *listing, const char *name) {
    size_t index;
    for (index = 0; index < listing->count; index++) {
        if (text_equal_ignoring_case(listing->entries[index].name, name)) {
            return &listing->entries[index];
        }
    }
    return NULL;
}

static path_state lookup_part(const char *folder_on_disk, const char *name, int is_last) {
    directory_listing listing;
    const directory_entry *entry;
    path_state state = PATH_MISSING;
    if (directory_list(folder_on_disk, &listing) != 0) {
        return PATH_MISSING;
    }
    entry = directory_find(&listing, name);
    if (entry != NULL) {
        state = entry->is_directory ? PATH_DIRECTORY : PATH_FILE;
    } else if (find_ignoring_case(&listing, name) != NULL) {
        state = PATH_CASE_DIFFERS;
    }
    directory_listing_free(&listing);
    if (state == PATH_FILE && !is_last) {
        return PATH_MISSING;
    }
    return state;
}

path_state path_lookup_exact(const char *repo_root, const char *relative) {
    string_list parts;
    char *folder = memory_copy_string(repo_root);
    path_state state = PATH_DIRECTORY;
    size_t index;
    if (relative[0] == '\0') {
        free(folder);
        return directory_is_directory(repo_root) ? PATH_DIRECTORY : PATH_MISSING;
    }
    string_list_init(&parts);
    split_parts(relative, &parts);
    for (index = 0; index < parts.count; index++) {
        char *next;
        state = lookup_part(folder, parts.items[index], index + 1 == parts.count);
        if (state != PATH_DIRECTORY) {
            break;
        }
        next = path_join(folder, parts.items[index]);
        free(folder);
        folder = next;
    }
    free(folder);
    string_list_free(&parts);
    return state;
}
