#ifndef GH_GAME_SCENE_AT_H
#define GH_GAME_SCENE_AT_H
#include "../assets/core_bundle.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original2C3000/2C3040 taggedpointer roles become explicitnative linkkinds.
 * END representsFFFFFFFF; SLOT ispreviouslisthead address withtopbitclear;
 * UNIT isoriginalunitpointer|80000000, not implemented as a hostaddressbit. */
typedef enum {GH_AT_LINK_END,GH_AT_LINK_SLOT,GH_AT_LINK_UNIT} gh_at_link_kind;
typedef struct {gh_at_link_kind kind;uint32_t index;} gh_at_link;
typedef struct {const uint8_t *bytes;size_t size;uint16_t count;} gh_at_unit_view;
struct gh_scene_at_units;
typedef struct {gh_bundle_entry headers[4];gh_at_unit_view units[4];
 gh_at_link heads[16];gh_at_unit_view jump_slots[8];
 uint32_t field60,field68,field6c,field70;uint8_t field64,ready;
 uint32_t frontier;int detail;struct gh_scene_at_units *native_units;} gh_scene_at_manager;
typedef enum {GH_AT_OK,GH_AT_ARGUMENT,GH_AT_BOUNDS,GH_AT_UNSUPPORTED} gh_scene_at_status;
/* Actual2C0F90 zero-record domain, includingrealR020 SCA/MEH/SCI/SCL/JMP.
 * One canonicalinitiallyBSS-zero owner. Positiveunits/jumps require real
 * unknown consumers andstop; nofakeunitinit/camera/area/scheduler callbacks. */
gh_scene_at_status gh_scene_at_initialize_002c0f90(gh_scene_at_manager *,const gh_bundle *);
#ifdef __cplusplus
}
#endif
#endif
