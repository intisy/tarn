#include "text.h"

#include "directory.h"
#include "memory.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static const char byte_order_mark[] = "\xEF\xBB\xBF";

static size_t count_lines(const char *content, size_t length) {
    size_t index;
    size_t lines = 0;
    for (index = 0; index < length; index++) {
        if (content[index] == '\n') {
            lines++;
        }
    }
    if (length > 0 && content[length - 1] != '\n') {
        lines++;
    }
    return lines;
}

static void strip_carriage_return(char *line, size_t length) {
    if (length > 0 && line[length - 1] == '\r') {
        line[length - 1] = '\0';
    }
}

void text_document_from_bytes(char *content, size_t length, text_document *out) {
    size_t start = 0;
    size_t index;
    size_t line_index = 0;
    if (length >= 3 && memcmp(content, byte_order_mark, 3) == 0) {
        start = 3;
    }
    out->storage = content;
    out->count = count_lines(content + start, length - start);
    out->lines = memory_allocate(out->count * sizeof *out->lines);
    for (index = start; index <= length && line_index < out->count; index++) {
        if (index == length || content[index] == '\n') {
            content[index] = '\0';
            strip_carriage_return(content + start, index - start);
            out->lines[line_index].text = content + start;
            out->lines[line_index].number = line_index + 1;
            line_index++;
            start = index + 1;
        }
    }
}

int text_document_load(const char *path, text_document *out) {
    char *content;
    size_t length;
    if (directory_read_file(path, &content, &length) != 0) {
        return -1;
    }
    text_document_from_bytes(content, length, out);
    return 0;
}

void text_document_free(text_document *document) {
    free(document->storage);
    free(document->lines);
    document->storage = NULL;
    document->lines = NULL;
    document->count = 0;
}

int text_is_blank(const char *line) {
    for (; *line != '\0'; line++) {
        if (!isspace((unsigned char)*line)) {
            return 0;
        }
    }
    return 1;
}

int text_starts_with(const char *text, const char *prefix) {
    return strncmp(text, prefix, strlen(prefix)) == 0;
}

int text_ends_with(const char *text, const char *suffix) {
    size_t text_length = strlen(text);
    size_t suffix_length = strlen(suffix);
    return text_length >= suffix_length && strcmp(text + text_length - suffix_length, suffix) == 0;
}

int text_equal_ignoring_case(const char *left, const char *right) {
    for (; *left != '\0' && *right != '\0'; left++, right++) {
        if (tolower((unsigned char)*left) != tolower((unsigned char)*right)) {
            return 0;
        }
    }
    return *left == '\0' && *right == '\0';
}

typedef struct {
    char marker;
    size_t length;
} fence;

static int read_fence(const char *line, fence *out) {
    size_t indent = 0;
    size_t length = 0;
    char marker;
    while (indent < 3 && line[indent] == ' ') {
        indent++;
    }
    marker = line[indent];
    if (marker != '`' && marker != '~') {
        return 0;
    }
    while (line[indent + length] == marker) {
        length++;
    }
    if (length < 3) {
        return 0;
    }
    out->marker = marker;
    out->length = length;
    return 1;
}

unsigned char *text_fenced_lines(const text_document *document) {
    unsigned char *fenced = memory_allocate(document->count);
    fence open = {0, 0};
    int inside = 0;
    size_t index;
    for (index = 0; index < document->count; index++) {
        fence current;
        int is_fence = read_fence(document->lines[index].text, &current);
        fenced[index] = (unsigned char)(inside || is_fence);
        if (!is_fence) {
            continue;
        }
        if (!inside) {
            open = current;
            inside = 1;
        } else if (current.marker == open.marker && current.length >= open.length) {
            inside = 0;
        }
    }
    return fenced;
}

static int is_comment_with_prefix(const char *line, const char *prefix) {
    static const char suffix[] = " -->";
    size_t content_start = strlen(prefix);
    size_t line_length = strlen(line);
    return text_starts_with(line, prefix) && text_ends_with(line, suffix) &&
           line_length > content_start + strlen(suffix);
}

int text_is_size_exception(const char *line) {
    return is_comment_with_prefix(line, "<!-- size: ");
}

int text_is_generated_end(const char *line) {
    return strcmp(line, "<!-- end generated -->") == 0;
}

int text_is_generated_marker(const char *line) {
    return text_is_generated_end(line) || is_comment_with_prefix(line, "<!-- generated: ");
}

char *text_generated_what(const char *line) {
    static const char prefix[] = "<!-- generated: ";
    static const char suffix[] = " -->";
    size_t what_length;
    if (!is_comment_with_prefix(line, prefix)) {
        return NULL;
    }
    what_length = strlen(line) - (sizeof prefix - 1) - (sizeof suffix - 1);
    return memory_copy_bytes(line + sizeof prefix - 1, what_length);
}

static size_t backtick_run(const char *line, size_t position) {
    size_t length = 0;
    while (line[position + length] == '`') {
        length++;
    }
    return length;
}

static size_t skip_inline_code(const char *line, size_t position) {
    size_t opening = backtick_run(line, position);
    size_t scan = position + opening;
    while (line[scan] != '\0') {
        if (line[scan] == '`') {
            size_t closing = backtick_run(line, scan);
            if (closing == opening) {
                return scan + closing;
            }
            scan += closing;
        } else {
            scan++;
        }
    }
    return position + opening;
}

int text_is_org_repos_line(const char *line, int *inside) {
    char *what = text_generated_what(line);
    int is_marker = what != NULL || text_is_generated_end(line);
    int was_inside = *inside;
    if (what != NULL) {
        *inside = strcmp(what, "org-repos") == 0;
    } else if (is_marker) {
        *inside = 0;
    }
    free(what);
    return was_inside && !is_marker;
}

char *text_without_inline_code(const char *line) {
    char *copy = memory_copy_string(line);
    size_t scan = 0;
    while (line[scan] != '\0') {
        if (line[scan] == '\\' && line[scan + 1] != '\0') {
            scan += 2;
        } else if (line[scan] == '`') {
            size_t end = skip_inline_code(line, scan);
            if (end > scan + backtick_run(line, scan)) {
                memset(copy + scan, ' ', end - scan);
            }
            scan = end;
        } else {
            scan++;
        }
    }
    return copy;
}

static size_t find_closing(const char *line, size_t position, char opening, char closing) {
    int depth = 0;
    for (; line[position] != '\0'; position++) {
        if (line[position] == '\\' && line[position + 1] != '\0') {
            position++;
        } else if (line[position] == opening) {
            depth++;
        } else if (line[position] == closing) {
            if (--depth == 0) {
                return position;
            }
        }
    }
    return 0;
}

static void trim_target(const char *line, size_t start, size_t end, markdown_link *out) {
    while (start < end && line[start] == ' ') {
        start++;
    }
    if (start < end && line[start] == '<') {
        const char *closing = memchr(line + start, '>', end - start);
        if (closing != NULL) {
            out->target = line + start + 1;
            out->length = (size_t)(closing - (line + start + 1));
            return;
        }
    }
    out->target = line + start;
    out->length = 0;
    while (start + out->length < end && line[start + out->length] != ' ') {
        out->length++;
    }
}

static int link_at(const char *line, size_t position, size_t *after, markdown_link *out) {
    size_t text_end = find_closing(line, position, '[', ']');
    size_t target_end;
    if (text_end == 0 || line[text_end + 1] != '(') {
        return 0;
    }
    target_end = find_closing(line, text_end + 1, '(', ')');
    if (target_end == 0) {
        return 0;
    }
    trim_target(line, text_end + 2, target_end, out);
    *after = target_end + 1;
    return 1;
}

int markdown_next_link(const char *line, size_t *position, markdown_link *out) {
    size_t scan = *position;
    while (line[scan] != '\0') {
        if (line[scan] == '\\' && line[scan + 1] != '\0') {
            scan += 2;
        } else if (line[scan] == '`') {
            scan = skip_inline_code(line, scan);
        } else if (line[scan] == '[' && link_at(line, scan, position, out)) {
            return 1;
        } else {
            scan++;
        }
    }
    *position = scan;
    return 0;
}
