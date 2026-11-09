#include "../src/core/rng.h"
#include "../src/core/boot.h"
#include "../src/platform/disc.h"
#include "../src/rooms/r207.h"
#include "../src/platform/memory.h"
#include "../src/core/tasks.h"
#include "../src/platform/task_threads.h"
#include "../src/assets/tim2.h"
#include "../src/assets/model.h"
#include "../src/assets/core_bundle.h"
#include "../src/assets/sound_bank.h"
#include "../src/core/startup_task.h"
#include "../src/core/dvd_jobs.h"
#include "../src/platform/startup_host.h"
#include "../src/assets/font_texture.h"
#include "../src/core/game_start.h"
#include "../src/core/mc_boot.h"
#include "../src/core/id_path.h"
#include "../src/core/mc_control_flow.h"
#include "../src/core/mc_exit.h"
#include "../src/core/game_cockpit.h"
#include "../src/core/game_objects.h"
#include "../src/core/game_prepare.h"
#include "../src/core/game_room_state.h"
#include "../src/core/game_room_pack.h"
#include "../src/core/game_room_pools.h"
#include "../src/core/game_camera.h"
#include "../src/core/game_movie_pool.h"
#include "../src/core/movie_task.h"
#include "../src/platform/game_services.h"
#include "../src/platform/native_input.h"
#include "../src/platform/video_player.h"
#include "../src/core/frame_state.h"
#include "../src/core/game_scene_setup.h"
#include "../src/core/sound_scene.h"
#include "../src/core/game_collision_pairs.h"
#include "../src/core/game_room_module.h"
#include "../src/core/screen_fade.h"
#include "../src/core/game_effects.h"
#include "../src/core/game_scene_filters.h"
#include "../src/core/bgm_requests.h"

// Link actual C objects from the future C++ host. No duplicated implementation.
int main()
{
    gh_rng_state random = {0};
    gh_boot_state boot = {};
    gh_boot_platform platform = {};
    gh_r207_move();
    if (gh_rng_0031cf30(&random) != 1.0f)
        return 1;
    if (gh_boot_0030b298(&boot, &platform).status != GH_BOOT_MISSING_CALLBACK)
        return 2;
    if (gh_disc_count(nullptr) != 0)
        return 3;
    if (gh_heap_used(nullptr) != 0)
        return 4;
    gh_task_manager manager = {};
    const gh_task_selection selection = gh_tasks_select_002d5b30(&manager, -1);
    if (selection.status != GH_TASK_OK || selection.index != -1)
        return 5;
    if (gh_task_threads_destroy(nullptr) != GH_THREAD_INVALID)
        return 6;
    gh_tim2_picture picture = {};
    if (gh_tim2_first(nullptr, 0, &picture) != GH_TIM2_ARGUMENT)
        return 7;
    gh_model_view model = {};
    if (gh_model_open_scr(nullptr, 0, &model) != GH_MODEL_ARGUMENT)
        return 8;
    gh_bundle bundle = {};
    if (gh_bundle_open(nullptr, 0, &bundle) != GH_BUNDLE_ARGUMENT)
        return 9;
    gh_sound_bank sound = {};
    if (gh_sound_bank_open(nullptr, 0, &sound) != GH_SOUND_ARGUMENT)
        return 10;
    if (gh_startup_task_0030d358(nullptr, nullptr) != -1 || gh_dvd_check(nullptr, 0) != 0)
        return 11;
    if (gh_startup_host_destroy(nullptr) != GH_HOST_ARGUMENT ||
        gh_audio_release_bank(nullptr, 0) != GH_AUDIO_ARGUMENT ||
        gh_font_initialize(nullptr, nullptr, 1) != GH_FONT_ARGUMENT ||
        gh_menu_load_tables_002ae990(nullptr, nullptr) != GH_MENU_ARGUMENT)
        return 12;
    if (gh_font_atlas_rgba(nullptr, nullptr, 0) != GH_FONT_ARGUMENT ||
        gh_game_assets_release(nullptr) != GH_GAME_ARGUMENT ||
        gh_mc_boot_checks_create(nullptr, nullptr) != GH_MC_BOOT_ARGUMENT)
        return 13;
    return 0;
}
