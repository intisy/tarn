#ifndef TARN_BOUNDARY_H
#define TARN_BOUNDARY_H

#include "context.h"
#include "text.h"

/** Refuses public-names-private and link-into-nonstandard in the document; does nothing without a snapshot. */
void boundary_check(const check_context *context, const char *path, const text_document *document);

#endif
