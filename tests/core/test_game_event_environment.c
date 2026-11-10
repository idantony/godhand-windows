#include "game_event_environment.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static unsigned half(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static const uint8_t*at(const uint8_t*b,size_t size,uint32_t address){
 size_t table=word(b+28);for(unsigned i=0;i<half(b+44);i++){const uint8_t*p=b+table+(size_t)i*half(b+42);uint32_t base=word(p+8),n=word(p+16),off=word(p+4);if(word(p)==1&&address>=base&&(uint64_t)address+12<=(uint64_t)base+n){C((uint64_t)off+n<=size);return b+off+(address-base);}}return NULL;
}
int main(int argc,char**argv){
 C(argc==2);FILE*f=fopen(argv[1],"rb");C(f);C(!fseek(f,0,SEEK_END));long length=ftell(f);C(length>52);rewind(f);uint8_t*elf=malloc((size_t)length);C(elf&&fread(elf,1,(size_t)length,f)==(size_t)length&&!fclose(f));
 gh_game_event_config config={0};C(!gh_game_event_config_from_elf(&config,elf,(size_t)length)&&config.initialized&&!config.record&&!config.flags);
 C(gh_game_event_config_from_elf(&config,elf,(size_t)length)==GH_EVENT_ARGUMENT);
 const int32_t numbers[]={0,1,2,-1,INT_MIN,INT_MAX};
 for(unsigned i=0;i<sizeof(numbers)/sizeof(numbers[0]);i++){
  uint64_t arithmetic=(uint64_t)(uint32_t)numbers[i]*12+0x3c0550;uint32_t address=(uint32_t)arithmetic;const uint8_t*expected=at(elf,(size_t)length,address);C(expected);
  C(!gh_game_event_config_set_00299320(&config,numbers[i],elf,(size_t)length)&&config.record==expected&&config.record_address==address&&config.flags==word(expected+4));
 }
 gh_game_event event;memset(&event,0xa5,sizeof(event));event.initialized=1;gh_game_event before=event,expected=event;
 expected.fields.text_token=0;expected.fields.state=expected.fields.phase=expected.fields.unknown06=expected.fields.unknown07=0;expected.fields.flags=0;expected.fields.data_number=0;
 C(!gh_game_event_reset_00297660(&event)&&!memcmp(&event,&expected,sizeof(event)));
 event=before;event.fields.flags=0x80000001;expected=event;expected.fields.text_token=0;expected.fields.state=expected.fields.phase=expected.fields.unknown06=expected.fields.unknown07=0;expected.fields.flags=2;expected.fields.data_number=1;
 C(!gh_game_event_init_environment_002975f8(&event,&config,1,elf,(size_t)length)&&!memcmp(&event,&expected,sizeof(event))&&config.record==at(elf,(size_t)length,0x3c055c));
 gh_game_event_config saved=config;C(!gh_game_event_init_environment_002975f8(&event,&config,1000000,NULL,0)&&!memcmp(&event,&expected,sizeof(event))&&!memcmp(&config,&saved,sizeof(config)));
 event.fields.flags=0;before=event;C(gh_game_event_init_environment_002975f8(&event,&config,1000000,elf,(size_t)length)==GH_EVENT_BOUNDS&&!memcmp(&event,&before,sizeof(event))&&!memcmp(&config,&saved,sizeof(config)));
 C(gh_game_event_config_set_00299320(&config,1,elf,51)==GH_EVENT_FORMAT&&!memcmp(&config,&saved,sizeof(config)));
 uint8_t actor[0x1690];memset(actor,0xa5,sizeof(actor));const uint32_t xyz[3]={0x3f800000,0xc0000000,0x40800000};memcpy(actor+0xb0,xyz,12);uint32_t yaw=0x3f000000;memcpy(actor+0x104,&yaw,4);
 gh_title_player_view view={.bytes=actor,.size=sizeof(actor),.alias_f0=actor+0xb0};gh_boot_work work={0};gh_screen_fade fade;memset(&fade,0x6d,sizeof(fade));gh_display_state display={.initialized=1,.screen_width=512,.gs_y_origin=1824};
 gh_event_play_start_bindings b={&event,&config,&work,&view,&fade,&display,elf,(size_t)length};uint32_t frontier=99;
 event=expected;event.fields.state=4;event.fields.phase=5;event.fields.unknown06=6;event.fields.unknown07=7;event.fields.position_bits[3]=0x12345678;event.fields.skip_ok=1;config.flags=0;gh_screen_fade untouched=fade;
 C(!gh_game_event_play_start_00296458(&b,99,&frontier)&&!frontier&&event.fields.flags==3&&event.fields.data_number==1&&!event.fields.skip_ok&&!event.fields.state&&!event.fields.phase&&!event.fields.unknown06&&!event.fields.unknown07);
 C(!memcmp(event.fields.position_bits,xyz,12)&&event.fields.position_bits[3]==0x12345678&&event.fields.angle_bits==yaw&&word(actor+0x54c)==0x42700000&&word((uint8_t*)&work+0x5e4)==0x20000000&&!memcmp(&fade,&untouched,sizeof(fade)));
 config.flags=8;C(!gh_game_event_play_start_00296458(&b,1,&frontier)&&fade.state==11&&fade.color_from==0xff000000&&fade.color_to==0xff000000&&!fade.time&&!fade.count&&fade.alpha==UINT64_C(0x8000000044));
 b.player=NULL;event.fields.flags=2;event.fields.skip_ok=1;C(gh_game_event_play_start_00296458(&b,1,&frontier)==GH_EVENT_UNSUPPORTED&&frontier==0x2db6b0&&event.fields.flags==3&&!event.fields.skip_ok);
 b.player=&view;b.display=NULL;C(gh_game_event_play_start_00296458(&b,1,&frontier)==GH_EVENT_UNSUPPORTED&&frontier==0x307208);
 free(elf);puts("PASS actual586B30 config/32bit record arithmetic, exact297660 preservation/once2975F8, actual296458 sameplayer pose/timer/Work gate/optionalzero-framefade, missingownerfrontiers; no Eventplaybackclaim");return 0;
}
