#include "check.h"
#include "report.h"

#include <stdio.h>
#include <string.h>

#define EXIT_BAD_USAGE 2

static const char usage[] = "usage: tarn-check [--quick] [--snapshot <file>] [<repo root>]\n";

typedef struct {
    const char *repo_root;
    check_options options;
} command_line;

static int is_option(const char *argument) {
    return argument[0] == '-' && argument[1] != '\0';
}

static int parse_command_line(int argument_count, char **arguments, command_line *parsed) {
    int index;
    parsed->repo_root = NULL;
    parsed->options.snapshot_path = NULL;
    parsed->options.quick = 0;
    for (index = 1; index < argument_count; index++) {
        const char *argument = arguments[index];
        if (strcmp(argument, "--quick") == 0) {
            parsed->options.quick = 1;
        } else if (strcmp(argument, "--snapshot") == 0 && index + 1 < argument_count) {
            parsed->options.snapshot_path = arguments[++index];
        } else if (is_option(argument) || parsed->repo_root != NULL) {
            return 0;
        } else {
            parsed->repo_root = argument;
        }
    }
    if (parsed->repo_root == NULL) {
        parsed->repo_root = ".";
    }
    return 1;
}

int main(int argument_count, char **arguments) {
    command_line parsed;
    report out;
    int exit_code;
    if (!parse_command_line(argument_count, arguments, &parsed)) {
        fputs(usage, stderr);
        return EXIT_BAD_USAGE;
    }
    report_init(&out);
    if (check_tree(parsed.repo_root, &parsed.options, &out) != 0) {
        fprintf(stderr, "tarn-check: %s is not a folder\n%s", parsed.repo_root, usage);
        report_free(&out);
        return EXIT_BAD_USAGE;
    }
    report_write(&out, stdout);
    exit_code = report_exit_code(&out);
    report_free(&out);
    return exit_code;
}
