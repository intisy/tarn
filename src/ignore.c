#include "ignore.h"

#include "memory.h"
#include "path.h"
#include "text.h"

#include <stdlib.h>
#include <string.h>

static const char *const fixed_folders[] = {"build", "node_modules", "vendor", "siblings"};

static void push_pattern(ignore_rules *rules, const ignore_pattern *pattern) {
    rules->patterns = memory_resize(rules->patterns, (rules->count + 1) * sizeof *rules->patterns);
    rules->patterns[rules->count++] = *pattern;
}

static void trim_trailing_spaces(char *line) {
    size_t length = strlen(line);
    while (length > 0 && line[length - 1] == ' ' && (length < 2 || line[length - 2] != '\\')) {
        line[--length] = '\0';
    }
}

static void parse_line(ignore_rules *rules, char *line) {
    ignore_pattern pattern = {NULL, 0, 0, 0};
    size_t length;
    trim_trailing_spaces(line);
    if (line[0] == '\0' || line[0] == '#') {
        return;
    }
    if (line[0] == '!') {
        pattern.negated = 1;
        line++;
    } else if (line[0] == '\\' && (line[1] == '#' || line[1] == '!')) {
        line++;
    }
    length = strlen(line);
    if (length > 0 && line[length - 1] == '/') {
        pattern.directory_only = 1;
        line[--length] = '\0';
    }
    if (strchr(line, '/') != NULL) {
        pattern.anchored = 1;
    }
    if (line[0] == '/') {
        line++;
    }
    if (line[0] == '\0') {
        return;
    }
    pattern.glob = memory_copy_string(line);
    push_pattern(rules, &pattern);
}

void ignore_load(const char *repo_root, ignore_rules *out) {
    char *path = path_on_disk(repo_root, ".gitignore");
    text_document document;
    size_t index;
    out->patterns = NULL;
    out->count = 0;
    if (text_document_load(path, &document) == 0) {
        for (index = 0; index < document.count; index++) {
            parse_line(out, document.lines[index].text);
        }
        text_document_free(&document);
    }
    free(path);
}

void ignore_free(ignore_rules *rules) {
    size_t index;
    for (index = 0; index < rules->count; index++) {
        free(rules->patterns[index].glob);
    }
    free(rules->patterns);
    rules->patterns = NULL;
    rules->count = 0;
}

static int class_matches(const char **pattern, char character) {
    const char *scan = *pattern + 1;
    int negated = 0;
    int matched = 0;
    if (*scan == '!' || *scan == '^') {
        negated = 1;
        scan++;
    }
    if (*scan == '\0') {
        return -1;
    }
    do {
        if (scan[1] == '-' && scan[2] != ']' && scan[2] != '\0') {
            matched |= character >= scan[0] && character <= scan[2];
            scan += 3;
        } else {
            matched |= character == *scan;
            scan++;
        }
    } while (*scan != ']' && *scan != '\0');
    if (*scan == '\0') {
        return -1;
    }
    *pattern = scan + 1;
    return matched != negated;
}

static int glob_matches(const char *pattern, const char *text);

static int match_any_segments(const char *pattern, const char *text) {
    for (;;) {
        const char *slash;
        if (glob_matches(pattern, text)) {
            return 1;
        }
        slash = strchr(text, '/');
        if (slash == NULL) {
            return 0;
        }
        text = slash + 1;
    }
}

static int match_star(const char *pattern, const char *text, int crosses_slash) {
    for (;;) {
        if (glob_matches(pattern, text)) {
            return 1;
        }
        if (*text == '\0' || (*text == '/' && !crosses_slash)) {
            return 0;
        }
        text++;
    }
}

static int glob_matches(const char *pattern, const char *text) {
    while (*pattern != '\0') {
        if (pattern[0] == '*' && pattern[1] == '*') {
            pattern += 2;
            if (*pattern == '/') {
                return match_any_segments(pattern + 1, text);
            }
            return match_star(pattern, text, 1);
        }
        if (*pattern == '*') {
            return match_star(pattern + 1, text, 0);
        }
        if (*text == '\0') {
            return 0;
        }
        if (*pattern == '?') {
            if (*text == '/') {
                return 0;
            }
        } else if (*pattern == '[') {
            int matched = class_matches(&pattern, *text);
            if (matched == 0) {
                return 0;
            }
            if (matched == 1) {
                text++;
                continue;
            }
            if (*text != '[') {
                return 0;
            }
        } else {
            if (*pattern == '\\' && pattern[1] != '\0') {
                pattern++;
            }
            if (*pattern != *text) {
                return 0;
            }
        }
        pattern++;
        text++;
    }
    return *text == '\0';
}

static int is_fixed(const char *relative, int is_directory) {
    const char *name = path_base_name(relative);
    size_t index;
    if (strcmp(name, ".git") == 0) {
        return 1;
    }
    if (!is_directory) {
        return 0;
    }
    if (strcmp(relative, "docs") == 0) {
        return 1;
    }
    for (index = 0; index < sizeof fixed_folders / sizeof fixed_folders[0]; index++) {
        if (strcmp(name, fixed_folders[index]) == 0) {
            return 1;
        }
    }
    return 0;
}

static int pattern_matches(const ignore_pattern *pattern, const char *relative, int is_directory) {
    if (pattern->directory_only && !is_directory) {
        return 0;
    }
    return glob_matches(pattern->glob, pattern->anchored ? relative : path_base_name(relative));
}

int ignore_matches(const ignore_rules *rules, const char *relative, int is_directory) {
    size_t index;
    int ignored = 0;
    if (is_fixed(relative, is_directory)) {
        return 1;
    }
    for (index = 0; index < rules->count; index++) {
        if (pattern_matches(&rules->patterns[index], relative, is_directory)) {
            ignored = !rules->patterns[index].negated;
        }
    }
    return ignored;
}

int ignore_covers(const ignore_rules *rules, const char *relative, int is_directory) {
    const char *scan;
    for (scan = strchr(relative, '/'); scan != NULL; scan = strchr(scan + 1, '/')) {
        char *folder = memory_copy_bytes(relative, (size_t)(scan - relative));
        int ignored = ignore_matches(rules, folder, 1);
        free(folder);
        if (ignored) {
            return 1;
        }
    }
    return ignore_matches(rules, relative, is_directory);
}
