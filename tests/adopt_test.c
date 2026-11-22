#include "adopt.h"
#include "check.h"
#include "directory.h"
#include "memory.h"
#include "report.h"
#include "snapshot.h"
#include "test_snapshot.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STANDARD_REF "0123456789abcdef0123456789abcdef01234567"
#define MOST_FILES 6

typedef struct {
    const char *name;
    adopt_kind kind;
    /** The plan of the first run, which a run with --write must print too. */
    const char *plan;
    /** Files compared byte for byte with their `.expected/` twin after the write. */
    const char *files[MOST_FILES];
    /** The report tarn-check gives the adopted tree, refusing what a kept file leaves wrong. */
    const char *report;
    /** NULL, or the snapshot file and the repository both the adoption and the check run with. */
    const char *snapshot;
    const char *repository;
} adopt_case;

typedef struct {
    char *data;
    size_t length;
} byte_buffer;

#define SNAPSHOT_ABSENT                                                                                              \
    "warned .:0 snapshot-absent no repos snapshot was given, so public-names-private and link-into-nonstandard "   \
    "were skipped and org-repos blocks were left as they are\n"
#define TO_WRITE(path, line)                                                                                         \
    "warned " path ":" line " prose-to-write the text to write is a placeholder for a person to replace\n"

static const adopt_case adopt_cases[] = {
    {"adopt-repo",
     ADOPT_REPO,
     "create docs/VERSION\n"
     "create docs/README.md\n"
     "create docs/source/README.md\n"
     "create docs/source/src/README.md\n"
     "create docs/source/src/net/README.md\n"
     "create .github/workflows/docs.yml\n",
     {"docs/VERSION", "docs/README.md", "docs/source/README.md", "docs/source/src/README.md",
      "docs/source/src/net/README.md", NULL},
     TO_WRITE("docs/source/README.md", "4") TO_WRITE("docs/source/README.md", "5")
         TO_WRITE("docs/source/src/README.md", "4") TO_WRITE("docs/source/src/README.md", "5")
             TO_WRITE("docs/source/src/net/README.md", "4") TO_WRITE("docs/source/src/net/README.md", "5")
                 TO_WRITE("docs/README.md", "1") TO_WRITE("docs/source/README.md", "1")
                     TO_WRITE("docs/source/src/README.md", "1") TO_WRITE("docs/source/src/net/README.md", "1")
                         SNAPSHOT_ABSENT,
     NULL,
     NULL},
    {"adopt-repo-keeps",
     ADOPT_REPO,
     "create docs/VERSION\n"
     "keep docs/README.md\n"
     "create docs/source/README.md\n"
     "keep docs/source/src/README.md\n"
     "create .github/workflows/docs.yml\n",
     {"docs/VERSION", "docs/README.md", "docs/source/README.md", "docs/source/src/README.md", NULL, NULL},
     TO_WRITE("docs/source/README.md", "4") TO_WRITE("docs/source/README.md", "1") SNAPSHOT_ABSENT,
     NULL,
     NULL},
    {"adopt-repo-case-differs",
     ADOPT_REPO,
     "create docs/VERSION\n"
     "keep docs/README.md\n"
     "create docs/source/README.md\n"
     "create docs/source/src/README.md\n"
     "create .github/workflows/docs.yml\n",
     {"docs/readme.md", NULL, NULL, NULL, NULL, NULL},
     TO_WRITE("docs/source/README.md", "4") TO_WRITE("docs/source/src/README.md", "4")
         "refused docs:0 missing-readme folder has no README.md\n" TO_WRITE("docs/source/README.md", "1")
             TO_WRITE("docs/source/src/README.md", "1") SNAPSHOT_ABSENT,
     NULL,
     NULL},
    {"adopt-org",
     ADOPT_ORG,
     "create VERSION\n"
     "create README.md\n"
     "create .github/workflows/docs.yml\n"
     "does-not-follow intisy/old\n",
     {"VERSION", "README.md", NULL, NULL, NULL, NULL},
     TO_WRITE("README.md", "8") TO_WRITE("README.md", "9") TO_WRITE("README.md", "10") TO_WRITE("README.md", "1")
         TO_WRITE("README.md", "3") TO_WRITE("README.md", "4") TO_WRITE("README.md", "5"),
     "intisy.snapshot",
     "intisy/docs"},
};

/** Lines of the caller asserted on their own, so a wrong template cannot pass by being compared with itself. */
static const char *const caller_lines[] = {
    "  push:\n",
    "  pull_request:\n",
    "  workflow_dispatch:\n",
    "  schedule:\n",
    "  contents: read\n",
    "  issues: write\n",
    "    uses: intisy/tarn/.github/workflows/check-docs.yml@" STANDARD_REF "\n",
    "      standard-ref: " STANDARD_REF "\n",
    "      mode: check\n",
    "      mode: warn\n",
    "      default-branch: ${{ github.ref_name }}\n",
};

static const adopt_case *find_case(const char *name) {
    size_t index;
    for (index = 0; index < sizeof adopt_cases / sizeof adopt_cases[0]; index++) {
        if (strcmp(adopt_cases[index].name, name) == 0) {
            return &adopt_cases[index];
        }
    }
    return NULL;
}

static void buffer_append(byte_buffer *buffer, const char *bytes, size_t count) {
    buffer->data = memory_resize(buffer->data, buffer->length + count + 1);
    memcpy(buffer->data + buffer->length, bytes, count);
    buffer->length += count;
    buffer->data[buffer->length] = '\0';
}

static void buffer_append_text(byte_buffer *buffer, const char *text) {
    buffer_append(buffer, text, strlen(text));
}

/** Every folder and file under root with each file's bytes, so two trees compare as one string each. */
static void fingerprint_folder(const char *root, const char *relative, byte_buffer *out) {
    char *folder = relative[0] == '\0' ? memory_copy_string(root) : test_path_join(root, relative);
    directory_listing listing;
    size_t index;
    if (folder == NULL || directory_list(folder, &listing) != 0) {
        buffer_append_text(out, "unlistable ");
        buffer_append_text(out, relative);
        buffer_append_text(out, "\n");
        free(folder);
        return;
    }
    for (index = 0; index < listing.count; index++) {
        const directory_entry *entry = &listing.entries[index];
        char *child = relative[0] == '\0' ? memory_copy_string(entry->name) : test_path_join(relative, entry->name);
        buffer_append_text(out, entry->is_directory ? "folder " : "file ");
        buffer_append_text(out, child);
        buffer_append_text(out, "\n");
        if (entry->is_directory) {
            fingerprint_folder(root, child, out);
        } else {
            char *path = test_path_join(root, child);
            char *content;
            size_t length;
            if (path != NULL && directory_read_file(path, &content, &length) == 0) {
                buffer_append(out, content, length);
                free(content);
            }
            free(path);
        }
        free(child);
    }
    directory_listing_free(&listing);
    free(folder);
}

static char *fingerprint(const char *root) {
    byte_buffer out = {NULL, 0};
    buffer_append_text(&out, "");
    fingerprint_folder(root, "", &out);
    return out.data;
}

static char *joined_lines(const string_list *lines) {
    byte_buffer out = {NULL, 0};
    size_t index;
    buffer_append_text(&out, "");
    for (index = 0; index < lines->count; index++) {
        buffer_append_text(&out, lines->items[index]);
        buffer_append_text(&out, "\n");
    }
    return out.data;
}

/** The plan a second adoption prints: every file kept, the does-not-follow lines unchanged. */
static char *kept_plan(const char *plan) {
    byte_buffer out = {NULL, 0};
    const char *line = plan;
    buffer_append_text(&out, "");
    while (*line != '\0') {
        const char *end = strchr(line, '\n');
        size_t length = end == NULL ? strlen(line) : (size_t)(end - line) + 1;
        if (strncmp(line, "create ", 7) == 0) {
            buffer_append_text(&out, "keep ");
            buffer_append(&out, line + 7, length - 7);
        } else {
            buffer_append(&out, line, length);
        }
        line += length;
    }
    return out.data;
}

static int text_matches(const char *label, const char *actual, const char *expected) {
    int same = strcmp(actual, expected) == 0;
    if (!same) {
        fprintf(stderr, "%s\nexpected:\n%s\nactual:\n%s\n", label, expected, actual);
    }
    return same;
}

static int adopt_and_compare(const char *root, adopt_options *options, const char *label, const char *expected) {
    adopt_outcome outcome;
    int result;
    char *plan;
    int passed;
    adopt_outcome_init(&outcome);
    result = adopt_tree(root, options, &outcome);
    plan = joined_lines(&outcome.lines);
    passed = result == ADOPT_DONE && text_matches(label, plan, expected);
    if (result != ADOPT_DONE) {
        fprintf(stderr, "%s: adopt_tree returned %d\n", label, result);
    }
    free(plan);
    adopt_outcome_free(&outcome);
    return passed;
}

static int same_bytes(const char *actual_path, const char *expected_path) {
    char *actual;
    char *expected;
    size_t actual_length;
    size_t expected_length;
    int same;
    if (directory_read_file(actual_path, &actual, &actual_length) != 0) {
        return 0;
    }
    if (directory_read_file(expected_path, &expected, &expected_length) != 0) {
        free(actual);
        return 0;
    }
    same = actual_length == expected_length && memcmp(actual, expected, actual_length) == 0;
    free(actual);
    free(expected);
    return same;
}

static int files_match(const char *root, const adopt_case *expected) {
    char *expected_folder = test_path_join(root, ".expected");
    size_t index;
    int all = expected_folder != NULL;
    for (index = 0; all && index < MOST_FILES && expected->files[index] != NULL; index++) {
        char *actual = test_path_join(root, expected->files[index]);
        char *twin = test_path_join(expected_folder, expected->files[index]);
        if (actual == NULL || twin == NULL || !same_bytes(actual, twin)) {
            fprintf(stderr, "%s differs from .expected/%s\n", expected->files[index], expected->files[index]);
            all = 0;
        }
        free(actual);
        free(twin);
    }
    free(expected_folder);
    return all;
}

static int caller_matches(const char *root) {
    char *path = test_path_join(root, ADOPT_CALLER_PATH);
    char *expected = adopt_caller_text(STANDARD_REF);
    char *content = NULL;
    size_t length;
    size_t index;
    int passed = path != NULL && directory_read_file(path, &content, &length) == 0;
    passed = passed && text_matches("the workflow caller", content, expected);
    for (index = 0; passed && index < sizeof caller_lines / sizeof caller_lines[0]; index++) {
        if (strstr(content, caller_lines[index]) == NULL) {
            fprintf(stderr, "the workflow caller lacks the line %s", caller_lines[index]);
            passed = 0;
        }
    }
    free(content);
    free(expected);
    free(path);
    return passed;
}

static int check_matches(const char *root, const adopt_options *adoption, const char *expected) {
    check_options options = {NULL, NULL, 0, 0, 0};
    report out;
    char *actual;
    int passed;
    options.snapshot = adoption->snapshot;
    options.repository = adoption->repository;
    report_init(&out);
    passed = check_tree(root, &options, &out) == CHECK_DONE;
    actual = report_format(&out);
    passed = passed && text_matches("the check of the adopted tree", actual, expected);
    free(actual);
    report_free(&out);
    return passed;
}

static int tree_unchanged(const char *label, const char *root, const char *before) {
    char *after = fingerprint(root);
    int same = strcmp(before, after) == 0;
    if (!same) {
        fprintf(stderr, "%s changed the tree\n", label);
    }
    free(after);
    return same;
}

static int run_dry(const char *root, adopt_options *options, const adopt_case *expected) {
    char *before = fingerprint(root);
    int passed;
    options->write = 0;
    passed = adopt_and_compare(root, options, "the plan without --write", expected->plan);
    passed = passed && tree_unchanged("a run without --write", root, before);
    free(before);
    return passed;
}

static int run_again(const char *root, adopt_options *options, const adopt_case *expected) {
    char *before = fingerprint(root);
    char *plan = kept_plan(expected->plan);
    int passed;
    options->write = 1;
    passed = adopt_and_compare(root, options, "the plan of a second adoption", plan);
    passed = passed && tree_unchanged("a second adoption", root, before);
    free(plan);
    free(before);
    return passed;
}

static int run_adoption(const char *root, adopt_options *options, const adopt_case *expected) {
    int passed = run_dry(root, options, expected);
    options->write = 1;
    passed = passed && adopt_and_compare(root, options, "the plan with --write", expected->plan);
    passed = passed && files_match(root, expected) && caller_matches(root);
    passed = passed && check_matches(root, options, expected->report);
    return passed && run_again(root, options, expected);
}

static int run_case(const char *trees, const char *snapshots, const adopt_case *expected) {
    adopt_options options = {ADOPT_REPO, STANDARD_REF, 0, NULL, NULL};
    repos_snapshot snapshot = {NULL, 0};
    char *root = test_path_join(trees, expected->name);
    int passed = root != NULL;
    options.kind = expected->kind;
    if (passed && expected->snapshot != NULL) {
        passed = test_snapshot_load(snapshots, expected->snapshot, &snapshot);
        options.snapshot = &snapshot;
        options.repository = expected->repository;
    }
    passed = passed && run_adoption(root, &options, expected);
    snapshot_free(&snapshot);
    free(root);
    return passed ? 0 : 1;
}

int main(int argument_count, char **arguments) {
    const adopt_case *expected;
    if (argument_count != 4) {
        fprintf(stderr, "usage: adopt_test <adopt-trees folder> <case> <snapshots folder>\n");
        return 2;
    }
    expected = find_case(arguments[2]);
    if (expected == NULL) {
        fprintf(stderr, "no expectation is written for the fixture %s\n", arguments[2]);
        return 1;
    }
    return run_case(arguments[1], arguments[3], expected);
}
