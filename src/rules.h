#ifndef TARN_RULES_H
#define TARN_RULES_H

#include "context.h"
#include "directory.h"
#include "text.h"

#define RULES_SIZE_BUDGET 400

/** The kind of an entry of a descent folder, read from its name, its type and where it sits. */
typedef enum {
    ENTRY_SKIPPED,
    ENTRY_README,
    ENTRY_PROSE,
    ENTRY_FOLDER,
    ENTRY_RECORD,
    ENTRY_NOT_DESCENT_KIND,
    ENTRY_RECORD_IN_REPO,
    ENTRY_RECORD_BELOW_ROOT,
    ENTRY_SOURCE_IN_ORG
} entry_kind;

entry_kind rules_classify(const check_context *context, const char *folder, const directory_entry *entry);
int rules_is_refused(entry_kind kind);
int rules_is_readme_child(entry_kind kind);
void rules_report_refused(const check_context *context, const char *path, const directory_entry *entry,
                          entry_kind kind);
int rules_is_code_file(const char *name);
void rules_check_size(const check_context *context, const char *path, const text_document *document);

#endif
