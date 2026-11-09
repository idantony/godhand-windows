#ifndef GH_PACKED_MESSAGE_H
#define GH_PACKED_MESSAGE_H
#include "id_base.h"
#include "font_state.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_id_view cursor;unsigned char environment[0x64];uint8_t initialized;} gh_packed_message_state;
/* Actual2A9F30: lookup(slot,arg+1), self.packed, slot3 MST/MSA, cursorMSD,
 * reset shared message environment then set mode2. Caller owns source data.
 * Missing packed returns original0; success returns original1 separately.
 * The environment is original guest-shaped scalar/null-pointer bytes only;
 * no guest pointer is executed or stored as a host pointer. */
gh_id_error gh_packed_message_bind(gh_id_base *,const gh_id_state *,gh_font_state *,
 gh_packed_message_state *,const gh_bundle *language,int32_t language_selector,
 uint32_t resource_slot,int32_t arg,int *original_return);
#ifdef __cplusplus
}
#endif
#endif
