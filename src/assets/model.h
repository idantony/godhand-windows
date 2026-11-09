#ifndef GH_MODEL_H
#define GH_MODEL_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_MODEL_OK,GH_MODEL_ARGUMENT,GH_MODEL_BOUNDS,GH_MODEL_UNSUPPORTED} gh_model_error;
typedef struct {const unsigned char *bytes;size_t size;uint16_t mesh_count;} gh_model_view;
typedef struct {size_t descriptor_offset,data_offset;char name[9];uint32_t flags;uint16_t data_num;uint8_t layer,alpha;} gh_model_mesh;
/* Read-only original SCR script view; relative offsets resolved with signed
 * arithmetic and retained as offsets, never converted to guest pointers. */
gh_model_error gh_model_open_scr(const void *,size_t,gh_model_view *);
gh_model_error gh_model_mesh_at(const gh_model_view *,size_t,gh_model_mesh *);
#ifdef __cplusplus
}
#endif
#endif
