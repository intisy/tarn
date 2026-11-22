#ifndef TARN_WALK_H
#define TARN_WALK_H

#include "context.h"

/** Fills context->code_folders with every source folder that holds code, the repo root as "". */
void walk_find_code_folders(check_context *context);
void walk_descent_tree(check_context *context);
/** Refuses every code-holding source folder that has no doc folder under docs/source/. */
void walk_check_mirror_complete(check_context *context);

#endif
