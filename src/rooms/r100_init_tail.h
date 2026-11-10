#ifndef GH_R100_INIT_TAIL_H
#define GH_R100_INIT_TAIL_H
#include "r100_init.h"
#include "../core/game_init.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef gh_r100_init_status (*gh_r100_tail_leaf)(void *,uint32_t *frontier);
typedef gh_r100_init_status (*gh_r100_tail_load)(void *,uint16_t kind,int wait,uint32_t *frontier);
typedef gh_r100_init_status (*gh_r100_tail_create)(void *,uint16_t kind,uint16_t id,gh_r100_om_view *,uint32_t *frontier);
typedef gh_r100_init_status (*gh_r100_tail_object_init)(void *,const gh_r100_om_view *,uint32_t *frontier);
typedef gh_r100_init_status (*gh_r100_tail_object_position)(void *,const gh_r100_om_view *,const float position[4],uint32_t *frontier);
typedef struct gh_r100_init_tail_bindings {const gh_core_save_view *save;const gh_boot_work *work;
 gh_r100_tail_leaf tutorial_room_init_00147028;
 gh_r100_tail_load load_wait_001fe8a0;
 gh_r100_tail_create create_object_0030fa98;
 gh_r100_tail_object_init virtual_init44;
 gh_r100_tail_object_position virtual_set_position74;void *context;
} gh_r100_init_tail_bindings;
typedef struct gh_r100_init_tail {uint32_t stage,frontier;int16_t clear_num;
 uint8_t difficulty,tutorial_required,complete,frontier_relative;
 gh_r100_om_view pending_object;
} gh_r100_init_tail;
/* Post-registration original208 suffix. No tutorial callback is called unless
 * signed clearNum==0 and difficulty!=2. REL748 operations retain completed
 * prefixes across retries, including the SAME newly-created object identity.
 * Providers must perform actual native operations, not acknowledge readiness.
 * NULL result from a successful real CreateObj is the original allocation-fail
 * branch; absence of the provider is never treated as a NULL object. */
gh_r100_init_status gh_r100_init_tail_step(gh_r100_init_tail *,const gh_r100_init_tail_bindings *);
#ifdef __cplusplus
}
#endif
#endif
