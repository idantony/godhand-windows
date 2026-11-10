#ifndef GH_GAME_PAD_ANALOG_H
#define GH_GAME_PAD_ANALOG_H
#include "boot.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_PAD_ANALOG_OK,GH_PAD_ANALOG_ARGUMENT,GH_PAD_ANALOG_STALE} gh_pad_analog_status;
typedef struct {
 const gh_boot_work *source;
 uint32_t digital_raw,raw_held,directions,old_directions,pressed_directions,mode_word10;
 uint8_t wire[4],inverted[4],previous_inverted[4];
 int8_t axes[4]; /* LX,LY,RX,RY -> sameWork1F0..1F3 */
} gh_pad_analog_result;
/* Actual31AF70: signed8(lowbyte-128), zero only[-63,63]. */
int8_t gh_pad_center_deadzone_0031af70(uint8_t);
/* Wire packetbytes4..7 = RX,RY,LX,LY, unsigned0..255 (drivercenter128).
 * Evaluates actual31B560 integer axes, cross-axis rescue and synthesizedraw
 * buttons, using SAME Work's PREVIOUS60..63 and Word10. digital_raw is the
 * actual active-high lower16buttonword, before any synthesizedstick bits.
 * Platform must derive edge/repeat/mappedbuttons from result.raw_held.
 * No radial deadzone, unit-circle normalization or hostcontroller velocity. */
gh_pad_analog_status gh_pad_analog_evaluate_0031b560(const gh_boot_work *,const uint8_t wire[4],uint32_t digital_raw,gh_pad_analog_result *);
/* Commits the axis-only31B560/31AFE0 slice to the SAME canonical Work:
 *60..63 invertedwire,70..73 processedaxes,1E0..1F3 20Bcopy,180/184/188dir.
 * Leaves raw/mappedbutton publication tocaller. Rejects stale/forgedresult
 * beforewrites. This is NOT whole31AFE0: angles74/78->1F4/1F8,31B2E0
 * turnscore,pressurewirecopy,replay/hold hooks remain outside this leaf. */
gh_pad_analog_status gh_pad_analog_commit_0031afe0(gh_boot_work *,const gh_pad_analog_result *);
#ifdef __cplusplus
}
#endif
#endif
