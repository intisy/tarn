#ifndef TARN_GENERATE_H
#define TARN_GENERATE_H

#include "context.h"
#include "report.h"

/**
 * Regenerates the marked blocks of the descent tree at repo_root: a stale block is refused as generated-block-stale,
 * or rewritten in place when write is set. Returns 0, or -1 when repo_root is not a folder.
 */
int regenerate(const char *repo_root, int write, report *out);
/** The same over a context the caller has opened; check_tree runs it before the walk so the walk reads fresh files. */
void generate_blocks(const check_context *context, int write);

#endif
