#ifndef GH_GAME_SCENE_FILTERS_H
#define GH_GAME_SCENE_FILTERS_H
#include "../assets/core_bundle.h"
#ifdef __cplusplus
extern "C" {
#endif
#define GH_FILTER_PRESET_BYTES 0x278u
#define GH_FILTER_FILE_BYTES 0x5188u
/* Scalar-only original cFilter subspan0059AC98..005A0128. Serialized fields
 * have no native pointers. Unknownbytes are preserved; copied floats are bits. */
typedef struct {uint64_t flags14098;uint32_t unknown140a0;
 uint32_t transition140a4,transition140a8,transition140ac,transition140b0;
 uint8_t unknown140b4[0x14],active140c8[0x2d8],presets143a0[GH_FILTER_FILE_BYTES];
} gh_scene_filter_work32;
typedef struct {gh_scene_filter_work32 work;uint32_t frontier;uint8_t ready;} gh_scene_filters;
typedef enum {GH_FILTER_OK,GH_FILTER_ARGUMENT,GH_FILTER_BOUNDS,GH_FILTER_UNSUPPORTED} gh_filter_status;
/* Original2A2818 choices0..4 choosefilepresets1..5; choice5 choosespreset0.
 * Activepadding/holebytes untouched; no host floatingpointrounding involved. */
gh_filter_status gh_scene_filter_select_002a2818(gh_scene_filters *,unsigned choice,const uint8_t **selected_preset);
/* Actual2993A0 FI2-presentbranch incloriginalcolor/mode overrides. Default
 * noFI2 branch still unsupported; no pretenddefaults or completedrender. */
gh_filter_status gh_scene_filters_initialize_002993a0(gh_scene_filters *,const gh_bundle *);
#ifdef __cplusplus
}
#endif
#endif
