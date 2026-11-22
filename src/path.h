#ifndef TARN_PATH_H
#define TARN_PATH_H

/* A relative path uses forward slashes, no leading or trailing slash, and is "" for the repo root. */

typedef enum { PATH_MISSING, PATH_FILE, PATH_DIRECTORY, PATH_CASE_DIFFERS } path_state;

char *path_join(const char *base, const char *child);
/** Resolves "." and ".." and drops empty parts; returns NULL when the path climbs above the root. */
char *path_normalize(const char *path);
char *path_parent(const char *path);
const char *path_base_name(const char *path);
char *path_on_disk(const char *repo_root, const char *relative);
/** Looks a relative path up component by component, comparing names byte for byte. */
path_state path_lookup_exact(const char *repo_root, const char *relative);

#endif
