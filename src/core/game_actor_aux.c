/* Patched SLUS_215.03 14A3E8 tail/1495A0 lifetime evidence. */
#include "game_actor_aux.h"
#include <string.h>
static uint32_t word(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
gh_actor_model_status gh_actor_auxiliary_0014a3e8(const gh_actor_model*m,unsigned parity,unsigned*out,uint32_t*f){
 if(f)*f=0;if(!m||!out||parity>1||!m->bound||m->native_identity!=m||!m->root.bytes||m->root.size<0x29c)return GH_ACTOR_MODEL_ARGUMENT;
 const gh_actor_object_packets*p;gh_actor_model_status e=gh_actor_model_object_packet(m,&p);if(e)return e;
 if(!p->auxiliary_constructor_bound||p->auxiliary_owner!=m){if(f)*f=0x31a600;return GH_ACTOR_MODEL_UNSUPPORTED;}
 for(unsigned n=2;n;n--){unsigned kind=n-1,index=kind*2+parity;if(!(p->auxiliary_absent_mask&(1u<<index))||word(m->root.bytes+0x28c+index*4)){if(f)*f=0x31a600;return GH_ACTOR_MODEL_UNSUPPORTED;}}
 *out=0;return GH_ACTOR_MODEL_OK;
}
