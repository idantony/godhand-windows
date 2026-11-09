#ifndef GODHAND_CORE_ID_PATH_H
#define GODHAND_CORE_ID_PATH_H
#include "id_base.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Exact-only: integral coordinate domain, exact squared distance and integral
 * square roots. Other inputs return UNSUPPORTED at the real numeric boundary.
 * No host FEnv, sqrt approximation or emulator is used. */
gh_id_error gh_id_path_reset_002abc30(gh_id_entry *);
/* Explicit development/reference numeric policy, NOT a hardware guarantee:
 * integral-coordinate domain as above; binary32 SQRT nearest-even followed by
 * binary32 positive ADD toward zero. Matches pinned PCSX2 default choice,
 * including its documented historical-behavior TODO; see evidence report. */
gh_id_error gh_id_path_reset_reference_nearest(gh_id_entry *);
#ifdef __cplusplus
}
#endif
#endif
