#ifndef GH_GAME_CAMERA_H
#define GH_GAME_CAMERA_H
#include "game_init.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_CAMERA_EXACT_UNAVAILABLE=0,GH_CAMERA_NATIVE_BINARY32_REFERENCE=1} gh_camera_numeric_policy;
typedef enum {GH_CAMERA_OK,GH_CAMERA_ARGUMENT,GH_CAMERA_BUSY,GH_CAMERA_UNSUPPORTED,GH_CAMERA_NUMERIC} gh_camera_status;
typedef struct {gh_game_camera *identity;uint8_t fields[0x530];gh_camera_numeric_policy policy;
 int constructed;uint32_t failed_entry;} gh_game_pl_camera;
/* Shared original1F8180 polynomial, explicit finite binary32 reference.
 * No libc atan or angle reduction. Output atomic and caller FEnv preserved. */
gh_camera_status gh_game_angle_001f8180(float y,float x,float *out,gh_camera_numeric_policy,uint32_t *frontier);/* Cold original139BC8 prefix construction, not the whole CamManager (0x1030 bytes).
 * identity is the EXISTING game_init.camera handle atoriginal463050; mode,
 * previousmode and previous/active identity stay canonical in game_init. */
gh_camera_status gh_game_camera_construct_pl_00139bc8(gh_game_pl_camera *,gh_game_camera *identity,gh_camera_numeric_policy);
/* CamManager+530 invokes the same base constructor, then the motion subtype. */
gh_camera_status gh_game_camera_construct_motion_00138be8(gh_game_pl_camera *,gh_game_camera *identity,gh_camera_numeric_policy);
/* Original138210 vdispatch ->actual41DBB0+8delta0/+C139090->138240.
 * Explicit nativebinary32 reference policy restores originalpolynomials and
 * operationorder, not a claim ofVU MAC/FDIV/SQRT hardwarebitidentity.
 * Only this recovered PlCamera subtype is accepted; others reportfrontier. */
gh_camera_status gh_game_camera_reset_00138210(gh_game_pl_camera *,const gh_game_camera *active);
/* Original early update 138F98 reads canonical CamManager+1028 only.
 * Mode bit4 delegates to real cCamera_move; other modes do no work here. */
/* Actual138D40 with source argument0: canonical mode/previous/active links;
 * no matrix-copy branch, allocation, projection or camera reset is implied. */
gh_camera_status gh_game_camera_restore_00138d40(gh_game_init_state *);
gh_camera_status gh_game_camera_before_move_00138f98(const gh_game_init_state *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
