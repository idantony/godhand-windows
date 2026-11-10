#ifndef GH_GAME_COLLISION_PAIRS_H
#define GH_GAME_COLLISION_PAIRS_H
#include "../platform/memory.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define GH_COLLISION_RECORD_BYTES 0x270u
#define GH_COLLISION_CAPACITY 400u
/* Canonical native owner for original005E8040. Record bytes retain original
 * stride but are NOT a hostpointer-bearing guest array. Consumers must recover
 * any embedded guest pointer fields before native use. */
struct gh_collision_pair_shapes;
typedef struct {gh_r207_allocation records;gh_heap *heap;uint32_t count,used;
 uint32_t pair_masks[32];uint8_t constructed;
 struct gh_collision_pair_shapes *native_shapes;uint32_t shape_serial;} gh_collision_pairs;
typedef enum {GH_COLLISION_OK,GH_COLLISION_ARGUMENT,GH_COLLISION_BUSY,GH_COLLISION_MEMORY} gh_collision_status;
/* Actual2B8AE0->2BA178. Matrix initialzero comes from originalstaticBSS,
 * not a clear performed by every2B8B50 invocation. Call once pernativeowner. */
gh_collision_status gh_collision_pairs_construct_002b8ae0(gh_collision_pairs *,gh_heap *);
/* Actual2B8D88. Any mode other than1 takes originaldisable branch, including
 * its secondrow OR-complement behavior. Native checks protect 32row bounds. */
gh_collision_status gh_collision_pair_set_002b8d88(gh_collision_pairs *,int mode,unsigned a,unsigned b);
/* Actual2B8B50 firstbootinit: realaligned400elementallocation/zeroing and
 * original26orderedcalls. Occupiedowner rejects rather than leaks oldstorage. */
gh_collision_status gh_collision_pairs_initialize_002b8b50(gh_collision_pairs *);
/* Native lifetime boundary; explicit realallocationrelease, preservesmatrix. */
gh_collision_status gh_collision_pairs_release(gh_collision_pairs *);
#ifdef __cplusplus
}
#endif
#endif
