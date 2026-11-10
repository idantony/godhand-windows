#ifndef GH_GAME_MATH_GLOBALS_H
#define GH_GAME_MATH_GLOBALS_H
#include "boot.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_MATH_GLOBALS_OK,GH_MATH_GLOBALS_ARGUMENT,GH_MATH_GLOBALS_BOUNDS,GH_MATH_GLOBALS_FORMAT} gh_math_globals_status;
/* ONE original747AC0 matrix +747B00 vector, separatefrom Work (ends620).
 * LoadedBSS differsfrom post3079C8 values. Floatscarryoriginalbinary32bits;
 * no hostcomputedidentity or syntheticzero substitute. */
typedef struct {float identity[16],zero_vector[4],source_identity[16];uint8_t loaded,initialized;} gh_game_math_globals;
gh_math_globals_status gh_game_math_globals_from_elf(gh_game_math_globals *,const void *,size_t);
/* Actualstaticinitializer branch initialize!=0 && priority==FFFF. Also
 * originalSQzero Work5C0; inactivebranch preservesallstate. Callonceatboot,
 * never duringroomInit toavoid resetting player spawn input. */
gh_math_globals_status gh_game_math_static_initialize_003079c8(gh_game_math_globals *,gh_boot_work *,int initialize,int priority);
#ifdef __cplusplus
}
#endif
#endif
