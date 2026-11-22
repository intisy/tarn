#include "check.h"
#include "report.h"
#include "snapshot.h"
#include "test_snapshot.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *name;
    int exit_code;
    const char *report;
} fixture_case;

/** A case listed here runs with the snapshot `<snapshots folder>/<snapshot>` and `--repository <repository>`. */
typedef struct {
    const char *name;
    const char *snapshot;
    const char *repository;
} snapshot_case;

#define SNAPSHOT_ABSENT                                                                                              \
    "warned .:0 snapshot-absent no repos snapshot was given, so public-names-private and link-into-nonstandard "   \
    "were skipped and org-repos blocks were left as they are\n"
#define NAMES_SECRET(line)                                                                                           \
    "refused docs/guide.md:" line " public-names-private names intisy/secret, which the snapshot marks private, " \
    "in a public docs tree\n"
#define LINKS_OLD(line)                                                                                              \
    "refused docs/guide.md:" line " link-into-nonstandard links into intisy/old, which the snapshot marks as not " \
    "following tarn\n"
#define ORG_LINE_FORM(line)                                                                                          \
    "refused README.md:" line " generated-line-form an org-repos line reads - [<owner>/<name>]"                    \
    "(https://github.com/<owner>/<name>) - <text>\n"
#define ORG_MISPLACED(path)                                                                                          \
    "refused " path ":3 generated-kind-misplaced org-repos belongs in the root README of an org docs tree\n"

/** A case listed here is judged against the newest version given beside it instead of the built-in one. */
typedef struct {
    const char *name;
    int newest_version;
} newest_case;

#define ORG_MISSES(repo)                                                                                             \
    "warned README.md:3 org-misses-repo does not list " repo ", which the snapshot gives for the org\n"

static const newest_case newest_cases[] = {
    {"older-version", 2},
};

static const snapshot_case snapshot_cases[] = {
    {"boundary-public-names-private", "intisy.snapshot", "intisy/public"},
    {"boundary-private-names-public", "intisy.snapshot", "intisy/secret"},
    {"boundary-link-into-nonstandard", "intisy.snapshot", "intisy/public"},
    {"boundary-link-into-itself", "intisy.snapshot", "intisy/old"},
    {"org-repos-stale", "intisy.snapshot", "intisy/docs"},
    {"org-repos-private", "intisy.snapshot", "intisy/records"},
    {"org-repos-gone", "intisy.snapshot", "intisy/docs"},
    {"org-repos-reordered", "intisy.snapshot", "intisy/docs"},
};

static const fixture_case fixture_cases[] = {
    {"missing-readme", 1,
     "refused docs/guide:0 missing-readme folder has no README.md\n" SNAPSHOT_ABSENT},
    {"readme-misses-child", 1,
     "refused docs/README.md:0 readme-misses-child does not list b.md\n" SNAPSHOT_ABSENT},
    {"readme-names-gone-child", 1,
     "refused docs/README.md:4 readme-names-gone-child lists gone.md, which does not exist\n" SNAPSHOT_ABSENT},
    {"readme-case-mismatch", 1,
     "refused docs/README.md:3 readme-case-mismatch lists communication.md, but the child is Communication.md\n"
     SNAPSHOT_ABSENT},
    {"readme-form", 1,
     "refused docs/README.md:4 readme-form a child line reads - [<name>](<name>) - <one line>\n"
     "refused docs/README.md:5 readme-form a child line's text and target both name the child\n"
     "refused docs/README.md:7 readme-form only child lines follow the opening paragraph\n"
     "refused docs/README.md:0 readme-misses-child does not list notes.md\n" SNAPSHOT_ABSENT},
    {"readme-to-write", 0,
     "warned docs/README.md:1 prose-to-write the text to write is a placeholder for a person to replace\n"
     "warned docs/README.md:3 prose-to-write the text to write is a placeholder for a person to replace\n"
     SNAPSHOT_ABSENT},
    {"kind-not-allowed", 1,
     "refused docs/TASKS.md:0 kind-not-allowed TASKS.md is a record, and a repo docs tree holds no records\n"
     "refused docs/data.json:0 kind-not-allowed a descent tree holds only README.md, prose .md files and folders\n"
     SNAPSHOT_ABSENT},
    {"source-folder-undocumented", 1,
     "refused docs/source/lib:0 source-folder-undocumented lib holds code and has no doc folder\n" SNAPSHOT_ABSENT},
    {"source-doc-for-gone-folder", 1,
     "refused docs/source/old:0 source-doc-for-gone-folder old does not exist\n" SNAPSHOT_ABSENT},
    {"source-readme-children", 1,
     "refused docs/source/src/README.md:4 readme-names-gone-child lists gone.c, which does not exist\n"
     "refused docs/source/src/README.md:5 readme-case-mismatch lists B.c, but the child is b.c\n" SNAPSHOT_ABSENT},
    {"dead-link", 1,
     "refused docs/guide.md:3 dead-link missing.md does not resolve\n"
     "refused docs/guide.md:4 dead-link readme.md differs in case from the path on disk\n"
     "refused docs/guide.md:5 dead-link ../../outside.md leaves the repo\n" SNAPSHOT_ABSENT},
    {"dir-link-resolves", 0, SNAPSHOT_ABSENT},
    {"anchor-link", 0, SNAPSHOT_ABSENT},
    {"crlf-bom", 0, SNAPSHOT_ABSENT},
    {"ignored-folders", 0, SNAPSHOT_ABSENT},
    {"size-over-budget", 0,
     "warned docs/long.md:401 size-over-budget 450 lines, over the budget of 400\n" SNAPSHOT_ABSENT},
    {"size-justified", 0, SNAPSHOT_ABSENT},
    {"org-tree", 0, SNAPSHOT_ABSENT},
    {"version-malformed", 1,
     "refused docs/VERSION:1 version-malformed the file must be exactly one line, tarn <n>\n"},
    {"directory-link", 0, SNAPSHOT_ABSENT},
    {"version-twice", 1,
     "refused VERSION:1 version-twice a repo carries one version file, and this one has VERSION and docs/VERSION\n"},
    {"version-missing", 1,
     "refused docs/VERSION:0 version-missing there is no docs/VERSION and no root VERSION, so this is not a tarn "
     "docs tree\n"},
    {"org-kinds", 1,
     "refused architecture/TASKS.md:0 kind-not-allowed TASKS.md is a record, and records sit at the root of an org "
     "docs tree only\n"
     "refused source:0 kind-not-allowed source/ is the source mirror of a repo docs tree, and an org docs tree has "
     "none\n" SNAPSHOT_ABSENT},
    {"generated-stale", 1,
     "refused docs/source/src/README.md:3 generated-block-stale the block differs from the text tarn-check "
     "generates, and --write rewrites it\n" SNAPSHOT_ABSENT},
    {"generated-current", 0,
     "warned docs/source/src/README.md:5 prose-to-write the text to write is a placeholder for a person to replace\n"
     SNAPSHOT_ABSENT},
    {"generated-kind-unknown", 1,
     "refused docs/README.md:3 generated-kind-unknown tarn-index is not a generated kind of version 1\n"
     SNAPSHOT_ABSENT},
    {"generated-unterminated", 1,
     "refused docs/README.md:3 generated-unterminated the block has no end marker\n" SNAPSHOT_ABSENT},
    {"generated-marker-unmatched", 1,
     "refused docs/README.md:4 generated-marker-unmatched an end marker has no start marker\n" SNAPSHOT_ABSENT},
    {"generated-nested", 1,
     "refused docs/README.md:4 generated-nested a block starts inside the block that started at line 3\n"
     SNAPSHOT_ABSENT},
    {"generated-misplaced", 1,
     "refused docs/README.md:3 generated-kind-misplaced source-children belongs in the README of a folder under "
     "docs/source\n" SNAPSHOT_ABSENT},
    {"generated-org-repos", 0,
     "warned README.md:4 prose-to-write the text to write is a placeholder for a person to replace\n" SNAPSHOT_ABSENT},
    {"generated-duplicate", 1,
     "refused README.md:6 generated-block-duplicate a second org-repos block in one file duplicates the first, "
     "which started at line 4\n" SNAPSHOT_ABSENT},
    {"generated-fenced", 0, SNAPSHOT_ABSENT},
    {"version-unknown", 1,
     "refused docs/VERSION:1 version-unknown declares tarn 2, and this checker knows version 1 only\n"},
    {"boundary-public-names-private", 1,
     NAMES_SECRET("3") NAMES_SECRET("4") NAMES_SECRET("5") NAMES_SECRET("7") NAMES_SECRET("8")
         NAMES_SECRET("9") NAMES_SECRET("10") NAMES_SECRET("11") NAMES_SECRET("12") NAMES_SECRET("15")},
    {"boundary-private-names-public", 0, ""},
    {"boundary-link-into-nonstandard", 1, LINKS_OLD("3") LINKS_OLD("4") LINKS_OLD("9") LINKS_OLD("10")},
    {"boundary-link-into-itself", 0, ""},
    {"boundary-without-snapshot", 0, SNAPSHOT_ABSENT},
    {"org-repos-misplaced", 1, ORG_MISPLACED("architecture/README.md") SNAPSHOT_ABSENT},
    {"org-repos-in-repo-tree", 1, ORG_MISPLACED("docs/README.md") SNAPSHOT_ABSENT},
    {"org-repos-line-form", 1,
     ORG_LINE_FORM("5") ORG_LINE_FORM("6") ORG_LINE_FORM("7") ORG_LINE_FORM("8") SNAPSHOT_ABSENT},
    {"org-repos-stale", 0, ORG_MISSES("intisy/old") ORG_MISSES("intisy/public")},
    {"org-repos-gone", 0,
     "warned README.md:4 org-names-gone-repo lists intisy/gone, which the snapshot does not list\n"},
    {"org-repos-reordered", 0,
     "warned README.md:3 generated-block-stale the block differs from the text tarn-check generates, and --write "
     "rewrites it\n"},
    {"older-version", 0,
     "warned docs/VERSION:1 older-version declares tarn 1, and the newest is tarn 2, so the tree is pinned until it "
     "moves\n" SNAPSHOT_ABSENT},
    {"source-doc-stale", 0,
     "warned docs/source:0 source-doc-stale the repo root has commits 30 days newer than its doc, 2023-11-14 "
     "against 2023-10-15\n"
     "warned docs/source/src:0 source-doc-stale src has commits 20 days newer than its doc, 2023-11-14 against "
     "2023-10-25\n" SNAPSHOT_ABSENT},
    {"source-doc-shallow", 0,
     "warned .:0 staleness-skipped the clone is shallow, so its commit dates cannot show a stale source doc and "
     "source-doc-stale was skipped\n" SNAPSHOT_ABSENT},
    {"org-repos-private", 0, ""},
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

static const snapshot_case *find_snapshot_case(const char *name) {
    size_t index;
    for (index = 0; index < sizeof snapshot_cases / sizeof snapshot_cases[0]; index++) {
        if (strcmp(snapshot_cases[index].name, name) == 0) {
            return &snapshot_cases[index];
        }
    }
    return NULL;
}

static int newest_version_for(const char *name) {
    size_t index;
    for (index = 0; index < sizeof newest_cases / sizeof newest_cases[0]; index++) {
        if (strcmp(newest_cases[index].name, name) == 0) {
            return newest_cases[index].newest_version;
        }
    }
    return 0;
}

static int check_case(const char *root, const check_options *options, const fixture_case *expected) {
    report out;
    char *actual;
    int exit_code;
    int passed;
    report_init(&out);
    if (check_tree(root, options, &out) != CHECK_DONE) {
        fprintf(stderr, "check_tree could not check %s\n", root);
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
    report_free(&out);
    return passed ? 0 : 1;
}

static int run_case(const char *trees, const char *snapshots, const fixture_case *expected) {
    const snapshot_case *with_snapshot = find_snapshot_case(expected->name);
    check_options options = {NULL, NULL, 0, 0, 0};
    repos_snapshot snapshot = {NULL, 0};
    char *root = test_path_join(trees, expected->name);
    int failed;
    if (root == NULL) {
        return 1;
    }
    options.newest_version = newest_version_for(expected->name);
    if (with_snapshot != NULL) {
        if (!test_snapshot_load(snapshots, with_snapshot->snapshot, &snapshot)) {
            free(root);
            return 1;
        }
        options.snapshot = &snapshot;
        options.repository = with_snapshot->repository;
    }
    failed = check_case(root, &options, expected);
    snapshot_free(&snapshot);
    free(root);
    return failed;
}

int main(int argument_count, char **arguments) {
    const fixture_case *expected;
    if (argument_count != 4) {
        fprintf(stderr, "usage: check_test <trees folder> <case> <snapshots folder>\n");
        return 2;
    }
    expected = find_case(arguments[2]);
    if (expected == NULL) {
        fprintf(stderr, "no expectation is written for the fixture %s\n", arguments[2]);
        return 1;
    }
    return run_case(arguments[1], arguments[3], expected);
}
