#ifndef GH_ID_ENTRY_RESET_H
#define GH_ID_ENTRY_RESET_H
#include "id_jobs.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual2AC9B8: clearbaseframe/B1D/B1E evenifentryNULL; resetselectedentry,
 * calcitsfiveanimations, applyhide mask; thenonlyDIRECTchildren inentryorder.
 * NoRAND/layout/frameadvance/iconpost. Hostbounds requireownedvalidentries.
 * Children alwaysclear7C00, independentofselectedentryhideargument. */
gh_id_error gh_id_entry_reset_family_002ac9b8(gh_id_base *,gh_id_entry *,int16_t hide,gh_id_frontier *);
/* Explicit caller-selected animation reference, including its path scratch
 * seed contract (id_animation.h). NULLprofile retains default frontier. */
gh_id_error gh_id_entry_reset_family_reference(gh_id_base *,gh_id_entry *,int16_t hide,
 const struct gh_id_animation_profile *,gh_id_frontier *);
#ifdef __cplusplus
}
#endif
#endif
