#ifndef GH_R003_EVENT_H
#define GH_R003_EVENT_H

#include "../core/game_door.h"
#include "../core/game_event_environment.h"
#include "../core/game_room_module.h"
#include "../core/game_scenario_soft_event.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {GH_R003_OK,GH_R003_ARGUMENT,GH_R003_MEMORY,GH_R003_UNSUPPORTED,GH_R003_CALLEE} gh_r003_status;
typedef gh_game_event_status (*gh_r003_end_soft)(void *,uint32_t *);
typedef gh_door_status (*gh_r003_begin_jump)(void *,uint16_t,const float[3],float,uint8_t,uint32_t,uint8_t,uint32_t *);

typedef struct {
    gh_room_module *module;
    gh_heap *heap;
    gh_disc *disc;
    gh_scene_scenario_view scenario;
    gh_task_threads *threads;
    gh_event_reserve_work *reserve;
    gh_soft_event_bindings soft;
    gh_event_play_start_bindings play;
    gh_r003_end_soft end_soft;
    gh_r003_begin_jump begin_jump;
    void *context;
} gh_r003_bindings;

typedef struct {
    gh_r003_bindings bindings;
    gh_r207_allocation work;
    gh_soft_event_start soft_start;
    float jump_position[3],jump_yaw;
    int32_t slot,event_number;
    uint32_t frontier;
    gh_r003_status status;
    uint16_t jump_room;
    uint8_t initialized,task_stage,finished,jump_force;
} gh_r003_event;

/* R003Init (REL+1A0) allocates its three MoveFlag scalars and registers
 * the original REL+248 task when both reserve bits are set. */
gh_r003_status gh_r003_event_init_000001a0(gh_r003_event *,const gh_r003_bindings *);
gh_r003_status gh_r003_event_release(gh_r003_event *);

#ifdef __cplusplus
}
#endif
#endif
