#ifndef GH_GAME_SCENE_STEP_H
#define GH_GAME_SCENE_STEP_H
#include "game_prepare.h"
#include "game_scene_setup.h"
#include "game_scene_tables.h"
#include "game_collision_pairs.h"
#include "game_collision_pair_shapes.h"
#include "game_title_player.h"
#include "screen_fade.h"
#include "game_event.h"
#include "game_scene_at.h"
#include "game_scene_at_age.h"
#include "game_hitarea_frame.h"
#include "game_action_buttons.h"
#include "display_state.h"
#include "../platform/task_threads.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_SCENE_STEP_OK,GH_SCENE_STEP_ARGUMENT,GH_SCENE_STEP_BOUNDS,
 GH_SCENE_STEP_UNSUPPORTED,GH_SCENE_STEP_CALLEE,GH_SCENE_STEP_THREAD} gh_scene_step_status;
/* Separate actual Main+18C NearCheck, NOT Main+0C's32 registrationrecords.
 * Serializedrelativeindices remain immutable/bounded, never hostpointers. */
typedef struct {gh_scene_table_view pairs,vertices,indices;uint32_t pair_count,
 vertex_count,index_count;int32_t cursor;uint8_t copy_mode;
 /* Native mutable copy of actual pair records; all consumers use pairs above.
  * Read-only PFD source/relative vertex-index data remain borrowed. */
 uint8_t *owned_pairs;size_t owned_pair_bytes;const uint8_t *source;size_t source_bytes;
 uint32_t active_pair_count;uint8_t active,iteration;
} gh_scene_nearcheck;
typedef struct {
 gh_scene_nearcheck nearcheck;int16_t cooldown_1b0;
 uint32_t profile_752cf0,profile_752cfc,profile_752d44,word_5caff4;
 uint32_t stage,frontier,collision_cursor,scenario_task_cursor;int detail;uint64_t completed_frames;
 uint8_t scenario_stage,scenario_before_stage,death_callback_pending,enemy_stage,scenario_status_pending;
 gh_collision_pair_frame collision_frame;
 gh_hitarea_frame hitarea_frame; /*pause scalar belongs to SAME init.object_pool+ACD0*/
 gh_scene_at_age at_frame_age;
} gh_game_scene_step;
/* Required semantic boundaries for as-yet-unported world/render consumers.
 * The address/argument identify original operations, NEVER callable guestcode.
 * An indirectargument token is its originalglobalpointer SLOT address;
 * adapters resolve canonicaltypedowners, not the token as a hostaddress.
 * Return OK only after actual work; NULL means precise frontier, not emptyframe.
 * Nativeprofiling counts may useQPC; they are not gameplay/world-time ticks. */
typedef gh_scene_step_status (*gh_scene_step_external)(void*,uint32_t address,
 uint32_t original_argument,uint32_t *frontier);
typedef uint32_t (*gh_scene_profile_counter)(void*);
typedef gh_scene_step_status (*gh_scenario_task_status)(void *,uint32_t *frontier);
typedef struct {
 gh_boot_work *work;gh_game_init_state *init;gh_game_prepare *prepare;
 gh_core_save_view save;
 gh_collision_pairs *collision;gh_game_scene_tables *actors;
 gh_scene_scenario_view scenario;gh_task_threads *main_threads,*scenario_threads;
 const gh_scene_hook *room_move;gh_game_title_player *title_player;
 gh_screen_fade *fade;
 const gh_display_state *display;
 gh_game_event *event; /*RootONEcanonical586AF0owner, not copied flags*/
 gh_scene_at_manager *scene_at;gh_game_action_buttons *action_buttons;
 uint8_t *cockpit_action_flags_1414; /*borrowonecanonicalbyte, notconstcast*/
 int binary32_reference; /*explicitfinite hostnumericprofile, notEEbitexact*/
 const gh_scene_hook *scenario_callback104;
 gh_scene_step_external external;void *external_context;
 gh_scene_profile_counter profile_counter;void *profile_context;
 gh_scenario_task_status scenario_status;void *scenario_status_context;
 const struct gh_dynamic_corner_tables *dynamic_corners; /*RootONE static globals*/
 const uint8_t *boot_elf;size_t boot_elf_bytes; /*borrow descriptor identities for HitArea LOS*/
} gh_game_scene_step_bindings;
/* Explicit cold host companion init: caller supplies original Main1A8 seed;
 * ignored whilePFD absent. Does not invent the earlier complete Mainctor. */
void gh_game_scene_step_construct(gh_game_scene_step*,int32_t scan_cursor,int16_t cooldown);
/* Actual31BE78 resets threecounts only, preservingviews/cursor/copy_mode. */
void gh_scene_nearcheck_reset_0031be78(gh_scene_nearcheck*);
gh_scene_step_status gh_scene_nearcheck_bind_0031c1d0(gh_scene_nearcheck*,const gh_bundle_entry*);
gh_scene_step_status gh_scene_nearcheck_step_002a6ff8(gh_scene_nearcheck*,const gh_boot_work*,gh_game_scene_tables*,uint32_t *frontier);
/* Native lifetime end after Main stops, before the borrowed room source frees.
 * Preserves original Main1A8 cursor and copy-mode scalar across rooms. */
void gh_scene_nearcheck_release(gh_scene_nearcheck*);
/* Only the originalcase3tail's Main+1B0=15 write. Caller invokes atits genuine
 * position after roomInit/PFD preparation, not before a failedroomInit. */
void gh_game_scene_step_case3_cooldown(gh_game_scene_step*);
/* Actual2C3678 room+12→Scenario16→pause consumer, with alloriginal gates.
 * Calling this directly is not a substitute for the earlierstate4 operations. */
gh_scene_step_status gh_game_scene_scenario_move_002c3678(gh_game_scene_step*,const gh_game_scene_step_bindings*);
gh_scene_step_status gh_game_scene_scenario_before_002c3500(gh_game_scene_step*,const gh_game_scene_step_bindings*);
/* Actual16-task traversal2D5358, including manager+34 parentSleep/revisit.
 * Unit/integration entry: it does not invoke roomMove orstand in forstate4. */
gh_scene_step_status gh_game_scene_tasks_002d5358(gh_game_scene_step*,const gh_game_scene_step_bindings*);
/* Actual2A6690 orderedprefix/update/render. Retainscompletedprefix across a
 * returned hostfrontier; never repeats timers/callbacks while retrying it. */
gh_scene_step_status gh_game_scene_step_002a6690(gh_game_scene_step*,const gh_game_scene_step_bindings*);
#ifdef __cplusplus
}
#endif
#endif
