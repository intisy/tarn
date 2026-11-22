#ifndef TARN_REPORT_H
#define TARN_REPORT_H

#include <stddef.h>
#include <stdio.h>

#if defined(__GNUC__) || defined(__clang__)
#define REPORT_PRINTF(format_index, first_argument) __attribute__((format(printf, format_index, first_argument)))
#else
#define REPORT_PRINTF(format_index, first_argument)
#endif

typedef enum { SEVERITY_REFUSED, SEVERITY_WARNED } report_severity;

/** One finding, printed as `refused|warned <path>:<line> <rule> <text>`; line 0 means the whole file or folder. */
typedef struct {
    report_severity severity;
    char *path;
    size_t line;
    char *rule;
    char *text;
} report_entry;

typedef struct {
    report_entry *entries;
    size_t count;
    size_t capacity;
} report;

void report_init(report *out);
void report_free(report *out);
void report_add(report *out, report_severity severity, const char *path, size_t line, const char *rule,
                const char *format, ...) REPORT_PRINTF(6, 7);
size_t report_refused_count(const report *out);
int report_exit_code(const report *out);
char *report_format(const report *out);
void report_write(const report *out, FILE *stream);
void report_write_warnings(const report *out, FILE *stream);
/** Warns `prose-to-write` at the line, whose text is still the placeholder TEXT_PLACEHOLDER. */
void report_prose_to_write(report *out, const char *path, size_t line);

#endif
