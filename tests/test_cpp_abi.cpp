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
    return 0;
}
