/* Original2D8408 positive-preset path. Scalar storage and native attachment
 * metadata belong to the SAME WorldLight; no second list or guest pointers. */
#include "game_world_light.h"
#include <string.h>
static uint16_t half(const uint8_t*p){return (uint16_t)(p[0]|(unsigned)p[1]<<8);}
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static gh_light_status fail(gh_game_world_light*s,uint32_t pc,int d,gh_light_status status){s->frontier=pc;s->detail=d;return status;}

gh_light_status gh_game_world_light_select_002d8408(gh_game_world_light*s,unsigned index,const gh_light_player_parts*player){
 if(!s||!s->constructed||!s->ready)return GH_LIGHT_ARGUMENT;
 if(index>=8)return GH_LIGHT_BOUNDS;
 if(player&&(player->count>255||(player->count&&!player->parts)))return GH_LIGHT_ARGUMENT;
 const uint8_t*source;size_t count;gh_light_status status=gh_game_world_light_preset_records(s,index,&source,&count);if(status)return status;
 uint8_t*p=s->scalar;size_t old_count=word(p+0x78),preserved=0,added=0;
 if(old_count>256)return fail(s,0x2d8408,0,GH_LIGHT_BOUNDS);
 /* Native preflight prevents claiming a partial selection when the required
  * real player/part owner is unavailable. It is not an original failure path. */
 if(source){
  for(size_t i=0;i<old_count;i++){const uint8_t*q=p+0x80+i*0x70;if(half(q)!=0xff&&half(q+2))preserved++;}
  for(size_t i=0;i<count;i++){const uint8_t*q=source+i*0x70;if(half(q)==0xff)continue;added++;
   if(word(q+0x50)&&(word(q+0x58)&0x4000000u)&&(!player||!player->owner))return fail(s,0x2db6b0,(int)i,GH_LIGHT_UNSUPPORTED);
  }
  if(preserved+added>256)return fail(s,0x2d8408,0,GH_LIGHT_BOUNDS);
 }
 const uint8_t*preset=p+0x15d00+index*0x80;
 put(p+0x16288,index);memcpy(p+0x30,preset,12);memcpy(p+0x40,preset+0x10,12);
 memcpy(p,preset+0x30,32);memcpy(p+0x20,preset+0x50,8);memcpy(p+0x60,preset+0x58,24);
 if(source){
  size_t destination=0;
  for(size_t i=0;i<old_count;i++){
   uint8_t*q=p+0x80+i*0x70;
   if(half(q)!=0xff&&half(q+2)){
    uint8_t saved[0x70];memcpy(saved,q,sizeof(saved));const void*owner=s->native_light_owners[i];
    q[0]=0xff;q[1]=0;memcpy(p+0x80+destination*0x70,saved,sizeof(saved));s->native_light_owners[destination]=owner;destination++;
   }
  }
  put(p+0x78,(uint32_t)destination);
  for(size_t i=0;i<count;i++){
   const uint8_t*q=source+i*0x70;if(half(q)==0xff)continue;
   uint8_t*d=p+0x80+destination*0x70;memcpy(d,q,0x70);s->native_light_owners[destination]=NULL;
   if(word(q+0x50)&&(word(q+0x58)&0x4000000u)){
    put(d+0x1c,0x3f800000u);uint32_t requested=word(q+0x4c);const void*part=NULL;
    /* Original unsigned >count clamps to FIRST part, ==count returnsNULL.
     * Preserve this nonstandard boundary rather than inventing last-part. */
    if(player->count<requested){if(player->count)part=player->parts[0];}
    else if(requested<player->count)part=player->parts[requested];
    s->native_light_owners[destination]=part;put(d+0x54,0); /* removedguestpointer; actualnativepart above */
   }
   destination++;put(p+0x78,(uint32_t)destination);
  }
 }
 s->frontier=0;s->detail=0;return GH_LIGHT_OK;
}
