#ifndef GH_WINDOWS_UTF8_ARGUMENT_H
#define GH_WINDOWS_UTF8_ARGUMENT_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdlib.h>

/* The caller owns the returned UTF-8 string. */
static inline char *gh_windows_utf8_argument(const wchar_t *argument)
{
    int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, argument, -1, NULL, 0, NULL, NULL);
    if (!bytes)
        return NULL;
    char *utf8 = (char *)malloc((size_t)bytes);
    if (!utf8)
        return NULL;
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, argument, -1, utf8, bytes, NULL, NULL)) {
        free(utf8);
        return NULL;
    }
    return utf8;
}

#endif
