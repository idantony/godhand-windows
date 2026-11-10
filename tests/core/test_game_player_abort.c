#include "game_player_abort.h"
#include "game_scenario_soft_event.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x)do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t word(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(void*p,uint32_t n){memcpy(p,&n,4);}
typedef struct{unsigned calls;uint32_t handle;} unavailable;
static gh_player_abort_status missing(void*ctx,uint32_t h,uint32_t*f){unavailable*u=ctx;u->calls++;u->handle=h;*f=0x375050;return GH_PLAYER_ABORT_UNSUPPORTED;}
int main(int argc,char**argv){C(argc==2);gh_disc*d;size_t i;gh_disc_entry file;C(!gh_disc_open(argv[1],&d)&&!gh_disc_find(d,"st1/r100.dat",&i)&&!gh_disc_entry_at(d,i,&file));uint8_t*raw=malloc((size_t)file.size);C(raw&&!gh_disc_read(d,i,0,raw,(size_t)file.size));gh_disc_close(d);gh_bundle room;C(!gh_bundle_open(raw,(size_t)file.size,&room));gh_heap*heap;gh_room_pools pools={0};C(!gh_heap_create(16*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&room,heap));gh_boot_work work={0};gh_game_title_player player={0};int nonnull;C(!gh_game_player_construct_00312f00(&player,&pools,&work,&nonnull)&&nonnull&&player.view.initial_memory==GH_TITLE_MEMORY_ORIGINAL_FACTORY_ZERO);uint8_t*p=player.view.bytes;C(!word(p+0x165c)&&!word(p+0x161c)&&!word(p+0x1620));
 /* Isolatedcanonical sound fixture, explicit input bits; production must borrow
  * Root's actuallive sound_control, never instantiate a new shadow for abort. */
 gh_sound_control sound={.flags_ac=0xffffffffu,.flags_b0=0xa5ffffffu,.initialized=1};gh_player_abort_bindings b={&player,&sound,NULL,NULL,NULL};gh_game_player_abort state={0};uint8_t saved[GH_TITLE_PLAYER_BYTES];memcpy(saved,p,sizeof(saved));C(!gh_game_player_abort_00126770(&state,&b)&&!state.active&&sound.flags_ac==0xffbfffff&&sound.flags_b0==0xa5bfffff&&!memcmp(saved,p,sizeof(saved)));
 sound.flags_ac|=0x400000;sound.flags_b0|=0x400000;C(!gh_game_player_abort_00126770(&state,&b)&&!(sound.flags_ac&0x400000)&&!(sound.flags_b0&0x400000));
 put(p+0x165c,0x123);sound.flags_ac=sound.flags_b0=0xffffffffu;C(gh_game_player_abort_00126770(&state,&b)==GH_PLAYER_ABORT_UNSUPPORTED&&state.frontier==0x1f6f10&&state.stage==0&&word(p+0x165c)==0x123&&sound.flags_ac==0xffffffffu);gh_sound_control foreign=sound;gh_player_abort_bindings wrong=b;wrong.sound=&foreign;C(gh_game_player_abort_00126770(&state,&wrong)==GH_PLAYER_ABORT_BUSY);
 // Fresh independentabort fixture: no positivecallee ever returns fakeOK.
 memset(&state,0,sizeof(state));put(p+0x165c,0);put(p+0x161c,0x42);C(gh_game_player_abort_00126770(&state,&b)==GH_PLAYER_ABORT_UNSUPPORTED&&state.frontier==0x2cbba0&&state.stage==1&&word(p+0x161c)==0x42);unavailable calls={0};b.se_stop_002cbba0=missing;b.context=&calls;C(gh_game_player_abort_00126770(&state,&b)==GH_PLAYER_ABORT_UNSUPPORTED&&state.frontier==0x375050&&state.stage==1&&calls.handle==0x42&&calls.calls==1&&word(p+0x161c)==0x42&&sound.flags_ac==0xffffffffu);put(p+0x161c,0x99);C(gh_game_player_abort_00126770(&state,&b)==GH_PLAYER_ABORT_UNSUPPORTED&&calls.handle==0x42&&calls.calls==2&&word(p+0x161c)==0x99&&sound.flags_ac==0xffffffffu);
 memset(&state,0,sizeof(state));put(p+0x161c,0);put(p+0x1620,0x43);calls.calls=0;C(gh_game_player_abort_00126770(&state,&b)==GH_PLAYER_ABORT_UNSUPPORTED&&state.stage==3&&calls.calls==1&&calls.handle==0x43&&word(p+0x1620)==0x43&&sound.flags_ac==0xffbfffffu&&sound.flags_b0==0xffbfffffu);sound.flags_ac|=0x400000;C(gh_game_player_abort_00126770(&state,&b)==GH_PLAYER_ABORT_UNSUPPORTED&&calls.calls==2&&(sound.flags_ac&0x400000));
 memset(&state,0,sizeof(state));put(p+0x1620,0);b.se_stop_002cbba0=NULL;C(!gh_game_player_abort_00126770(&state,&b));sound.initialized=0;C(gh_game_player_abort_00126770(&state,&b)==GH_PLAYER_ABORT_ARGUMENT);C(!gh_game_title_player_release(&player)&&!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));free(raw);puts("PASS actualR100312F00 factoryzero abort-domain reads, realzero-SeStop branch, sameSoundB0/AC masks/order, positiveaction/voice precisefrontiers+cachedargument/prefix/foreignowner/repeatedcalls; no fakepositive action/audio success");return 0;}
