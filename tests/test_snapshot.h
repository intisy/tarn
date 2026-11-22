#ifndef TARN_TEST_SNAPSHOT_H
#define TARN_TEST_SNAPSHOT_H

#include "snapshot.h"

/** `<base>/<name>` as a new string the caller frees, or NULL when it cannot be made. */
char *test_path_join(const char *base, const char *name);
/** Loads `<folder>/<file>` into out and says on stderr why when it cannot; returns 1 when it is loaded. */
int test_snapshot_load(const char *folder, const char *file, repos_snapshot *out);

#endif
