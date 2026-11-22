#include "snapshot.h"

#include "memory.h"
#include "text.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *start;
    size_t length;
} line_word;

int snapshot_is_name_character(char character) {
    return isalnum((unsigned char)character) || character == '.' || character == '_' || character == '-';
}

int snapshot_is_repo_name(const char *text, size_t length) {
    const char *slash = memchr(text, '/', length);
    size_t index;
    if (slash == NULL || slash == text || slash == text + length - 1) {
        return 0;
    }
    for (index = 0; index < length; index++) {
        if (text + index != slash && !snapshot_is_name_character(text[index])) {
            return 0;
        }
    }
    return 1;
}

static int same_ignoring_case(const char *left, const char *right, size_t length) {
    size_t index;
    for (index = 0; index < length; index++) {
        if (tolower((unsigned char)left[index]) != tolower((unsigned char)right[index])) {
            return 0;
        }
    }
    return 1;
}

const snapshot_repo *snapshot_find(const repos_snapshot *snapshot, const char *full_name, size_t length) {
    size_t index;
    for (index = 0; index < snapshot->count; index++) {
        const snapshot_repo *repo = &snapshot->repos[index];
        if (strlen(repo->full_name) == length && same_ignoring_case(repo->full_name, full_name, length)) {
            return repo;
        }
    }
    return NULL;
}

int snapshot_same_owner(const snapshot_repo *left, const snapshot_repo *right) {
    return left->owner_length == right->owner_length &&
           same_ignoring_case(left->full_name, right->full_name, left->owner_length);
}

static size_t split_words(const char *line, line_word *words, size_t most) {
    size_t count = 0;
    const char *scan = line;
    while (*scan != '\0') {
        const char *space = strchr(scan, ' ');
        size_t length = space == NULL ? strlen(scan) : (size_t)(space - scan);
        if (length == 0 || count == most) {
            return most + 1;
        }
        words[count].start = scan;
        words[count].length = length;
        count++;
        scan += length;
        if (*scan == ' ') {
            scan++;
            if (*scan == '\0') {
                return most + 1;
            }
        }
    }
    return count;
}

static int word_is(const line_word *word, const char *expected) {
    return word->length == strlen(expected) && memcmp(word->start, expected, word->length) == 0;
}

static int parse_line(const char *line, snapshot_repo *repo) {
    line_word words[3];
    if (split_words(line, words, 3) != 3 || !snapshot_is_repo_name(words[0].start, words[0].length)) {
        return 0;
    }
    if (!word_is(&words[1], "public") && !word_is(&words[1], "private")) {
        return 0;
    }
    if (!word_is(&words[2], "follows") && !word_is(&words[2], "not")) {
        return 0;
    }
    repo->full_name = memory_copy_bytes(words[0].start, words[0].length);
    repo->owner_length = (size_t)((const char *)memchr(words[0].start, '/', words[0].length) - words[0].start);
    repo->visibility = word_is(&words[1], "public") ? VISIBILITY_PUBLIC : VISIBILITY_PRIVATE;
    repo->follows = word_is(&words[2], "follows");
    return 1;
}

static snapshot_status read_lines(const text_document *document, repos_snapshot *out, size_t *bad_line) {
    size_t index;
    out->repos = memory_allocate((document->count + 1) * sizeof *out->repos);
    for (index = 0; index < document->count; index++) {
        snapshot_repo repo;
        *bad_line = document->lines[index].number;
        if (!parse_line(document->lines[index].text, &repo)) {
            return SNAPSHOT_MALFORMED;
        }
        if (snapshot_find(out, repo.full_name, strlen(repo.full_name)) != NULL) {
            free(repo.full_name);
            return SNAPSHOT_DUPLICATE;
        }
        out->repos[out->count++] = repo;
    }
    return SNAPSHOT_READ;
}

snapshot_status snapshot_load(const char *path, repos_snapshot *out, size_t *bad_line) {
    text_document document;
    snapshot_status status;
    out->repos = NULL;
    out->count = 0;
    *bad_line = 0;
    if (text_document_load(path, &document) != 0) {
        return SNAPSHOT_UNREADABLE;
    }
    status = read_lines(&document, out, bad_line);
    text_document_free(&document);
    if (status != SNAPSHOT_READ) {
        snapshot_free(out);
    }
    return status;
}

void snapshot_free(repos_snapshot *snapshot) {
    size_t index;
    for (index = 0; index < snapshot->count; index++) {
        free(snapshot->repos[index].full_name);
    }
    free(snapshot->repos);
    snapshot->repos = NULL;
    snapshot->count = 0;
}
