#ifndef GH_R100_EXIT_H
#define GH_R100_EXIT_H
#include "../core/game_door.h"
#include "../core/game_event.h"
#include "../core/game_action_button_step.h"
#include "../core/game_room_state.h"
#include "../core/game_scene_at_step.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef gh_door_status (*gh_r100_exit_begin_room)(void *,uint16_t room,
    const float position[3],float yaw,uint8_t kind,uint32_t flags,uint8_t force,
    uint32_t *frontier);

typedef struct {
    gh_boot_work *work;
    gh_room_state *room;
    gh_scene_at_manager *at;
    gh_game_action_buttons *buttons;
    gh_scene_scenario_view scenario;
    gh_task_threads *threads;
    gh_event_reserve_work *event_work;
    gh_r100_exit_begin_room begin_room;
    void *context;
} gh_r100_exit_bindings;

typedef struct {
    gh_r100_exit_bindings bindings;
    uint32_t frontier;
    gh_scene_at_status status;
    int32_t slot;
    uint8_t pending;
} gh_r100_exit;

/* Original SceneAt deferred request and kind2 ActionButton invocation for
 * R100 jump0. The owner and bindings outlive the registered Scenario task. */
gh_scene_at_status gh_r100_exit_spawn_001f7858(void *,const gh_scene_at_task_request *,uint32_t *frontier);
gh_action_step_status gh_r100_exit_action_002c2138(void *,uint32_t original_entry,
    void *unit_argument,uint32_t *frontier);

#ifdef __cplusplus
}
#endif
#endif
