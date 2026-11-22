#include "readme.h"

#include "memory.h"
#include "path.h"

#include <stdlib.h>
#include <string.h>

static const char form_opening[] = "a README opens with one paragraph";
static const char form_child_line[] = "a child line reads - [<name>](<name>) - <one line>";
static const char form_names_child[] = "a child line's text and target both name the child";
static const char form_after_paragraph[] = "only child lines follow the opening paragraph";

typedef enum { BEFORE_PARAGRAPH, IN_PARAGRAPH, AFTER_PARAGRAPH } form_state;

typedef struct {
    char *name;
    char *target;
} child_line;

typedef struct {
    const char *name;
    int is_folder;
    int is_source_file;
} child_wanted;

void readme_children_add(readme_children *children, const char *name, int is_folder, int is_source_file) {
    readme_child *child;
    children->items = memory_resize(children->items, (children->count + 1) * sizeof *children->items);
    child = &children->items[children->count++];
    child->name = memory_copy_string(name);
    child->is_folder = is_folder;
    child->is_source_file = is_source_file;
    child->listed = 0;
}

void readme_children_free(readme_children *children) {
    size_t index;
    for (index = 0; index < children->count; index++) {
        free(children->items[index].name);
    }
    free(children->items);
    children->items = NULL;
    children->count = 0;
}

static void refuse(const check_context *context, const readme_place *place, size_t line, const char *rule,
                   const char *text) {
    report_add(context->out, SEVERITY_REFUSED, place->readme_path, line, rule, "%s", text);
}

static const char *slash_for(int is_folder) {
    return is_folder ? "/" : "";
}

static const char *folder_display(const char *folder) {
    return folder[0] == '\0' ? "the repo root" : folder;
}

static int parse_child_line(const char *line, child_line *out) {
    const char *name_start = line + 3;
    const char *name_end;
    const char *target_start;
    const char *target_end;
    const char *description;
    if (strncmp(line, "- [", 3) != 0) {
        return 0;
    }
    name_end = strchr(name_start, ']');
    if (name_end == NULL || name_end == name_start || name_end[1] != '(') {
        return 0;
    }
    target_start = name_end + 2;
    target_end = strchr(target_start, ')');
    if (target_end == NULL || target_end == target_start ||
        memchr(target_start, ' ', (size_t)(target_end - target_start)) != NULL) {
        return 0;
    }
    description = target_end + 1;
    if (strncmp(description, " - ", 3) != 0 || description[3] == '\0' || description[3] == ' ') {
        return 0;
    }
    out->name = memory_copy_bytes(name_start, (size_t)(name_end - name_start));
    out->target = memory_copy_bytes(target_start, (size_t)(target_end - target_start));
    return 1;
}

static readme_child *find_child(readme_children *children, const child_wanted *wanted, int ignoring_case) {
    size_t index;
    for (index = 0; index < children->count; index++) {
        readme_child *child = &children->items[index];
        int same_name = ignoring_case ? text_equal_ignoring_case(child->name, wanted->name)
                                      : strcmp(child->name, wanted->name) == 0;
        if (same_name && child->is_folder == wanted->is_folder && child->is_source_file == wanted->is_source_file) {
            return child;
        }
    }
    return NULL;
}

static void match_child(const check_context *context, const readme_place *place, size_t line,
                        readme_children *children, const child_wanted *wanted) {
    readme_child *child = find_child(children, wanted, 0);
    const char *slash = slash_for(wanted->is_folder);
    if (child != NULL) {
        child->listed = 1;
        return;
    }
    child = find_child(children, wanted, 1);
    if (child != NULL) {
        child->listed = 1;
        report_add(context->out, SEVERITY_REFUSED, place->readme_path, line, "readme-case-mismatch",
                   "lists %s%s, but the child is %s%s", wanted->name, slash, child->name, slash);
        return;
    }
    report_add(context->out, SEVERITY_REFUSED, place->readme_path, line, "readme-names-gone-child",
               "lists %s%s, which does not exist", wanted->name, slash);
}

static void check_code_file_line(const check_context *context, const readme_place *place, size_t line,
                                 readme_children *children, const child_line *child) {
    char *joined = path_join(place->folder, child->target);
    char *resolved = path_normalize(joined);
    char *parent = resolved == NULL ? NULL : path_parent(resolved);
    if (parent == NULL || strcmp(parent, place->source_folder) != 0) {
        report_add(context->out, SEVERITY_REFUSED, place->readme_path, line, "readme-form",
                   "a code-file line links a file directly in %s", folder_display(place->source_folder));
    } else if (strcmp(path_base_name(resolved), child->name) != 0) {
        refuse(context, place, line, "readme-form", "a code-file line's text is the file's base name");
    } else {
        child_wanted wanted = {child->name, 0, 1};
        match_child(context, place, line, children, &wanted);
    }
    free(parent);
    free(resolved);
    free(joined);
}

static void check_child_line(const check_context *context, const readme_place *place, size_t line,
                             readme_children *children, child_line *child) {
    size_t name_length = strlen(child->name);
    int is_folder = child->name[name_length - 1] == '/';
    if (strcmp(child->name, child->target) != 0) {
        if (place->source_folder != NULL && !is_folder) {
            check_code_file_line(context, place, line, children, child);
        } else {
            refuse(context, place, line, "readme-form", form_names_child);
        }
        return;
    }
    if (is_folder) {
        child->name[name_length - 1] = '\0';
    }
    if (child->name[0] == '\0' || strchr(child->name, '/') != NULL) {
        refuse(context, place, line, "readme-form", form_names_child);
        return;
    }
    {
        child_wanted wanted = {child->name, is_folder, 0};
        match_child(context, place, line, children, &wanted);
    }
}

static void read_child_line(const check_context *context, const readme_place *place, const text_line *line,
                            readme_children *children) {
    child_line child;
    if (!parse_child_line(line->text, &child)) {
        refuse(context, place, line->number, "readme-form", form_child_line);
        return;
    }
    check_child_line(context, place, line->number, children, &child);
    free(child.name);
    free(child.target);
}

static int is_child_line_candidate(const char *line) {
    return strncmp(line, "- ", 2) == 0;
}

static form_state read_line(const check_context *context, const readme_place *place, const text_line *line,
                            form_state state, readme_children *children, unsigned char *is_child_line) {
    if (text_is_blank(line->text)) {
        return state == IN_PARAGRAPH ? AFTER_PARAGRAPH : state;
    }
    if (text_is_generated_marker(line->text) || is_child_line_candidate(line->text)) {
        if (state == BEFORE_PARAGRAPH) {
            refuse(context, place, line->number, "readme-form", form_opening);
        }
        if (is_child_line_candidate(line->text)) {
            *is_child_line = 1;
            read_child_line(context, place, line, children);
        }
        return AFTER_PARAGRAPH;
    }
    if (state == AFTER_PARAGRAPH) {
        refuse(context, place, line->number, "readme-form", form_after_paragraph);
        return AFTER_PARAGRAPH;
    }
    return IN_PARAGRAPH;
}

static void report_unlisted(const check_context *context, const readme_place *place,
                            const readme_children *children) {
    size_t index;
    for (index = 0; index < children->count; index++) {
        const readme_child *child = &children->items[index];
        if (!child->listed) {
            report_add(context->out, SEVERITY_REFUSED, place->readme_path, 0, "readme-misses-child",
                       "does not list %s%s", child->name, slash_for(child->is_folder));
        }
    }
}

void readme_check(const check_context *context, const readme_place *place, const text_document *document,
                  readme_children *children, unsigned char *child_lines) {
    unsigned char *fenced = text_fenced_lines(document);
    form_state state = BEFORE_PARAGRAPH;
    size_t index = 0;
    memset(child_lines, 0, document->count);
    if (document->count > 0 && text_is_size_exception(document->lines[0].text)) {
        index = 1;
    }
    for (; index < document->count; index++) {
        if (!fenced[index]) {
            state = read_line(context, place, &document->lines[index], state, children, &child_lines[index]);
        }
    }
    if (state == BEFORE_PARAGRAPH) {
        refuse(context, place, 0, "readme-form", form_opening);
    }
    report_unlisted(context, place, children);
    free(fenced);
}
