#ifndef TARN_CONTEXT_H
#define TARN_CONTEXT_H

#include "ignore.h"
#include "report.h"
#include "snapshot.h"
#include "string_list.h"

typedef enum { TREE_REPO, TREE_ORG } tree_kind;

/** What every rule module needs to know about the tree being checked. */
typedef struct {
    const char *repo_root;
    tree_kind kind;
    int newest_version;
    const char *descent_root;
    ignore_rules ignore;
    string_list code_folders;
    /** The repos snapshot and the checked repo's line in it, both NULL when no snapshot was given. */
    const repos_snapshot *snapshot;
    const snapshot_repo *self;
    report *out;
} check_context;

#endif
