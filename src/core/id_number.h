#ifndef GH_ID_NUMBER_H
#define GH_ID_NUMBER_H
#include "id_jobs.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual2AAFF0->2C70A8->2C6D48 decimal sprite rendering. Every returned
 * quad is a DISTINCT OT packet, in original left-to-right enqueue order.
 * At most9digits plusminus. Explicit finite nativebinary32 reference. */
gh_id_error gh_id_number_prepare(const gh_id_state *,const gh_id_base *,
 const gh_id_entry *,gh_id_texture_quad *,size_t capacity,size_t *count,uint32_t *frontier);
gh_id_error gh_id_number_texture_rgba(const gh_id_texture *,void *,size_t);
/* Bounded observed numeric-atlas materials:128x128TIM2/T4 and T32/T8. */
gh_id_error gh_id_number_texture_open(const gh_id_state *,uint32_t slot,uint32_t one_based,gh_id_texture *);
#ifdef __cplusplus
}
#endif
#endif
