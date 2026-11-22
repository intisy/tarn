#include "string_list.h"

#include "memory.h"

#include <stdlib.h>
#include <string.h>

void string_list_init(string_list *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void string_list_push(string_list *list, const char *text) {
    if (list->count == list->capacity) {
        list->capacity = list->capacity == 0 ? 8 : list->capacity * 2;
        list->items = memory_resize(list->items, list->capacity * sizeof *list->items);
    }
    list->items[list->count++] = memory_copy_string(text);
}

int string_list_contains(const string_list *list, const char *text) {
    size_t index;
    for (index = 0; index < list->count; index++) {
        if (strcmp(list->items[index], text) == 0) {
            return 1;
        }
    }
    return 0;
}

static int compare_strings(const void *left, const void *right) {
    return strcmp(*(char *const *)left, *(char *const *)right);
}

void string_list_sort(string_list *list) {
    if (list->count > 1) {
        qsort(list->items, list->count, sizeof *list->items, compare_strings);
    }
}

void string_list_free(string_list *list) {
    size_t index;
    for (index = 0; index < list->count; index++) {
        free(list->items[index]);
    }
    free(list->items);
    string_list_init(list);
}
