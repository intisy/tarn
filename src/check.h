#ifndef TARN_CHECK_H
#define TARN_CHECK_H

#include "context.h"
#include "report.h"
#include "snapshot.h"

typedef struct {
    /** NULL when no snapshot was given; otherwise repository names the checked repo's line in it. */
    const repos_snapshot *snapshot;
    const char *repository;
    int quick;
    int write;
    /** The newest version a declared one is judged against; 0 means the newest this checker knows. */
    int newest_version;
} check_options;

#define CHECK_NEWEST_VERSION 1

enum { CHECK_DONE = 0, CHECK_NOT_A_FOLDER = -1, CHECK_REPOSITORY_UNKNOWN = -2 };

/**
 * Checks the docs tree of the repo at repo_root into out, rewriting stale generated blocks first when options->write is
 * set. Returns CHECK_DONE, CHECK_NOT_A_FOLDER, or CHECK_REPOSITORY_UNKNOWN when a snapshot is given and does not list
 * options->repository.
 */
int check_tree(const char *repo_root, const check_options *options, report *out);

/**
 * Decides the tree kind, judging the declared version against newest_version (0 for the newest this checker knows),
 * and loads what the rules need into context. Returns -1 when repo_root is not a folder, 0 when
 * it is not a tarn docs tree (the refusal is in out), and 1 when context is ready. Close it after any non-negative
 * result.
 */
int check_context_open(const char *repo_root, int newest_version, report *out, check_context *context);
/**
 * Opens a context for a tree of the given kind whatever version file it has or lacks, which is what adoption needs
 * before it writes one. Returns -1 when repo_root is not a folder and 1 otherwise; close it after 1.
 */
int check_context_open_as(const char *repo_root, tree_kind kind, report *out, check_context *context);
void check_context_close(check_context *context);
/**
 * Whether the repo carries a version file of the given kind that another kind's could not sit beside: a docs/VERSION
 * in any state, and a root VERSION reading `tarn <n>` (a root VERSION of another form is a project's own, not tarn's).
 */
int check_version_decides(const char *repo_root, tree_kind kind);
/** Whether relative names a file that is not a tarn version file, such as a project's own `VERSION` reading 1.2.3. */
int check_version_is_foreign(const char *repo_root, const char *relative);

#endif
