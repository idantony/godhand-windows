/* Actual patched ELF2A5B18 and callees. Evidence:game-init-evidence.json. */
#include "game_init.h"
#include <string.h>
static void w16(uint8_t *b,size_t o,uint16_t v){b[o]=(uint8_t)v;b[o+1]=(uint8_t)(v>>8);}
static void w32(uint8_t *b,size_t o,uint32_t v){for(unsigned i=0;i<4;i++)b[o+i]=(uint8_t)(v>>(i*8));}
static uint32_t r32(const uint8_t *b,size_t o){return (uint32_t)b[o]|(uint32_t)b[o+1]<<8|(uint32_t)b[o+2]<<16|(uint32_t)b[o+3]<<24;}
static int save_valid(gh_core_save_view v){return v.bytes && v.size>=GH_CORE_SAVE_BYTES;}
gh_init_status gh_core_save_system_init(gh_core_save_view v){
 if(!save_valid(v))return GH_INIT_BOUNDS;
 memset(v.bytes,0,GH_CORE_SAVE_BYTES);
 for(unsigned i=0;i<128;i++)w16(v.bytes,0x28a+(size_t)i*16,0xffff);
 w16(v.bytes,0x288,1);memset(v.bytes+0xb0,0xff,128);v.bytes[0x1f]=1;return GH_INIT_OK;
}
uint8_t gh_core_save_costume(gh_core_save_view v){return save_valid(v)?v.bytes[0xae]:0;}
uint8_t gh_core_save_difficulty(gh_core_save_view v){return save_valid(v)?v.bytes[0x1f]:1;}
static void level(uint8_t *b,unsigned n){
 static const uint16_t points[]={0,2000,3500,3500,5000};w16(b,0x1c,points[n-1]);b[0x1e]=2;
}
gh_init_status gh_core_save_game_init(gh_core_save_view v,const gh_boot_work *work){
 uint8_t *b=v.bytes;static const uint8_t reels[]={1,2,3,7,11,14,15,23};
 static const uint8_t single[]={0,29,37,84,85,52};
 if(!save_valid(v) || !work)return GH_INIT_ARGUMENT;
 memset(b+0xa8,0,6);w32(b,0x20,0);level(b,1);
 if(work->flags598&0x08000000u)b[0x154]=13;
 w32(b,0x80,(uint32_t)b[0x154]*20+100);w32(b,0x84,r32(b,0x80));
 /* initCombos writes IDs/levels/max, preserving the two padding bytes. */
 for(unsigned set=0;set<6;set++){
  size_t base=0x1b0+(size_t)set*0x24;
  for(unsigned slot=0;slot<6;slot++){w32(b,base+slot*4,0);b[base+0x18+slot]=0;}
  w32(b,base+0x20,0);
 }
 for(unsigned set=0;set<6;set++){
  size_t base=0x1b0+(size_t)set*0x24;w32(b,base,single[set]);w32(b,base+0x20,set?1:4);
 }
 w32(b,0x1b4,4);w32(b,0x1b8,(r32((const uint8_t *)work,0x594)&0x800u)?13:10);
 w32(b,0x1bc,(r32((const uint8_t *)work,0x594)&0x800u)?39:13);
 /* setGodReel also fills first free reel slot before later initReelSlots.
  * Final slots below supersede those temporary writes, unlock word is ORed. */
 {uint32_t bits=r32(b,0x130);for(unsigned i=0;i<sizeof(reels);i++)bits|=UINT32_C(1)<<reels[i];w32(b,0x130,bits);}
 w16(b,0x88,0x80);b[0x8a]=4;for(unsigned i=0;i<4;i++)w32(b,0x70+i*4,0);
 w16(b,0x10,0);memset(b+0x8c,0,10);w16(b,0x96,0);w32(b,0x24,0);
 w16(b,0x12,0);w32(b,0xa4,0);b[0xae]=b[0xaf]=0;memset(b+0x180,0,0x30);
 memset(b+0x98,0x1f,10);b[0x98]=1;b[0x99]=2;b[0x9a]=3;b[0x9b]=7;b[0x9c]=14;b[0x9d]=23;
 w32(b,0xb90,0);b[0x156]=2;memset(b+0xb98,0,16);w16(b,0xbb8,0);
 memset(b+0xbac,0,10);w16(b,0xbb6,0);w32(b,0xbbc,0);
 if(b[0x1f]==0){b[0x156]=3;level(b,1);}else level(b,b[0x1f]==2?5:1);
 /* Three calls addGodItem(1), respecting usable slot count. */
 for(unsigned n=0;n<3;n++)for(unsigned i=0;i<6 && i<b[0x156];i++)if(!b[0xa8+i]){b[0xa8+i]=1;break;}
 return GH_INIT_OK;
}
void gh_game_mt_seed_0030f2b0(gh_game_mt *s,uint32_t seed){
 if(!s)return;s->words[0]=seed;
 for(uint32_t i=1;i<624;i++)s->words[i]=UINT32_C(0x6c078965)*(s->words[i-1]^(s->words[i-1]>>30))+i;
 s->index=624;
}
gh_init_status gh_game_mt_next_0030f348(gh_game_mt *s,uint32_t *out){
 uint32_t y;if(!s || !out || s->index>625)return GH_INIT_ARGUMENT;
 if(s->index>=624){
  if(s->index==625)gh_game_mt_seed_0030f2b0(s,5489);
  for(unsigned i=0;i<227;i++){y=(s->words[i]&0x80000000u)|(s->words[i+1]&0x7fffffffu);
   s->words[i]=s->words[i+397]^(y>>1)^((s->words[i+1]&1)?0x9908b0dfu:0);}
  for(unsigned i=227;i<623;i++){y=(s->words[i]&0x80000000u)|(s->words[i+1]&0x7fffffffu);
   s->words[i]=s->words[i-227]^(y>>1)^((s->words[i+1]&1)?0x9908b0dfu:0);}
  y=(s->words[623]&0x80000000u)|(s->words[0]&0x7fffffffu);
  s->words[623]=s->words[396]^(y>>1)^((s->words[0]&1)?0x9908b0dfu:0);s->index=0;
 }
 y=s->words[s->index++];y^=y>>11;y^=(y&0x013a58adu)<<7;y^=(y&0x0001df8cu)<<15;
 *out=y^(y>>18);return GH_INIT_OK;
}
void gh_game_init_state_initialize(gh_game_init_state *s){if(s){memset(s,0,sizeof(*s));s->random.index=625;}}
static gh_init_status pool_record(uint8_t *b,gh_game_damage_link **links,unsigned kind){
 /*1FCD00 clears original head+3C before1FD988(mode4). Therefore no prior
  * nodes are visited or freed: drop native head too, preserve target objects. */
 *links=0;
 w32(b,0,0);b[0x4a]=0;
 for(unsigned i=0;i<3;i++){w32(b,0x10+i*4,0);w32(b,0x20+i*4,0);}
 w32(b,0x30,0);w32(b,0x38,0);w32(b,0x3c,0);w16(b,0x44,0);w16(b,0x46,0);w32(b,0x40,0xffffffffu);
 b[0x48]=4;
 w32(b,0x4c,0);w32(b,0x34,0);b[0x49]=(uint8_t)kind;if(kind==2)w32(b,0x60,0);
 return GH_INIT_OK;
}
gh_init_status gh_game_object_pool_reset_001fdea8(gh_game_init_state*s){
 gh_init_status e;if(!s)return GH_INIT_ARGUMENT;
 for(unsigned i=0;i<160;i++){e=pool_record(s->object_pool+(size_t)i*0x60,&s->object_links[i],1);if(e)return e;}
 for(unsigned i=0;i<256;i++){e=pool_record(s->object_pool+0x3c00+(size_t)i*0x70,&s->object_links[160+i],2);if(e)return e;}
 s->object_pool[0xac49]=s->object_pool[0xaca9]=0;
 for(unsigned bank=0;bank<2;bank++){
  unsigned count=bank?512:160;
  for(unsigned i=0;i<count;i++){
   uint8_t*b=bank?s->records_b[i]:s->records_a[i];
   w32(b,0x24,0);w32(b,0x28,0);w16(b,0x30,0);w32(b,0x20,0);w16(b,0x2c,(uint16_t)i);
  }
 }
 return GH_INIT_OK;
}

gh_init_status gh_game_init_002a5b18(gh_game_init_state *s,gh_boot_work *work,
 uint8_t *started,float *rate,gh_core_save_view save){
 uint32_t random;gh_init_status e;uint8_t *em;
 if(!s || !work || !started || !rate || !save_valid(save))return GH_INIT_ARGUMENT;
 s->world_time=0;work->flags|=0x10;
 if(s->camera.mode!=1 && s->camera.mode!=16){s->camera.previous_mode=s->camera.mode;s->camera.previous=s->active_camera;s->active_camera=&s->camera;s->camera.mode=1;}
 *started=1;em=s->enemies;
 for(unsigned i=0;i<64;i++){size_t off=(size_t)i*20;w32(em,off,0);w32(em,off+4,0);w32(em,off+8,0);w32(em,off+12,0xffffffffu);em[off+16]=0;s->enemy_actors[i]=0;}
 s->enemy_top=s->enemy_last=0;memset(s->enemy_kept,0,sizeof(s->enemy_kept));
 for(unsigned off=0x500;off<=0x50c;off+=4)w32(em,off,0);w32(em,0x55c,0);w32(em,0x548,0x3f800000);
 e=gh_game_mt_next_0030f348(&s->random,&random);if(e)return e;
 em[0x53e]=em[0x540]=em[0x541]=0;em[0x53f]=(uint8_t)(random%5);
 memset(em+0x560,0,0x14);memset(em+0x588,0,0x10);memset(em+0x5ac,0,8);memset(em+0x574,0,20);
 em[0x518]=0;s->side_counter[4]=0;w32(s->side_counter,8,0);w32(s->side_counter,12,0);em[0x5b5]=0;
 e=gh_game_object_pool_reset_001fdea8(s);if(e)return e;
 /* Heat reset precedes CoreSave gameInit and therefore reads persistent
  * tension upgrades, with the original cheat-side effect. All conversions
  * here are exact small integers and the resulting current/max ratio is0. */
 if(work->flags598&0x08000000u)save.bytes[0x155]=5;
 {float maximum=(float)((uint32_t)save.bytes[0x155]*30+120);uint32_t bits;memcpy(&bits,&maximum,4);
  w32(s->heat,0x14,0);w32(s->heat,0,bits);w32(s->heat,4,0);w32(s->heat,8,0);w32(s->heat,12,0x42f00000);}
 w32(s->input,0,0);w32(s->input,8,0);w16(s->input,12,0);w16(s->input,14,0);w16(s->input,16,0);w16(s->input,18,0);
 e=gh_core_save_game_init(save,work);if(e)return e;*rate=0.5f;return GH_INIT_OK;
}


gh_init_status gh_game_object_pool_release_001fdfa8(gh_game_init_state*s,uint32_t*f){
 if(f)*f=0;if(!s)return GH_INIT_ARGUMENT;
 for(unsigned i=0;i<416;i++){uint8_t*b=i<160?s->object_pool+(size_t)i*0x60:s->object_pool+0x3c00+(size_t)(i-160)*0x70;
  if(s->object_links[i]||b[0x3c]||b[0x3d]||b[0x3e]||b[0x3f]){if(f)*f=0x1fd580;return GH_INIT_LINKS;}
  gh_init_status e=pool_record(b,&s->object_links[i],i<160?1:2);if(e)return e;
 }return GH_INIT_OK;
}
