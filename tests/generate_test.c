#include "check.h"
#include "report.h"
#include "snapshot.h"
#include "test_snapshot.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *name;
    const char *report;
    const char *files[3];
    /** NULL, or the snapshot file and the repository the case runs with. */
    const char *snapshot;
    const char *repository;
} write_case;

#define SNAPSHOT_ABSENT                                                                                              \
    "warned .:0 snapshot-absent no repos snapshot was given, so public-names-private and link-into-nonstandard "   \
    "were skipped and org-repos blocks were left as they are\n"
#define TO_WRITE(path, line)                                                                                         \
    "warned " path ":" line " prose-to-write the text to write is a placeholder for a person to replace\n"

static const write_case write_cases[] = {
    {"write-source-children",
     TO_WRITE("docs/source/src/README.md", "4") TO_WRITE("docs/source/src/README.md", "6")
         TO_WRITE("docs/source/src/README.md", "7") SNAPSHOT_ABSENT,
     {"docs/source/src/README.md", NULL, NULL},
     NULL,
     NULL},
    {"write-crlf-bom",
     TO_WRITE("docs/source/src/README.md", "4") SNAPSHOT_ABSENT,
     {"docs/source/src/README.md", NULL, NULL},
     NULL,
     NULL},
    {"write-outside-untouched",
     TO_WRITE("docs/source/src/README.md", "5") SNAPSHOT_ABSENT,
     {"docs/source/src/README.md", "docs/guide.md", "docs/README.md"},
     NULL,
     NULL},
    {"write-leaves-broken-file",
     "refused docs/source/src/README.md:3 generated-unterminated the block has no end marker\n" SNAPSHOT_ABSENT,
     {"docs/source/src/README.md", NULL, NULL},
     NULL,
     NULL},
    {"write-org-repos", TO_WRITE("README.md", "5"), {"README.md", NULL, NULL}, "intisy.snapshot", "intisy/docs"},
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
    char *actual = test_path_join(root, relative);
    char *expected_folder = test_path_join(root, ".expected");
    char *expected = expected_folder == NULL ? NULL : test_path_join(expected_folder, relative);
    int same = actual != NULL && expected != NULL && same_bytes(actual, expected);
    if (!same) {
        fprintf(stderr, "%s differs from .expected/%s\n", relative, relative);
    }
    free(actual);
    free(expected);
    free(expected_folder);
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

static int check_report_matches(const char *label, const char *root, const check_options *options,
                                const char *expected) {
    report out;
    int same;
    report_init(&out);
    same = check_tree(root, options, &out) == CHECK_DONE && report_matches(label, &out, expected);
    report_free(&out);
    return same;
}

/** Runs `--write`, compares the files, then checks again and expects the same report, so no stale block remains. */
static int write_and_check(const char *root, check_options *options, const write_case *expected) {
    int passed;
    options->write = 1;
    passed = check_report_matches("write", root, options, expected->report);
    passed = passed && files_match(root, expected);
    options->write = 0;
    return passed && check_report_matches("check after write", root, options, expected->report);
}

static int run_case(const char *trees, const char *snapshots, const write_case *expected) {
    check_options options = {NULL, NULL, 0, 0};
    repos_snapshot snapshot = {NULL, 0};
    char *root = test_path_join(trees, expected->name);
    int passed = root != NULL;
    if (passed && expected->snapshot != NULL) {
        passed = test_snapshot_load(snapshots, expected->snapshot, &snapshot);
        options.snapshot = &snapshot;
        options.repository = expected->repository;
    }
    passed = passed && write_and_check(root, &options, expected);
    snapshot_free(&snapshot);
    free(root);
    return passed ? 0 : 1;
}

int main(int argument_count, char **arguments) {
    const write_case *expected;
    if (argument_count != 4) {
        fprintf(stderr, "usage: generate_test <write-trees folder> <case> <snapshots folder>\n");
        return 2;
    }
    expected = find_case(arguments[2]);
    if (expected == NULL) {
        fprintf(stderr, "no expectation is written for the fixture %s\n", arguments[2]);
        return 1;
    }
    return run_case(arguments[1], arguments[3], expected);
}
