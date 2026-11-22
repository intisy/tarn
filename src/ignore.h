#ifndef TARN_IGNORE_H
#define TARN_IGNORE_H

#include <stddef.h>

typedef struct {
    char *glob;
    int negated;
    int directory_only;
    int anchored;
} ignore_pattern;

/** The source mirror's ignore list: the root `.gitignore` plus the fixed folders of source.md rule 4. */
typedef struct {
    ignore_pattern *patterns;
    size_t count;
} ignore_rules;

void ignore_load(const char *repo_root, ignore_rules *out);
void ignore_free(ignore_rules *rules);
/** Whether the entry itself is ignored; the walk never descends into an ignored folder. */
int ignore_matches(const ignore_rules *rules, const char *relative, int is_directory);
/** Whether the entry or any folder above it is ignored. */
int ignore_covers(const ignore_rules *rules, const char *relative, int is_directory);

#endif
