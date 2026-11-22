#ifndef TARN_DRIFT_H
#define TARN_DRIFT_H

#include "context.h"
#include "report.h"
#include "string_list.h"

#include <stddef.h>

#define DRIFT_STALE_DAYS 14

/**
 * Warns org-misses-repo for each repo of expected that current lacks, and org-names-gone-repo for each line of current
 * naming a repo the snapshot does not list. first_line is the block's start marker; returns how many it warned.
 */
size_t drift_warn_org_members(const check_context *context, const char *path, size_t first_line,
                              const string_list *current, const string_list *expected);
void drift_warn_older_version(report *out, const char *path, long declared, int newest);
/**
 * Warns source-doc-stale for each source folder whose code has commits more than DRIFT_STALE_DAYS newer than its doc
 * folder's.
 * @implNote Each side counts only the files directly in its folder, since a subfolder has a doc of its own. A path
 * git cannot be asked about safely, or that has no history, is skipped.
 */
void drift_check_source_docs(const check_context *context);

#endif
