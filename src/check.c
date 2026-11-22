#include "check.h"

#include "context.h"
#include "directory.h"
#include "generate.h"
#include "path.h"
#include "walk.h"

#include <stdlib.h>
#include <string.h>

#define NEWEST_VERSION 1
#define MOST_VERSION_DIGITS 9

typedef enum { VERSION_ABSENT, VERSION_MALFORMED, VERSION_READ } version_state;

typedef struct {
    version_state state;
    long number;
} version_file;

static int is_line_end(const char *rest, const char *end) {
    size_t remaining = (size_t)(end - rest);
    return remaining == 0 || (remaining == 1 && rest[0] == '\n') ||
           (remaining == 2 && rest[0] == '\r' && rest[1] == '\n');
}

static version_file parse_version(const char *content, size_t length) {
    version_file version = {VERSION_MALFORMED, 0};
    const char *end = content + length;
    const char *scan = content;
    size_t digits = 0;
    if (length >= 3 && memcmp(content, "\xEF\xBB\xBF", 3) == 0) {
        scan += 3;
    }
    if ((size_t)(end - scan) < 5 || memcmp(scan, "tarn ", 5) != 0) {
        return version;
    }
    scan += 5;
    while (scan + digits < end && scan[digits] >= '0' && scan[digits] <= '9') {
        digits++;
    }
    if (digits == 0 || digits > MOST_VERSION_DIGITS || !is_line_end(scan + digits, end)) {
        return version;
    }
    for (; scan < content + length && *scan >= '0' && *scan <= '9'; scan++) {
        version.number = version.number * 10 + (*scan - '0');
    }
    version.state = VERSION_READ;
    return version;
}

static version_file read_version(const char *repo_root, const char *relative) {
    version_file version = {VERSION_ABSENT, 0};
    char *on_disk;
    char *content;
    size_t length;
    if (path_lookup_exact(repo_root, relative) != PATH_FILE) {
        return version;
    }
    on_disk = path_on_disk(repo_root, relative);
    if (directory_read_file(on_disk, &content, &length) != 0) {
        free(on_disk);
        version.state = VERSION_MALFORMED;
        return version;
    }
    version = parse_version(content, length);
    free(content);
    free(on_disk);
    return version;
}

static int accept_version(report *out, const char *path, version_file version) {
    if (version.state == VERSION_MALFORMED) {
        report_add(out, SEVERITY_REFUSED, path, 1, "version-malformed",
                   "the file must be exactly one line, tarn <n>");
        return 0;
    }
    if (version.number != NEWEST_VERSION) {
        report_add(out, SEVERITY_REFUSED, path, 1, "version-unknown",
                   "declares tarn %ld, and this checker knows version %d only", version.number, NEWEST_VERSION);
        return 0;
    }
    return 1;
}

static int decide_tree_kind(check_context *context) {
    version_file repo_version = read_version(context->repo_root, "docs/VERSION");
    version_file org_version = read_version(context->repo_root, "VERSION");
    if (repo_version.state != VERSION_ABSENT && org_version.state == VERSION_READ) {
        report_add(context->out, SEVERITY_REFUSED, "VERSION", 1, "version-twice",
                   "a repo carries one version file, and this one has VERSION and docs/VERSION");
        return 0;
    }
    if (repo_version.state != VERSION_ABSENT) {
        context->kind = TREE_REPO;
        context->descent_root = "docs";
        return accept_version(context->out, "docs/VERSION", repo_version);
    }
    if (org_version.state != VERSION_ABSENT) {
        context->kind = TREE_ORG;
        context->descent_root = "";
        return accept_version(context->out, "VERSION", org_version);
    }
    report_add(context->out, SEVERITY_REFUSED, "docs/VERSION", 0, "version-missing",
               "there is no docs/VERSION and no root VERSION, so this is not a tarn docs tree");
    return 0;
}

int check_context_open(const char *repo_root, report *out, check_context *context) {
    if (!directory_is_directory(repo_root)) {
        return -1;
    }
    memset(context, 0, sizeof *context);
    context->repo_root = repo_root;
    context->out = out;
    string_list_init(&context->code_folders);
    if (!decide_tree_kind(context)) {
        return 0;
    }
    ignore_load(repo_root, &context->ignore);
    if (context->kind == TREE_REPO) {
        walk_find_code_folders(context);
    }
    return 1;
}

void check_context_close(check_context *context) {
    ignore_free(&context->ignore);
    string_list_free(&context->code_folders);
}

int check_tree(const char *repo_root, const check_options *options, report *out) {
    check_context context;
    int opened = check_context_open(repo_root, out, &context);
    if (opened < 0) {
        return -1;
    }
    if (opened > 0) {
        generate_blocks(&context, options->write);
        walk_descent_tree(&context);
        if (context.kind == TREE_REPO) {
            walk_check_mirror_complete(&context);
        }
    }
    check_context_close(&context);
    return 0;
}
