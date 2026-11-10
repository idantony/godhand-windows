#ifndef GH_GAME_INIT_H
#define GH_GAME_INIT_H
#include "boot.h"
#ifdef __cplusplus
extern "C" {
#endif
#define GH_CORE_SAVE_BYTES 0x14a0
typedef struct {uint8_t *bytes;size_t size;} gh_core_save_view;
typedef enum {GH_INIT_OK,GH_INIT_ARGUMENT,GH_INIT_BOUNDS,GH_INIT_LINKS} gh_init_status;
/* Byte views are the actual persistent format, not pointer-bearing guest ABI.
 * gameInit preserves difficulty and all fields it does not originally touch. */
gh_init_status gh_core_save_system_init(gh_core_save_view);
gh_init_status gh_core_save_game_init(gh_core_save_view,const gh_boot_work *);
uint8_t gh_core_save_costume(gh_core_save_view);
uint8_t gh_core_save_difficulty(gh_core_save_view);
typedef struct {uint32_t words[624],index;} gh_game_mt;
void gh_game_mt_seed_0030f2b0(gh_game_mt *,uint32_t);
gh_init_status gh_game_mt_next_0030f348(gh_game_mt *,uint32_t *);
typedef struct gh_game_camera {
 int32_t mode,previous_mode;struct gh_game_camera *previous;
} gh_game_camera;
/* Original damage linked node+28 points to an object whose+10 gets a mask.
 * Native pointers replace both guest links. The initializer clears its head
 * before setting mode4, so previous nodes are not visited/freed or modified.
 * Caller owns links/target words. */
typedef struct gh_game_damage_link {
 struct gh_game_damage_link *next;uint32_t *target_mask;
} gh_game_damage_link;
struct gh_hitarea_native;
typedef struct gh_game_init_state {
 uint32_t world_time;
 gh_game_camera camera,*active_camera;
 uint8_t enemies[0x5b8];void *enemy_actors[64],*enemy_top,*enemy_last,*enemy_kept[16];
 uint8_t side_counter[16];
 uint8_t object_pool[0xacb0];gh_game_damage_link *object_links[416];
 uint8_t records_a[160][0x40],records_b[512][0x40];
 uint8_t heat[0x18],input[0x14];
 gh_game_mt random;
 struct gh_hitarea_native *hitarea_native;uint32_t hitarea_generation;
} gh_game_init_state;
/* Host storage initialization, not the earlier complete PS2 boot. Existing
 * save data must separately be system-initialized or loaded by its owner. */
void gh_game_init_state_initialize(gh_game_init_state *);
/* Actual1FDEA8, also reached by later1FDF88. Same canonical actorpool/links
 * andrecordbanks ascase0, without enemy/Heat/RNG resets. Clears eachdamage
 * head BEFORE mode4, so priorlinks are dropped withouttraversal/free. */
gh_init_status gh_game_object_pool_reset_001fdea8(gh_game_init_state *);
/* Actual room-exit1FDFA8 only the160/256 object records; it first visits
 * each damage chain before clearing that record. Unknown live nodes stop. */
gh_init_status gh_game_object_pool_release_001fdfa8(gh_game_init_state *,uint32_t *frontier);
/* Actual290230->290110 sameEmManage reset usedincase0. ConsumesONE
 * original31CFE0 MTresult and unsignedDIVU5, not a secondnativeRNG. */
gh_init_status gh_game_enemy_reset_00290230(gh_game_init_state *);
/* Actual case0 task handler2A5B18; these dependencies are native concrete
 * writes/list reset/RNG/save operations, no successful empty callbacks. */
gh_init_status gh_game_init_002a5b18(gh_game_init_state *,gh_boot_work *,
 uint8_t *game_state_byte,float *game_rate,gh_core_save_view);
#ifdef __cplusplus
}
#endif
#endif
