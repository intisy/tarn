#ifndef TARN_CONTEXT_H
#define TARN_CONTEXT_H

#include "ignore.h"
#include "report.h"
#include "string_list.h"

typedef enum { TREE_REPO, TREE_ORG } tree_kind;

/** What every rule module needs to know about the tree being checked. */
typedef struct {
    const char *repo_root;
    tree_kind kind;
    const char *descent_root;
    ignore_rules ignore;
    string_list code_folders;
    report *out;
} check_context;

#endif
