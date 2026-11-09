#ifndef GH_GAME_SCENE_UI_H
#define GH_GAME_SCENE_UI_H
#include "font_state.h"
#include "menu_state.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Disjointnativecompanion for originalcMenu +4roomMSD/+10active/+14free/
 * +18sixteenBCblocks/+BD8generation. Existingmenu.data(+0),texture_data(+C),
 * timerBDA/stateBF0 stay in the singleborrowedrootowner. This is not the
 * MCpackedmessage cursor/environment owner and createsno secondlanguagebank. */
typedef struct {uint16_t flags,mode;int32_t previous,next;uint8_t payload[0xb0];} gh_scene_message_block;
typedef struct {gh_menu_state *root;gh_bundle_entry room_data;
 gh_scene_message_block blocks[16];int32_t active_head,free_head;uint16_t generation_bd8;
 uint8_t ready;uint32_t frontier;} gh_game_scene_ui;
typedef enum {GH_SCENE_UI_OK,GH_SCENE_UI_ARGUMENT,GH_SCENE_UI_BOUNDS,GH_SCENE_UI_UNSUPPORTED} gh_scene_ui_status;
gh_scene_ui_status gh_scene_font_bind_002a4c38(gh_font_state *,const gh_bundle *,int32_t language);
/* Actual2AEA18/2B22E8 orderedcalls: first15pointtonext, lastNULL; each
 * iterationresetsitsownprev, thus finalprevsALLNULL (donotrepairdoublylinks).
 * Onlyheader12bytes perblockreset; opaque payload staysunchanged. */
gh_scene_ui_status gh_scene_message_initialize_002aea18(gh_game_scene_ui *,gh_menu_state *,const gh_bundle *,int32_t language);
#ifdef __cplusplus
}
#endif
#endif
