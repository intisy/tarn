#include "test_snapshot.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *test_path_join(const char *base, const char *name) {
    size_t length = strlen(base) + strlen(name) + 2;
    char *path = malloc(length);
    int written;
    if (path == NULL) {
        return NULL;
    }
    written = snprintf(path, length, "%s/%s", base, name);
    if (written < 0 || (size_t)written >= length) {
        free(path);
        return NULL;
    }
    return path;
}

int test_snapshot_load(const char *folder, const char *file, repos_snapshot *out) {
    char *path = test_path_join(folder, file);
    size_t bad_line;
    snapshot_status status;
    if (path == NULL) {
        return 0;
    }
    status = snapshot_load(path, out, &bad_line);
    if (status != SNAPSHOT_READ) {
        fprintf(stderr, "the snapshot %s does not load (status %d, line %zu)\n", path, (int)status, bad_line);
    }
    free(path);
    return status == SNAPSHOT_READ;
}
