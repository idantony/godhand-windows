#ifndef GH_GAME_ACTION_BUTTONS_H
#define GH_GAME_ACTION_BUTTONS_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Native explicitreferences replace PS2pointerwords; this is NOTguestABI.
 * Unknowncallback IDs are opaque (never functionpointers). */
typedef struct gh_action_prompt {uint32_t flags;struct gh_action_prompt *next;
 uint32_t unknown08;int32_t state0c;uint32_t callback_id10;void *callback_argument;
 int32_t priority,life,unknown20,kind;const float *matrix;int32_t unknown2c;
 uint16_t unknown30;int16_t unknown32;uint8_t command_count,unknown35,unknown36,unknown37;
 const int32_t *command_table;const uint16_t *id_table;const uint8_t *argument_table;
} gh_action_prompt;
typedef struct gh_action_pool_link {uint32_t unknown0;struct gh_action_pool_link *previous;
 uint32_t unknown8,flags;} gh_action_pool_link;
typedef enum {GH_ACTION_LINK_END,GH_ACTION_LINK_HEADER,GH_ACTION_LINK_PROMPT,GH_ACTION_LINK_UNKNOWN} gh_action_link_kind;
typedef struct {gh_action_link_kind kind;uint32_t index;} gh_action_link;
typedef struct gh_game_action_buttons {uint32_t unknown00;gh_action_pool_link pool[16];gh_action_prompt entries[8];
 /* SAME original pointer graph, typed to support mixed header/prompt nodes.
  * Legacy previous/next pointers are compatibility header-only views; after
  * publication consumers MUST traverse these authoritative typed links. */
 gh_action_link heads[16],prompt_next[8];const struct gh_game_action_buttons *native_identity;uint8_t links_ready;
} gh_game_action_buttons;
/* Actual1F72C0->1F77D0. Rebuildprevlinkchain andreset8promptfields, preserving
 * allunspecifiedbytes/references. Doesnotfreeforeignobjects orclaimUIrender. */
void gh_game_action_buttons_reset_001f72c0(gh_game_action_buttons *);
#ifdef __cplusplus
}
#endif
#endif
