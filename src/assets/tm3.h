#ifndef GH_TM3_H
#define GH_TM3_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_TM3_OK,GH_TM3_ARGUMENT,GH_TM3_BOUNDS,GH_TM3_UNSUPPORTED,GH_TM3_CAPACITY} gh_tm3_error;
typedef struct {const uint8_t *bytes;size_t size;uint32_t count;} gh_tm3_bundle;
typedef struct {const uint8_t*source,*image,*clut;size_t source_bytes,image_bytes,clut_bytes;
 uint16_t width,height;uint64_t tex0;uint8_t transfer_kind;} gh_tm3_picture;
gh_tm3_error gh_tm3_open(const void *,size_t,gh_tm3_bundle *);
/* Actual1522C0 zero-based selector; originalcaller supplies an8-bittexNo. */
gh_tm3_error gh_tm3_member(const gh_tm3_bundle *,unsigned,gh_tm3_picture *);
/* Original151990 directPSMT4 or151CC0/151168 CT32-uploadedPSMT4.
 * Output alpha stays in raw GS byte domain (not normalized/saturated QA alpha).
 * Supports actual R100 single-mipRGBA32CLUT16 profiles; unknowns reject. */
gh_tm3_error gh_tm3_rgba_gs(const gh_tm3_picture *,void *,size_t);
#ifdef __cplusplus
}
#endif
#endif
