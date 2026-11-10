/* MIT LICENSE.upstream; exactscene resourcebinding leaves. */
#include "game_scene_tables.h"
#include "game_actor_scroll_links.h"
#include <string.h>
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static unsigned u16(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static void work32(gh_boot_work*w,size_t off,uint32_t v){memcpy((uint8_t*)w+off,&v,4);}
static gh_scene_table_status fail(gh_game_scene_tables*s,uint32_t f,int d,gh_scene_table_status e){s->frontier=f;s->detail=d;return e;}
gh_scene_table_status gh_scene_environment_002a71b0(gh_game_scene_tables*s,gh_boot_work*w,const gh_bundle*b){
 if(!s||!w||!b)return GH_TABLE_ARGUMENT;gh_bundle_entry e;s->frontier=0;
 gh_bundle_error r=gh_bundle_find(b,"ENV",0,&e);
 if(r==GH_BUNDLE_OK){s->environment=(gh_scene_table_view){e.bytes,e.size};
  work32(w,0x608,GH_SCENE_ENVIRONMENT_NATIVE_HANDLE);return GH_TABLE_OK;}
 if(r!=GH_BUNDLE_NOT_FOUND)return fail(s,0x31ccf0,r,GH_TABLE_BOUNDS);
 s->environment=(gh_scene_table_view){0};work32(w,0x608,0);return GH_TABLE_OK;
}
gh_scene_table_status gh_scene_scrolls_0031d1b0(gh_game_scene_tables*s,gh_boot_work*w,const gh_bundle*b){
 if(!s||!w||!b)return GH_TABLE_ARGUMENT;s->frontier=0;
 ((uint8_t*)w)[0x600]=0; /*actual31D1E4 SB; preserve601..603*/
 for(unsigned i=32;i;i--){s->scroll_pairs[i-1][0]=s->scroll_pairs[i-1][1]=0;s->scroll_words[i-1]=0;}
 gh_bundle_entry e;gh_bundle_error r=gh_bundle_find(b,"SCP",0,&e);
 if(r==GH_BUNDLE_OK)return fail(s,0x31d1b0,0,GH_TABLE_UNSUPPORTED);
 if(r!=GH_BUNDLE_NOT_FOUND)return fail(s,0x31ccf0,r,GH_TABLE_BOUNDS);
 /* SearchData(NULL,T3L/TM3/SCR) actuallyreturnsNULL inoriginal. */
 s->scroll_texture=(gh_scene_table_view){0};work32(w,0x5fc,0);return GH_TABLE_OK;
}
gh_scene_table_status gh_scene_actors_0012e670(gh_game_scene_tables*s,const gh_bundle*b){
 if(!s||!b)return GH_TABLE_ARGUMENT;
 if(s->akg.bytes)return s->actors_ready?GH_TABLE_OK:GH_TABLE_UNSUPPORTED;
 s->frontier=0;gh_bundle_entry akt,akg;gh_bundle_error r=gh_bundle_find(b,"AKT",0,&akt);
 if(r==GH_BUNDLE_NOT_FOUND)return GH_TABLE_OK;if(r)return fail(s,0x31ccf0,r,GH_TABLE_BOUNDS);
 gh_bundle child;if(gh_bundle_open(akt.bytes,akt.size,&child)!=GH_BUNDLE_OK)return fail(s,0x31ccf0,0,GH_TABLE_BOUNDS);
 r=gh_bundle_find(&child,"AKG",0,&akg);if(r==GH_BUNDLE_NOT_FOUND)return GH_TABLE_OK;if(r||akg.size<0x66)return fail(s,0x12e670,r,GH_TABLE_BOUNDS);
 gh_scene_actor_link links[8];
 for(unsigned i=0;i<8;i++){
  uint32_t raw=u32(akg.bytes+4*i);int64_t rel=raw<0x80000000u?(int64_t)raw:(int64_t)raw-INT64_C(0x100000000);
  int64_t offset=(int64_t)akg.offset+rel;
  if(offset<0||(uint64_t)offset>akt.size)return fail(s,0x12e670,(int)i,GH_TABLE_BOUNDS);
  links[i]=(gh_scene_actor_link){raw,{akt.bytes+(size_t)offset,akt.size-(size_t)offset}};
 }
 s->akt=akt;s->akg=akg;memcpy(s->actor_links,links,sizeof(links));
 memcpy(s->actor_vec50,akg.bytes+0x48,12);memcpy(s->actor_vec30,akg.bytes+0x30,12);memcpy(s->actor_vec40,akg.bytes+0x3c,12);
 s->actor_count60=u16(akg.bytes+0x60);s->actor_count64=u16(akg.bytes+0x62);s->actor_count68=u16(akg.bytes+0x64);
 if(s->pending_actor_head)return fail(s,0x12ecf0,0,GH_TABLE_UNSUPPORTED);
 s->actors_ready=1;return GH_TABLE_OK;
}
gh_scene_table_status gh_scene_actor_scroll_links_00307a60(gh_game_scene_tables*s){
 if(!s)return GH_TABLE_ARGUMENT;s->frontier=0;
 if(s->akg.bytes){if(s->akg.size<0x2c)return fail(s,0x307a60,0,GH_TABLE_BOUNDS);
  if(u16(s->akg.bytes+0x2a))return fail(s,0x2c5420,0,GH_TABLE_UNSUPPORTED);}
 for(unsigned i=32;i;i--)s->scroll_link_words[i-1]=0;
 return GH_TABLE_OK;
}

gh_scene_table_status gh_scene_actors_reset_0012e800(gh_game_scene_tables*s){
 if(!s)return GH_TABLE_ARGUMENT;
 gh_scene_actor_scroll_links_release(s);s->akg=(gh_bundle_entry){0};memset(s->actor_links,0,sizeof(s->actor_links));s->actors_ready=0;
 /* akt is the bounded containing asset, not an original additional pointer
  * field; retain its provenance view until the actual room asset release. */
 if(s->pending_actor_head)return fail(s,0x12ecf0,0,GH_TABLE_UNSUPPORTED);
 s->frontier=0;return GH_TABLE_OK;
}

