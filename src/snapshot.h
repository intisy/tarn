#ifndef TARN_SNAPSHOT_H
#define TARN_SNAPSHOT_H

#include <stddef.h>

typedef enum { VISIBILITY_PUBLIC, VISIBILITY_PRIVATE } repo_visibility;

/** One line of the repos snapshot, `<owner>/<name> <public|private> <follows|not>`. */
typedef struct {
    char *full_name;
    size_t owner_length;
    repo_visibility visibility;
    int follows;
} snapshot_repo;

typedef struct {
    snapshot_repo *repos;
    size_t count;
} repos_snapshot;

typedef enum { SNAPSHOT_READ, SNAPSHOT_UNREADABLE, SNAPSHOT_MALFORMED, SNAPSHOT_DUPLICATE } snapshot_status;

/** Loads the snapshot at path; for SNAPSHOT_MALFORMED and SNAPSHOT_DUPLICATE, *bad_line is the line at fault. */
snapshot_status snapshot_load(const char *path, repos_snapshot *out, size_t *bad_line);
void snapshot_free(repos_snapshot *snapshot);

/** Finds `<owner>/<name>` given as length bytes, ignoring ASCII case as GitHub does; NULL when it is not listed. */
const snapshot_repo *snapshot_find(const repos_snapshot *snapshot, const char *full_name, size_t length);
int snapshot_same_owner(const snapshot_repo *left, const snapshot_repo *right);

int snapshot_is_name_character(char character);
/** Tells whether the length bytes at text are `<owner>/<name>`, each a non-empty run of name characters. */
int snapshot_is_repo_name(const char *text, size_t length);

#endif
