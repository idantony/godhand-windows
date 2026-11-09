/* MIT LICENSE.upstream; actualcSceAtManager initialization slice. */
#include "game_scene_at.h"
static unsigned h(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static uint32_t w(const uint8_t*p){return (uint32_t)h(p)|(uint32_t)h(p+2)<<16;}
static gh_scene_at_status fail(gh_scene_at_manager*s,uint32_t f,int d,gh_scene_at_status e){s->frontier=f;s->detail=d;return e;}
gh_scene_at_status gh_scene_at_initialize_002c0f90(gh_scene_at_manager*s,const gh_bundle*b){
 static const char*const names[]={"SCA","MEH","SCI","SCL"};
 if(!s||!b)return GH_AT_ARGUMENT;s->ready=0;s->frontier=0;s->detail=0;
 /* Originalclearsu0/u1 only. Invalidversionh2/h3 retainearlierpointer,
  * so do notsilentlyclear allfourunitviews onsubsequentinitialization. */
 s->units[0]=(gh_at_unit_view){0};s->units[1]=(gh_at_unit_view){0};
 s->heads[0]=(gh_at_link){GH_AT_LINK_END,0};
 for(unsigned i=1;i<16;i++)s->heads[i]=(gh_at_link){GH_AT_LINK_SLOT,i-1};
 for(unsigned i=0;i<4;i++){
  gh_bundle_entry e;gh_bundle_error r=gh_bundle_find(b,names[i],0,&e);
  if(r==GH_BUNDLE_NOT_FOUND){s->headers[i]=(gh_bundle_entry){0};continue;}
  if(r||e.size<8)return fail(s,0x31ccf0,(int)i,GH_AT_BOUNDS);
  s->headers[i]=e;
  if(h(e.bytes+4)!=0x100)continue;
  if(e.size<16)return fail(s,0x2c0f90,(int)i,GH_AT_BOUNDS);
  unsigned count=h(e.bytes+6);s->units[i]=(gh_at_unit_view){e.bytes+16,e.size-16,(uint16_t)count};
  if(count)return fail(s,0x2c3040,(int)i,GH_AT_UNSUPPORTED);
 }
 /*2C12C8 clears8pointers beforelookup; actualR020 JMPcount0. */
 for(unsigned i=0;i<8;i++)s->jump_slots[i]=(gh_at_unit_view){0};
 gh_bundle_entry jump;gh_bundle_error r=gh_bundle_find(b,"JMP",0,&jump);
 if(r==GH_BUNDLE_OK){if(jump.size<4)return fail(s,0x2c12c8,0,GH_AT_BOUNDS);if(w(jump.bytes))return fail(s,0x2c12c8,0,GH_AT_UNSUPPORTED);}
 else if(r!=GH_BUNDLE_NOT_FOUND)return fail(s,0x31ccf0,r,GH_AT_BOUNDS);
 /*2C2D20 walks thetypedheads: there aregenuinely noUNITlinks inthisdomain,
  * thus no state/camera callback runs. Originaltailzeroesfollow. */
 s->field60=s->field68=s->field6c=s->field70=0;s->field64=0;s->ready=1;return GH_AT_OK;
}
