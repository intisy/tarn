#include "org_repos.h"

#include "memory.h"
#include "snapshot.h"
#include "text.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char github_url[] = "https://github.com/";
static const char not_following[] = " (does not follow tarn yet)";
static const char placeholder_text[] = "to write";

typedef struct {
    const char *name;
    size_t name_length;
    const char *text;
} org_line;

static int parse_org_line(const char *line, org_line *out) {
    const char *name_end;
    const char *target;
    if (strncmp(line, "- [", 3) != 0) {
        return 0;
    }
    out->name = line + 3;
    name_end = strstr(out->name, "](");
    if (name_end == NULL) {
        return 0;
    }
    out->name_length = (size_t)(name_end - out->name);
    target = name_end + 2;
    if (strncmp(target, github_url, sizeof github_url - 1) != 0) {
        return 0;
    }
    target += sizeof github_url - 1;
    if (strncmp(target, out->name, out->name_length) != 0 || strncmp(target + out->name_length, ") - ", 4) != 0) {
        return 0;
    }
    out->text = target + out->name_length + 4;
    return snapshot_is_repo_name(out->name, out->name_length) && out->text[0] != '\0' && out->text[0] != ' ';
}

int org_repos_is_line(const char *line) {
    org_line parsed;
    return parse_org_line(line, &parsed);
}

char *org_repos_line_name(const char *line) {
    org_line parsed;
    if (!parse_org_line(line, &parsed)) {
        return NULL;
    }
    return memory_copy_bytes(parsed.name, parsed.name_length);
}

/** The text a person wrote, without the marker the generator appends to a repo that does not follow. */
static char *written_text(const char *text) {
    size_t length = strlen(text);
    size_t marker_length = sizeof not_following - 1;
    if (length > marker_length && text_ends_with(text, not_following)) {
        return memory_copy_bytes(text, length - marker_length);
    }
    return memory_copy_string(text);
}

int org_repos_is_unwritten(const char *line) {
    org_line parsed;
    char *text;
    int unwritten;
    if (!parse_org_line(line, &parsed)) {
        return 0;
    }
    text = written_text(parsed.text);
    unwritten = strcmp(text, placeholder_text) == 0;
    free(text);
    return unwritten;
}

/** Repo names compare ignoring case, as the snapshot lookup does, so a kept text survives a change of case. */
static int is_same_repo(const org_line *parsed, const char *full_name) {
    char *name = memory_copy_bytes(parsed->name, parsed->name_length);
    int same = text_equal_ignoring_case(name, full_name);
    free(name);
    return same;
}

static char *kept_text(const string_list *current_lines, const char *full_name) {
    size_t index;
    for (index = 0; index < current_lines->count; index++) {
        org_line parsed;
        if (parse_org_line(current_lines->items[index], &parsed) && is_same_repo(&parsed, full_name)) {
            return written_text(parsed.text);
        }
    }
    return memory_copy_string(placeholder_text);
}

/** A public docs repo lists only the public repos, so its block never names a private one. */
static int is_listed(const check_context *context, const snapshot_repo *repo) {
    const snapshot_repo *self = context->self;
    return repo != self && snapshot_same_owner(repo, self) &&
           (self->visibility == VISIBILITY_PRIVATE || repo->visibility == VISIBILITY_PUBLIC);
}

static int compare_repos(const void *left, const void *right) {
    return strcmp((*(const snapshot_repo *const *)left)->full_name, (*(const snapshot_repo *const *)right)->full_name);
}

static char *format_line(const snapshot_repo *repo, const char *text) {
    const char *marker = repo->follows ? "" : not_following;
    int length = snprintf(NULL, 0, "- [%s](%s%s) - %s%s", repo->full_name, github_url, repo->full_name, text, marker);
    char *line;
    if (length < 0) {
        abort();
    }
    line = memory_allocate((size_t)length + 1);
    if (snprintf(line, (size_t)length + 1, "- [%s](%s%s) - %s%s", repo->full_name, github_url, repo->full_name, text,
                 marker) != length) {
        abort();
    }
    return line;
}

void org_repos_lines(const check_context *context, const string_list *current_lines, string_list *out) {
    const repos_snapshot *snapshot = context->snapshot;
    const snapshot_repo **listed = memory_allocate((snapshot->count + 1) * sizeof *listed);
    size_t count = 0;
    size_t index;
    for (index = 0; index < snapshot->count; index++) {
        if (is_listed(context, &snapshot->repos[index])) {
            listed[count++] = &snapshot->repos[index];
        }
    }
    if (count > 1) {
        qsort(listed, count, sizeof *listed, compare_repos);
    }
    for (index = 0; index < count; index++) {
        char *text = kept_text(current_lines, listed[index]->full_name);
        char *line = format_line(listed[index], text);
        string_list_push(out, line);
        free(line);
        free(text);
    }
    free(listed);
}
