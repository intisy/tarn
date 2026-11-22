#include "adopt.h"
#include "check.h"
#include "report.h"
#include "snapshot.h"

#include <stdio.h>
#include <string.h>

#define EXIT_NOT_ADOPTED 1
#define EXIT_BAD_USAGE 2

static const char usage[] =
    "usage: tarn-check [--quick] [--write] [--warn-only] [--snapshot <file> --repository <owner>/<name>] "
    "[<repo root>]\n"
    "       tarn-check adopt <repo|org> --standard-ref <commit> [--write] "
    "[--snapshot <file> --repository <owner>/<name>] [<repo root>]\n";

typedef struct {
    const char *repo_root;
    const char *snapshot_path;
    int warn_only;
    check_options options;
} command_line;

/** `--snapshot` and `--repository` are required for an org, which fills its org-repos block, and refused for a repo. */
typedef struct {
    const char *repo_root;
    const char *snapshot_path;
    adopt_options options;
} adopt_command;

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

static int parse_adopt_kind(const char *argument, adopt_kind *kind) {
    if (strcmp(argument, "repo") == 0) {
        *kind = ADOPT_REPO;
        return 1;
    }
    if (strcmp(argument, "org") == 0) {
        *kind = ADOPT_ORG;
        return 1;
    }
    return 0;
}

static int parse_adopt_arguments(int argument_count, char **arguments, adopt_command *parsed) {
    int index;
    for (index = 3; index < argument_count; index++) {
        const char *argument = arguments[index];
        if (strcmp(argument, "--write") == 0) {
            parsed->options.write = 1;
        } else if (strcmp(argument, "--standard-ref") == 0 && has_value(index, argument_count)) {
            parsed->options.standard_ref = arguments[++index];
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
    return 1;
}

static int parse_adopt_command(int argument_count, char **arguments, adopt_command *parsed) {
    int has_snapshot;
    memset(parsed, 0, sizeof *parsed);
    if (argument_count < 3 || !parse_adopt_kind(arguments[2], &parsed->options.kind) ||
        !parse_adopt_arguments(argument_count, arguments, parsed)) {
        return 0;
    }
    if (parsed->repo_root == NULL) {
        parsed->repo_root = ".";
    }
    has_snapshot = parsed->snapshot_path != NULL;
    return parsed->options.standard_ref != NULL && adopt_is_standard_ref(parsed->options.standard_ref) &&
           has_snapshot == (parsed->options.repository != NULL) &&
           has_snapshot == (parsed->options.kind == ADOPT_ORG);
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

static int report_adopt_refusal(const adopt_command *parsed, int result) {
    int as_repo = parsed->options.kind == ADOPT_REPO;
    if (result == ADOPT_NOT_A_FOLDER) {
        fprintf(stderr, "tarn-check: %s is not a folder\n%s", parsed->repo_root, usage);
        return EXIT_BAD_USAGE;
    }
    if (result == ADOPT_REPOSITORY_UNKNOWN) {
        fprintf(stderr, "tarn-check: the snapshot %s does not list %s\n", parsed->snapshot_path,
                parsed->options.repository);
        return EXIT_BAD_USAGE;
    }
    fprintf(stderr, "tarn-check: %s is already %s, so it cannot adopt as %s\n", parsed->repo_root,
            as_repo ? "an org docs tree (VERSION)" : "a repo docs tree (docs/VERSION)", as_repo ? "a repo" : "an org");
    return EXIT_NOT_ADOPTED;
}

static int run_adopt(const adopt_command *parsed) {
    adopt_outcome outcome;
    int result;
    int exit_code = 0;
    size_t index;
    adopt_outcome_init(&outcome);
    result = adopt_tree(parsed->repo_root, &parsed->options, &outcome);
    if (result != ADOPT_DONE && result != ADOPT_UNWRITABLE) {
        adopt_outcome_free(&outcome);
        return report_adopt_refusal(parsed, result);
    }
    for (index = 0; index < outcome.lines.count; index++) {
        printf("%s\n", outcome.lines.items[index]);
    }
    if (result == ADOPT_UNWRITABLE) {
        fprintf(stderr, "tarn-check: %s cannot be written, and no file after it was\n", outcome.unwritable);
        exit_code = EXIT_NOT_ADOPTED;
    }
    adopt_outcome_free(&outcome);
    return exit_code;
}

static int adopt_main(int argument_count, char **arguments) {
    adopt_command parsed;
    repos_snapshot snapshot;
    int exit_code;
    if (!parse_adopt_command(argument_count, arguments, &parsed)) {
        fputs(usage, stderr);
        return EXIT_BAD_USAGE;
    }
    if (parsed.snapshot_path == NULL) {
        return run_adopt(&parsed);
    }
    if (!load_snapshot(parsed.snapshot_path, &snapshot)) {
        return EXIT_BAD_USAGE;
    }
    parsed.options.snapshot = &snapshot;
    exit_code = run_adopt(&parsed);
    snapshot_free(&snapshot);
    return exit_code;
}

int main(int argument_count, char **arguments) {
    command_line parsed;
    repos_snapshot snapshot;
    int exit_code;
    if (argument_count >= 2 && strcmp(arguments[1], "adopt") == 0) {
        return adopt_main(argument_count, arguments);
    }
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
