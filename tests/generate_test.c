#include "check.h"
#include "generate.h"
#include "report.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *name;
    const char *report;
    const char *files[3];
} write_case;

static const write_case write_cases[] = {
    {"write-source-children",
     "warned docs/source/src/README.md:4 prose-to-write the text to write is a placeholder for a person to replace\n"
     "warned docs/source/src/README.md:6 prose-to-write the text to write is a placeholder for a person to replace\n"
     "warned docs/source/src/README.md:7 prose-to-write the text to write is a placeholder for a person to replace\n",
     {"docs/source/src/README.md", NULL, NULL}},
    {"write-crlf-bom",
     "warned docs/source/src/README.md:4 prose-to-write the text to write is a placeholder for a person to replace\n",
     {"docs/source/src/README.md", NULL, NULL}},
    {"write-outside-untouched",
     "warned docs/source/src/README.md:5 prose-to-write the text to write is a placeholder for a person to replace\n",
     {"docs/source/src/README.md", "docs/guide.md", "docs/README.md"}},
    {"write-leaves-broken-file",
     "refused docs/source/src/README.md:3 generated-unterminated the block has no end marker\n",
     {"docs/source/src/README.md", NULL, NULL}},
};

static const write_case *find_case(const char *name) {
    size_t index;
    for (index = 0; index < sizeof write_cases / sizeof write_cases[0]; index++) {
        if (strcmp(write_cases[index].name, name) == 0) {
            return &write_cases[index];
        }
    }
    return NULL;
}

static char *join_path(const char *base, const char *middle, const char *name) {
    size_t length = strlen(base) + strlen(middle) + strlen(name) + 3;
    char *path = malloc(length);
    if (path != NULL) {
        int written = snprintf(path, length, "%s/%s%s%s", base, middle, middle[0] == '\0' ? "" : "/", name);
        if (written < 0 || (size_t)written >= length) {
            free(path);
            return NULL;
        }
    }
    return path;
}

static char *read_bytes(const char *path, size_t *length) {
    FILE *file = fopen(path, "rb");
    char *content = NULL;
    size_t capacity = 0;
    *length = 0;
    if (file == NULL) {
        return NULL;
    }
    for (;;) {
        char *grown;
        size_t got;
        if (*length == capacity) {
            capacity = capacity == 0 ? 4096 : capacity * 2;
            grown = realloc(content, capacity);
            if (grown == NULL) {
                free(content);
                fclose(file);
                return NULL;
            }
            content = grown;
        }
        got = fread(content + *length, 1, capacity - *length, file);
        *length += got;
        if (got == 0) {
            break;
        }
    }
    fclose(file);
    return content == NULL ? malloc(1) : content;
}

static int same_bytes(const char *actual_path, const char *expected_path) {
    size_t actual_length;
    size_t expected_length;
    char *actual = read_bytes(actual_path, &actual_length);
    char *expected = read_bytes(expected_path, &expected_length);
    int same = actual != NULL && expected != NULL && actual_length == expected_length &&
               memcmp(actual, expected, actual_length) == 0;
    free(actual);
    free(expected);
    return same;
}

static int report_matches(const char *label, report *out, const char *expected) {
    char *actual = report_format(out);
    int same = strcmp(actual, expected) == 0;
    if (!same) {
        fprintf(stderr, "%s report\nexpected:\n%s\nactual:\n%s\n", label, expected, actual);
    }
    free(actual);
    return same;
}

static int file_matches(const char *root, const char *relative) {
    char *actual = join_path(root, "", relative);
    char *expected = join_path(root, "expected", relative);
    int same = actual != NULL && expected != NULL && same_bytes(actual, expected);
    if (!same) {
        fprintf(stderr, "%s differs from expected/%s\n", relative, relative);
    }
    free(actual);
    free(expected);
    return same;
}

static int files_match(const char *root, const write_case *expected) {
    size_t index;
    int all = 1;
    for (index = 0; index < 3 && expected->files[index] != NULL; index++) {
        all &= file_matches(root, expected->files[index]);
    }
    return all;
}

static int run_case(const char *trees, const write_case *expected) {
    check_options options = {NULL, 0, 0};
    char *root = join_path(trees, "", expected->name);
    report written;
    report checked;
    int passed;
    if (root == NULL) {
        return 1;
    }
    report_init(&written);
    report_init(&checked);
    passed = regenerate(root, 1, &written) == 0 && report_matches("regenerate", &written, expected->report);
    passed = passed && files_match(root, expected);
    passed = passed && check_tree(root, &options, &checked) == 0 &&
             report_matches("check after write", &checked, expected->report);
    report_free(&written);
    report_free(&checked);
    free(root);
    return passed ? 0 : 1;
}

int main(int argument_count, char **arguments) {
    const write_case *expected;
    if (argument_count != 3) {
        fprintf(stderr, "usage: generate_test <write-trees folder> <case>\n");
        return 2;
    }
    expected = find_case(arguments[2]);
    if (expected == NULL) {
        fprintf(stderr, "no expectation is written for the fixture %s\n", arguments[2]);
        return 1;
    }
    return run_case(arguments[1], expected);
}
