#ifndef GH_GAME_PREPARE_H
#define GH_GAME_PREPARE_H
#include "game_init.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original global5E8658 span through5F38E8. This is distinct from original
 * Work-area *3C264C=5F3970 and VBS5E80E0; never duplicate their storage. */
typedef struct {uint8_t fields[0xb290];uint32_t global_0061a994;} gh_game_prepare;
/* Actual2A5D40:2BF8C8->1F9B10(save),2BEB80(global);2CABE0 really is a
 * single return in this ELF; then state3. No platform completion fabricated.
 * NULL/zero save view follows the original absent-save pointer branches. */
gh_init_status gh_game_prepare_002a5d40(gh_game_prepare *,gh_core_save_view,int16_t room,uint8_t *game_state);
#ifdef __cplusplus
}
#endif
#endif
