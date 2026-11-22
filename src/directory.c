#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include "directory.h"

#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

typedef struct {
    directory_entry *entries;
    size_t count;
    size_t capacity;
} entry_builder;

static void builder_push(entry_builder *builder, char *name, int is_directory) {
    if (builder->count == builder->capacity) {
        builder->capacity = builder->capacity == 0 ? 16 : builder->capacity * 2;
        builder->entries = memory_resize(builder->entries, builder->capacity * sizeof *builder->entries);
    }
    builder->entries[builder->count].name = name;
    builder->entries[builder->count].is_directory = is_directory;
    builder->count++;
}

static int is_dot_entry(const char *name) {
    return strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
}

static int compare_entries(const void *left, const void *right) {
    return strcmp(((const directory_entry *)left)->name, ((const directory_entry *)right)->name);
}

static void builder_finish(entry_builder *builder, directory_listing *out) {
    if (builder->count > 1) {
        qsort(builder->entries, builder->count, sizeof *builder->entries, compare_entries);
    }
    out->entries = builder->entries;
    out->count = builder->count;
}

#ifdef _WIN32

static wchar_t *widen(const char *text) {
    int length = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    wchar_t *wide;
    if (length <= 0) {
        return NULL;
    }
    wide = memory_allocate((size_t)length * sizeof *wide);
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wide, length);
    return wide;
}

static char *narrow(const wchar_t *wide) {
    int length = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
    char *text;
    if (length <= 0) {
        return memory_copy_string("");
    }
    text = memory_allocate((size_t)length);
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, text, length, NULL, NULL);
    return text;
}

static int is_real_directory(DWORD attributes) {
    return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0 && (attributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0;
}

int directory_list(const char *path, directory_listing *out) {
    size_t length = strlen(path);
    char *pattern = memory_allocate(length + 3);
    wchar_t *wide_pattern;
    WIN32_FIND_DATAW found;
    HANDLE search;
    entry_builder builder = {NULL, 0, 0};
    memcpy(pattern, path, length);
    memcpy(pattern + length, "/*", 3);
    wide_pattern = widen(pattern);
    free(pattern);
    if (wide_pattern == NULL) {
        return -1;
    }
    search = FindFirstFileW(wide_pattern, &found);
    free(wide_pattern);
    if (search == INVALID_HANDLE_VALUE) {
        return -1;
    }
    do {
        char *name = narrow(found.cFileName);
        if (is_dot_entry(name)) {
            free(name);
        } else {
            builder_push(&builder, name, is_real_directory(found.dwFileAttributes));
        }
    } while (FindNextFileW(search, &found));
    FindClose(search);
    builder_finish(&builder, out);
    return 0;
}

int directory_is_directory(const char *path) {
    wchar_t *wide = widen(path);
    DWORD attributes;
    if (wide == NULL) {
        return 0;
    }
    attributes = GetFileAttributesW(wide);
    free(wide);
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

static FILE *open_for_reading(const char *path) {
    wchar_t *wide = widen(path);
    FILE *file;
    if (wide == NULL) {
        return NULL;
    }
    file = _wfopen(wide, L"rb");
    free(wide);
    return file;
}

int directory_make(const char *path) {
    wchar_t *wide = widen(path);
    int made;
    if (wide == NULL) {
        return -1;
    }
    made = CreateDirectoryW(wide, NULL) != 0;
    free(wide);
    return made ? 0 : -1;
}

static FILE *open_new_file(const char *path) {
    wchar_t *wide = widen(path);
    FILE *file;
    if (wide == NULL) {
        return NULL;
    }
    file = _wfopen(wide, L"wbx");
    free(wide);
    return file;
}

#else

static char *join_with_slash(const char *folder, const char *name) {
    size_t folder_length = strlen(folder);
    size_t name_length = strlen(name);
    char *path = memory_allocate(folder_length + name_length + 2);
    memcpy(path, folder, folder_length);
    path[folder_length] = '/';
    memcpy(path + folder_length + 1, name, name_length + 1);
    return path;
}

static int is_real_directory(const char *path) {
    struct stat status;
    return lstat(path, &status) == 0 && S_ISDIR(status.st_mode);
}

int directory_list(const char *path, directory_listing *out) {
    DIR *folder = opendir(path);
    struct dirent *item;
    entry_builder builder = {NULL, 0, 0};
    if (folder == NULL) {
        return -1;
    }
    while ((item = readdir(folder)) != NULL) {
        char *full_path;
        if (is_dot_entry(item->d_name)) {
            continue;
        }
        full_path = join_with_slash(path, item->d_name);
        builder_push(&builder, memory_copy_string(item->d_name), is_real_directory(full_path));
        free(full_path);
    }
    closedir(folder);
    builder_finish(&builder, out);
    return 0;
}

int directory_is_directory(const char *path) {
    struct stat status;
    return stat(path, &status) == 0 && S_ISDIR(status.st_mode);
}

static FILE *open_for_reading(const char *path) {
    return fopen(path, "rb");
}

int directory_make(const char *path) {
    return mkdir(path, 0777);
}

static FILE *open_new_file(const char *path) {
    return fopen(path, "wbx");
}

#endif

void directory_listing_free(directory_listing *listing) {
    size_t index;
    for (index = 0; index < listing->count; index++) {
        free(listing->entries[index].name);
    }
    free(listing->entries);
    listing->entries = NULL;
    listing->count = 0;
}

const directory_entry *directory_find(const directory_listing *listing, const char *name) {
    size_t index;
    for (index = 0; index < listing->count; index++) {
        if (strcmp(listing->entries[index].name, name) == 0) {
            return &listing->entries[index];
        }
    }
    return NULL;
}

int directory_read_file(const char *path, char **content, size_t *length) {
    FILE *file = open_for_reading(path);
    size_t capacity = 4096;
    size_t used = 0;
    char *buffer;
    if (file == NULL) {
        return -1;
    }
    buffer = memory_allocate(capacity + 1);
    for (;;) {
        size_t read = fread(buffer + used, 1, capacity - used, file);
        used += read;
        if (used < capacity) {
            break;
        }
        capacity *= 2;
        buffer = memory_resize(buffer, capacity + 1);
    }
    if (ferror(file)) {
        fclose(file);
        free(buffer);
        return -1;
    }
    fclose(file);
    buffer[used] = '\0';
    *content = buffer;
    *length = used;
    return 0;
}

int directory_create_file(const char *path, const char *content, size_t length) {
    FILE *file = open_new_file(path);
    int written;
    if (file == NULL) {
        return -1;
    }
    written = fwrite(content, 1, length, file) == length;
    if (fclose(file) != 0) {
        written = 0;
    }
    return written ? 0 : -1;
}
