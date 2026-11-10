#ifndef GH_GAME_ACTOR_LIGHT_GLOBALS_H
#define GH_GAME_ACTOR_LIGHT_GLOBALS_H
#include "game_actor_model.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original mutable .data vectors, separate from canonical3BD880 owner.
 * Cold-load once and borrow; future recovered writers must use this owner,
 * never replace runtime changes by another ELF read per model/frame. */
typedef struct {uint8_t bytes_3bd870[16],bytes_3bd890[16];uint8_t initialized;} gh_actor_light_globals;
gh_actor_model_status gh_actor_light_globals_from_elf(gh_actor_light_globals *,const void *,size_t);
#ifdef __cplusplus
}
#endif
#endif
