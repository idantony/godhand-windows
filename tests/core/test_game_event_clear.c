#include "game_event_clear.h"
#include "../platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t rd(const void*p){uint32_t x;memcpy(&x,p,4);return x;}
static void wr(void*p,uint32_t x){memcpy(p,&x,4);}
/* Dispatch-only fixture: never selected by production, never an animation
 * implementation. It checks phase ordering separately from strict playerprefix. */
static gh_game_event_status dispatch_fixture(void*ctx,uint32_t*f){unsigned*c=ctx;(*c)++;(void)f;return GH_EVENT_OK;}
static gh_object_status no_slot(void*ctx,int32_t slot,uint32_t*f){(void)ctx;(void)slot;*f=0xdeadbeef;return GH_OBJECT_UNSUPPORTED;}
int main(int argc,char**argv){
 C(argc==2);gh_disc*d=NULL;C(!gh_disc_open(argv[1],&d));size_t n=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(n);C(elf&&!gh_disc_read_boot_elf(d,0,elf,n));gh_disc_close(d);
 gh_game_event event={0};gh_game_event_config config={0};C(!gh_game_event_from_elf(&event,elf,n)&&!gh_game_event_config_from_elf(&config,elf,n));event.fields.state=1;
 uint8_t bytes[GH_TITLE_PLAYER_BYTES];memset(bytes,0xa5,sizeof(bytes));wr(bytes+0x15f4,0);wr(bytes+0x165c,0);wr(bytes+0x161c,0);wr(bytes+0x1620,0);
 gh_game_title_player player={.view={.bytes=bytes,.size=sizeof(bytes)}};gh_sound_control sound={.initialized=1,.flags_ac=0xffffffff,.flags_b0=0xffffffff};gh_player_abort_bindings abort={.player=&player,.sound=&sound};
 gh_game_init_state init={0};gh_room_pools pools={.initialized=1};gh_event_clear_bindings b={.event=&event,.config=&config,.init=&init,.pools=&pools,.player_abort=&abort};gh_event_clear_state state={0};int done=99;
 C(gh_event_clear_00296818(&state,&b,&done)==GH_EVENT_UNSUPPORTED&&state.frontier==0x2a8578&&!done&&event.fields.phase==0);
 C(!rd(bytes+0x2f4)&&bytes[0x2f3]==0xa5&&bytes[0x2f8]==0xa5&&sound.flags_ac==0xffbfffff&&sound.flags_b0==0xffbfffff);
 wr(bytes+0x2f4,0x12345678);C(gh_event_clear_00296818(&state,&b,&done)==GH_EVENT_UNSUPPORTED&&rd(bytes+0x2f4)==0x12345678); /* no repeated prefix */
 memset(&state,0,sizeof(state));wr(bytes+0x15f4,2);C(gh_event_clear_00296818(&state,&b,&done)==GH_EVENT_UNSUPPORTED&&state.frontier==0x13b9b8&&!rd(bytes+0x15f4));
 unsigned calls=0;b.player_clear_0012c418=dispatch_fixture;b.context=&calls;memset(&state,0,sizeof(state));event.fields.phase=0;
 C(!gh_event_clear_00296818(&state,&b,&done)&&!done&&event.fields.phase==1&&calls==1);
 for(unsigned p=1;p<8;p++){C(!gh_event_clear_00296818(&state,&b,&done)&&!done&&event.fields.phase==p+1);}
 C(gh_event_clear_00296818(&state,&b,&done)==GH_EVENT_UNSUPPORTED&&state.frontier==0x1fef30&&calls==9);
 memset(&state,0,sizeof(state));event.fields.phase=6;config.flags=0x400;C(!gh_event_clear_00296818(&state,&b,&done)&&done&&event.fields.state==2&&!event.fields.phase);
 event.fields.state=1;event.fields.phase=0;config.flags=1;C(!gh_event_clear_00296818(&state,&b,&done)&&done&&calls==11);
 event.fields.state=1;event.fields.phase=0;config.flags=0;wr(init.enemies+0x500,1);C(gh_event_clear_00296818(&state,&b,&done)==GH_EVENT_UNSUPPORTED&&state.frontier==0x290770&&!done&&!event.fields.phase);
 memset(&state,0,sizeof(state));event.fields.phase=8;wr(init.enemies+0x500,0);gh_stage_slots slots={0};gh_game_objects*objects=NULL;C(!gh_game_objects_create(&slots,0,&objects));C(!gh_stage_slots_reset(&slots,NULL));
 for(unsigned i=0;i<80;i++)wr(slots.records+(size_t)i*0x5c+4,0xffffffff);b.room_objects=objects;b.release_sound_slot=no_slot;
 C(!gh_event_clear_00296818(&state,&b,&done)&&done&&event.fields.state==2);for(unsigned i=0;i<80;i++)C(!rd(slots.records+(size_t)i*0x5c+4));gh_game_objects_destroy(objects);
 free(elf);puts("PASS actual ISO Event cold state, real12C418 prefix/126770/byte-neighbors and strictanimationfrontier; isolated dispatch fixture phase0..8/config gates/positive enemy rejection (not completed player clear)");return 0;
}
