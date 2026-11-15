#ifndef GODHAND_TEST_SCRATCH_H
#define GODHAND_TEST_SCRATCH_H

#include <windows.h>
#include <stdio.h>

/* The native builder supplies TEMP and TMP under its selected scratch root. */
enum { GH_TEST_SCRATCH_PATH_BYTES = MAX_PATH * 4 };

static inline int gh_test_scratch_directory(const char *path, int create)
{
    wchar_t wide[MAX_PATH];
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, MAX_PATH))
        return 0;
    if (create)
        return CreateDirectoryW(wide, NULL) != 0;
    return RemoveDirectoryW(wide) != 0;
}

static inline int gh_test_scratch_child(char path[GH_TEST_SCRATCH_PATH_BYTES],
                                      const char *directory, const char *name)
{
    int length = snprintf(path, GH_TEST_SCRATCH_PATH_BYTES, "%s/%s", directory, name);
    return length > 0 && length < GH_TEST_SCRATCH_PATH_BYTES;
}

static inline int gh_test_scratch_open(char directory[GH_TEST_SCRATCH_PATH_BYTES],
                                     char absent[GH_TEST_SCRATCH_PATH_BYTES], const char *name)
{
    wchar_t temp[MAX_PATH];
    DWORD length = GetTempPathW(MAX_PATH, temp);
    char utf8[GH_TEST_SCRATCH_PATH_BYTES];
    if (!length || length >= MAX_PATH ||
        !WideCharToMultiByte(CP_UTF8, 0, temp, -1, utf8, sizeof(utf8), NULL, NULL))
        return 0;
    int bytes = snprintf(directory, GH_TEST_SCRATCH_PATH_BYTES, "%sgh-test-%lu-%llu", utf8,
                         (unsigned long)GetCurrentProcessId(), (unsigned long long)GetTickCount64());
    if (bytes <= 0 || bytes >= GH_TEST_SCRATCH_PATH_BYTES ||
        !gh_test_scratch_child(absent, directory, name))
        return 0;
    return gh_test_scratch_directory(directory, 1);
}

#endif
