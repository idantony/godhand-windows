#include "game_world_light_step.h"
#include "disc.h"
#include "frame_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static uint64_t wide(const uint8_t*p){uint64_t n;memcpy(&n,p,8);return n;}
static void half(uint8_t*p,uint16_t n){memcpy(p,&n,2);}
static void number(uint8_t*p,float n){memcpy(p,&n,4);}
typedef struct {gh_room_pools*pools;gh_title_player_view views[6];uint32_t handles[6];unsigned calls;} actor_fixture;
static gh_light_status actor_view(void*opaque,uint32_t handle,gh_title_player_view*out,uint32_t*pc){
 actor_fixture*c=opaque;c->calls++;void*identity=NULL;
 if(gh_room_object_lookup(c->pools,handle,999,c,&identity)){*pc=0x2d74b8;return GH_LIGHT_UNSUPPORTED;}
 for(unsigned i=0;i<6;i++)if(handle==c->handles[i]&&identity==&c->views[i]){*out=c->views[i];return GH_LIGHT_OK;}
 *pc=0x2d74b8;return GH_LIGHT_UNSUPPORTED;
}
static void live_models(gh_game_world_light*s,gh_boot_work*work,gh_room_pools*pools){
 const uint16_t yes[]={0x100,0x1ff,0x200,0x300,0x500,0x5ff,0x600,0xff01,0xff02};
 const uint16_t no[]={0,0xff,0x601,0xff00,0xff03,0xffff};
 for(unsigned i=0;i<sizeof(yes)/sizeof(*yes);i++)C(gh_game_world_light_class_002d74b8(yes[i]));
 for(unsigned i=0;i<sizeof(no)/sizeof(*no);i++)C(!gh_game_world_light_class_002d74b8(no[i]));
 gh_bundle_entry original_table=s->table;C(original_table.bytes&&original_table.size>=16&&word(original_table.bytes)<=(original_table.size-16)/0x50);
 uint8_t lar[16+3*0x50]={0};put(lar,3);
 for(unsigned i=0;i<3;i++){uint8_t*r=lar+16+i*0x50;r[2]=(uint8_t)(10+i*10);uint8_t*shape=r+0x20;shape[1]=2;number(shape+4,0);number(shape+8,2);number(shape+0xc,2);number(shape+0x10,i==2?100:0);number(shape+0x14,0);}
 s->table=(gh_bundle_entry){.bytes=lar,.size=sizeof(lar)};
 uint8_t raw[6][0x5b0]={{0}},before[6][0x5b0];actor_fixture c={.pools=pools};
 const uint16_t kinds[]={0x100,0x500,0x100,0x601,0x100,0xff02};
 const uint32_t flags[]={0,0,0x80000,0x400,0x8000,0};
 for(unsigned i=0;i<6;i++){C(!gh_game_object_construct_002a7338(raw[i],sizeof(raw[i]),&c.views[i]));half(raw[i]+0x2fe,kinds[i]);put(raw[i]+0x250,flags[i]);raw[i][0x2cc]=0xa5;C(!gh_room_object_register(pools,&c.views[i],&c,999,8,&c.handles[i]));}
 c.views[1].size=0x300;put(raw[5]+0x4a8,0x10000000);memcpy(before,raw,sizeof(raw));
 gh_scene_at_manager at={0};gh_light_move_bindings b={.scene_at=&at,.actor_view=actor_view,.context=&c};const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
 for(unsigned m=0;m<4;m++){
  memcpy(raw,before,sizeof(raw));C(!fesetround(modes[m]));feclearexcept(FE_ALL_EXCEPT);feraiseexcept(FE_INVALID);
  C(!gh_game_world_light_move_native_002d7560(s,work,pools,&b,GH_CAMERA_NATIVE_BINARY32_REFERENCE)&&c.calls==(m+1)*6&&fegetround()==modes[m]&&fetestexcept(FE_ALL_EXCEPT)==FE_INVALID);
  C(word(raw[0]+0x250)==0x200000&&raw[0][0x2cc]==10); /* both inside: index0 wins */
  C(word(raw[1]+0x250)==0x200000&&!raw[1][0x2cc]); /* class500: no +4A8 read */
  C(word(raw[2]+0x250)==0x280000&&raw[2][0x2cc]==0xa5);
  C(word(raw[3]+0x250)==0x400&&raw[3][0x2cc]==0xa5);
  C(word(raw[4]+0x250)==0x8000&&raw[4][0x2cc]==0xa5);
  C(word(raw[5]+0x250)==0x200000&&!raw[5][0x2cc]);
 }
 C(!fesetround(FE_TONEAREST));memcpy(raw,before,sizeof(raw));
 uint8_t light_before[sizeof(s->scalar)];memcpy(light_before,s->scalar,sizeof(light_before));gh_boot_work work_before=*work;
 at.field68=0x1234;C(gh_game_world_light_move_native_002d7560(s,work,pools,&b,GH_CAMERA_NATIVE_BINARY32_REFERENCE)==GH_LIGHT_UNSUPPORTED&&s->frontier==0x2c3190&&!memcmp(raw,before,sizeof(raw))&&!memcmp(s->scalar,light_before,sizeof(light_before))&&!memcmp(work,&work_before,sizeof(*work)));at.field68=0;
 c.views[1].size=0x2ff;C(gh_game_world_light_move_native_002d7560(s,work,pools,&b,GH_CAMERA_NATIVE_BINARY32_REFERENCE)==GH_LIGHT_BOUNDS&&!memcmp(raw,before,sizeof(raw))&&!memcmp(s->scalar,light_before,sizeof(light_before)));c.views[1].size=0x300;
 s->table.size=15;C(gh_game_world_light_move_native_002d7560(s,work,pools,&b,GH_CAMERA_NATIVE_BINARY32_REFERENCE)==GH_LIGHT_BOUNDS&&!memcmp(raw,before,sizeof(raw))&&!memcmp(s->scalar,light_before,sizeof(light_before)));
 s->table=(gh_bundle_entry){0};c.views[0].size=0x300;C(!gh_game_world_light_move_native_002d7560(s,work,pools,&b,GH_CAMERA_NATIVE_BINARY32_REFERENCE)&&!raw[0][0x2cc]);
 for(unsigned i=0;i<6;i++)C(!gh_room_object_unregister(pools,c.handles[i],&c));s->table=original_table;
}
static void actual_r003_fog(gh_disc *disc,gh_heap *heap)
{
 size_t index;
 gh_disc_entry entry;
 C(!gh_disc_find(disc,"st0/r003.dat",&index)&&!gh_disc_entry_at(disc,index,&entry));
 uint8_t *bytes=malloc((size_t)entry.size);
 C(bytes&&!gh_disc_read(disc,index,0,bytes,(size_t)entry.size));
 gh_bundle room;
 C(!gh_bundle_open(bytes,(size_t)entry.size,&room));
 gh_room_pools pools={0};
 C(!gh_game_room_pools_initialize_002a5d80(&pools,&room,heap));
 gh_game_world_light *light=calloc(1,sizeof(*light));
 C(light&&!gh_game_world_light_construct_002d6d80(light)&&
   !gh_game_world_light_initialize_with_heap_002d6f40(light,&room,heap));
 gh_light_player_parts parts={.owner=light};
 C(!gh_game_world_light_select_002d8408(light,0,&parts));
 C(light->area.bytes&&light->area.size==384&&word(light->area.bytes)==4);
 C(word(light->slots[0].bytes)==3);

 gh_boot_work work={0};
 uint8_t raw[0xbc]={0};
 gh_title_player_view player={.bytes=raw,.size=sizeof(raw)};
 gh_light_move_bindings bindings={.player=&player};
 uint8_t *state=light->scalar;
 const uint32_t expected[2][5]={
  {0xbf51e875,0x417ec88f,0x40ab851e,0x40a28f5c,0x407ae147},
  {0xbdf5c28f,0x41bf0a3d,0x41028f5c,0x40f0a3d7,0x40c8f5c2},
 };
 const float y[]={0,-10,-50};
 const uint32_t areas[]={1,3,0};
 for(unsigned i=0;i<3;i++){
  number(raw+0xb4,y[i]);
  for(unsigned repeat=0;repeat<2;repeat++){
   memset(state+0x16150,0,20);
   C(!fesetround(FE_DOWNWARD)&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_INVALID));
   C(!gh_game_world_light_move_native_002d7560(light,&work,&pools,&bindings,
     GH_CAMERA_NATIVE_BINARY32_REFERENCE));
   C(fegetround()==FE_DOWNWARD&&fetestexcept(FE_ALL_EXCEPT)==FE_INVALID);
   C(word(state+0x16124)==areas[i]&&word(state+0x16128)==areas[i]);
   if(i<2){
    C(word(state+0x1612c)==i*2);
    for(unsigned j=0;j<5;j++)C(word(state+0x16150+j*4)==expected[i][j]);
   }
  }
 }
 C(!fesetround(FE_TONEAREST));
 uint8_t before[sizeof(light->scalar)];
 memcpy(before,state,sizeof(before));
 gh_boot_work work_before=work;
 C(gh_game_world_light_move_002d7560(light,&work,&pools,
   GH_CAMERA_NATIVE_BINARY32_REFERENCE)==GH_LIGHT_UNSUPPORTED&&light->frontier==0x2db6b0);
 C(!memcmp(before,state,sizeof(before))&&!memcmp(&work_before,&work,sizeof(work)));
 light->area.size=15;
 C(gh_game_world_light_move_native_002d7560(light,&work,&pools,&bindings,
   GH_CAMERA_NATIVE_BINARY32_REFERENCE)==GH_LIGHT_BOUNDS&&light->frontier==0x1f83e8);
 C(!memcmp(before,state,sizeof(before))&&!memcmp(&work_before,&work,sizeof(work)));
 C(!gh_game_world_light_release_presets(light)&&!gh_game_room_pools_release(&pools));
 free(light);free(bytes);
 puts("PASS actual R003 FAR first region, LI33 fog rows, cache repeat, no-match and FEnv");
}

static void actual_ev04_waves(gh_disc *disc,gh_game_world_light *s,gh_boot_work *work,
 const gh_room_pools *pools)
{
 size_t index;
 gh_disc_entry entry;
 C(!gh_disc_find(disc,"event/ev04.dat",&index)&&!gh_disc_entry_at(disc,index,&entry));
 uint8_t *data=malloc((size_t)entry.size);
 C(data&&!gh_disc_read(disc,index,0,data,(size_t)entry.size));
 const uint32_t track=word(data+0x58);
 C(track+16+7*0x44<=entry.size&&word(data+track)==0x00514553&&word(data+track+4)==70);
 const uint8_t *key=data+track+16+6*0x44;
 C(key[3]==3&&key[0x0d]==0x11&&key[0x0e]==2&&word(key+0x40)==0x5550000a);
 uint8_t *world=s->scalar,*light=world+0x80;
 uint8_t *state=world+0x7080;
 uint8_t saved_light[0x70],saved_state[GH_WORLD_LIGHT_WAVE_BYTES];
 memcpy(saved_light,light,sizeof(saved_light));
 memcpy(saved_state,state,sizeof(saved_state));
 const unsigned count=word(world+0x78);
 put(world+0x78,1);
 put(light+0x58,0xf0980000u);
 memcpy(light+0x5c,key+0x42,2);
 memcpy(light+0x5e,key+0x40,2);
 memcpy(light+0x60,key+0x3c,4);
 free(data);
 memset(state,0,GH_WORLD_LIGHT_WAVE_BYTES);

 C(gh_game_world_light_wave_002d9018(s,0)==state&&
   gh_game_world_light_wave_002d9018(s,255)==state+47*GH_WORLD_LIGHT_WAVE_BYTES);
 C(!gh_game_world_light_move_002d7560(s,work,pools,GH_CAMERA_NATIVE_BINARY32_REFERENCE));
 C(state[0x12]==255&&!state[0x10]&&!word(state+4)&&!word(state+12));
 state[0x12]=1;
 C(gh_game_world_light_move_002d7560(s,work,pools,GH_CAMERA_NATIVE_BINARY32_REFERENCE)==
   GH_LIGHT_UNSUPPORTED&&s->frontier==0x31d020&&state[0x12]==1&&!word(state+4));

 gh_game_mt rng,reference;
 gh_game_mt_seed_0030f2b0(&rng,0xd8a9523d);
 reference=rng;
 uint32_t noise[2];
 for(unsigned i=0;i<2;i++){
  C(!gh_game_mt_next_0030f348(&reference,noise+i));
  noise[i]=gh_frame_jitter_bits_0031d020_word(noise[i]);
 }
 float first,second;
 memcpy(&first,noise,sizeof(first));
 memcpy(&second,noise+1,sizeof(second));
 const volatile float first_scaled=first*4,second_scaled=second*4;
 const volatile float target_speed=85+first_scaled,target_amount=(10+second_scaled)/100.0f;
 gh_light_move_bindings bindings={.world_random=&rng};
 C(!gh_game_world_light_move_native_002d7560(s,work,pools,&bindings,
     GH_CAMERA_NATIVE_BINARY32_REFERENCE));
 float saved_speed,saved_amount;
 memcpy(&saved_speed,state,4);
 memcpy(&saved_amount,state+8,4);
 C(state[0x12]==20&&!state[0x10]&&!word(state+4)&&!word(state+12)&&
   saved_speed==target_speed&&saved_amount==target_amount&&!memcmp(&rng,&reference,sizeof(rng)));
 C(!gh_game_world_light_move_native_002d7560(s,work,pools,&bindings,
     GH_CAMERA_NATIVE_BINARY32_REFERENCE));
 float speed,amount;
 memcpy(&speed,state+4,4);
 memcpy(&amount,state+12,4);
 C(speed==saved_speed*.05f&&amount==saved_amount*.05f&&state[0x10]==3&&state[0x12]==19);
 C(!memcmp(&rng,&reference,sizeof(rng)));

 light[0x60]=0;
 half(light+0x5c,0x55a0); /* 90 degrees */
 half(light+0x5e,25);
 C(!gh_game_world_light_move_native_002d7560(s,work,pools,&bindings,
     GH_CAMERA_NATIVE_BINARY32_REFERENCE));
 C(state[0x10]==67&&state[0x12]==19&&word(state+4)==0x42b40000&&word(state+12)==0x3e800000);
 C(!memcmp(&rng,&reference,sizeof(rng)));

 put(world+0x78,count);
 memcpy(light,saved_light,sizeof(saved_light));
 memcpy(state,saved_state,sizeof(saved_state));
 puts("PASS actual EV04 cut3 oscillator bytes, cold phase, deterministic 2x MT, smoothing and static mode");
}


int main(int argc,char**argv){gh_disc*d;C(argc==2&&!gh_disc_open(argv[1],&d));size_t index;gh_disc_entry e;C(!gh_disc_find(d,"st1/r100.dat",&index)&&!gh_disc_entry_at(d,index,&e));uint8_t*b=malloc((size_t)e.size);C(b&&!gh_disc_read(d,index,0,b,(size_t)e.size));gh_bundle room;C(!gh_bundle_open(b,(size_t)e.size,&room));
 gh_heap*heap;gh_room_pools pools={0};C(!gh_heap_create(64*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&room,heap));size_t base=gh_heap_used(heap);
 gh_game_world_light*s=calloc(1,sizeof(*s));C(s&&!gh_game_world_light_construct_002d6d80(s)&&!gh_game_world_light_initialize_with_heap_002d6f40(s,&room,heap));
 /* Explicit attachment fixture only. Runtimecaller supplies its actualplayer. */
 int player_identity;gh_light_player_parts parts={&player_identity,NULL,0};C(!gh_game_world_light_select_002d8408(s,0,&parts)&&word(s->scalar+0x78)==13&&s->table.bytes&&!s->area.bytes&&gh_heap_used(heap)==base+2912);
 gh_boot_work work={0};uint8_t*p=s->scalar,*w=(uint8_t*)&work;uint8_t*snapshot=malloc(sizeof(s->scalar));C(snapshot);memcpy(snapshot,p,sizeof(s->scalar));
 put(w+0x5d8,0x4000);C(!gh_game_world_light_move_002d7560(s,&work,NULL,GH_CAMERA_EXACT_UNAVAILABLE)&&!memcmp(snapshot,p,sizeof(s->scalar)));put(w+0x5d8,0);
 uint64_t a=UINT64_C(0xabcdef0123456abc),c=UINT64_C(0x123456789abc7fff);memcpy(w+0x3e0,&a,8);memcpy(w+0x270,&c,8);
 const uint32_t expected[5]={0x3ffbbb73,0x410df34a,0x40ee147b,0x410e147b,0x41228f5c};const int modes[4]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
 for(unsigned m=0;m<4;m++){
  memset(p+0x16150,0,20);C(!fesetround(modes[m])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));
  C(!gh_game_world_light_move_002d7560(s,&work,&pools,GH_CAMERA_NATIVE_BINARY32_REFERENCE)&&fegetround()==modes[m]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);
  for(unsigned j=0;j<5;j++)C(word(p+0x16150+4*j)==expected[j]);
 }
 C(!fesetround(FE_TONEAREST));C(word(w+0x60c)==0x97976f&&wide(w+0x3e0)==((a&UINT64_C(0xffffffffffff8000))|0x16)&&wide(w+0x270)==((c&UINT64_C(0xffffffffffff8000))|0x16));
 C(word(p+0x78)==13&&word(p+0x16134)==0&&word(p+0x16144)==255&&word(p+0x16148)==255&&word(p+0x1614c)==255);
 gh_boot_work before=work;memcpy(snapshot,p,sizeof(s->scalar));int actor;uint32_t handle;
 C(!gh_room_object_register(&pools,&actor,&actor,999,8,&handle));C(gh_game_world_light_move_002d7560(s,&work,&pools,GH_CAMERA_NATIVE_BINARY32_REFERENCE)==GH_LIGHT_UNSUPPORTED&&s->frontier==0x2d74b8&&!memcmp(snapshot,p,sizeof(s->scalar))&&!memcmp(&before,&work,sizeof(work)));C(!gh_room_object_unregister(&pools,handle,&actor));
 put(p+0x80+4,1);C(gh_game_world_light_move_002d7560(s,&work,&pools,GH_CAMERA_NATIVE_BINARY32_REFERENCE)==GH_LIGHT_UNSUPPORTED&&s->frontier==0x2d7560);put(p+0x80+4,0);
 actual_ev04_waves(d,s,&work,&pools);
 live_models(s,&work,&pools);
 C(!gh_game_set_background_0030e5c0(&work,0,0,0,0)&&word(w+0x60c)==0x97976f&&(wide(w+0x270)&0x7fff)==0x10);before=work;C(!gh_game_set_background_0030e5c0(&work,99,1,2,3)&&!memcmp(&before,&work,sizeof(work)));
 C(!gh_game_world_light_release_presets(s)&&!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap));actual_r003_fog(d,heap);C(!gh_heap_used(heap)&&!gh_heap_destroy(heap));free(snapshot);free(s);free(b);gh_disc_close(d);
 puts("PASS actualR100 13light/emptycanonicalregistry3 move, independentfogvectors/4FEnv, genuineWorkGSbackgroundwrites and explicitpositivefrontiers");return 0;}
