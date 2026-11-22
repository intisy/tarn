#include "boundary.h"

#include "memory.h"
#include "snapshot.h"

#include <stdlib.h>
#include <string.h>

static const char github_host[] = "github.com/";
static const char *const github_prefixes[] = {"https://github.com/", "http://github.com/"};

/** The private repos one line has already been refused for, so a repo named twice on a line is refused once. */
typedef struct {
    const snapshot_repo **items;
    size_t count;
} refused_repos;

typedef struct {
    const check_context *context;
    const char *path;
    size_t line;
    refused_repos refused;
} line_scan;

static int is_token_character(char character) {
    return snapshot_is_name_character(character) || character == '/';
}

/** The length of the `<owner>/<name>` text starts with, or 0; a sentence's final full stops are not part of it. */
static size_t repo_name_length(const char *text) {
    size_t owner = 0;
    size_t name = 0;
    while (snapshot_is_name_character(text[owner])) {
        owner++;
    }
    if (owner == 0 || text[owner] != '/') {
        return 0;
    }
    while (snapshot_is_name_character(text[owner + 1 + name])) {
        name++;
    }
    while (name > 0 && text[owner + name] == '.') {
        name--;
    }
    return name == 0 ? 0 : owner + 1 + name;
}

static int already_refused(const refused_repos *refused, const snapshot_repo *repo) {
    size_t index;
    for (index = 0; index < refused->count; index++) {
        if (refused->items[index] == repo) {
            return 1;
        }
    }
    return 0;
}

static void consider_name(line_scan *scan, const char *name, size_t length) {
    const snapshot_repo *repo = snapshot_find(scan->context->snapshot, name, length);
    if (repo == NULL || repo->visibility != VISIBILITY_PRIVATE || already_refused(&scan->refused, repo)) {
        return;
    }
    scan->refused.items = memory_resize(scan->refused.items, (scan->refused.count + 1) * sizeof *scan->refused.items);
    scan->refused.items[scan->refused.count++] = repo;
    report_add(scan->context->out, SEVERITY_REFUSED, scan->path, scan->line, "public-names-private",
               "names %s, which the snapshot marks private, in a public docs tree", repo->full_name);
}

static void scan_tokens(line_scan *scan, const char *text) {
    size_t position = 0;
    while (text[position] != '\0') {
        size_t start = position;
        size_t end;
        if (!is_token_character(text[position])) {
            position++;
            continue;
        }
        while (is_token_character(text[position])) {
            position++;
        }
        end = position;
        while (end > start && text[end - 1] == '.') {
            end--;
        }
        if (snapshot_is_repo_name(text + start, end - start)) {
            consider_name(scan, text + start, end - start);
        }
    }
}

static void scan_urls(line_scan *scan, const char *text) {
    const char *found = strstr(text, github_host);
    while (found != NULL) {
        if (found == text || !snapshot_is_name_character(found[-1])) {
            const char *name = found + sizeof github_host - 1;
            size_t length = repo_name_length(name);
            if (length > 0) {
                consider_name(scan, name, length);
            }
        }
        found = strstr(found + 1, github_host);
    }
}

static void check_names(const check_context *context, const char *path, const text_line *line) {
    char *text = text_without_inline_code(line->text);
    line_scan scan;
    scan.context = context;
    scan.path = path;
    scan.line = line->number;
    scan.refused.items = NULL;
    scan.refused.count = 0;
    scan_tokens(&scan, text);
    scan_urls(&scan, text);
    free(scan.refused.items);
    free(text);
}

static const snapshot_repo *linked_repo(const check_context *context, const char *target) {
    size_t index;
    for (index = 0; index < sizeof github_prefixes / sizeof github_prefixes[0]; index++) {
        size_t prefix_length = strlen(github_prefixes[index]);
        if (strncmp(target, github_prefixes[index], prefix_length) == 0) {
            size_t length = repo_name_length(target + prefix_length);
            return length == 0 ? NULL : snapshot_find(context->snapshot, target + prefix_length, length);
        }
    }
    return NULL;
}

static void check_links(const check_context *context, const char *path, const text_line *line) {
    size_t position = 0;
    markdown_link link;
    while (markdown_next_link(line->text, &position, &link)) {
        char *target = memory_copy_bytes(link.target, link.length);
        const snapshot_repo *repo = linked_repo(context, target);
        if (repo != NULL && !repo->follows && repo != context->self) {
            report_add(context->out, SEVERITY_REFUSED, path, line->number, "link-into-nonstandard",
                       "links into %s, which the snapshot marks as not following tarn", repo->full_name);
        }
        free(target);
    }
}

void boundary_check(const check_context *context, const char *path, const text_document *document) {
    unsigned char *fenced;
    int checks_names;
    int inside_org_repos = 0;
    size_t index;
    if (context->snapshot == NULL) {
        return;
    }
    fenced = text_fenced_lines(document);
    checks_names = context->self->visibility == VISIBILITY_PUBLIC;
    for (index = 0; index < document->count; index++) {
        const text_line *line = &document->lines[index];
        if (fenced[index]) {
            continue;
        }
        if (checks_names) {
            check_names(context, path, line);
        }
        if (!text_is_org_repos_line(line->text, &inside_org_repos)) {
            check_links(context, path, line);
        }
    }
    free(fenced);
}
