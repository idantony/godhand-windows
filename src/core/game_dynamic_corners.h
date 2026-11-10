#ifndef GH_GAME_DYNAMIC_CORNERS_H
#define GH_GAME_DYNAMIC_CORNERS_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_dynamic_corner_tables {uint32_t quad[4][4],box[8][4],quad_indices[4],box_faces[6][4];const void *identity;uint8_t loaded,initialized;} gh_dynamic_corner_tables;
typedef enum {GH_DYNAMIC_CORNERS_OK,GH_DYNAMIC_CORNERS_ARGUMENT,GH_DYNAMIC_CORNERS_BOUNDS,GH_DYNAMIC_CORNERS_FORMAT,GH_DYNAMIC_CORNERS_BUSY} gh_dynamic_corner_status;
/* ONE original462F00/462F40 staticowner. Load actualPT_LOAD initial bytes;
 * verifies actual133670/133968 code profile and3FC88C constructor-list slot. */
gh_dynamic_corner_status gh_dynamic_corners_from_elf(gh_dynamic_corner_tables *,const void *,size_t);
/* Exact133670 guards: priority==0x0000FFFF and initialize!=0. Repeating a
 * matching call rewrites constants like source; no invented once-success. */
gh_dynamic_corner_status gh_dynamic_corners_initialize_00133670(gh_dynamic_corner_tables *,int initialize,uint32_t priority);
#ifdef __cplusplus
}
#endif
#endif
