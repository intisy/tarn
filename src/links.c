#include "links.h"

#include "memory.h"
#include "path.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef enum { LINK_RESOLVES, LINK_MISSING, LINK_CASE_DIFFERS, LINK_LEAVES_REPO } link_outcome;

static int is_external(const char *target) {
    return strstr(target, "://") != NULL || strncmp(target, "mailto:", 7) == 0;
}

static int hex_value(char digit) {
    if (digit >= '0' && digit <= '9') {
        return digit - '0';
    }
    digit = (char)tolower((unsigned char)digit);
    if (digit >= 'a' && digit <= 'f') {
        return digit - 'a' + 10;
    }
    return -1;
}

static void decode_percent_escapes(char *text) {
    char *write = text;
    const char *read = text;
    while (*read != '\0') {
        if (read[0] == '%' && hex_value(read[1]) >= 0 && hex_value(read[2]) >= 0) {
            *write++ = (char)(hex_value(read[1]) * 16 + hex_value(read[2]));
            read += 3;
        } else {
            *write++ = *read++;
        }
    }
    *write = '\0';
}

static char *path_part_of(const char *target) {
    size_t length = strcspn(target, "#?");
    char *path = memory_copy_bytes(target, length);
    decode_percent_escapes(path);
    return path;
}

static char *resolve_from(const char *document_path, const char *link_path) {
    char *base = link_path[0] == '/' ? memory_copy_string("") : path_parent(document_path);
    char *joined = path_join(base, link_path[0] == '/' ? link_path + 1 : link_path);
    char *resolved = path_normalize(joined);
    free(joined);
    free(base);
    return resolved;
}

static link_outcome resolve_link(const check_context *context, const char *document_path, const char *target) {
    char *link_path = path_part_of(target);
    char *resolved;
    link_outcome outcome = LINK_RESOLVES;
    if (link_path[0] == '\0') {
        free(link_path);
        return LINK_RESOLVES;
    }
    resolved = resolve_from(document_path, link_path);
    if (resolved == NULL) {
        outcome = LINK_LEAVES_REPO;
    } else {
        path_state state = path_lookup_exact(context->repo_root, resolved);
        if (state == PATH_MISSING) {
            outcome = LINK_MISSING;
        } else if (state == PATH_CASE_DIFFERS) {
            outcome = LINK_CASE_DIFFERS;
        }
    }
    free(resolved);
    free(link_path);
    return outcome;
}

static void check_target(const check_context *context, const char *path, size_t line, const char *target) {
    link_outcome outcome;
    if (target[0] == '\0' || is_external(target)) {
        return;
    }
    outcome = resolve_link(context, path, target);
    if (outcome == LINK_MISSING) {
        report_add(context->out, SEVERITY_REFUSED, path, line, "dead-link", "%s does not resolve", target);
    } else if (outcome == LINK_CASE_DIFFERS) {
        report_add(context->out, SEVERITY_REFUSED, path, line, "dead-link",
                   "%s differs in case from the path on disk", target);
    } else if (outcome == LINK_LEAVES_REPO) {
        report_add(context->out, SEVERITY_REFUSED, path, line, "dead-link", "%s leaves the repo", target);
    }
}

static void check_line(const check_context *context, const char *path, const text_line *line) {
    size_t position = 0;
    markdown_link link;
    while (markdown_next_link(line->text, &position, &link)) {
        char *target = memory_copy_bytes(link.target, link.length);
        check_target(context, path, line->number, target);
        free(target);
    }
}

void links_check(const check_context *context, const char *path, const text_document *document,
                 const unsigned char *skip_lines) {
    unsigned char *fenced = text_fenced_lines(document);
    size_t index;
    for (index = 0; index < document->count; index++) {
        if (!fenced[index] && (skip_lines == NULL || !skip_lines[index])) {
            check_line(context, path, &document->lines[index]);
        }
    }
    free(fenced);
}
