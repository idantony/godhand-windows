#include "game_actor_scroll_links.h"
#include <stdlib.h>
#include <string.h>
struct gh_scene_actor_scroll_state {const uint8_t *akg,*names;size_t count;gh_actor_scroll_entry *entries;};
static uint32_t word(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint64_t key(const uint8_t*p){return word(p)|(uint64_t)word(p+4)<<32;}
static gh_scene_table_status stop(gh_game_scene_tables*s,uint32_t f,int d,gh_scene_table_status e){s->frontier=f;s->detail=d;return e;}
static gh_scene_table_status records(const gh_game_scene_tables*s,size_t*n,gh_scene_table_view*v){
 if(!s||!n||!v)return GH_TABLE_ARGUMENT;*n=0;*v=(gh_scene_table_view){0};if(!s->akg.bytes)return GH_TABLE_OK;
 if(!s->actors_ready||s->akg.size<0x2c)return GH_TABLE_BOUNDS;*n=s->akg.bytes[0x2a]|(unsigned)s->akg.bytes[0x2b]<<8;*v=s->actor_links[7].view;
 if(*n&&(!v->bytes||*n>v->size/8))return GH_TABLE_BOUNDS;return GH_TABLE_OK;
}
void gh_scene_actor_scroll_links_release(gh_game_scene_tables*s){if(s&&s->actor_scroll_native){free(s->actor_scroll_native->entries);free(s->actor_scroll_native);s->actor_scroll_native=NULL;}}
gh_scene_table_status gh_scene_actor_scroll_entry_001333b8(const gh_game_scene_tables*s,uint16_t index,gh_actor_scroll_entry*out){
 if(!s||!out)return GH_TABLE_ARGUMENT;size_t n;gh_scene_table_view v;gh_scene_table_status e=records(s,&n,&v);if(e)return e;if(index>=n)return GH_TABLE_BOUNDS;
 if(s->actor_scroll_native){const struct gh_scene_actor_scroll_state*a=s->actor_scroll_native;if(a->akg!=s->akg.bytes||a->names!=v.bytes||a->count!=n)return GH_TABLE_ARGUMENT;*out=a->entries[index];}
 else{uint64_t k=key(v.bytes+(size_t)index*8);*out=(gh_actor_scroll_entry){k,k,0,0};}return GH_TABLE_OK;
}
gh_scene_table_status gh_scene_search_scroll_002c5420(const gh_room_pools*p,const gh_scene_meshes*m,uint64_t wanted,uint32_t*out){
 if(!p||!m||!out||m->pools!=p)return GH_TABLE_ARGUMENT;const gh_room_vector*v;if(gh_room_registry_view(p,0x754c38,&v)!=GH_POOL_OK)return GH_TABLE_BOUNDS;
 const uint8_t*b=v->allocation.pointer;
 for(uint32_t i=0;i<v->count;i++){uint32_t h=word(b+(size_t)i*4);void*raw=NULL;if(gh_room_object_lookup(p,h,GH_ROOM_OBJECT_SCROLL_MESH,m,&raw)!=GH_POOL_OK)return GH_TABLE_UNSUPPORTED;
  const gh_scene_mesh_object*o=raw;const gh_scene_mesh_descriptor*d=&o->descriptor;size_t offset=d->mesh.descriptor_offset;
  if(!o->constructor_complete||o->handle!=h||!d->script.bytes||offset>d->script.size||d->script.size-offset<16)return GH_TABLE_BOUNDS;
  if(key(d->script.bytes+offset+8)==wanted){*out=h;return GH_TABLE_OK;}
 }*out=0;return GH_TABLE_OK;
}
gh_scene_table_status gh_scene_actor_scroll_links_native_00307a60(gh_game_scene_tables*s,const gh_room_pools*p,const gh_scene_meshes*m){
 if(!s)return GH_TABLE_ARGUMENT;s->frontier=0;size_t n;gh_scene_table_view v;gh_scene_table_status e=records(s,&n,&v);if(e)return stop(s,0x1333b8,0,e);
 if(n){if(!p||!m)return stop(s,0x2c5420,0,GH_TABLE_ARGUMENT);
  struct gh_scene_actor_scroll_state*a=s->actor_scroll_native;
  if(a&&(a->akg!=s->akg.bytes||a->names!=v.bytes||a->count!=n))return stop(s,0x1333b8,0,GH_TABLE_ARGUMENT);
  if(!a){a=calloc(1,sizeof(*a));if(!a)return stop(s,0x307a60,0,GH_TABLE_MEMORY);a->entries=calloc(n,sizeof(*a->entries));if(!a->entries){free(a);return stop(s,0x307a60,0,GH_TABLE_MEMORY);}a->akg=s->akg.bytes;a->names=v.bytes;a->count=n;for(size_t i=0;i<n;i++){uint64_t k=key(v.bytes+i*8);a->entries[i]=(gh_actor_scroll_entry){k,k,0,0};}s->actor_scroll_native=a;}
  for(size_t i=0;i<n;i++){gh_actor_scroll_entry*r=a->entries+i;if(!(uint32_t)r->current_key)continue;uint32_t handle=0;e=gh_scene_search_scroll_002c5420(p,m,r->current_key,&handle);if(e)return stop(s,0x2c5420,(int)i,e);if(handle){r->current_key=0;r->scroll_handle=handle;r->resolved=1;}}
 }
 for(unsigned i=32;i;i--)s->scroll_link_words[i-1]=0;return GH_TABLE_OK;
}

