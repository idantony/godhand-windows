/* ActualR020 placementbindings; MIT LICENSE.upstream. */
#include "game_scene_placements.h"
#include <string.h>
#include "game_om_factory.h"
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
void gh_om_scene_construct_002a7308(gh_om_scene_placements*s){if(s)*s=(gh_om_scene_placements){0};}
gh_scene_placement_status gh_om_scene_bind_001c3cb8(gh_om_scene_placements*s,const gh_bundle_entry*e,int*out){
 if(!s||!out)return GH_PLACE_ARGUMENT;*out=0;s->frontier=0;
 if(!e||!e->bytes)return GH_PLACE_OK;if(e->size<12)return GH_PLACE_BOUNDS;
 s->file=*e;if(w(e->bytes)!=0x534d4f||!w(e->bytes+4)){s->file=(gh_bundle_entry){0};return GH_PLACE_OK;}
 size_t offset=w(e->bytes+8)<2?12:16;if(offset>e->size)return GH_PLACE_BOUNDS;
 s->entries=*e;s->entries.bytes+=offset;s->entries.offset+=offset;s->entries.size-=offset;
 s->count=w(e->bytes+4);s->version=w(e->bytes+8);
 if(s->count>65535||s->count>s->entries.size/64)return GH_PLACE_BOUNDS;
 s->frontier=0x2bed08;return GH_PLACE_UNSUPPORTED;
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

static uint64_t qword(const uint8_t*p){return (uint64_t)w(p)|(uint64_t)w(p+4)<<32;}
gh_scene_placement_status gh_om_room_slot_used_002bed08(const gh_room_state*r,uint64_t key,int*out){
 if(!r||!r->active||r->size<0x1610||!out)return GH_PLACE_ARGUMENT;*out=0;if(!key)return GH_PLACE_OK;
 for(unsigned i=0;i<256;i++){const uint8_t*p=r->active+0x610+i*16;if(qword(p)==key){*out=p[8]==1;return GH_PLACE_OK;}}return GH_PLACE_OK;
}
gh_scene_placement_status gh_om_scene_record_at(const gh_om_scene_placements*s,uint32_t index,gh_om_placement_record*out){
 if(!s||!out||!s->entries.bytes)return GH_PLACE_ARGUMENT;if(index>=s->count||index>s->entries.size/64||64>s->entries.size-(size_t)index*64)return GH_PLACE_BOUNDS;
 const uint8_t*p=s->entries.bytes+(size_t)index*64;uint32_t prefix=s->version>=2&&(p[0x2e]&0x80)?0x400:0x300;
 *out=(gh_om_placement_record){p,64,index,prefix|p[0x24],s->version>=2?qword(p+0x30):0,s->version>=2?qword(p+0x38):0,s->version};return GH_PLACE_OK;
}
gh_scene_placement_status gh_om_scene_bind_native_001c3cb8(gh_om_scene_placements*s,const gh_bundle_entry*e,gh_om_factory*f,int*out){
 if(!s||!f||!out)return GH_PLACE_ARGUMENT;*out=0;
 if(!s->native_bound){gh_scene_placement_status p=gh_om_scene_bind_001c3cb8(s,e,out);if(p!=GH_PLACE_UNSUPPORTED)return p;s->native_bound=1;}
 else if(!e||s->file.bytes!=e->bytes||s->file.size!=e->size)return GH_PLACE_BUSY;
 while(s->cursor<s->count){gh_om_placement_record record;gh_scene_placement_status p=gh_om_scene_record_at(s,s->cursor,&record);if(p)return p;int used=0;
  if(s->version>=2){p=gh_om_room_slot_used_002bed08(f->bindings.room_state,record.key,&used);if(p){s->frontier=0x2bed08;return p;}}
  if(used){s->frontier=0x1331b8;return GH_PLACE_UNSUPPORTED;} /* real linkedcollisiondisable notyetrestored */
  int nonnull=0;p=gh_om_factory_create_record(f,&record,&nonnull);if(p){s->frontier=f->frontier;return p;}
  if(nonnull)s->created++;else s->skipped++;s->cursor++;
 }
 s->frontier=0;*out=1;return GH_PLACE_OK;
}
