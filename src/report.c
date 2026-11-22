#include "report.h"

#include "memory.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

void report_init(report *out) {
    out->entries = NULL;
    out->count = 0;
    out->capacity = 0;
}

void report_free(report *out) {
    size_t index;
    for (index = 0; index < out->count; index++) {
        free(out->entries[index].path);
        free(out->entries[index].rule);
        free(out->entries[index].text);
    }
    free(out->entries);
    report_init(out);
}

static char *format_text(const char *format, va_list arguments) {
    va_list measuring;
    int length;
    char *text;
    va_copy(measuring, arguments);
    length = vsnprintf(NULL, 0, format, measuring);
    va_end(measuring);
    if (length < 0) {
        return memory_copy_string(format);
    }
    text = memory_allocate((size_t)length + 1);
    vsnprintf(text, (size_t)length + 1, format, arguments);
    return text;
}

void report_add(report *out, report_severity severity, const char *path, size_t line, const char *rule,
                const char *format, ...) {
    va_list arguments;
    report_entry *entry;
    if (out->count == out->capacity) {
        out->capacity = out->capacity == 0 ? 16 : out->capacity * 2;
        out->entries = memory_resize(out->entries, out->capacity * sizeof *out->entries);
    }
    entry = &out->entries[out->count++];
    entry->severity = severity;
    entry->path = memory_copy_string(path[0] == '\0' ? "." : path);
    entry->line = line;
    entry->rule = memory_copy_string(rule);
    va_start(arguments, format);
    entry->text = format_text(format, arguments);
    va_end(arguments);
}

size_t report_refused_count(const report *out) {
    size_t index;
    size_t refused = 0;
    for (index = 0; index < out->count; index++) {
        if (out->entries[index].severity == SEVERITY_REFUSED) {
            refused++;
        }
    }
    return refused;
}

int report_exit_code(const report *out) {
    return report_refused_count(out) > 0 ? 1 : 0;
}

static const char *severity_word(report_severity severity) {
    return severity == SEVERITY_REFUSED ? "refused" : "warned";
}

static char *format_entry(const report_entry *entry) {
    const char *severity = severity_word(entry->severity);
    int length = snprintf(NULL, 0, "%s %s:%zu %s %s\n", severity, entry->path, entry->line, entry->rule, entry->text);
    char *line;
    if (length < 0) {
        return memory_copy_string("");
    }
    line = memory_allocate((size_t)length + 1);
    snprintf(line, (size_t)length + 1, "%s %s:%zu %s %s\n", severity, entry->path, entry->line, entry->rule,
             entry->text);
    return line;
}

char *report_format(const report *out) {
    size_t index;
    size_t length = 0;
    char *text = memory_allocate(1);
    text[0] = '\0';
    for (index = 0; index < out->count; index++) {
        char *line = format_entry(&out->entries[index]);
        size_t line_length = strlen(line);
        text = memory_resize(text, length + line_length + 1);
        memcpy(text + length, line, line_length + 1);
        length += line_length;
        free(line);
    }
    return text;
}

void report_write(const report *out, FILE *stream) {
    char *text = report_format(out);
    fputs(text, stream);
    free(text);
}

void report_prose_to_write(report *out, const char *path, size_t line) {
    report_add(out, SEVERITY_WARNED, path, line, "prose-to-write",
               "the text to write is a placeholder for a person to replace");
}
