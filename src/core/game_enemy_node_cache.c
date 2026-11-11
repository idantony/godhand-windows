#include "game_enemy_node_cache.h"
#include <string.h>
static uint32_t word(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static uint16_t half(const uint8_t*p){uint16_t v;memcpy(&v,p,2);return v;}
static int mapped(const uint8_t*b,size_t n,uint32_t table,unsigned stride,unsigned count,uint32_t va,void*out,size_t bytes){for(unsigned i=0;i<count;i++){const uint8_t*p=b+table+(size_t)i*stride;uint32_t o=word(p+4),v=word(p+8),z=word(p+16);if(word(p)!=1||va<v||(uint64_t)va+bytes>(uint64_t)v+z)continue;if(o>n||z>n-o)return 0;memcpy(out,b+o+(va-v),bytes);return 1;}return 0;}
gh_player_nodes_status gh_enemy_node_names_from_elf(gh_enemy_node_names*s,const void*raw,size_t n){
 if(!s||!raw||s->initialized)return GH_PLAYER_NODES_ARGUMENT;const uint8_t*b=raw;if(n<52)return GH_PLAYER_NODES_BOUNDS;if(memcmp(b,"ELF",7)||half(b+18)!=8)return GH_PLAYER_NODES_UNSUPPORTED;
 uint32_t table=word(b+28);unsigned stride=half(b+42),count=half(b+44);if(stride<32||count>128||table>n||count>(n-table)/stride)return GH_PLAYER_NODES_BOUNDS;
 uint32_t first;if(!mapped(b,n,table,stride,count,0x14b730,&first,4))return GH_PLAYER_NODES_BOUNDS;if(first!=0x80a20000)return GH_PLAYER_NODES_UNSUPPORTED;
 gh_enemy_node_names next={0};for(unsigned i=0;i<11;i++){uint32_t va=i<8?0x42c9e0+i*8:i==8?0x42ca28:i==9?0x42cab0:0x42cab8;if(!mapped(b,n,table,stride,count,va,next.names[i],8))return GH_PLAYER_NODES_BOUNDS;}next.initialized=1;*s=next;return GH_PLAYER_NODES_OK;
}
gh_player_nodes_status gh_enemy_node_cache_initialize_002055b8(gh_enemy_node_cache*s,gh_enemy_solids*solids,const gh_enemy_node_names*names){
 if(!s||!solids||solids->stage!=4||!solids->shadow||!names||!names->initialized)return GH_PLAYER_NODES_ARGUMENT;
 if(s->stage&&(s->solids!=solids||s->names!=names))return GH_PLAYER_NODES_ARGUMENT;
 gh_enemy_entry*e=solids->shadow->pose->motion->hp->tail->entry;gh_actor_model*m=&e->initialize.model.model;uint8_t*p=e->object.view.bytes;
 if(!s->stage){s->solids=solids;s->names=names;s->stage=1;}
 if(s->stage==1){for(;s->cursor<11;s->cursor++){unsigned i=s->cursor;int original;gh_player_nodes_status status=gh_actor_model_find_mesh_0014b730(m,names->names[i],8,&s->nodes[i],&original);if(status){s->frontier=0x14b730;return status;}
   if(i<8){uint32_t zero=0;memcpy(p+0x1774+i*4,&zero,4);}else if(i<10){uint32_t zero=0;memcpy(p+(i==8?0x1794:0x179c),&zero,4);}
   if(i>=8&&s->nodes[i].bytes){if(gh_actor_model_node_or_flags(m,(unsigned)s->nodes[i].mesh_index,0x380,1)){s->frontier=0x14b730;return GH_PLAYER_NODES_UNSUPPORTED;}}
   if(i==8){uint32_t zero=0;memcpy(p+0x1798,&zero,4);}
  }
  p[0x17c3]=0;if(word(p+0x1644)&0x20000000){uint16_t flags=half(p+0x434)|2;p[0x17c3]=1;memcpy(p+0x434,&flags,2);}s->stage=2;
 }
 s->frontier=0x205ae4;e->frontier=s->frontier;return GH_PLAYER_NODES_UNSUPPORTED;
}
