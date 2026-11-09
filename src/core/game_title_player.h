#ifndef GH_GAME_TITLE_PLAYER_H
#define GH_GAME_TITLE_PLAYER_H
#include "boot.h"
#include "../platform/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
#define GH_TITLE_PLAYER_BYTES 0x1690u
/* Native counterpart of R020 Work.playerObj. Bytefields are scalar/opaqueguest
 * tokens, not a pointer-bearing PS2 ABI struct. Actualself-pointerlinksD0/F0
 * are replaced by native aliases. Diagnostic vtable IDs cannot be executed. */
typedef enum {GH_TITLE_MEMORY_UNSPECIFIED,GH_TITLE_MEMORY_PROVIDED_BYTES,
 GH_TITLE_MEMORY_ZERO_REFERENCE} gh_title_memory_policy;
typedef struct {gh_title_memory_policy policy;const uint8_t *bytes;size_t size;} gh_title_memory_profile;
typedef struct {uint8_t *bytes;size_t size;uint8_t *alias_d0,*alias_f0;
 uint32_t original_vtable214,original_kind4ac;gh_title_memory_policy initial_memory;} gh_title_player_view;
typedef struct {gh_r207_allocation allocation;gh_heap *heap;gh_title_player_view view;
 uint32_t frontier;uint8_t ready;} gh_game_title_player;
typedef enum {GH_PLAYER_OK,GH_PLAYER_ARGUMENT,GH_PLAYER_BOUNDS,GH_PLAYER_MEMORY,GH_PLAYER_BUSY,GH_PLAYER_UNSUPPORTED} gh_title_player_status;
/* Actual107E40 constructorchain. Preserves every unspecified input byte.
 * RawD0/F0 wordszero are explicitremovedguestpointers; nativealiases carry
 * their genuine self+30/self+B0 meaning. No classvirtualfunction is invoked. */
gh_title_player_status gh_title_player_construct_00107e40(void *,size_t,gh_title_player_view *);
/* Actual31C4E0 room20 allocation/constructorbranch. Realcanonicalheap owns
 *1690bytes/alignment16. Callerpublishes the view as its typedWork.playerObj;
 * never truncates allocation.pointer into originalWork+5F4word. */
gh_title_player_status gh_game_title_player_create_0031c4e0(gh_game_title_player *,gh_heap *,const gh_boot_work *);
/* Explicit native initial-memory adaptation, approved for the R020 path.
 * ZERO_REFERENCE clears only the newly allocated input before originalctors;
 * PROVIDED_BYTES copiescallerdefined1690bytes. Neither implies originalPS2heap
 * waszero. Defaultcreateabove remains UNSPECIFIED andpreservesunknownbytes. */
gh_title_player_status gh_game_title_player_create_reference_0031c4e0(gh_game_title_player *,gh_heap *,const gh_boot_work *,const gh_title_memory_profile *);
/* Actualcheckpoint1F9BEC/1F9C00 reads unwrittenconstructorfield54A.
 * UNSPECIFIED input returns preciseunsupported ratherthanhostmallocgarbage. */
gh_title_player_status gh_title_player_read_vitality(const gh_title_player_view *,int16_t *,uint32_t *frontier);
/* Nativeownedallocationcleanup only, not a fabricated gamevirtualdestructor.
 * Thisconstructoronly path attaches no subordinate heap/GPU/collisionowners. */
gh_title_player_status gh_game_title_player_release(gh_game_title_player *);
#ifdef __cplusplus
}
#endif
#endif
