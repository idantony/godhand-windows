#ifndef GH_GAME_PLAYER_SHADOW_H
#define GH_GAME_PLAYER_SHADOW_H
#include "game_actor_model.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_title_player_view *object;uint8_t *packets[2];uint32_t pattern_original;uint8_t initialized;} gh_player_shadow;
/* Actual14EF50(self,self+6B0,3C3F68), two15EAC8 templates. No GS execution
 * or shadow-render acknowledgement; pointers44C/450 become this borrowedview. */
gh_title_player_status gh_game_player_shadow_initialize_0014ef50(gh_player_shadow *,gh_title_player_view *);
/* Same original14EF50 constructor with explicit aligned, non-overlapping
 * objectbuffer offset. pattern_original is an opaque originalidentity; no
 * pattern dereference, shadowdraw or GPU acknowledgment. */
gh_title_player_status gh_game_actor_shadow_initialize_0014ef50(gh_player_shadow *,gh_title_player_view *,size_t buffer_offset,uint32_t pattern_original);
/* Actual150128(self,4) and next107F58 part27/28 flags; same modelparts. */
gh_title_player_status gh_game_player_initial_part_flags(gh_actor_model *);
#ifdef __cplusplus
}
#endif
#endif
