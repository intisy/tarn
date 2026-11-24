#ifndef TARN_ADOPT_H
#define TARN_ADOPT_H

#include "snapshot.h"
#include "string_list.h"

typedef enum { ADOPT_REPO, ADOPT_ORG } adopt_kind;

typedef struct {
    adopt_kind kind;
    /** The intisy/tarn commit the workflow caller pins, 40 hexadecimal characters. */
    const char *standard_ref;
    int write;
    /** Required for ADOPT_ORG, which fills its org-repos block from them; unused for ADOPT_REPO. */
    const repos_snapshot *snapshot;
    const char *repository;
} adopt_options;

/**
 * One line per planned file, `create <path>` or `keep <path>`, then for an org one `does-not-follow <owner>/<name>`
 * line per repo its org-repos block marks as not following. blocking names the path a refusal or a failed write is
 * about: the file a write failed on, the version file that is not tarn's, the folder that differs only in case, or the record a public org tree holds.
 */
typedef struct {
    string_list lines;
    char *blocking;
} adopt_outcome;

enum {
    ADOPT_DONE = 0,
    ADOPT_NOT_A_FOLDER = -1,
    ADOPT_REPOSITORY_UNKNOWN = -2,
    ADOPT_OTHER_KIND_DECLARED = -3,
    ADOPT_UNWRITABLE = -4,
    ADOPT_VERSION_FOREIGN = -5,
    ADOPT_CASE_DIFFERS = -6,
    ADOPT_RECORD_IN_PUBLIC_TREE = -7
};

void adopt_outcome_init(adopt_outcome *outcome);
void adopt_outcome_free(adopt_outcome *outcome);

/**
 * Plans the files that adopting the standard needs in the repo at repo_root and, when options->write is set, creates
 * the planned ones that are absent. A file that exists, under any case, is kept and never opened for writing.
 */
int adopt_tree(const char *repo_root, const adopt_options *options, adopt_outcome *outcome);

/** Whether text is a full commit id: 40 lowercase hexadecimal characters. */
int adopt_is_standard_ref(const char *text);
/** The thin workflow caller pinned to standard_ref, as a new string the caller frees. */
char *adopt_caller_text(const char *standard_ref);

#define ADOPT_CALLER_PATH ".github/workflows/docs.yml"

#endif
