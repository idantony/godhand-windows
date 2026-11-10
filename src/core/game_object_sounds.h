#ifndef GH_GAME_OBJECT_SOUNDS_H
#define GH_GAME_OBJECT_SOUNDS_H
#include "game_objects.h"
#include "enemy_se_banks.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Native programcounter only. Holder records, samplebanks and task stacks are
 * borrowed SAME owners. Reentering a completed call never appends/reacquires. */
typedef struct {gh_game_objects *manager;gh_enemy_se_banks *banks;gh_task_threads *threads;const gh_boot_work *work;const uint8_t *source;
 uint32_t kind,dependency,flags,stage,cursor,count,requests[5];uint8_t missing;} gh_object_sound_load;
gh_object_status gh_game_object_sounds_001feaa0(gh_object_sound_load *,gh_game_objects *,uint32_t kind,uint32_t dependency,uint32_t flags,const gh_boot_work *,gh_enemy_se_banks *,gh_task_threads *,uint32_t *frontier);
/* Real2CB778 bridge suitable for existingresourcebindings.sound_check. */
gh_object_status gh_enemy_se_object_sound_check(void *,int32_t slot,int *original_ready,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
