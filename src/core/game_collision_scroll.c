#include "game_collision_scroll.h"
#include <stdlib.h>
#include <string.h>
static uint16_t h(const uint8_t*p){return (uint16_t)(p[0]|(unsigned)p[1]<<8);}
static uint32_t w(const uint8_t*p){return (uint32_t)h(p)|(uint32_t)h(p+2)<<16;}
static void ww(uint8_t*p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8));}
static uint64_t key(const char*p){uint64_t v=0;for(unsigned i=0;i<8&&p[i];i++)v|=(uint64_t)(int64_t)(signed char)p[i]<<(8*i);return v;}
static uint64_t record_key(const uint8_t*p){uint64_t v=0;for(unsigned i=0;i<8&&p[i];i++)v|=(uint64_t)(int64_t)(int8_t)p[i]<<(8*i);return v;}
gh_scene_table_status gh_collision_scroll_set_enabled_001332d0(gh_game_scene_tables*s,gh_collision_scroll_mutation*m,const char*name,int enabled,uint32_t*changed){
 if(changed)*changed=0;if(!s||!m||!name||!changed)return GH_TABLE_ARGUMENT;
 if(!s->akg.bytes)return GH_TABLE_OK;if(!s->actors_ready||s->akg.size<0x22)return GH_TABLE_ARGUMENT;
 if(m->owned&&(m->tables!=s||m->source_akg!=s->akg.bytes||s->actor_links[4].view.bytes!=m->owned))return GH_TABLE_ARGUMENT;
 unsigned count=h(s->akg.bytes+0x20);gh_scene_table_view names=s->actor_links[0].view,tri=s->actor_links[4].view;
 if((size_t)count>names.size/20)return GH_TABLE_BOUNDS;uint64_t wanted=key(name);
 for(unsigned i=0;i<count;i++){const uint8_t*p=names.bytes+(size_t)i*20;if(record_key(p+12)!=wanted)continue;unsigned start=h(p+8),n=h(p+10);if((uint64_t)start+n>tri.size/20)return GH_TABLE_BOUNDS;if(!n)return GH_TABLE_OK;
  if(!m->owned){uint8_t*copy=malloc(tri.size);if(!copy)return GH_TABLE_BOUNDS;memcpy(copy,tri.bytes,tri.size);m->tables=s;m->source_akg=s->akg.bytes;m->original=tri;m->owned=copy;m->bytes=tri.size;s->actor_links[4].view=(gh_scene_table_view){copy,tri.size};}
  for(unsigned j=0;j<n;j++){uint8_t*q=m->owned+((size_t)start+j)*20+16;ww(q,enabled?w(q)&~2u:w(q)|2u);}*changed=n;return GH_TABLE_OK;
 }return GH_TABLE_OK;
}
gh_scene_table_status gh_collision_scroll_mutation_release(gh_collision_scroll_mutation*m){if(!m)return GH_TABLE_ARGUMENT;if(m->owned&&m->tables&&m->tables->actor_links[4].view.bytes==m->owned)m->tables->actor_links[4].view=m->original;free(m->owned);memset(m,0,sizeof(*m));return GH_TABLE_OK;}
