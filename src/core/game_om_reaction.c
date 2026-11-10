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

/* These four original methods differ only in the halfword offset. */
static gh_object_step_status reaction_countdown15(const gh_title_player_view *actor,
                                                  size_t timer_offset,
                                                  uint32_t *frontier)
{
 if (frontier) *frontier = 0;
 if (!actor || !actor->bytes || actor->size < timer_offset + 2)
  return GH_OBJSTEP_ARGUMENT;
 uint8_t *bytes = actor->bytes;
 uint16_t timer;
 switch (bytes[0x2f5]) {
 case 0:
  timer = 15;
  bytes[0x2f5] = 1;
  bytes[0x2f6] = 0;
  bytes[0x2f7] = 0;
  break;
 case 1:
  timer = (uint16_t)half(bytes + timer_offset);
  break;
 default:
  return GH_OBJSTEP_OK;
 }
 timer = (uint16_t)(timer - 1u);
 memcpy(bytes + timer_offset, &timer, sizeof(timer));
 /* Original SLL16/BGTZ tests the stored halfword as signed. */
 if (!timer || timer >= 0x8000) memset(bytes + 0x2f4, 0, 4);
 return GH_OBJSTEP_OK;
}

gh_object_step_status gh_om_reaction_00188190(const gh_title_player_view *actor,
                                             uint32_t *frontier)
{
 return reaction_countdown15(actor, 0x600, frontier);
}

gh_object_step_status gh_om_segment_reaction(const gh_title_player_view *actor,
                                             uint32_t original_entry,
                                             uint32_t *frontier)
{
 switch (original_entry) {
 case 0x196978:
 case 0x197fa0:
  return reaction_countdown15(actor, 0x9d4, frontier);
 case 0x1a1768:
  return reaction_countdown15(actor, 0xd94, frontier);
 default:
  if (frontier) *frontier = original_entry;
  return GH_OBJSTEP_UNSUPPORTED;
 }
}
