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
} check_options;

enum { CHECK_DONE = 0, CHECK_NOT_A_FOLDER = -1, CHECK_REPOSITORY_UNKNOWN = -2 };

/**
 * Checks the docs tree of the repo at repo_root into out, rewriting stale generated blocks first when options->write is
 * set. Returns CHECK_DONE, CHECK_NOT_A_FOLDER, or CHECK_REPOSITORY_UNKNOWN when a snapshot is given and does not list
 * options->repository.
 */
int check_tree(const char *repo_root, const check_options *options, report *out);

/**
 * Decides the tree kind and loads what the rules need into context. Returns -1 when repo_root is not a folder, 0 when
 * it is not a tarn docs tree (the refusal is in out), and 1 when context is ready. Close it after any non-negative
 * result.
 */
int check_context_open(const char *repo_root, report *out, check_context *context);
void check_context_close(check_context *context);

#endif
