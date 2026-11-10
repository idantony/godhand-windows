#ifndef GH_GAME_PLAYER_SPRINGS_H
#define GH_GAME_PLAYER_SPRINGS_H
#include "game_player_child.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_SPRINGS_OK,GH_SPRINGS_ARGUMENT,GH_SPRINGS_BOUNDS,GH_SPRINGS_FORMAT,GH_SPRINGS_NUMERIC} gh_player_springs_status;
/* Owned exact index/parameter copies from original ELF, never guest pointers.
 * table2084 is optional bounded file backing; actual profiles point to mutable BSS and leave it NULL. Later simulator must
 * recover its canonical scratch owner and record shape; never read BSS through a fake ELF file pointer. */
typedef struct {uint32_t count,table_addresses[11],frontier;uint8_t indices[7][44];
 uint32_t coefficients[3][44];const uint8_t *table2084;size_t table2084_available;
 gh_game_player_child *child;uint8_t configured,initialized;unsigned costume;} gh_game_player_springs;
/* Actual100870 config then1038E0(f12=.1,f13=0). Borrows caller's actualboot
 * ELF for table2084; keep it alive or rebind a bounded owned source beforeuse.
 * Samechild model/parts are mandatory, no restpose shadow. Explicit binary32
 * nearest reference, not a claim of hardware-VU bitexactness. Caller must run
 * original149EB8 before this leaf in the real100378 chain. */
gh_player_springs_status gh_game_player_springs_initialize_00100870(gh_game_player_springs *,gh_game_player_child *,gh_core_save_view,const void *elf,size_t,gh_static_transform_policy);
typedef struct {
    uint8_t indices[7][44];
    uint32_t coefficients[3][10];
    uint32_t original_scratch;
    uint8_t loaded;
} gh_spring_profile_2a0;
/* Original 2DE980's 2A0 table selection and the common 1038E0 record initializer. */
gh_player_springs_status gh_spring_profile_2a0_from_elf(gh_spring_profile_2a0 *, const void *, size_t);
gh_player_springs_status gh_spring_records_2a0_001038e0(const gh_spring_profile_2a0 *, gh_actor_model *,
    uint8_t *records, size_t bytes, gh_static_transform_policy, uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
