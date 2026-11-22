#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include "drift.h"

#include "memory.h"
#include "org_repos.h"
#include "path.h"
#include "rules.h"
#include "snapshot.h"
#include "text.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#define SHELL_QUOTE '"'
#define DISCARD_ERRORS " 2>NUL"
#define IS_WINDOWS 1
#else
#define SHELL_QUOTE '\''
#define DISCARD_ERRORS " 2>/dev/null"
#define IS_WINDOWS 0
#endif

#define SECONDS_PER_DAY 86400LL
#define DATE_LENGTH 11

static const char source_root[] = "docs/source";
static const char glob_magic[] = ":(glob)";

typedef struct {
    char *data;
    size_t length;
} command_text;

/** Whether git has been asked if the clone is shallow; it is asked once, when a folder first has history. */
typedef enum { SHALLOW_UNASKED, SHALLOW_NO, SHALLOW_YES } shallow_state;

static int names_repo(const string_list *lines, const char *full_name) {
    size_t index;
    for (index = 0; index < lines->count; index++) {
        char *name = org_repos_line_name(lines->items[index]);
        int same = name != NULL && text_equal_ignoring_case(name, full_name);
        free(name);
        if (same) {
            return 1;
        }
    }
    return 0;
}

static size_t warn_missing_repos(const check_context *context, const char *path, size_t first_line,
                                 const string_list *current, const string_list *expected) {
    size_t warned = 0;
    size_t index;
    for (index = 0; index < expected->count; index++) {
        char *name = org_repos_line_name(expected->items[index]);
        if (name != NULL && !names_repo(current, name)) {
            report_add(context->out, SEVERITY_WARNED, path, first_line, "org-misses-repo",
                       "does not list %s, which the snapshot gives for the org", name);
            warned++;
        }
        free(name);
    }
    return warned;
}

static size_t warn_gone_repos(const check_context *context, const char *path, size_t first_line,
                              const string_list *current) {
    size_t warned = 0;
    size_t index;
    for (index = 0; index < current->count; index++) {
        char *name = org_repos_line_name(current->items[index]);
        if (name != NULL && snapshot_find(context->snapshot, name, strlen(name)) == NULL) {
            report_add(context->out, SEVERITY_WARNED, path, first_line + 1 + index, "org-names-gone-repo",
                       "lists %s, which the snapshot does not list", name);
            warned++;
        }
        free(name);
    }
    return warned;
}

size_t drift_warn_org_members(const check_context *context, const char *path, size_t first_line,
                              const string_list *current, const string_list *expected) {
    size_t warned = warn_missing_repos(context, path, first_line, current, expected);
    return warned + warn_gone_repos(context, path, first_line, current);
}

void drift_warn_older_version(report *out, const char *path, long declared, int newest) {
    report_add(out, SEVERITY_WARNED, path, 1, "older-version",
               "declares tarn %ld, and the newest is tarn %d, so the tree is pinned until it moves", declared, newest);
}

/**
 * Tells whether text can sit inside one quoted shell word on both cmd.exe and a POSIX shell, unchanged.
 * @implNote Non-ASCII is refused on Windows, where _popen hands the command to cmd.exe in the ANSI code page.
 */
static int is_shell_safe(const char *text) {
    for (; *text != '\0'; text++) {
        unsigned char character = (unsigned char)*text;
        if (character < 0x20 || character == '"' || character == '\'' || character == '%' || character == '!' ||
            (IS_WINDOWS && character >= 0x80)) {
            return 0;
        }
    }
    return 1;
}

/** Tells whether folder holds no character a glob pathspec would read as a pattern. */
static int is_glob_safe(const char *folder) {
    return strpbrk(folder, IS_WINDOWS ? "*?[" : "*?[\\") == NULL;
}

static int is_separator(char character) {
    return character == '/' || (IS_WINDOWS && character == '\\');
}

/** The repo root without trailing separators, which would escape the closing quote on Windows; `/` and `C:/` stay. */
static char *trimmed_root(const char *repo_root) {
    size_t length = strlen(repo_root);
    while (length > 1 && is_separator(repo_root[length - 1]) && repo_root[length - 2] != ':') {
        length--;
    }
    return memory_copy_bytes(repo_root, length);
}

static void command_append(command_text *command, const char *text) {
    size_t length = strlen(text);
    command->data = memory_resize(command->data, command->length + length + 1);
    memcpy(command->data + command->length, text, length + 1);
    command->length += length;
}

static void command_append_quoted(command_text *command, const char *text) {
    char quote[2] = {SHELL_QUOTE, '\0'};
    command_append(command, " ");
    command_append(command, quote);
    command_append(command, text);
    command_append(command, quote);
}

/** The pathspec `:(glob)<folder>/<pattern>`, matching files directly in folder only; folder "" is the repo root. */
static char *glob_spec(const char *folder, const char *pattern) {
    const char *slash = folder[0] == '\0' ? "" : "/";
    int length = snprintf(NULL, 0, "%s%s%s%s", glob_magic, folder, slash, pattern);
    char *spec;
    if (length < 0) {
        abort();
    }
    spec = memory_allocate((size_t)length + 1);
    if (snprintf(spec, (size_t)length + 1, "%s%s%s%s", glob_magic, folder, slash, pattern) != length) {
        abort();
    }
    return spec;
}

static void push_spec(string_list *specs, const char *folder, const char *pattern) {
    char *spec = glob_spec(folder, pattern);
    string_list_push(specs, spec);
    free(spec);
}

static void add_code_specs(const char *folder, string_list *specs) {
    size_t count;
    const char *const *extensions = rules_code_extensions(&count);
    size_t index;
    for (index = 0; index < count; index++) {
        char *pattern = memory_allocate(strlen(extensions[index]) + 2);
        pattern[0] = '*';
        strcpy(pattern + 1, extensions[index]);
        push_spec(specs, folder, pattern);
        free(pattern);
    }
    push_spec(specs, folder, RULES_CODE_FILE_NAME);
}

/**
 * Builds `git -C <root> <arguments>` with each of specs quoted after it (specs may be NULL), or returns NULL when a
 * path it would quote is not shell safe.
 */
static char *build_git_command(const char *repo_root, const char *arguments, const string_list *specs) {
    command_text command = {NULL, 0};
    size_t count = specs == NULL ? 0 : specs->count;
    size_t index;
    if (!is_shell_safe(repo_root)) {
        return NULL;
    }
    for (index = 0; index < count; index++) {
        if (!is_shell_safe(specs->items[index])) {
            return NULL;
        }
    }
    command_append(&command, "git -C");
    command_append_quoted(&command, repo_root);
    command_append(&command, arguments);
    for (index = 0; index < count; index++) {
        command_append_quoted(&command, specs->items[index]);
    }
    command_append(&command, DISCARD_ERRORS);
    return command.data;
}

static FILE *command_open(const char *command) {
#ifdef _WIN32
    return _popen(command, "r");
#else
    return popen(command, "r");
#endif
}

static void command_close(FILE *stream) {
#ifdef _WIN32
    _pclose(stream);
#else
    pclose(stream);
#endif
}

static int parse_seconds(const char *line, long long *seconds) {
    char *end;
    long long value;
    if (line[0] < '0' || line[0] > '9') {
        return 0;
    }
    value = strtoll(line, &end, 10);
    if (*end != '\0' && *end != '\n' && *end != '\r') {
        return 0;
    }
    *seconds = value;
    return 1;
}

/** Runs command and keeps the first line it prints; returns 0 when it cannot run or prints nothing. */
static int first_output_line(char *command, char *line, int size) {
    char rest[64];
    FILE *stream;
    int found;
    if (command == NULL) {
        return 0;
    }
    stream = command_open(command);
    free(command);
    if (stream == NULL) {
        return 0;
    }
    found = fgets(line, size, stream) != NULL;
    while (fgets(rest, sizeof rest, stream) != NULL) {
        continue;
    }
    command_close(stream);
    return found;
}

/**
 * Asks git for the newest author time of specs; returns 0 when git fails, prints nothing, or cannot be asked.
 * @implNote The author time, because a rebase resets every committer time and would make a rebased doc look fresh.
 */
static int last_commit_time(const char *repo_root, const string_list *specs, long long *seconds) {
    char line[64];
    char *command = build_git_command(repo_root, " log -1 --format=%at --", specs);
    return first_output_line(command, line, (int)sizeof line) && parse_seconds(line, seconds);
}

static int is_shallow_clone(const char *repo_root) {
    char line[64];
    char *command = build_git_command(repo_root, " rev-parse --is-shallow-repository", NULL);
    return first_output_line(command, line, (int)sizeof line) && strncmp(line, "true", 4) == 0 &&
           (line[4] == '\0' || line[4] == '\n' || line[4] == '\r');
}

static int folder_commit_time(const char *repo_root, const char *folder, int is_doc, long long *seconds) {
    string_list specs;
    int found;
    string_list_init(&specs);
    if (is_doc) {
        push_spec(&specs, folder, "*");
    } else {
        add_code_specs(folder, &specs);
    }
    found = last_commit_time(repo_root, &specs, seconds);
    string_list_free(&specs);
    return found;
}

static void format_date(long long seconds, char date[DATE_LENGTH]) {
    time_t moment = (time_t)seconds;
    struct tm *calendar = gmtime(&moment);
    if (calendar == NULL || strftime(date, DATE_LENGTH, "%Y-%m-%d", calendar) == 0) {
        strcpy(date, "unknown");
    }
}

/** Reports a shallow clone once; returns 0 when the run must stop, since a shallow clone's dates say nothing. */
static int accept_history(const check_context *context, const char *repo_root, shallow_state *shallow) {
    if (*shallow == SHALLOW_UNASKED) {
        *shallow = is_shallow_clone(repo_root) ? SHALLOW_YES : SHALLOW_NO;
        if (*shallow == SHALLOW_YES) {
            report_add(context->out, SEVERITY_WARNED, "", 0, "staleness-skipped",
                       "the clone is shallow, so its commit dates cannot show a stale source doc and source-doc-stale "
                       "was skipped");
        }
    }
    return *shallow == SHALLOW_NO;
}

static void warn_stale(const check_context *context, const char *folder, const char *doc_folder, long long code_time,
                       long long doc_time) {
    char code_date[DATE_LENGTH];
    char doc_date[DATE_LENGTH];
    format_date(code_time, code_date);
    format_date(doc_time, doc_date);
    report_add(context->out, SEVERITY_WARNED, doc_folder, 0, "source-doc-stale",
               "%s has commits %lld days newer than its doc, %s against %s",
               folder[0] == '\0' ? "the repo root" : folder, (code_time - doc_time) / SECONDS_PER_DAY, code_date,
               doc_date);
}

/** Returns 0 when the whole check must stop. */
static int check_source_doc(const check_context *context, const char *repo_root, const char *folder,
                            const char *doc_folder, shallow_state *shallow) {
    long long code_time;
    long long doc_time;
    if (!is_glob_safe(folder) || !folder_commit_time(repo_root, folder, 0, &code_time) ||
        !folder_commit_time(repo_root, doc_folder, 1, &doc_time)) {
        return 1;
    }
    if (!accept_history(context, repo_root, shallow)) {
        return 0;
    }
    if (code_time - doc_time > DRIFT_STALE_DAYS * SECONDS_PER_DAY) {
        warn_stale(context, folder, doc_folder, code_time, doc_time);
    }
    return 1;
}

void drift_check_source_docs(const check_context *context) {
    char *repo_root = trimmed_root(context->repo_root);
    shallow_state shallow = SHALLOW_UNASKED;
    int going = 1;
    size_t index;
    for (index = 0; going && index < context->code_folders.count; index++) {
        const char *folder = context->code_folders.items[index];
        char *doc_folder = path_join(source_root, folder);
        if (path_lookup_exact(context->repo_root, doc_folder) == PATH_DIRECTORY) {
            going = check_source_doc(context, repo_root, folder, doc_folder, &shallow);
        }
        free(doc_folder);
    }
    free(repo_root);
}
