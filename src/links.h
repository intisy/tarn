#ifndef TARN_LINKS_H
#define TARN_LINKS_H

#include "context.h"
#include "text.h"

/** Refuses every link in the document that does not resolve; lines marked in skip_lines (may be NULL) are skipped. */
void links_check(const check_context *context, const char *path, const text_document *document,
                 const unsigned char *skip_lines);

#endif
