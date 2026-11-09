#include "game_prepare.h"
#include <string.h>
static void zero16(uint8_t *p){p[0]=p[1]=0;}
gh_init_status gh_game_prepare_002a5d40(gh_game_prepare *s,gh_core_save_view save,int16_t room,uint8_t *state){
 if(!s||!state||(!save.bytes&&save.size)||(save.bytes&&save.size<GH_CORE_SAVE_BYTES))return GH_INIT_ARGUMENT;
 /* Original CoreSave1F9B10, in original call order. */
 if(save.bytes){
  for(unsigned i=0;i<5;i++)zero16(save.bytes+0x8c+i*2); /*1FBDD0*/
  zero16(save.bytes+0x96); /*1FBEB8*/zero16(save.bytes+0x12); /*1FBF60*/
  for(unsigned i=0;i<128;i++){uint8_t *p=save.bytes+0x28a+i*0x10;p[0]=p[1]=255;} /*1FAFA8*/
  save.bytes[0x288]=1;save.bytes[0x289]=0;
 }
 s->global_0061a994=0; /*2D9D48(61A990,0) writes+4 only*/
 if(room!=0x20&&save.bytes)memcpy(save.bytes+0x84,save.bytes+0x80,4); /*1FAC80/1FAC98*/
 /*2BEB80 clears two distinct ranges, preserving+4word and1658..165Fgap. */
 memset(s->fields+0x1660,0,0x9c30);memset(s->fields+8,0,0x1650);memset(s->fields,0,4);
 /*2CABE0 is verified jr ra;nop, not an unimplemented successful callback. */
 *state=3;return GH_INIT_OK;
}
