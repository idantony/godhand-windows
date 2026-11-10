/* Actual patched SLUS_215.03 1C2C40, VT303 A8/B8 ->1C2B80/1C2C00.
 * Field5E4/5F4 gates are genuine; no effect or sound acknowledgment. */
#include "game_om_reaction.h"
#include <string.h>
static uint32_t word(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static unsigned half(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
gh_object_step_status gh_om_reaction_001c2c40(const gh_title_player_view*a,uint32_t*f){
 if(f)*f=0;if(!a||!a->bytes||a->size<0x610||!a->alias_f0)return GH_OBJSTEP_ARGUMENT;uint8_t*p=a->bytes;
 if(p[0x2f5]==0){
  if(word(p+0x5e4)){if(f)*f=0x306838;return GH_OBJSTEP_UNSUPPORTED;}
  if(half(p+0x5f4)<0x8000){if(f)*f=0x2cba48;return GH_OBJSTEP_UNSUPPORTED;}
  uint16_t three=3;memcpy(p+0x60e,&three,2);p[0x2f5]=1;p[0x2f6]=p[0x2f7]=0;
 }else if(p[0x2f5]!=1)return GH_OBJSTEP_OK;
 uint16_t next=(uint16_t)(half(p+0x60e)-1u);memcpy(p+0x60e,&next,2);
 /* SLL16/BGTZ tests the stored lowhalf as signed, not unsigned >0. */
 if(!next||next>=0x8000)p[0x2f4]=p[0x2f5]=p[0x2f6]=p[0x2f7]=0;
 return GH_OBJSTEP_OK;
}
