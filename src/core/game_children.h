#ifndef GH_GAME_CHILDREN_H
#define GH_GAME_CHILDREN_H
#include <stddef.h>
#include <stdint.h>
#include "id_base.h"
#include "boot.h"
#include "game_render_steps.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_game_children gh_game_children;
typedef enum {GH_CHILD_OK,GH_CHILD_ARGUMENT,GH_CHILD_MEMORY,GH_CHILD_BOUNDS,GH_CHILD_UNSUPPORTED} gh_child_status;
typedef struct gh_game_child_view {
 const uint8_t *bytes;size_t size; /*boundedspan, not a sizeofclass claim*/uint32_t original_offset,vtable_address,init_entry,kind;
 int initialized;gh_id_base *native_base;
} gh_game_child_view;
/* Actual1612C8 construction with native handles for pointer fields. Vtable
 * addresses in views are provenance IDs, never callable host addresses.
 * room_jump_at_construction replaces original global747A44 copied to+868;
 * normal cold construction precedes loading and suppliesNULL. */
gh_child_status gh_game_children_create(const void *room_jump_at_construction,gh_game_children **);
const void *gh_game_children_room_jump(const gh_game_children *);
void gh_game_children_destroy(gh_game_children *);
/* Actual15FD98: resetpairarray,insert9children in original order,invoke their
 * proven concrete initializer bodies,thenfixed footer reset. No UI/scene calls
 * are replaced with no-ops; the original nine methods only reset fields. */
gh_child_status gh_game_children_initialize_0015fd98(gh_game_children *);
/* Actual15FEE0: nine descendingfooterwordsEE8..F08=-1 andbyteF15=0.
 * No virtualwidgetdispatch, IDallocation or objectreconstruction occurs. */
gh_child_status gh_game_children_room_reset_0015fee0(gh_game_children *);
gh_child_status gh_game_child_at(const gh_game_children *,uint32_t registry_index,gh_game_child_view *);
/* NativeIDBase views reuse the already-recovered IDruntime implementation.
 * This module initializes but neverallocates IDentryruns; later consumers must
 * release their runs through IDruntime before destroying owner/runtime. */
gh_child_status gh_game_children_id_base(gh_game_children *,uint32_t original_offset,gh_id_base **);
gh_child_status gh_game_children_manager_view(const gh_game_children *,const uint8_t **,size_t *);
/* Actual161288->15FD18 lookup of manager+48 kind; -1/notfound returnsNULL.
 * It selects one of the SAME nine registered objects, never copies a child. */
gh_child_status gh_game_children_active_00161288(const gh_game_children *,const gh_game_child_view **);
/* Actual160C48 manager color-alpha transition. Valid phase2/3 counts0..4
 * use exact integer-equivalent quarter/128 arithmetic, no host FEnv. Other
 * unknownfloatdomains return explicitunsupported before claiming transition. */
gh_child_status gh_game_children_alpha_step_00160c48(gh_game_children *,uint32_t *frontier);
/* Actual1601D8 cold/default/null-selected state machine and genuine early
 * gates. Active unrecovered child operations/fade/IO remain precise frontiers.
 * Does not iterate nine widgets, allocate ID runs or acknowledge UI drawing. */
gh_child_status gh_game_children_move_001601d8(gh_game_children *,gh_boot_work *,uint32_t *frontier);
/* Exact160D50 untextured fullscreenstrip. Backendmustreallyallocate/submit
 * the15B880 operation (evenalpha0) andapplyTEST3001D beforeconsumption.
 * emitted=0 denotes genuineoriginalpacketallocationfailure; notemptyack.
 * emitted mustbeexplicitlywritten0/1. Bank/depthareoriginalOTcoordinates. */
typedef struct {float vertices[4][4];uint32_t rgba,mode,bank,depth,descriptor_flags;uint64_t alpha,test_override;} gh_child_cover_quad;
typedef gh_child_status (*gh_child_cover_submit)(void *,const gh_child_cover_quad *,int *original_packet_emitted,uint32_t *frontier);
gh_child_status gh_game_children_cover_00160d50(const gh_game_children *,gh_render_draw_env *,gh_child_cover_submit,void *,uint32_t *frontier);
/* Actual160B90: Work58C/sign and5DC/100000gates; unconditionalcover THEN
 * sharedDrawInit/bank7depth1 THENselectedchild/state/drawgates. Unknownactive
 * virtualdrawreturnsitsactualvtable+34target, neverafakeemptycallback. */
gh_child_status gh_game_children_draw_00160b90(const gh_game_children *,const gh_boot_work *,gh_render_draw_env *,gh_child_cover_submit,void *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
