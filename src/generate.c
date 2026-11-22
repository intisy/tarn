#include "generate.h"

#include "directory.h"
#include "memory.h"
#include "org_repos.h"
#include "path.h"
#include "rules.h"
#include "source_children.h"
#include "text.h"
#include "walk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { KIND_SOURCE_CHILDREN, KIND_ORG_REPOS, KIND_UNKNOWN } block_kind;

typedef struct {
    size_t first;
    size_t last;
    char *what;
} generated_block;

typedef struct {
    generated_block *items;
    size_t count;
} generated_blocks;

typedef struct {
    size_t start;
    size_t end;
    size_t next;
} line_span;

typedef struct {
    char *data;
    size_t length;
} byte_buffer;

typedef struct {
    const check_context *context;
    const char *path;
    const char *mirrored;
    int write;
    char *raw;
    size_t length;
    text_document document;
    line_span *spans;
} generated_file;

typedef struct {
    int is_open;
    size_t first;
    char *what;
    int well_formed;
} scan_state;

typedef struct {
    byte_buffer out;
    size_t copied;
    long shift;
    int changed;
} rewrite_state;

static const char bom_bytes[] = "\xEF\xBB\xBF";

static void buffer_append(byte_buffer *buffer, const char *bytes, size_t count) {
    buffer->data = memory_resize(buffer->data, buffer->length + count + 1);
    memcpy(buffer->data + buffer->length, bytes, count);
    buffer->length += count;
}

static void compute_spans(generated_file *file) {
    size_t start = (file->length >= 3 && memcmp(file->raw, bom_bytes, 3) == 0) ? 3 : 0;
    size_t line = 0;
    size_t index;
    file->spans = memory_allocate(file->document.count * sizeof *file->spans);
    for (index = start; line < file->document.count; index++) {
        if (index == file->length || file->raw[index] == '\n') {
            file->spans[line].start = start;
            file->spans[line].end = index;
            file->spans[line].next = index < file->length ? index + 1 : file->length;
            line++;
            start = index + 1;
        }
    }
}

static const char *line_ending(const generated_file *file, size_t line) {
    const line_span *span = &file->spans[line];
    return span->end > span->start && file->raw[span->end - 1] == '\r' ? "\r\n" : "\n";
}

static block_kind kind_of(const char *what) {
    if (strcmp(what, "source-children") == 0) {
        return KIND_SOURCE_CHILDREN;
    }
    return strcmp(what, "org-repos") == 0 ? KIND_ORG_REPOS : KIND_UNKNOWN;
}

static void add_block(generated_blocks *blocks, size_t first, size_t last, char *what) {
    blocks->items = memory_resize(blocks->items, (blocks->count + 1) * sizeof *blocks->items);
    blocks->items[blocks->count].first = first;
    blocks->items[blocks->count].last = last;
    blocks->items[blocks->count].what = what;
    blocks->count++;
}

static void free_blocks(generated_blocks *blocks) {
    size_t index;
    for (index = 0; index < blocks->count; index++) {
        free(blocks->items[index].what);
    }
    free(blocks->items);
}

static const generated_block *find_block_of_kind(const generated_blocks *blocks, const char *what) {
    size_t index;
    for (index = 0; index < blocks->count; index++) {
        if (strcmp(blocks->items[index].what, what) == 0) {
            return &blocks->items[index];
        }
    }
    return NULL;
}

static void refuse_duplicate(const generated_file *file, const text_line *line, const generated_block *first,
                             const char *what) {
    report_add(file->context->out, SEVERITY_REFUSED, file->path, line->number, "generated-block-duplicate",
               "a second %s block in one file duplicates the first, which started at line %zu", what,
               file->document.lines[first->first].number);
}

static void scan_line(const generated_file *file, size_t index, scan_state *state, generated_blocks *blocks) {
    const text_line *line = &file->document.lines[index];
    char *what = text_generated_what(line->text);
    report *out = file->context->out;
    if (what != NULL && state->is_open) {
        report_add(out, SEVERITY_REFUSED, file->path, line->number, "generated-nested",
                   "a block starts inside the block that started at line %zu",
                   file->document.lines[state->first].number);
        state->well_formed = 0;
        free(what);
    } else if (what != NULL) {
        const generated_block *first = find_block_of_kind(blocks, what);
        if (first != NULL) {
            refuse_duplicate(file, line, first, what);
            state->well_formed = 0;
        }
        state->is_open = 1;
        state->first = index;
        state->what = what;
    } else if (text_is_generated_end(line->text) && !state->is_open) {
        report_add(out, SEVERITY_REFUSED, file->path, line->number, "generated-marker-unmatched",
                   "an end marker has no start marker");
        state->well_formed = 0;
    } else if (text_is_generated_end(line->text)) {
        add_block(blocks, state->first, index, state->what);
        state->is_open = 0;
    }
}

static int find_blocks(const generated_file *file, const unsigned char *fenced, generated_blocks *blocks) {
    scan_state state = {0, 0, NULL, 1};
    size_t index;
    for (index = 0; index < file->document.count; index++) {
        if (!fenced[index]) {
            scan_line(file, index, &state, blocks);
        }
    }
    if (state.is_open) {
        report_add(file->context->out, SEVERITY_REFUSED, file->path, file->document.lines[state.first].number,
                   "generated-unterminated", "the block has no end marker");
        free(state.what);
        state.well_formed = 0;
    }
    return state.well_formed;
}

static void copy_lines(const string_list *from, string_list *to) {
    size_t index;
    for (index = 0; index < from->count; index++) {
        string_list_push(to, from->items[index]);
    }
}

static void copy_body(const generated_file *file, const generated_block *block, string_list *body) {
    size_t index;
    for (index = block->first + 1; index < block->last; index++) {
        string_list_push(body, file->document.lines[index].text);
    }
}

static int same_lines(const string_list *left, const string_list *right) {
    size_t index;
    if (left->count != right->count) {
        return 0;
    }
    for (index = 0; index < left->count; index++) {
        if (strcmp(left->items[index], right->items[index]) != 0) {
            return 0;
        }
    }
    return 1;
}

static int is_org_root_readme(const generated_file *file) {
    return file->context->kind == TREE_ORG && strcmp(file->path, "README.md") == 0;
}

static int accept_kind(const generated_file *file, const generated_block *block, block_kind kind) {
    size_t line = file->document.lines[block->first].number;
    if (kind == KIND_UNKNOWN) {
        report_add(file->context->out, SEVERITY_REFUSED, file->path, line, "generated-kind-unknown",
                   "%s is not a generated kind of version 1", block->what);
        return 0;
    }
    if (kind == KIND_SOURCE_CHILDREN && file->mirrored == NULL) {
        report_add(file->context->out, SEVERITY_REFUSED, file->path, line, "generated-kind-misplaced",
                   "source-children belongs in the README of a folder under docs/source");
        return 0;
    }
    if (kind == KIND_ORG_REPOS && !is_org_root_readme(file)) {
        report_add(file->context->out, SEVERITY_REFUSED, file->path, line, "generated-kind-misplaced",
                   "org-repos belongs in the root README of an org docs tree");
        return 0;
    }
    return 1;
}

/** Builds the block's lines into expected; returns 0 when this run has nothing to build them from. */
static int generate_lines(const generated_file *file, block_kind kind, const string_list *current,
                          string_list *expected) {
    if (kind == KIND_SOURCE_CHILDREN) {
        source_children_lines(file->context, file->mirrored, current, expected);
        return 1;
    }
    if (file->context->snapshot == NULL) {
        return 0;
    }
    org_repos_lines(file->context, current, expected);
    return 1;
}

/** Leaves the lines the block holds afterwards in final_lines, and sets rewrite when they replace the old ones. */
static void resolve_body(const generated_file *file, const generated_block *block, block_kind kind,
                         const string_list *current, string_list *final_lines, int *rewrite) {
    string_list expected;
    int stale = 0;
    string_list_init(&expected);
    if (generate_lines(file, kind, current, &expected)) {
        stale = !same_lines(current, &expected);
    }
    *rewrite = stale && file->write;
    copy_lines(*rewrite ? &expected : current, final_lines);
    string_list_free(&expected);
    if (stale && !file->write) {
        report_add(file->context->out, SEVERITY_REFUSED, file->path, file->document.lines[block->first].number,
                   "generated-block-stale",
                   "the block differs from the text tarn-check generates, and --write rewrites it");
    }
}

static size_t final_line_number(const generated_file *file, const generated_block *block, long shift, size_t index) {
    return (size_t)((long)file->document.lines[block->first].number + shift) + 1 + index;
}

static void refuse_org_repos_forms(const generated_file *file, const generated_block *block, long shift,
                                   const string_list *lines) {
    size_t index;
    for (index = 0; index < lines->count; index++) {
        if (!org_repos_is_line(lines->items[index])) {
            report_add(file->context->out, SEVERITY_REFUSED, file->path, final_line_number(file, block, shift, index),
                       "generated-line-form",
                       "an org-repos line reads - [<owner>/<name>](https://github.com/<owner>/<name>) - <text>");
        }
    }
}

static void warn_unwritten(const generated_file *file, const generated_block *block, block_kind kind, long shift,
                           const string_list *lines) {
    size_t index;
    for (index = 0; index < lines->count; index++) {
        const char *line = lines->items[index];
        int unwritten = kind == KIND_ORG_REPOS ? org_repos_is_unwritten(line) : source_children_is_unwritten(line);
        if (unwritten) {
            report_add(file->context->out, SEVERITY_WARNED, file->path, final_line_number(file, block, shift, index),
                       "prose-to-write", "the text to write is a placeholder for a person to replace");
        }
    }
}

static void append_replacement(const generated_file *file, const generated_block *block, const string_list *lines,
                               rewrite_state *state) {
    const char *ending = line_ending(file, block->first);
    size_t ending_length = strlen(ending);
    size_t index;
    buffer_append(&state->out, file->raw + state->copied, file->spans[block->first].next - state->copied);
    for (index = 0; index < lines->count; index++) {
        buffer_append(&state->out, lines->items[index], strlen(lines->items[index]));
        buffer_append(&state->out, ending, ending_length);
    }
    state->copied = file->spans[block->last].start;
}

static void process_block(const generated_file *file, const generated_block *block, rewrite_state *state) {
    block_kind kind = kind_of(block->what);
    string_list current;
    string_list final_lines;
    int rewrite;
    if (!accept_kind(file, block, kind)) {
        return;
    }
    string_list_init(&current);
    string_list_init(&final_lines);
    copy_body(file, block, &current);
    resolve_body(file, block, kind, &current, &final_lines, &rewrite);
    if (kind == KIND_ORG_REPOS) {
        refuse_org_repos_forms(file, block, state->shift, &final_lines);
    }
    warn_unwritten(file, block, kind, state->shift, &final_lines);
    if (rewrite) {
        append_replacement(file, block, &final_lines, state);
        state->shift += (long)final_lines.count - (long)current.count;
        state->changed = 1;
    }
    string_list_free(&current);
    string_list_free(&final_lines);
}

static int write_file(const generated_file *file, const byte_buffer *content) {
    char *on_disk = path_on_disk(file->context->repo_root, file->path);
    FILE *stream = fopen(on_disk, "wb");
    int written = stream != NULL && fwrite(content->data, 1, content->length, stream) == content->length;
    if (stream != NULL && fclose(stream) != 0) {
        written = 0;
    }
    free(on_disk);
    return written;
}

static void rewrite_blocks(const generated_file *file, const generated_blocks *blocks) {
    rewrite_state state = {{NULL, 0}, 0, 0, 0};
    size_t index;
    for (index = 0; index < blocks->count; index++) {
        process_block(file, &blocks->items[index], &state);
    }
    if (state.changed) {
        buffer_append(&state.out, file->raw + state.copied, file->length - state.copied);
        if (!write_file(file, &state.out)) {
            report_add(file->context->out, SEVERITY_REFUSED, file->path, 0, "file-unwritable",
                       "the file cannot be written");
        }
    }
    free(state.out.data);
}

static void generate_loaded_file(generated_file *file) {
    generated_blocks blocks = {NULL, 0};
    unsigned char *fenced = text_fenced_lines(&file->document);
    compute_spans(file);
    if (find_blocks(file, fenced, &blocks)) {
        rewrite_blocks(file, &blocks);
    }
    free_blocks(&blocks);
    free(fenced);
    free(file->spans);
}

static void generate_file(const check_context *context, const char *path, const char *mirrored, int write) {
    generated_file file;
    char *on_disk = path_on_disk(context->repo_root, path);
    int unreadable = directory_read_file(on_disk, &file.raw, &file.length) != 0;
    free(on_disk);
    if (unreadable) {
        return;
    }
    file.context = context;
    file.path = path;
    file.mirrored = mirrored;
    file.write = write;
    text_document_from_bytes(memory_copy_bytes(file.raw, file.length), file.length, &file.document);
    generate_loaded_file(&file);
    text_document_free(&file.document);
    free(file.raw);
}

static void generate_folder(const check_context *context, const char *folder, int write) {
    const char *mirrored = walk_mirrored_folder(context, folder);
    char *on_disk;
    directory_listing listing;
    size_t index;
    if (mirrored != NULL && !string_list_contains(&context->code_folders, mirrored)) {
        return;
    }
    on_disk = path_on_disk(context->repo_root, folder);
    if (directory_list(on_disk, &listing) == 0) {
        for (index = 0; index < listing.count; index++) {
            const directory_entry *entry = &listing.entries[index];
            entry_kind kind = rules_classify(context, folder, entry);
            char *path = path_join(folder, entry->name);
            if (kind == ENTRY_README) {
                generate_file(context, path, mirrored, write);
            } else if (kind == ENTRY_PROSE) {
                generate_file(context, path, NULL, write);
            } else if (kind == ENTRY_FOLDER) {
                generate_folder(context, path, write);
            }
            free(path);
        }
        directory_listing_free(&listing);
    }
    free(on_disk);
}

void generate_blocks(const check_context *context, int write) {
    generate_folder(context, context->descent_root, write);
}
