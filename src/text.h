#ifndef TARN_TEXT_H
#define TARN_TEXT_H

#include <stddef.h>

typedef struct {
    char *text;
    size_t number;
} text_line;

/** A file split into lines, with a leading UTF-8 BOM and each line's trailing CR removed. */
typedef struct {
    char *storage;
    text_line *lines;
    size_t count;
} text_document;

typedef struct {
    const char *target;
    size_t length;
} markdown_link;

int text_document_load(const char *path, text_document *out);
/** Takes ownership of content, which must hold length + 1 bytes. */
void text_document_from_bytes(char *content, size_t length, text_document *out);
void text_document_free(text_document *document);

int text_is_blank(const char *line);
int text_starts_with(const char *text, const char *prefix);
int text_ends_with(const char *text, const char *suffix);
int text_equal_ignoring_case(const char *left, const char *right);

/** Marks each line that is a fence or sits inside a fenced code block. The caller frees the result. */
unsigned char *text_fenced_lines(const text_document *document);
int text_is_size_exception(const char *line);
int text_is_generated_marker(const char *line);
int text_is_generated_end(const char *line);
/** The <what> of a `<!-- generated: <what> -->` line as a new string the caller frees, or NULL for any other line. */
char *text_generated_what(const char *line);

/** Finds the next link at or after *position, skipping inline code; returns 0 when there is none. */
int markdown_next_link(const char *line, size_t *position, markdown_link *out);

#endif
