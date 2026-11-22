#ifndef TARN_CHECK_H
#define TARN_CHECK_H

#include "report.h"

typedef struct {
    const char *snapshot_path;
    int quick;
} check_options;

/** Checks the docs tree of the repo at repo_root into out; returns 0, or -1 when repo_root is not a folder. */
int check_tree(const char *repo_root, const check_options *options, report *out);

#endif
