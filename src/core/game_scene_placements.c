/* ActualR020 placementbindings; MIT LICENSE.upstream. */
#include "game_scene_placements.h"
#include <string.h>
static uint32_t w(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
gh_scene_placement_status gh_em_scene_room_init_00294b38(gh_em_scene_placements*s,const gh_boot_work*b,uint8_t scenario14){
 if(!s||!s->stop_flag||!b)return GH_PLACE_ARGUMENT;
 s->file=(gh_bundle_entry){0};s->source_entries=(gh_bundle_entry){0};s->frontier=0;
 if(!(b->flags&0x01000000u)&&scenario14!=1){memset(s->scratch10,0,0x30);s->room04=0xffff;}
 return GH_PLACE_OK;
}
gh_scene_placement_status gh_em_scene_bind_00294c68(gh_em_scene_placements*s,gh_room_state*r,const gh_bundle_entry*e,int*out){
 if(!s||!s->stop_flag||!r||!r->active||r->size<9||!out)return GH_PLACE_ARGUMENT;
 *out=0;s->frontier=0;
 if(w(r->active)&0x80000000u){*out=1;if(*s->stop_flag==1){*s->stop_flag=0;return GH_PLACE_OK;}s->frontier=0x294ca0;return GH_PLACE_UNSUPPORTED;}
 if(!e||!e->bytes)return GH_PLACE_OK;if(e->size<8)return GH_PLACE_BOUNDS;
 s->file=*e;if(w(e->bytes)!=0x534d45){s->file=(gh_bundle_entry){0};return GH_PLACE_OK;}
 if(!w(e->bytes+4)){r->active[8]=0;put(r->active,w(r->active)|0x80000000u);s->file=(gh_bundle_entry){0};return GH_PLACE_OK;}
 if(e->size<12)return GH_PLACE_BOUNDS;
 s->source_entries=*e;s->source_entries.bytes+=12;s->source_entries.offset+=12;s->source_entries.size-=12;
 s->frontier=0x2bede8;return GH_PLACE_UNSUPPORTED;
}
void gh_om_scene_construct_002a7308(gh_om_scene_placements*s){if(s){s->file=(gh_bundle_entry){0};s->entries=(gh_bundle_entry){0};s->frontier=0;}}
gh_scene_placement_status gh_om_scene_bind_001c3cb8(gh_om_scene_placements*s,const gh_bundle_entry*e,int*out){
 if(!s||!out)return GH_PLACE_ARGUMENT;*out=0;s->frontier=0;
 if(!e||!e->bytes)return GH_PLACE_OK;if(e->size<12)return GH_PLACE_BOUNDS;
 s->file=*e;if(w(e->bytes)!=0x534d4f||!w(e->bytes+4)){s->file=(gh_bundle_entry){0};return GH_PLACE_OK;}
 size_t offset=w(e->bytes+8)<2?12:16;if(offset>e->size)return GH_PLACE_BOUNDS;
 s->entries=*e;s->entries.bytes+=offset;s->entries.offset+=offset;s->entries.size-=offset;
 s->frontier=0x1c3cb8;return GH_PLACE_UNSUPPORTED;
}
gh_scene_placement_status gh_acd_scene_bind_00137ae8(gh_acd_scene_binding*s,const gh_bundle_entry*e,int*out){
 if(!s||!out)return GH_PLACE_ARGUMENT;*out=0;s->frontier=0;
 if(e&&e->bytes){if(e->size<0x2c)return GH_PLACE_BOUNDS;memcpy(s->scalar04,e->bytes,0x2c);
  if(w(s->scalar04)==0x444341){size_t offset=w(s->scalar04+4)==1?0x18:0x2c;
   s->records=e->bytes+offset;s->record_bytes=e->size-offset;
   if(w(s->scalar04+4)==1)put(s->scalar04+0x18,0x3f87ae14);
   *out=1;return GH_PLACE_OK;
  }
 }
 s->records=0;s->record_bytes=0;s->field30=0;memset(s->scalar04,0,0x2c);put(s->scalar04+0x18,0x3f87ae14);return GH_PLACE_OK;
}
