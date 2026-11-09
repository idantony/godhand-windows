#ifndef GH_CORE_BUNDLE_H
#define GH_CORE_BUNDLE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_BUNDLE_OK,GH_BUNDLE_ARGUMENT,GH_BUNDLE_BOUNDS,GH_BUNDLE_UNSUPPORTED,GH_BUNDLE_NOT_FOUND} gh_bundle_error;
typedef struct {const unsigned char *bytes;size_t size;uint32_t count;} gh_bundle;
typedef struct {const unsigned char *bytes;size_t offset,size;char tag[4];} gh_bundle_entry;
/* Immutable host views; lengths are bounded extents to next offset, not
 * asserted semantic payload sizes. No offset32 word is overwritten. */
gh_bundle_error gh_bundle_open(const void *,size_t,gh_bundle *);
gh_bundle_error gh_bundle_at(const gh_bundle *,uint32_t,gh_bundle_entry *);
gh_bundle_error gh_bundle_find(const gh_bundle *,const char *tag,uint32_t occurrence,gh_bundle_entry *);
#ifdef __cplusplus
}
#endif
#endif
