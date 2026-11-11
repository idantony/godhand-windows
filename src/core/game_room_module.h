#ifndef GH_GAME_ROOM_MODULE_H
#define GH_GAME_ROOM_MODULE_H
#include "game_room_pack.h"
#include "game_scene_setup.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_ROOM_MODULE_OK,GH_ROOM_MODULE_ARGUMENT,GH_ROOM_MODULE_BUSY,
 GH_ROOM_MODULE_DVD,GH_ROOM_MODULE_VERSION,GH_ROOM_MODULE_PLATFORM,
 GH_ROOM_MODULE_UNSUPPORTED} gh_room_module_status;
/* Native module identity, not an EE executable mapping. The original module
 * constructors have an empty list; the room object is allocated by Init later.
 * Keep the original allocation alive for bounded source/asset diagnostics. */
typedef struct {
 gh_room_pack input;uint16_t room;uint8_t bound;
 gh_scene_hook hook4,hook8,hook12;
 uint32_t init_offset,move_offset,end_offset,frontier;int detail;
} gh_room_module;
/* Actual2BE708/2BE838/2BE560 load/wait portion, pinned R003/R020/R100 profiles.
 * Release aliases DebugOptimize via actual1FF858/1FFA68 FST directory policy;
 * the canonical physical path below is not a generic basename fallback.
 * R100 PreInit+4 and each profile's +8/+C remain UNRESOLVED until native
 * implementations attach. A hash match never executes REL machine code.
 * Never invokes REL instructions or substitutes an empty successful callback. */
gh_room_module_status gh_room_module_load_002be708(gh_room_module *,uint16_t);
gh_room_module_status gh_room_module_release(gh_room_module *);
#ifdef __cplusplus
}
#endif
#endif
