#ifndef TARN_STRING_LIST_H
#define TARN_STRING_LIST_H

#include <stddef.h>

typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} string_list;

void string_list_init(string_list *list);
void string_list_push(string_list *list, const char *text);
int string_list_contains(const string_list *list, const char *text);
void string_list_sort(string_list *list);
void string_list_free(string_list *list);

#endif
