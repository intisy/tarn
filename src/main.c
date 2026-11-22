#include "check.h"
#include "report.h"
#include "snapshot.h"

#include <stdio.h>
#include <string.h>

#define EXIT_BAD_USAGE 2

static const char usage[] =
    "usage: tarn-check [--quick] [--write] [--warn-only] [--snapshot <file> --repository <owner>/<name>] "
    "[<repo root>]\n";

typedef struct {
    const char *repo_root;
    const char *snapshot_path;
    int warn_only;
    check_options options;
} command_line;

static int is_option(const char *argument) {
    return argument[0] == '-' && argument[1] != '\0';
}

static int has_value(int index, int argument_count) {
    return index + 1 < argument_count;
}

static int parse_command_line(int argument_count, char **arguments, command_line *parsed) {
    int index;
    memset(parsed, 0, sizeof *parsed);
    for (index = 1; index < argument_count; index++) {
        const char *argument = arguments[index];
        if (strcmp(argument, "--quick") == 0) {
            parsed->options.quick = 1;
        } else if (strcmp(argument, "--write") == 0) {
            parsed->options.write = 1;
        } else if (strcmp(argument, "--warn-only") == 0) {
            parsed->warn_only = 1;
        } else if (strcmp(argument, "--snapshot") == 0 && has_value(index, argument_count)) {
            parsed->snapshot_path = arguments[++index];
        } else if (strcmp(argument, "--repository") == 0 && has_value(index, argument_count)) {
            parsed->options.repository = arguments[++index];
        } else if (is_option(argument) || parsed->repo_root != NULL) {
            return 0;
        } else {
            parsed->repo_root = argument;
        }
    }
    if (parsed->repo_root == NULL) {
        parsed->repo_root = ".";
    }
    return (parsed->snapshot_path == NULL) == (parsed->options.repository == NULL);
}

static int load_snapshot(const char *path, repos_snapshot *snapshot) {
    size_t bad_line;
    snapshot_status status = snapshot_load(path, snapshot, &bad_line);
    if (status == SNAPSHOT_UNREADABLE) {
        fprintf(stderr, "tarn-check: the snapshot %s cannot be read\n", path);
    } else if (status == SNAPSHOT_MALFORMED) {
        fprintf(stderr, "tarn-check: %s:%zu does not read <owner>/<name> <public|private> <follows|not>\n", path,
                bad_line);
    } else if (status == SNAPSHOT_DUPLICATE) {
        fprintf(stderr, "tarn-check: %s:%zu lists a repo a second time\n", path, bad_line);
    }
    return status == SNAPSHOT_READ;
}

static int run(const command_line *parsed) {
    report out;
    int result;
    int exit_code;
    report_init(&out);
    result = check_tree(parsed->repo_root, &parsed->options, &out);
    if (result == CHECK_NOT_A_FOLDER) {
        fprintf(stderr, "tarn-check: %s is not a folder\n%s", parsed->repo_root, usage);
    } else if (result == CHECK_REPOSITORY_UNKNOWN) {
        fprintf(stderr, "tarn-check: the snapshot %s does not list %s\n", parsed->snapshot_path,
                parsed->options.repository);
    }
    if (result != CHECK_DONE) {
        report_free(&out);
        return EXIT_BAD_USAGE;
    }
    if (parsed->warn_only) {
        report_write_warnings(&out, stdout);
        exit_code = 0;
    } else {
        report_write(&out, stdout);
        exit_code = report_exit_code(&out);
    }
    report_free(&out);
    return exit_code;
}

int main(int argument_count, char **arguments) {
    command_line parsed;
    repos_snapshot snapshot;
    int exit_code;
    if (!parse_command_line(argument_count, arguments, &parsed)) {
        fputs(usage, stderr);
        return EXIT_BAD_USAGE;
    }
    if (parsed.snapshot_path == NULL) {
        return run(&parsed);
    }
    if (!load_snapshot(parsed.snapshot_path, &snapshot)) {
        return EXIT_BAD_USAGE;
    }
    parsed.options.snapshot = &snapshot;
    exit_code = run(&parsed);
    snapshot_free(&snapshot);
    return exit_code;
}
