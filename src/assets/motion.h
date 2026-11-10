#ifndef GH_MOTION_H
#define GH_MOTION_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_MOTION_OK,GH_MOTION_ARGUMENT,GH_MOTION_BOUNDS,GH_MOTION_FORMAT,GH_MOTION_UNSUPPORTED,GH_MOTION_NUMERIC} gh_motion_status;
typedef struct {const uint8_t *bytes;size_t size,offset;uint16_t frames;uint8_t count,flags;} gh_motion_view;
typedef struct {uint32_t packed,control,data;uint16_t keys;uint8_t part,channel,method;} gh_motion_track;
/* Native equivalent of one1599B0/159DA0 cache: offsets replace guest pointers.
 * Endpoint/tangent values follow original decoding, not bind-pose data. */
typedef struct {size_t first_key,next_key;uint16_t first_frame,next_frame;float first_value,next_value,out_tangent,in_tangent;uint8_t method;} gh_motion_cache;
gh_motion_status gh_motion_open_mtb3(const void *,size_t,size_t,gh_motion_view *);
gh_motion_status gh_motion_track_at(const gh_motion_view *,unsigned,gh_motion_track *);
/* Actual158CC8/158CD0/158FC8/159200/159300 bounded nearest-binary32
 * reference. Restores caller FEnv; not a claim of EE ChopZero equivalence.
 * Original methods2/6 really return0; unsupported dispatches are rejected. */
gh_motion_status gh_motion_sample_reference(const gh_motion_view *,unsigned,float,float *,gh_motion_cache *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
