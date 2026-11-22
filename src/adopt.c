#include "adopt.h"

#include "check.h"
#include "directory.h"
#include "memory.h"
#include "org_repos.h"
#include "path.h"
#include "report.h"
#include "rules.h"
#include "source_children.h"
#include "text.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STANDARD_REF_LENGTH 40

static const char standard_ref_placeholder[] = "<standard-ref>";

static const char caller_template[] =
    "name: Docs\n"
    "\n"
    "on:\n"
    "  push:\n"
    "  pull_request:\n"
    "  workflow_dispatch:\n"
    "  schedule:\n"
    "    - cron: '17 5 * * 1'\n"
    "\n"
    "permissions:\n"
    "  contents: read\n"
    "  issues: write\n"
    "\n"
    "jobs:\n"
    "  check:\n"
    "    if: ${{ github.event_name == 'push' || github.event_name == 'pull_request' }}\n"
    "    uses: intisy/tarn/.github/workflows/check-docs.yml@<standard-ref>\n"
    "    with:\n"
    "      standard-ref: <standard-ref>\n"
    "      mode: check\n"
    "    secrets:\n"
    "      envPAT: ${{ secrets.TARN_TOKEN }}\n"
    "\n"
    "  warn:\n"
    "    if: ${{ github.event_name == 'schedule' || github.event_name == 'workflow_dispatch' }}\n"
    "    uses: intisy/tarn/.github/workflows/check-docs.yml@<standard-ref>\n"
    "    with:\n"
    "      standard-ref: <standard-ref>\n"
    "      mode: warn\n"
    "      default-branch: ${{ github.ref_name }}\n"
    "    secrets:\n"
    "      envPAT: ${{ secrets.TARN_TOKEN }}\n";

static const char source_folder_text[] = "the source mirror: one doc per source folder that holds code";

typedef struct {
    char *data;
    size_t length;
} text_builder;

typedef struct {
    char *path;
    /** What adopting writes, or NULL when an entry of that name already exists and is kept. */
    char *content;
} planned_file;

typedef struct {
    planned_file *items;
    size_t count;
    /** The first planned folder that exists only under another case, which no file can be written into. */
    char *case_differing_folder;
} adoption_plan;

void adopt_outcome_init(adopt_outcome *outcome) {
    string_list_init(&outcome->lines);
    outcome->blocking = NULL;
}

void adopt_outcome_free(adopt_outcome *outcome) {
    string_list_free(&outcome->lines);
    free(outcome->blocking);
    outcome->blocking = NULL;
}

static void builder_start(text_builder *builder) {
    builder->data = memory_copy_string("");
    builder->length = 0;
}

static void builder_append_bytes(text_builder *builder, const char *bytes, size_t count) {
    builder->data = memory_resize(builder->data, builder->length + count + 1);
    memcpy(builder->data + builder->length, bytes, count);
    builder->length += count;
    builder->data[builder->length] = '\0';
}

static void builder_append(text_builder *builder, const char *text) {
    builder_append_bytes(builder, text, strlen(text));
}

static void builder_append_line(text_builder *builder, const char *text) {
    builder_append(builder, text);
    builder_append(builder, "\n");
}

int adopt_is_standard_ref(const char *text) {
    size_t index;
    if (strlen(text) != STANDARD_REF_LENGTH) {
        return 0;
    }
    for (index = 0; index < STANDARD_REF_LENGTH; index++) {
        char character = text[index];
        if (!((character >= '0' && character <= '9') || (character >= 'a' && character <= 'f'))) {
            return 0;
        }
    }
    return 1;
}

char *adopt_caller_text(const char *standard_ref) {
    text_builder builder;
    const char *rest = caller_template;
    const char *found;
    builder_start(&builder);
    while ((found = strstr(rest, standard_ref_placeholder)) != NULL) {
        builder_append_bytes(&builder, rest, (size_t)(found - rest));
        builder_append(&builder, standard_ref);
        rest = found + sizeof standard_ref_placeholder - 1;
    }
    builder_append(&builder, rest);
    return builder.data;
}

static char *format_text(const char *format, ...) REPORT_PRINTF(1, 2);

static char *format_text(const char *format, ...) {
    va_list arguments;
    va_list measuring;
    int length;
    char *text;
    va_start(arguments, format);
    va_copy(measuring, arguments);
    length = vsnprintf(NULL, 0, format, measuring);
    va_end(measuring);
    if (length < 0) {
        abort();
    }
    text = memory_allocate((size_t)length + 1);
    if (vsnprintf(text, (size_t)length + 1, format, arguments) != length) {
        abort();
    }
    va_end(arguments);
    return text;
}

static char *version_text(void) {
    return format_text("tarn %d\n", CHECK_NEWEST_VERSION);
}

static char *child_line(const char *name, const char *text) {
    return format_text("- [%s](%s) - %s", name, name, text);
}

/** The topmost folder of path that exists only under another case, or NULL when none does. */
static char *find_case_differing_folder(const char *repo_root, const char *path) {
    char *folder = path_parent(path);
    char *found = NULL;
    while (folder[0] != '\0') {
        char *parent = path_parent(folder);
        if (path_lookup_exact(repo_root, folder) == PATH_CASE_DIFFERS) {
            free(found);
            found = memory_copy_string(folder);
        }
        free(folder);
        folder = parent;
    }
    free(folder);
    return found;
}

/** Adds the file to the plan, kept when the repo already holds an entry of that name in any case. */
static void plan_add(const check_context *context, adoption_plan *plan, const char *path, char *content) {
    planned_file *file;
    path_state state = path_lookup_exact(context->repo_root, path);
    if (state == PATH_CASE_DIFFERS && plan->case_differing_folder == NULL) {
        plan->case_differing_folder = find_case_differing_folder(context->repo_root, path);
    }
    if (state != PATH_MISSING) {
        free(content);
        content = NULL;
    }
    plan->items = memory_resize(plan->items, (plan->count + 1) * sizeof *plan->items);
    file = &plan->items[plan->count++];
    file->path = memory_copy_string(path);
    file->content = content;
}

static void plan_free(adoption_plan *plan) {
    size_t index;
    for (index = 0; index < plan->count; index++) {
        free(plan->items[index].path);
        free(plan->items[index].content);
    }
    free(plan->items);
    free(plan->case_differing_folder);
}

static int is_mirrored_doc_folder(const check_context *context, const char *mirrored, const directory_entry *entry) {
    char *source_folder;
    int mirrored_by_block;
    if (mirrored == NULL || !entry->is_directory) {
        return 0;
    }
    source_folder = path_join(mirrored, entry->name);
    mirrored_by_block = string_list_contains(&context->code_folders, source_folder);
    free(source_folder);
    return mirrored_by_block;
}

/**
 * The names a README in folder must list by hand, a folder's with its trailing slash: every child the checker counts,
 * bar the doc folders a source-children block already lists when folder mirrors the source folder mirrored.
 */
static void collect_child_names(const check_context *context, const char *folder, const char *mirrored,
                                string_list *names) {
    char *on_disk = path_on_disk(context->repo_root, folder);
    directory_listing listing;
    size_t index;
    if (directory_list(on_disk, &listing) == 0) {
        for (index = 0; index < listing.count; index++) {
            const directory_entry *entry = &listing.entries[index];
            if (rules_is_readme_child(rules_classify(context, folder, entry)) &&
                !is_mirrored_doc_folder(context, mirrored, entry)) {
                char *name = format_text("%s%s", entry->name, entry->is_directory ? "/" : "");
                string_list_push(names, name);
                free(name);
            }
        }
        directory_listing_free(&listing);
    }
    free(on_disk);
}

static void child_lines_for(const string_list *names, const char *(*text_for)(const char *name), string_list *lines) {
    size_t index;
    for (index = 0; index < names->count; index++) {
        char *line = child_line(names->items[index], text_for(names->items[index]));
        string_list_push(lines, line);
        free(line);
    }
}

static const char *placeholder_for(const char *name) {
    (void)name;
    return TEXT_PLACEHOLDER;
}

static const char *docs_child_text(const char *name) {
    return strcmp(name, "source/") == 0 ? source_folder_text : TEXT_PLACEHOLDER;
}

/** A README of a placeholder paragraph, then the hand-written child lines, then the block of kind what, if any. */
static char *readme_skeleton(const string_list *child_lines, const char *what, const string_list *block_lines) {
    text_builder builder;
    size_t index;
    builder_start(&builder);
    builder_append_line(&builder, TEXT_PLACEHOLDER);
    if (child_lines->count > 0) {
        builder_append(&builder, "\n");
        for (index = 0; index < child_lines->count; index++) {
            builder_append_line(&builder, child_lines->items[index]);
        }
    }
    if (what != NULL) {
        builder_append(&builder, "\n<!-- generated: ");
        builder_append(&builder, what);
        builder_append_line(&builder, " -->");
        for (index = 0; index < block_lines->count; index++) {
            builder_append_line(&builder, block_lines->items[index]);
        }
        builder_append_line(&builder, "<!-- end generated -->");
    }
    return builder.data;
}

static void plan_docs_readme(const check_context *context, adoption_plan *plan) {
    string_list names;
    string_list lines;
    string_list_init(&names);
    string_list_init(&lines);
    collect_child_names(context, "docs", NULL, &names);
    if (context->code_folders.count > 0 && !string_list_contains(&names, "source/")) {
        string_list_push(&names, "source/");
    }
    string_list_sort(&names);
    child_lines_for(&names, docs_child_text, &lines);
    plan_add(context, plan, "docs/README.md", readme_skeleton(&lines, NULL, NULL));
    string_list_free(&lines);
    string_list_free(&names);
}

static void plan_folder_readme(const check_context *context, adoption_plan *plan, const char *folder) {
    char *readme = path_join(folder, "README.md");
    string_list names;
    string_list lines;
    string_list_init(&names);
    string_list_init(&lines);
    collect_child_names(context, folder, NULL, &names);
    child_lines_for(&names, placeholder_for, &lines);
    plan_add(context, plan, readme, readme_skeleton(&lines, NULL, NULL));
    string_list_free(&lines);
    string_list_free(&names);
    free(readme);
}

/** Plans a README for every descent folder below folder, the source mirror aside, which plan_source_readme covers. */
static void plan_descent_subfolders(const check_context *context, adoption_plan *plan, const char *folder) {
    char *on_disk = path_on_disk(context->repo_root, folder);
    directory_listing listing;
    size_t index;
    if (directory_list(on_disk, &listing) == 0) {
        for (index = 0; index < listing.count; index++) {
            char *child = path_join(folder, listing.entries[index].name);
            if (rules_classify(context, folder, &listing.entries[index]) == ENTRY_FOLDER &&
                strcmp(child, "docs/source") != 0) {
                plan_folder_readme(context, plan, child);
                plan_descent_subfolders(context, plan, child);
            }
            free(child);
        }
        directory_listing_free(&listing);
    }
    free(on_disk);
}

static void plan_source_readme(const check_context *context, adoption_plan *plan, const char *mirrored) {
    char *folder = path_join("docs/source", mirrored);
    char *readme = path_join(folder, "README.md");
    string_list names;
    string_list lines;
    string_list no_lines;
    string_list block;
    string_list_init(&names);
    string_list_init(&lines);
    string_list_init(&no_lines);
    string_list_init(&block);
    collect_child_names(context, folder, mirrored, &names);
    child_lines_for(&names, placeholder_for, &lines);
    source_children_lines(context, mirrored, &no_lines, &block);
    plan_add(context, plan, readme, readme_skeleton(&lines, "source-children", &block));
    string_list_free(&block);
    string_list_free(&no_lines);
    string_list_free(&lines);
    string_list_free(&names);
    free(readme);
    free(folder);
}

static void plan_repo(const check_context *context, const char *standard_ref, adoption_plan *plan) {
    size_t index;
    plan_add(context, plan, "docs/VERSION", version_text());
    plan_docs_readme(context, plan);
    plan_descent_subfolders(context, plan, "docs");
    for (index = 0; index < context->code_folders.count; index++) {
        plan_source_readme(context, plan, context->code_folders.items[index]);
    }
    plan_add(context, plan, ADOPT_CALLER_PATH, adopt_caller_text(standard_ref));
}

static void plan_org(const check_context *context, const char *standard_ref, adoption_plan *plan) {
    string_list names;
    string_list lines;
    string_list no_lines;
    string_list block;
    string_list_init(&names);
    string_list_init(&lines);
    string_list_init(&no_lines);
    string_list_init(&block);
    plan_add(context, plan, "VERSION", version_text());
    collect_child_names(context, "", NULL, &names);
    string_list_sort(&names);
    child_lines_for(&names, placeholder_for, &lines);
    org_repos_lines(context, &no_lines, &block);
    plan_add(context, plan, "README.md", readme_skeleton(&lines, "org-repos", &block));
    plan_descent_subfolders(context, plan, "");
    plan_add(context, plan, ADOPT_CALLER_PATH, adopt_caller_text(standard_ref));
    string_list_free(&block);
    string_list_free(&no_lines);
    string_list_free(&lines);
    string_list_free(&names);
}

static void describe_plan(const adoption_plan *plan, adopt_outcome *outcome) {
    size_t index;
    for (index = 0; index < plan->count; index++) {
        const planned_file *file = &plan->items[index];
        char *line = format_text("%s %s", file->content != NULL ? "create" : "keep", file->path);
        string_list_push(&outcome->lines, line);
        free(line);
    }
}

static void describe_not_following(const check_context *context, adopt_outcome *outcome) {
    string_list names;
    size_t index;
    string_list_init(&names);
    for (index = 0; index < context->snapshot->count; index++) {
        const snapshot_repo *repo = &context->snapshot->repos[index];
        if (!repo->follows && org_repos_is_listed(context, repo)) {
            string_list_push(&names, repo->full_name);
        }
    }
    string_list_sort(&names);
    for (index = 0; index < names.count; index++) {
        char *line = format_text("does-not-follow %s", names.items[index]);
        string_list_push(&outcome->lines, line);
        free(line);
    }
    string_list_free(&names);
}

static int make_folder(const char *repo_root, const char *folder) {
    char *on_disk = path_on_disk(repo_root, folder);
    int made = directory_make(on_disk) == 0;
    free(on_disk);
    return made;
}

static int ensure_parent_folders(const char *repo_root, const char *path) {
    char *parent = path_parent(path);
    int ready = 1;
    if (parent[0] != '\0') {
        path_state state = path_lookup_exact(repo_root, parent);
        if (state == PATH_MISSING) {
            ready = ensure_parent_folders(repo_root, parent) && make_folder(repo_root, parent);
        } else {
            ready = state == PATH_DIRECTORY;
        }
    }
    free(parent);
    return ready;
}

static int write_planned_file(const char *repo_root, const planned_file *file) {
    char *on_disk;
    int written;
    if (!ensure_parent_folders(repo_root, file->path)) {
        return 0;
    }
    on_disk = path_on_disk(repo_root, file->path);
    written = directory_create_file(on_disk, file->content, strlen(file->content)) == 0;
    free(on_disk);
    return written;
}

static int write_plan(const char *repo_root, const adoption_plan *plan, adopt_outcome *outcome) {
    size_t index;
    for (index = 0; index < plan->count; index++) {
        const planned_file *file = &plan->items[index];
        if (file->content != NULL && !write_planned_file(repo_root, file)) {
            outcome->blocking = memory_copy_string(file->path);
            return ADOPT_UNWRITABLE;
        }
    }
    return ADOPT_DONE;
}

static int run_adoption(check_context *context, const adopt_options *options, adopt_outcome *outcome) {
    adoption_plan plan = {NULL, 0, NULL};
    int result = ADOPT_DONE;
    if (context->kind == TREE_REPO) {
        plan_repo(context, options->standard_ref, &plan);
    } else {
        plan_org(context, options->standard_ref, &plan);
    }
    if (plan.case_differing_folder != NULL) {
        outcome->blocking = memory_copy_string(plan.case_differing_folder);
        plan_free(&plan);
        return ADOPT_CASE_DIFFERS;
    }
    describe_plan(&plan, outcome);
    if (context->kind == TREE_ORG) {
        describe_not_following(context, outcome);
    }
    if (options->write) {
        result = write_plan(context->repo_root, &plan, outcome);
    }
    plan_free(&plan);
    return result;
}

static const snapshot_repo *find_self(const adopt_options *options) {
    if (options->snapshot == NULL || options->repository == NULL) {
        return NULL;
    }
    return snapshot_find(options->snapshot, options->repository, strlen(options->repository));
}

int adopt_tree(const char *repo_root, const adopt_options *options, adopt_outcome *outcome) {
    tree_kind kind = options->kind == ADOPT_REPO ? TREE_REPO : TREE_ORG;
    const char *version_path = kind == TREE_REPO ? "docs/VERSION" : "VERSION";
    const snapshot_repo *self = find_self(options);
    report unused;
    check_context context;
    int result;
    if (!directory_is_directory(repo_root)) {
        return ADOPT_NOT_A_FOLDER;
    }
    if (kind == TREE_ORG && self == NULL) {
        return ADOPT_REPOSITORY_UNKNOWN;
    }
    if (check_version_decides(repo_root, kind == TREE_REPO ? TREE_ORG : TREE_REPO)) {
        return ADOPT_OTHER_KIND_DECLARED;
    }
    if (check_version_is_foreign(repo_root, version_path)) {
        outcome->blocking = memory_copy_string(version_path);
        return ADOPT_VERSION_FOREIGN;
    }
    report_init(&unused);
    check_context_open_as(repo_root, kind, &unused, &context);
    context.snapshot = options->snapshot;
    context.self = self;
    result = run_adoption(&context, options, outcome);
    check_context_close(&context);
    report_free(&unused);
    return result;
}
