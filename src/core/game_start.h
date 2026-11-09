#ifndef GH_GAME_START_H
#define GH_GAME_START_H
#include "boot.h"
#include "dvd_jobs.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_GAME_OK,GH_GAME_ARGUMENT,GH_GAME_MEMORY,GH_GAME_DISC,
 GH_GAME_THREAD,GH_GAME_UNAVAILABLE,GH_GAME_BOUNDS} gh_game_status;
typedef struct gh_game_assets {
 gh_boot_work *work;gh_dvd_jobs *dvd;gh_heap *heap;gh_task_threads *threads;
 gh_r207_allocation core,room_jump,objects[43];
 size_t core_size,room_jump_size,object_sizes[43];
 void *sound_context;int (*sound_startup)(void *);
 gh_game_status error;uint32_t failed_entry;int detail;
} gh_game_assets;
/* Original30D5C8 task body using real queue/fiber waits. Caller drives DVDpump
 * between task visits and supplies costume number from original save state.
 * Native errors stop without setting original completion flags. Partial owned
 * allocations remain accessible; cancel tasks/jobs before release. */
gh_game_status gh_game_assets_0030d5c8(gh_game_assets *,uint8_t costume);
/* Release native allocations only, never pending queue jobs. */
gh_game_status gh_game_assets_release(gh_game_assets *);
/* Exact307918 filename subset with host bounds; 7 actual prefix strings. */
gh_game_status gh_game_object_path(uint32_t id,char out[12]);
typedef struct gh_game_loop {
 gh_boot_work *work;uint8_t game_state[4];gh_task_threads *threads;
 uint32_t failed_entry;int platform_error;
} gh_game_loop;
/* Semantic dependencies must execute the actual subsystem work or return an
 * error. 2A6CA8/2A6DA8 include conditional rendering/input waits, not no-ops. */
typedef int (*gh_game_operation)(void *,gh_game_loop *);
typedef struct gh_game_platform {
 void *context;gh_game_operation dvd_cover,modal_input,states[6];
} gh_game_platform;
gh_game_status gh_game_main_002a5998(gh_game_loop *,const gh_game_platform *);
#ifdef __cplusplus
}
#endif
#endif
