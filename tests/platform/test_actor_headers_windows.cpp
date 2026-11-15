/* Regression: real Win32 headers define the legacy far macro. Public actor
 * APIs must remain usable when windows.h precedes them in native C++ glue. */
#include <windows.h>
#ifndef far
#error expected Windows legacy far macro
#endif
#include "game_actor_model_trans.h"
#include "game_actor_model_draw.h"
int main(){gh_actor_trans_request trans{};gh_actor_draw_request draw{};trans.far_mode=1;draw.far_mode=1;return trans.far_mode!=1||draw.far_mode!=1;}
