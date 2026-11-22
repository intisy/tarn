#include "walk.h"

#include "directory.h"
#include "links.h"
#include "memory.h"
#include "path.h"
#include "readme.h"
#include "rules.h"
#include "text.h"

#include <stdlib.h>
#include <string.h>

static const char source_root[] = "docs/source";

static const char *folder_display(const char *folder) {
    return folder[0] == '\0' ? "the repo root" : folder;
}

static int list_relative(const check_context *context, const char *relative, directory_listing *listing) {
    char *on_disk = path_on_disk(context->repo_root, relative);
    int result = directory_list(on_disk, listing);
    free(on_disk);
    return result;
}

static int scan_source_folder(check_context *context, const char *folder) {
    directory_listing listing;
    size_t index;
    int holds_code = 0;
    if (list_relative(context, folder, &listing) != 0) {
        return 0;
    }
    for (index = 0; index < listing.count; index++) {
        const directory_entry *entry = &listing.entries[index];
        char *relative = path_join(folder, entry->name);
        if (!ignore_matches(&context->ignore, relative, entry->is_directory)) {
            if (entry->is_directory) {
                holds_code |= scan_source_folder(context, relative);
            } else {
                holds_code |= rules_is_code_file(entry->name);
            }
        }
        free(relative);
    }
    directory_listing_free(&listing);
    if (holds_code) {
        string_list_push(&context->code_folders, folder);
    }
    return holds_code;
}

void walk_find_code_folders(check_context *context) {
    scan_source_folder(context, "");
    string_list_sort(&context->code_folders);
}

const char *walk_mirrored_folder(const check_context *context, const char *folder) {
    size_t root_length = strlen(source_root);
    if (context->kind != TREE_REPO || strncmp(folder, source_root, root_length) != 0) {
        return NULL;
    }
    if (folder[root_length] == '\0') {
        return "";
    }
    return folder[root_length] == '/' ? folder + root_length + 1 : NULL;
}

static void report_gone_mirror(const check_context *context, const char *folder, const char *mirrored) {
    const char *rule = "source-doc-for-gone-folder";
    if (mirrored[0] == '\0') {
        report_add(context->out, SEVERITY_REFUSED, folder, 0, rule, "the repo root holds no code");
    } else if (path_lookup_exact(context->repo_root, mirrored) != PATH_DIRECTORY) {
        report_add(context->out, SEVERITY_REFUSED, folder, 0, rule, "%s does not exist", mirrored);
    } else if (ignore_covers(&context->ignore, mirrored, 1)) {
        report_add(context->out, SEVERITY_REFUSED, folder, 0, rule, "%s is ignored", mirrored);
    } else {
        report_add(context->out, SEVERITY_REFUSED, folder, 0, rule, "%s holds no code", mirrored);
    }
}

static int load_document(const check_context *context, const char *path, text_document *document) {
    char *on_disk = path_on_disk(context->repo_root, path);
    int result = text_document_load(on_disk, document);
    free(on_disk);
    if (result != 0) {
        report_add(context->out, SEVERITY_REFUSED, path, 0, "file-unreadable", "the file cannot be read");
    }
    return result;
}

static void add_source_files(const check_context *context, const char *mirrored, readme_children *children) {
    directory_listing listing;
    size_t index;
    if (list_relative(context, mirrored, &listing) != 0) {
        return;
    }
    for (index = 0; index < listing.count; index++) {
        const directory_entry *entry = &listing.entries[index];
        char *relative = path_join(mirrored, entry->name);
        if (!entry->is_directory && rules_is_code_file(entry->name) &&
            !ignore_matches(&context->ignore, relative, 0)) {
            readme_children_add(children, entry->name, 0, 1);
        }
        free(relative);
    }
    directory_listing_free(&listing);
}

static void collect_children(const check_context *context, const directory_listing *listing,
                             const entry_kind *kinds, const char *mirrored, readme_children *children) {
    size_t index;
    for (index = 0; index < listing->count; index++) {
        if (rules_is_readme_child(kinds[index])) {
            readme_children_add(children, listing->entries[index].name, listing->entries[index].is_directory, 0);
        }
    }
    if (mirrored != NULL) {
        add_source_files(context, mirrored, children);
    }
}

static int has_readme(const directory_listing *listing, const entry_kind *kinds) {
    size_t index;
    for (index = 0; index < listing->count; index++) {
        if (kinds[index] == ENTRY_README) {
            return 1;
        }
    }
    return 0;
}

static void check_readme_file(const check_context *context, const readme_place *place, readme_children *children) {
    text_document document;
    unsigned char *child_lines;
    if (load_document(context, place->readme_path, &document) != 0) {
        return;
    }
    child_lines = memory_allocate(document.count);
    readme_check(context, place, &document, children, child_lines);
    rules_check_size(context, place->readme_path, &document);
    links_check(context, place->readme_path, &document, child_lines);
    free(child_lines);
    text_document_free(&document);
}

static void check_folder_readme(const check_context *context, const char *folder, const char *mirrored,
                                const directory_listing *listing, const entry_kind *kinds) {
    readme_children children = {NULL, 0};
    readme_place place;
    char *readme_path;
    if (!has_readme(listing, kinds)) {
        report_add(context->out, SEVERITY_REFUSED, folder, 0, "missing-readme", "folder has no README.md");
        return;
    }
    readme_path = path_join(folder, "README.md");
    place.readme_path = readme_path;
    place.folder = folder;
    place.source_folder = mirrored;
    collect_children(context, listing, kinds, mirrored, &children);
    check_readme_file(context, &place, &children);
    readme_children_free(&children);
    free(readme_path);
}

static void check_prose_file(const check_context *context, const char *path) {
    text_document document;
    if (load_document(context, path, &document) != 0) {
        return;
    }
    rules_check_size(context, path, &document);
    links_check(context, path, &document, NULL);
    text_document_free(&document);
}

static void walk_folder(check_context *context, const char *folder);

static void walk_entries(check_context *context, const char *folder, const directory_listing *listing,
                         const entry_kind *kinds) {
    size_t index;
    for (index = 0; index < listing->count; index++) {
        const directory_entry *entry = &listing->entries[index];
        char *path = path_join(folder, entry->name);
        if (rules_is_refused(kinds[index])) {
            rules_report_refused(context, path, entry, kinds[index]);
        } else if (kinds[index] == ENTRY_PROSE) {
            check_prose_file(context, path);
        } else if (kinds[index] == ENTRY_FOLDER) {
            walk_folder(context, path);
        }
        free(path);
    }
}

static void walk_folder(check_context *context, const char *folder) {
    const char *mirrored = walk_mirrored_folder(context, folder);
    directory_listing listing;
    entry_kind *kinds;
    size_t index;
    if (mirrored != NULL && !string_list_contains(&context->code_folders, mirrored)) {
        report_gone_mirror(context, folder, mirrored);
        return;
    }
    if (list_relative(context, folder, &listing) != 0) {
        report_add(context->out, SEVERITY_REFUSED, folder, 0, "folder-unreadable", "the folder cannot be read");
        return;
    }
    kinds = memory_allocate(listing.count * sizeof *kinds);
    for (index = 0; index < listing.count; index++) {
        kinds[index] = rules_classify(context, folder, &listing.entries[index]);
    }
    check_folder_readme(context, folder, mirrored, &listing, kinds);
    walk_entries(context, folder, &listing, kinds);
    free(kinds);
    directory_listing_free(&listing);
}

static void check_content_file(const check_context *context) {
    text_document document;
    if (path_lookup_exact(context->repo_root, "CONTENT.md") != PATH_FILE) {
        return;
    }
    if (load_document(context, "CONTENT.md", &document) != 0) {
        return;
    }
    links_check(context, "CONTENT.md", &document, NULL);
    text_document_free(&document);
}

void walk_descent_tree(check_context *context) {
    walk_folder(context, context->descent_root);
    if (context->kind == TREE_REPO) {
        check_content_file(context);
    }
}

void walk_check_mirror_complete(check_context *context) {
    size_t index;
    for (index = 0; index < context->code_folders.count; index++) {
        const char *folder = context->code_folders.items[index];
        char *doc_folder = path_join(source_root, folder);
        if (path_lookup_exact(context->repo_root, doc_folder) != PATH_DIRECTORY) {
            report_add(context->out, SEVERITY_REFUSED, doc_folder, 0, "source-folder-undocumented",
                       "%s holds code and has no doc folder", folder_display(folder));
        }
        free(doc_folder);
    }
}
