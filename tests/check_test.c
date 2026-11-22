#include "check.h"
#include "report.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *name;
    int exit_code;
    const char *report;
} fixture_case;

static const fixture_case fixture_cases[] = {
    {"missing-readme", 1,
     "refused docs/guide:0 missing-readme folder has no README.md\n"},
    {"readme-misses-child", 1,
     "refused docs/README.md:0 readme-misses-child does not list b.md\n"},
    {"readme-names-gone-child", 1,
     "refused docs/README.md:4 readme-names-gone-child lists gone.md, which does not exist\n"},
    {"readme-case-mismatch", 1,
     "refused docs/README.md:3 readme-case-mismatch lists communication.md, but the child is Communication.md\n"},
    {"readme-form", 1,
     "refused docs/README.md:4 readme-form a child line reads - [<name>](<name>) - <one line>\n"
     "refused docs/README.md:5 readme-form a child line's text and target both name the child\n"
     "refused docs/README.md:7 readme-form only child lines follow the opening paragraph\n"
     "refused docs/README.md:0 readme-misses-child does not list notes.md\n"},
    {"kind-not-allowed", 1,
     "refused docs/TASKS.md:0 kind-not-allowed TASKS.md is a record, and a repo docs tree holds no records\n"
     "refused docs/data.json:0 kind-not-allowed a descent tree holds only README.md, prose .md files and folders\n"},
    {"source-folder-undocumented", 1,
     "refused docs/source/lib:0 source-folder-undocumented lib holds code and has no doc folder\n"},
    {"source-doc-for-gone-folder", 1,
     "refused docs/source/old:0 source-doc-for-gone-folder old does not exist\n"},
    {"source-readme-children", 1,
     "refused docs/source/src/README.md:4 readme-names-gone-child lists gone.c, which does not exist\n"
     "refused docs/source/src/README.md:5 readme-case-mismatch lists B.c, but the child is b.c\n"},
    {"dead-link", 1,
     "refused docs/guide.md:3 dead-link missing.md does not resolve\n"
     "refused docs/guide.md:4 dead-link readme.md differs in case from the path on disk\n"
     "refused docs/guide.md:5 dead-link ../../outside.md leaves the repo\n"},
    {"dir-link-resolves", 0, ""},
    {"anchor-link", 0, ""},
    {"crlf-bom", 0, ""},
    {"ignored-folders", 0, ""},
    {"size-over-budget", 0,
     "warned docs/long.md:401 size-over-budget 450 lines, over the budget of 400\n"},
    {"size-justified", 0, ""},
    {"org-tree", 0, ""},
    {"version-malformed", 1,
     "refused docs/VERSION:1 version-malformed the file must be exactly one line, tarn <n>\n"},
    {"directory-link", 0, ""},
    {"version-twice", 1,
     "refused VERSION:1 version-twice a repo carries one version file, and this one has VERSION and docs/VERSION\n"},
    {"version-missing", 1,
     "refused docs/VERSION:0 version-missing there is no docs/VERSION and no root VERSION, so this is not a tarn "
     "docs tree\n"},
    {"org-kinds", 1,
     "refused architecture/TASKS.md:0 kind-not-allowed TASKS.md is a record, and records sit at the root of an org "
     "docs tree only\n"
     "refused source:0 kind-not-allowed source/ is the source mirror of a repo docs tree, and an org docs tree has "
     "none\n"},
    {"generated-stale", 1,
     "refused docs/source/src/README.md:3 generated-block-stale the block differs from the text tarn-check "
     "generates, and --write rewrites it\n"},
    {"generated-current", 0,
     "warned docs/source/src/README.md:5 prose-to-write the text to write is a placeholder for a person to replace\n"},
    {"generated-kind-unknown", 1,
     "refused docs/README.md:3 generated-kind-unknown tarn-index is not a generated kind of version 1\n"},
    {"generated-unterminated", 1,
     "refused docs/README.md:3 generated-unterminated the block has no end marker\n"},
    {"generated-marker-unmatched", 1,
     "refused docs/README.md:4 generated-marker-unmatched an end marker has no start marker\n"},
    {"generated-nested", 1,
     "refused docs/README.md:4 generated-nested a block starts inside the block that started at line 3\n"},
    {"generated-misplaced", 1,
     "refused docs/README.md:3 generated-kind-misplaced source-children belongs in the README of a folder under "
     "docs/source\n"},
    {"generated-org-repos", 0,
     "warned README.md:4 prose-to-write the text to write is a placeholder for a person to replace\n"},
    {"generated-duplicate", 1,
     "refused docs/README.md:6 generated-block-duplicate a second org-repos block in one file duplicates the first, "
     "which started at line 4\n"},
    {"generated-fenced", 0, ""},
    {"version-unknown", 1,
     "refused docs/VERSION:1 version-unknown declares tarn 2, and this checker knows version 1 only\n"},
};

static const fixture_case *find_case(const char *name) {
    size_t index;
    for (index = 0; index < sizeof fixture_cases / sizeof fixture_cases[0]; index++) {
        if (strcmp(fixture_cases[index].name, name) == 0) {
            return &fixture_cases[index];
        }
    }
    return NULL;
}

static char *join_tree_path(const char *trees, const char *name) {
    size_t trees_length = strlen(trees);
    size_t name_length = strlen(name);
    char *path = malloc(trees_length + name_length + 2);
    if (path == NULL) {
        return NULL;
    }
    memcpy(path, trees, trees_length);
    path[trees_length] = '/';
    memcpy(path + trees_length + 1, name, name_length + 1);
    return path;
}

static int run_case(const char *trees, const fixture_case *expected) {
    check_options options = {NULL, 0, 0};
    report out;
    char *root = join_tree_path(trees, expected->name);
    char *actual;
    int exit_code;
    int passed;
    if (root == NULL) {
        return 1;
    }
    report_init(&out);
    if (check_tree(root, &options, &out) != 0) {
        fprintf(stderr, "check_tree could not read %s\n", root);
        free(root);
        report_free(&out);
        return 1;
    }
    actual = report_format(&out);
    exit_code = report_exit_code(&out);
    passed = strcmp(actual, expected->report) == 0 && exit_code == expected->exit_code;
    if (!passed) {
        fprintf(stderr, "case %s\nexpected exit %d, report:\n%s\nactual exit %d, report:\n%s\n", expected->name,
                expected->exit_code, expected->report, exit_code, actual);
    }
    free(actual);
    free(root);
    report_free(&out);
    return passed ? 0 : 1;
}

int main(int argument_count, char **arguments) {
    const fixture_case *expected;
    if (argument_count != 3) {
        fprintf(stderr, "usage: check_test <trees folder> <case>\n");
        return 2;
    }
    expected = find_case(arguments[2]);
    if (expected == NULL) {
        fprintf(stderr, "no expectation is written for the fixture %s\n", arguments[2]);
        return 1;
    }
    return run_case(arguments[1], expected);
}
