#include "game_world_light_step.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static uint64_t wide(const uint8_t*p){uint64_t n;memcpy(&n,p,8);return n;}
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
 uint32_t flags=word(p+0x80+0x58);put(p+0x80+0x58,flags|0x100000);C(gh_game_world_light_move_002d7560(s,&work,&pools,GH_CAMERA_NATIVE_BINARY32_REFERENCE)==GH_LIGHT_UNSUPPORTED&&s->frontier==0x156488);put(p+0x80+0x58,flags);
 C(!gh_game_set_background_0030e5c0(&work,0,0,0,0)&&word(w+0x60c)==0x97976f&&(wide(w+0x270)&0x7fff)==0x10);before=work;C(!gh_game_set_background_0030e5c0(&work,99,1,2,3)&&!memcmp(&before,&work,sizeof(work)));
 C(!gh_game_world_light_release_presets(s)&&!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));free(snapshot);free(s);free(b);gh_disc_close(d);
 puts("PASS actualR100 13light/emptycanonicalregistry3 move, independentfogvectors/4FEnv, genuineWorkGSbackgroundwrites and explicitpositivefrontiers");return 0;}
