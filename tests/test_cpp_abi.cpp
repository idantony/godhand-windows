#include "../src/core/rng.h"
#include "../src/core/boot.h"
#include "../src/platform/disc.h"
#include "../src/rooms/r207.h"

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
    return 0;
}
