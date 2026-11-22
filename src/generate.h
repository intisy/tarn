#ifndef TARN_GENERATE_H
#define TARN_GENERATE_H

#include "context.h"

/**
 * Regenerates the marked blocks of the descent tree: a stale block is refused as generated-block-stale, or rewritten in
 * place when write is set. check_tree runs it before the walk so the walk reads the fresh files.
 */
void generate_blocks(const check_context *context, int write);

#endif
