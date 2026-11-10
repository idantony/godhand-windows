#ifndef GH_GAME_EVENT_COMPONENTS_H
#define GH_GAME_EVENT_COMPONENTS_H
#include "game_event_module.h"
#ifdef __cplusplus
extern "C" {
#endif
enum {GH_EVENT_COMPONENT_COUNT=15,GH_EVENT_COMPONENT_BYTES=2228};
typedef struct {uint8_t *bytes;size_t size;uint32_t original_offset;const char *name;} gh_event_component_view;
typedef struct {const void *module;size_t module_size;gh_event_component_view components[15];
 uint8_t storage[GH_EVENT_COMPONENT_BYTES];uint32_t set_event_no_offset,frontier;
 uint8_t bound,constructed;int32_t event_no;} gh_event_components;
/* Resolve actual Main SNR2 imports to event.rel exports as native identities.
 * No guest instruction execution or pointer relocation. Requires ORIGINAL
 * file-backed BootELF and real loaded event.rel. Every15 HI/LO pairs and final
 * JAL0 import at2977C8 are checked; missing imports are not no-ops. */
gh_game_event_status gh_event_components_bind(gh_event_components *,const gh_event_module *,const void *boot_elf,size_t);
/* Actual297680: clear15 globals in order/size, then realREL54B8 setter of
 * EventEnv's firstword. This does not acknowledge later component initialization,
 * allocations, file binding, animation or playback readiness. */
gh_game_event_status gh_event_components_construct_00297680(gh_event_components *,int32_t event_no);
gh_game_event_status gh_event_component_at(gh_event_components *,unsigned,gh_event_component_view *);
#ifdef __cplusplus
}
#endif
#endif
