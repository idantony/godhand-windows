#include "game_init.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t r32(const uint8_t *b,size_t o){return (uint32_t)b[o]|(uint32_t)b[o+1]<<8|(uint32_t)b[o+2]<<16|(uint32_t)b[o+3]<<24;}
static unsigned r16(const uint8_t *b,size_t o){return (unsigned)b[o]|(unsigned)b[o+1]<<8;}
static void w32(uint8_t *b,size_t o,uint32_t v){for(unsigned i=0;i<4;i++)b[o+i]=(uint8_t)(v>>(8*i));}
static float rf(const uint8_t *b,size_t o){uint32_t v=r32(b,o);float f;memcpy(&f,&v,4);return f;}
int main(void){
 uint8_t record[GH_CORE_SAVE_BYTES],before[GH_CORE_SAVE_BYTES];
 gh_core_save_view save={record,sizeof(record)};gh_game_init_state *s=malloc(sizeof(*s));
 gh_boot_work work={0};uint8_t started;float rate;uint32_t number,target=123;
 gh_game_damage_link link={0,&target};gh_game_camera prior={0};CHECK(s);
 CHECK(gh_core_save_system_init(save)==GH_INIT_OK);
 CHECK(record[0x1f]==1 && record[0xb0]==255 && r16(record,0x288)==1);
 for(unsigned i=0;i<128;i++)CHECK(r16(record,0x28a+i*16)==65535);
 {static const uint32_t vector[]={3499211612u,581869302u,3890346734u,3586334585u,545404204u,
 4161255391u,3922919429u,949333985u,2715962298u,1323567403u};
  gh_game_init_state_initialize(s);
  for(unsigned i=0;i<10;i++){CHECK(gh_game_mt_next_0030f348(&s->random,&number)==GH_INIT_OK);CHECK(number==vector[i]);}
  gh_game_mt_seed_0030f2b0(&s->random,0);CHECK(gh_game_mt_next_0030f348(&s->random,&number)==GH_INIT_OK && number==2357136044u);
 }
 for(unsigned difficulty=0;difficulty<4;difficulty++)for(unsigned cheat=0;cheat<2;cheat++){
  memset(record,0xa5,sizeof(record));record[0x1f]=(uint8_t)(difficulty==3?7:difficulty);
  record[0x154]=2;record[0x155]=3;memcpy(before,record,sizeof(record));
  gh_game_init_state_initialize(s);memset(s->enemies,0xa5,sizeof(s->enemies));
  memset(s->object_pool,0xa5,sizeof(s->object_pool));memset(s->records_a,0xa5,sizeof(s->records_a));
  memset(s->records_b,0xa5,sizeof(s->records_b));memset(s->input,0xa5,sizeof(s->input));
  memset(s->heat,0xa5,sizeof(s->heat));memset(s->side_counter,0xa5,sizeof(s->side_counter));
  s->object_links[0]=&link;s->object_links[415]=&link;s->active_camera=&prior;s->camera.mode=7;
  memset(&work,0,sizeof(work));work.flags=0x80000000u;
  if(cheat){work.flags598=0x08000000u;w32((uint8_t *)&work,0x594,0x800);}
  started=0;rate=9;
  CHECK(gh_game_init_002a5b18(s,&work,&started,&rate,save)==GH_INIT_OK);
  CHECK(started==1 && rate==0.5f && work.flags==0x80000010u && s->world_time==0);
  CHECK(s->active_camera==&s->camera && s->camera.previous==&prior && s->camera.previous_mode==7 && s->camera.mode==1);
  for(unsigned i=0;i<64;i++){
   CHECK(r32(s->enemies,i*20)==0 && r32(s->enemies,i*20+4)==0 && r32(s->enemies,i*20+8)==0);
   CHECK(r32(s->enemies,i*20+12)==0xffffffffu && s->enemies[i*20+16]==0 && s->enemies[i*20+17]==0xa5);
  }
  CHECK(s->enemies[0x53f]==2 && rf(s->enemies,0x548)==1.0f);
  CHECK(r32(s->enemies,0x510)==0xa5a5a5a5 && s->enemies[0x539]==0xa5);
  CHECK(r32(s->side_counter,0)==0xa5a5a5a5 && s->side_counter[4]==0 && r32(s->side_counter,8)==0);
  for(unsigned i=0;i<256;i++){
   size_t a=i*0x60,b=0x3c00+i*0x70;
   CHECK((i>=160 || s->object_pool[a+0x49]==1) && s->object_pool[b+0x49]==2);
   CHECK((i>=160 || s->object_pool[a+0x48]==4) && s->object_pool[b+0x48]==4);
   CHECK((i>=160 || r32(s->object_pool,a+0x3c)==0) && r32(s->object_pool,b+0x60)==0);
   CHECK((i>=160 || r32(s->object_pool,a+0x1c)==0xa5a5a5a5) && r32(s->object_pool,b+0x2c)==0xa5a5a5a5);
  }
  CHECK(!s->object_links[0] && !s->object_links[415] && target==123);
  for(unsigned i=0;i<160;i++)CHECK(r16(s->records_a[i],0x2c)==i && r32(s->records_a[i],0x20)==0 && s->records_a[i][0x2e]==0xa5);
  for(unsigned i=0;i<512;i++)CHECK(r16(s->records_b[i],0x2c)==i && r32(s->records_b[i],0x28)==0);
  CHECK(rf(s->heat,0)==(cheat?270.0f:210.0f) && rf(s->heat,4)==0 && rf(s->heat,12)==120.0f && r32(s->heat,16)==0xa5a5a5a5);
  CHECK(r32(s->input,4)==0xa5a5a5a5 && r32(s->input,0)==0 && r32(s->input,8)==0 && r16(s->input,18)==0);
  CHECK(record[0x1f]==before[0x1f] && record[0x154]==(cheat?13:2) && record[0x155]==(cheat?5:3));
  CHECK(r32(record,0x80)==(cheat?360:140) && r32(record,0x84)==r32(record,0x80));
  CHECK(r16(record,0x1c)==(difficulty==2?5000:0) && record[0x1e]==2 && record[0x156]==(difficulty==0?3:2));
  CHECK(record[0xa8]==1 && record[0xa9]==1 && record[0xaa]==(difficulty==0?1:0));
  CHECK(r32(record,0x1b8)==(cheat?13u:10u) && r32(record,0x1bc)==(cheat?39u:13u));
  CHECK(r32(record,0x1d0)==4 && record[0x1ce]==0xa5 && record[0xb0]==0xa5);
  CHECK(r32(record,0x130)==(0xa5a5a5a5u|(1u<<1)|(1u<<2)|(1u<<3)|(1u<<7)|(1u<<11)|(1u<<14)|(1u<<15)|(1u<<23)));
  CHECK(r32(record,0)==0xa5a5a5a5 && r32(record,0x14)==0xa5a5a5a5 && r32(record,0xb8c)==0xa5a5a5a5);
  CHECK(r32(record,0x24)==0 && r32(record,0x28)==0xa5a5a5a5 && r32(record,0xb90)==0 && r32(record,0xbbc)==0);
  CHECK(gh_core_save_costume(save)==0 && gh_core_save_difficulty(save)==record[0x1f]);
 }
 /* Existing camera modes1/16 do not rebind globalactive camera. */
 for(unsigned mode=1;mode<=16;mode+=15){
  gh_game_init_state_initialize(s);s->camera.mode=(int32_t)mode;s->camera.previous_mode=9;s->active_camera=&prior;
  CHECK(gh_game_init_002a5b18(s,&work,&started,&rate,save)==GH_INIT_OK);
  CHECK(s->active_camera==&prior && s->camera.previous_mode==9 && s->camera.mode==(int32_t)mode);
 }
 {gh_core_save_view short_view={record,GH_CORE_SAVE_BYTES-1};CHECK(gh_core_save_game_init(short_view,&work)!=GH_INIT_OK);}
 free(s);puts("PASS actual case0 manager fields, pool counts/handles, MT vectors and persistent CoreSave reset branches");return 0;
}
