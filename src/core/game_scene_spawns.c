/* Actual1FB330 branch/filter; MIT LICENSE.upstream. */
#include "game_scene_spawns.h"
static unsigned h(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static int16_t sh(const uint8_t*p){unsigned n=h(p);return (int16_t)(n<32768?(int)n:(int)n-65536);}
gh_saved_item_spawn_status gh_core_save_spawn_items_001fb330(gh_core_save_view save,
 uint16_t room,gh_saved_item_spawn*pending,uint32_t*f){
 if(f)*f=0;if(pending)*pending=(gh_saved_item_spawn){0};
 if(!(room>>8)||!save.bytes)return GH_SPAWN_OK;
 if(save.size<GH_CORE_SAVE_BYTES)return GH_SPAWN_BOUNDS;
 for(unsigned i=0;i<128;i++){
  const uint8_t*p=save.bytes+0x28a+(size_t)i*16;
  if(h(p)!=room)continue;
  if(pending){pending->index=i;pending->stage=(uint16_t)h(p);pending->kind=(uint16_t)h(p+2);
   pending->flags=p[4];pending->yaw_half_degrees=(int8_t)(p[5]<128?(int)p[5]:(int)p[5]-256);
   pending->serial=(uint16_t)h(p+6);pending->extra=(uint16_t)h(p+14);
   for(unsigned j=0;j<3;j++)pending->position_tenths[j]=sh(p+8+j*2);
  }
  if(f)*f=0x30fa98;return GH_SPAWN_UNSUPPORTED;
 }
 return GH_SPAWN_OK;
}
