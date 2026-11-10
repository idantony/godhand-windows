/* Readable original constructorchain; MIT LICENSE.upstream. */
#include "game_title_player.h"
#include <string.h>
static void w(uint8_t*p,size_t o,uint32_t x){memcpy(p+o,&x,4);}
static void h(uint8_t*p,size_t o,uint16_t x){memcpy(p+o,&x,2);}
static void vf0(uint8_t*p,size_t o){memset(p+o,0,12);w(p,o+12,0x3f800000);}
static void identity(uint8_t*p,size_t o){memset(p+o,0,64);for(unsigned i=0;i<4;i++)w(p,o+i*20,0x3f800000);}
static void transform_0014ff80(uint8_t*p){
 w(p,0x214,0x41eca0);vf0(p,0xc0);w(p,0xd0,0);vf0(p,0xe0);w(p,0xf0,0);vf0(p,0x100);
 for(unsigned i=0;i<4;i++){w(p,0x110+i*4,0x3f800000);w(p,0x120+i*4,0x3f800000);}
 vf0(p,0x130);p[0x140]=0;w(p,0x144,0);w(p,0x148,0);w(p,0x14c,0);w(p,0x158,0);vf0(p,0x1a0);
 identity(p,0);identity(p,0x80);
}
static void model_001495a0(uint8_t*p){
 transform_0014ff80(p);w(p,0x214,0x41ea90);
 for(unsigned i=0;i<4;i++)w(p,0x240+i*4,0x3f800000);
 w(p,0x250,0);w(p,0x25c,0);memset(p+0x260,0,0x18); /* BoundingBox1F8A40 */
 w(p,0x2c0,10);w(p,0x2c4,3);p[0x2b1]=p[0x2b4]=p[0x2b5]=0;
 w(p,0x254,0);w(p,0x258,0);
 for(unsigned i=0;i<7;i++)w(p,0x220+i*4,0);w(p,0x2c8,0);w(p,0x288,0);
 for(unsigned i=2;i;i--){w(p,0x28c+(i-1)*8,0);w(p,0x290+(i-1)*8,0);w(p,0x29c+(i-1)*8,0);w(p,0x2a0+(i-1)*8,0);}
 h(p,0x2ae,1000);p[0x2b7]=255;p[0x2b0]=200;h(p,0x2b2,180);p[0x2be]=254;p[0x2b6]=255;p[0x2bc]=255;p[0x2bd]=0;
}
static void actor_0014e990(uint8_t*p){
 model_001495a0(p);w(p,0x214,0x41eaf0); /* cObj14E598 */
 w(p,0x214,0x41ebe8);
 static const unsigned offsets[]={0x310,0x320,0x330,0x360,0x370,0x380,0x390,0x460,0x480,0x490,0x4c0};
 for(unsigned i=0;i<sizeof(offsets)/sizeof(offsets[0]);i++)vf0(p,offsets[i]);
 w(p,0x4ac,0x41eb80);w(p,0x4a8,0);w(p,0x4a0,0xffffffffu);
 w(p,0x4b4,0);w(p,0x4b0,0);w(p,0x4a4,0);w(p,0x44c,0);p[0x420]=0;
}
static void game_object_002a7338(uint8_t*p){
 actor_0014e990(p);w(p,0x214,0x44ae28);w(p,0x520,0);w(p,0x52c,0x3f800000);w(p,0x524,0);w(p,0x528,0);
 vf0(p,0x550);vf0(p,0x580);vf0(p,0x590);w(p,0x4ac,0x44adf0);w(p,0x5a8,0x3f800000);w(p,0x4d0,0xffff);p[0x531]=255;
 w(p,0x510,0);memset(p+0x538,0,16);w(p,0x5a0,0);
}
gh_title_player_status gh_title_player_construct_00107e40(void*raw,size_t n,gh_title_player_view*out){
 if(!raw||!out)return GH_PLAYER_ARGUMENT;if(n<GH_TITLE_PLAYER_BYTES)return GH_PLAYER_BOUNDS;
 uint8_t*p=raw;game_object_002a7338(p);
 w(p,0x214,0x41b598);w(p,0x4ac,0x41b590); /* cPlBase100280 */
 w(p,0x214,0x41d220);vf0(p,0x630);vf0(p,0x650);vf0(p,0x660);vf0(p,0x1580);vf0(p,0x1590);vf0(p,0x15e0);
 w(p,0x698,0);w(p,0x4ac,0x41b800);w(p,0x6a0,0);w(p,0x69c,0);w(p,0x6a4,0);w(p,0x6a8,0);
 *out=(gh_title_player_view){p,n,p+0x30,p+0xb0,0x41d220,0x41b800,GH_TITLE_MEMORY_UNSPECIFIED};return GH_PLAYER_OK;
}
static gh_title_player_status create(gh_game_title_player*s,gh_heap*h0,const gh_boot_work*w0,const gh_title_memory_profile*profile){
 if(!s||!h0||!w0)return GH_PLAYER_ARGUMENT;if(s->allocation.pointer||s->arena_allocation.pointer)return GH_PLAYER_BUSY;
 if(profile){
  if(profile->policy==GH_TITLE_MEMORY_PROVIDED_BYTES){if(!profile->bytes||profile->size<GH_TITLE_PLAYER_BYTES)return GH_PLAYER_ARGUMENT;}
  else if(profile->policy==GH_TITLE_MEMORY_ZERO_REFERENCE){if(profile->bytes||profile->size)return GH_PLAYER_ARGUMENT;}
  else if(profile->policy!=GH_TITLE_MEMORY_UNSPECIFIED)return GH_PLAYER_ARGUMENT;
 }
 s->frontier=0;if((uint16_t)w0->room!=0x20){s->frontier=0x31c4e0;return GH_PLAYER_UNSUPPORTED;}
 if(gh_heap_allocate(h0,GH_TITLE_PLAYER_BYTES,16,&s->allocation)!=GH_MEM_OK){s->frontier=0x30f550;return GH_PLAYER_MEMORY;}
 s->heap=h0;
 if(profile&&profile->policy==GH_TITLE_MEMORY_PROVIDED_BYTES)memcpy(s->allocation.pointer,profile->bytes,GH_TITLE_PLAYER_BYTES);
 else if(profile&&profile->policy==GH_TITLE_MEMORY_ZERO_REFERENCE)memset(s->allocation.pointer,0,GH_TITLE_PLAYER_BYTES);
 gh_title_player_status r=gh_title_player_construct_00107e40(s->allocation.pointer,GH_TITLE_PLAYER_BYTES,&s->view);
 if(r)return r;s->view.initial_memory=profile?profile->policy:GH_TITLE_MEMORY_UNSPECIFIED;s->ready=1;return GH_PLAYER_OK;
}
gh_title_player_status gh_game_title_player_release(gh_game_title_player*s){
 if(!s)return GH_PLAYER_ARGUMENT;
 if(s->allocation_domain==GH_PLAYER_ALLOCATION_ROOM_ARENA){if(s->arena_allocation.pointer&&gh_room_arena_free_002a9680(s->arena_allocation.pools,s->arena_allocation)!=GH_POOL_OK)return GH_PLAYER_MEMORY;}
 else if(s->allocation_domain==GH_PLAYER_ALLOCATION_DEFAULT_HEAP){if(s->allocation.pointer&&(!s->heap||gh_heap_release(s->heap,s->allocation)!=GH_MEM_OK))return GH_PLAYER_MEMORY;}
 else return GH_PLAYER_ARGUMENT;
 *s=(gh_game_title_player){0};return GH_PLAYER_OK;
}

gh_title_player_status gh_game_title_player_create_0031c4e0(gh_game_title_player*s,gh_heap*h,const gh_boot_work*w){return create(s,h,w,0);}
gh_title_player_status gh_game_title_player_create_reference_0031c4e0(gh_game_title_player*s,gh_heap*h,const gh_boot_work*w,const gh_title_memory_profile*p){if(!p)return GH_PLAYER_ARGUMENT;return create(s,h,w,p);}
gh_title_player_status gh_title_player_read_vitality(const gh_title_player_view*v,int16_t*out,uint32_t*f){
 if(f)*f=0;if(!v||!v->bytes||v->size<GH_TITLE_PLAYER_BYTES||!out)return GH_PLAYER_ARGUMENT;
 if(v->initial_memory==GH_TITLE_MEMORY_UNSPECIFIED){if(f)*f=0x1f9bec;return GH_PLAYER_UNSUPPORTED;}
 if(v->initial_memory!=GH_TITLE_MEMORY_PROVIDED_BYTES&&v->initial_memory!=GH_TITLE_MEMORY_ZERO_REFERENCE&&v->initial_memory!=GH_TITLE_MEMORY_ORIGINAL_FACTORY_ZERO)return GH_PLAYER_ARGUMENT;
 unsigned n=v->bytes[0x54a]|(unsigned)v->bytes[0x54b]<<8;*out=(int16_t)(n<32768?(int)n:(int)n-65536);return GH_PLAYER_OK;
}


gh_title_player_status gh_game_player_construct_00312f00(gh_game_title_player*s,gh_room_pools*pools,const gh_boot_work*work,int*original){
 if(!s||!pools||!work||!original)return GH_PLAYER_ARGUMENT;*original=0;if(s->allocation.pointer||s->arena_allocation.pointer)return GH_PLAYER_BUSY;
 const uint8_t*raw=(const uint8_t*)work;uint32_t flags;memcpy(&flags,raw+0x594,4);if(flags&0x10000)return GH_PLAYER_OK;
 gh_room_arena_allocation allocation;gh_room_pool_status e=gh_room_arena_allocate_002a9538(pools,0x754c10,flags,GH_TITLE_PLAYER_BYTES,16,&allocation);if(e){s->frontier=0x2a9538;return GH_PLAYER_MEMORY;}if(!allocation.pointer)return GH_PLAYER_OK;
 s->arena_allocation=allocation;s->allocation=(gh_r207_allocation){allocation.pointer,&pools->arenas[2]};s->allocation_domain=GH_PLAYER_ALLOCATION_ROOM_ARENA;s->heap=pools->heap;
 memset(allocation.pointer,0,GH_TITLE_PLAYER_BYTES);gh_title_player_status result=gh_title_player_construct_00107e40(allocation.pointer,GH_TITLE_PLAYER_BYTES,&s->view);if(result)return result;
 s->view.initial_memory=GH_TITLE_MEMORY_ORIGINAL_FACTORY_ZERO;s->ready=1;s->frontier=0;*original=1;return GH_PLAYER_OK;
}


gh_title_player_status gh_game_object_construct_002a7338(void*data,size_t bytes,gh_title_player_view*out){
 if(!data||!out)return GH_PLAYER_ARGUMENT;if(bytes<0x5b0)return GH_PLAYER_BOUNDS;uint8_t*p=data;game_object_002a7338(p);*out=(gh_title_player_view){p,bytes,p+0x30,p+0xb0,0x44ae28,0x44adf0,GH_TITLE_MEMORY_UNSPECIFIED};return GH_PLAYER_OK;
}
