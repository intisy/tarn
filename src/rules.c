#include "rules.h"

#include "text.h"

#include <string.h>

static const char *const record_files[] = {"TASKS.md", "DECISIONS.md", "HANDOFF.md", "FAQ.md", "RULES.md"};
static const char *const record_folders[] = {"plans", "specs", "records"};
static const char *const code_extensions[] = {".c",  ".h",  ".cpp", ".hpp", ".ts", ".js", ".mjs", ".java",
                                              ".kt", ".rs", ".go",  ".py",  ".lua", ".sh", ".ps1", ".cmake"};

static int name_in(const char *name, const char *const *names, size_t count) {
    size_t index;
    for (index = 0; index < count; index++) {
        if (strcmp(name, names[index]) == 0) {
            return 1;
        }
    }
    return 0;
}

static int is_record(const directory_entry *entry) {
    if (entry->is_directory) {
        return name_in(entry->name, record_folders, sizeof record_folders / sizeof record_folders[0]);
    }
    return name_in(entry->name, record_files, sizeof record_files / sizeof record_files[0]);
}

static int is_ignored_org_root_entry(const directory_entry *entry) {
    return entry->name[0] == '.' || strcmp(entry->name, "LICENSE") == 0 || strcmp(entry->name, "VERSION") == 0;
}

static int is_org_root_record(const directory_entry *entry) {
    return is_record(entry) || (!entry->is_directory && strcmp(entry->name, "CLAUDE.md") == 0);
}

static entry_kind classify_at_root(const check_context *context, const directory_entry *entry, int *decided) {
    *decided = 1;
    if (context->kind == TREE_ORG) {
        if (is_ignored_org_root_entry(entry)) {
            return ENTRY_SKIPPED;
        }
        if (is_org_root_record(entry)) {
            return ENTRY_RECORD;
        }
        if (entry->is_directory && strcmp(entry->name, "source") == 0) {
            return ENTRY_SOURCE_IN_ORG;
        }
    } else if (!entry->is_directory && strcmp(entry->name, "VERSION") == 0) {
        return ENTRY_SKIPPED;
    }
    *decided = 0;
    return ENTRY_SKIPPED;
}

entry_kind rules_classify(const check_context *context, const char *folder, const directory_entry *entry) {
    if (strcmp(folder, context->descent_root) == 0) {
        int decided;
        entry_kind kind = classify_at_root(context, entry, &decided);
        if (decided) {
            return kind;
        }
    }
    if (is_record(entry)) {
        return context->kind == TREE_REPO ? ENTRY_RECORD_IN_REPO : ENTRY_RECORD_BELOW_ROOT;
    }
    if (entry->is_directory) {
        return ENTRY_FOLDER;
    }
    if (strcmp(entry->name, "README.md") == 0) {
        return ENTRY_README;
    }
    if (text_ends_with(entry->name, ".md")) {
        return ENTRY_PROSE;
    }
    return ENTRY_NOT_DESCENT_KIND;
}

int rules_is_refused(entry_kind kind) {
    return kind == ENTRY_NOT_DESCENT_KIND || kind == ENTRY_RECORD_IN_REPO || kind == ENTRY_RECORD_BELOW_ROOT ||
           kind == ENTRY_SOURCE_IN_ORG;
}

int rules_is_readme_child(entry_kind kind) {
    return kind != ENTRY_SKIPPED && kind != ENTRY_README;
}

void rules_report_refused(const check_context *context, const char *path, const directory_entry *entry,
                          entry_kind kind) {
    const char *slash = entry->is_directory ? "/" : "";
    switch (kind) {
    case ENTRY_RECORD_IN_REPO:
        report_add(context->out, SEVERITY_REFUSED, path, 0, "kind-not-allowed",
                   "%s%s is a record, and a repo docs tree holds no records", entry->name, slash);
        break;
    case ENTRY_RECORD_BELOW_ROOT:
        report_add(context->out, SEVERITY_REFUSED, path, 0, "kind-not-allowed",
                   "%s%s is a record, and records sit at the root of an org docs tree only", entry->name, slash);
        break;
    case ENTRY_SOURCE_IN_ORG:
        report_add(context->out, SEVERITY_REFUSED, path, 0, "kind-not-allowed",
                   "source/ is the source mirror of a repo docs tree, and an org docs tree has none");
        break;
    default:
        report_add(context->out, SEVERITY_REFUSED, path, 0, "kind-not-allowed",
                   "a descent tree holds only README.md, prose .md files and folders");
        break;
    }
}

int rules_is_code_file(const char *name) {
    size_t index;
    if (strcmp(name, "CMakeLists.txt") == 0) {
        return 1;
    }
    for (index = 0; index < sizeof code_extensions / sizeof code_extensions[0]; index++) {
        if (text_ends_with(name, code_extensions[index]) && strlen(name) > strlen(code_extensions[index])) {
            return 1;
        }
    }
    return 0;
}

void rules_check_size(const check_context *context, const char *path, const text_document *document) {
    if (document->count <= RULES_SIZE_BUDGET) {
        return;
    }
    if (text_is_size_exception(document->lines[0].text)) {
        return;
    }
    report_add(context->out, SEVERITY_WARNED, path, RULES_SIZE_BUDGET + 1, "size-over-budget",
               "%zu lines, over the budget of %d", document->count, RULES_SIZE_BUDGET);
}
