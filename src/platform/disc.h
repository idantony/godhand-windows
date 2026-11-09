#ifndef GH_DISC_H
#define GH_DISC_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    GH_DISC_OK = 0, GH_DISC_ARGUMENT, GH_DISC_IO, GH_DISC_FORMAT,
    GH_DISC_BOUNDS, GH_DISC_NOT_FOUND, GH_DISC_MEMORY, GH_DISC_UNSAFE_PATH
} gh_disc_error;
typedef struct gh_disc gh_disc;
typedef struct {
    const char *path; /* Valid until close. Original case and '/' separators. */
    uint32_t afs_index;
    uint64_t iso_offset, size;
} gh_disc_entry;
/* All file access is read-only. One handle must not be used concurrently.
 * UTF-8 ISO filenames are accepted on Windows. No asset is extracted. */
gh_disc_error gh_disc_open(const char *iso_path, gh_disc **out);
void gh_disc_close(gh_disc *disc);
size_t gh_disc_count(const gh_disc *disc);
gh_disc_error gh_disc_entry_at(const gh_disc *disc, size_t index, gh_disc_entry *out);
/* Case-insensitive resource lookup; '/' or '\\' separator. Rejects absolute,
 * empty, '.' and '..' components. Index is an iteration index, not AFS slot. */
gh_disc_error gh_disc_find(const gh_disc *disc, const char *path, size_t *index);
gh_disc_error gh_disc_read(gh_disc *disc, size_t index, uint64_t offset,
                           void *buffer, size_t bytes);
const char *gh_disc_error_string(gh_disc_error error);
#ifdef __cplusplus
}
#endif
#endif
