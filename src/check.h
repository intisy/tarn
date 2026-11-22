#ifndef TARN_CHECK_H
#define TARN_CHECK_H

#include "context.h"
#include "report.h"

typedef struct {
    const char *snapshot_path;
    int quick;
    int write;
} check_options;

/** Checks the docs tree of the repo at repo_root into out; returns 0, or -1 when repo_root is not a folder. */
int check_tree(const char *repo_root, const check_options *options, report *out);

/**
 * Decides the tree kind and loads what the rules need into context. Returns -1 when repo_root is not a folder, 0 when
 * it is not a tarn docs tree (the refusal is in out), and 1 when context is ready. Close it after any non-negative
 * result.
 */
int check_context_open(const char *repo_root, report *out, check_context *context);
void check_context_close(check_context *context);

#endif
