#include "../../src/platform/native_input.h"
#include "../../src/platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(int argc,char**argv){gh_disc*d;size_t n;unsigned char*elf;gh_native_input_map map;gh_native_input_config config;gh_native_input input;gh_native_input_snapshot s;unsigned i;C(argc==2);C(gh_disc_open(argv[1],&d)==GH_DISC_OK);n=(size_t)gh_disc_boot_elf_size(d);elf=malloc(n);C(elf&&gh_disc_read_boot_elf(d,0,elf,n)==GH_DISC_OK);C(gh_native_input_map_from_elf(elf,n,0,&map));C(!gh_native_input_map_from_elf(elf,40,0,&map));C(!gh_native_input_map_from_elf(elf,n,2,&map));gh_native_input_defaults(&config);C(gh_native_input_init(&input,&config,map));gh_native_input_focus(&input,1);
 gh_native_input_key(&input,'Z',1);C(gh_native_input_tick(&input,0,&s));C(s.raw_pressed==0x40&&s.pressed==UINT64_C(0x220000000));gh_native_input_key(&input,'Z',1);C(gh_native_input_tick(&input,0,&s)&&!s.pressed);gh_native_input_key(&input,'Z',0);C(gh_native_input_tick(&input,0,&s)&&s.raw_released==0x40&&!s.held);
 gh_native_input_key(&input,'X',1);C(gh_native_input_tick(&input,0,&s)&&s.raw_pressed==0x20&&s.pressed==UINT64_C(0x810100001));for(i=0;i<6;i++)C(gh_native_input_tick(&input,0,&s)&&!s.repeat_menu);C(gh_native_input_tick(&input,0,&s)&&s.repeat_menu==s.held);
 gh_native_input_focus(&input,0);C(gh_native_input_tick(&input,0xffff,&s)&&!s.held&&!s.pressed&&!s.released&&!s.repeat_menu);gh_native_input_key(&input,'X',1);C(gh_native_input_tick(&input,0,&s)&&!s.held);gh_native_input_focus(&input,1);C(gh_native_input_tick(&input,0,&s)&&!s.held);
 C(gh_native_input_tick(&input,0x11,&s)&&s.raw_pressed==(0x1000|0x800));C((s.pressed&UINT64_C(0xe00000))==UINT64_C(0xe00000));C(s.pressed&UINT64_C(0x1000000));
 config.repeat_multiplier=2;C(gh_native_input_init(&input,&config,map));gh_native_input_focus(&input,1);gh_native_input_key(&input,'X',1);C(gh_native_input_tick(&input,0,&s)&&s.repeat_menu);for(i=0;i<12;i++)C(gh_native_input_tick(&input,0,&s)&&!s.repeat_menu);C(gh_native_input_tick(&input,0,&s)&&s.repeat_menu);
 C(gh_native_input_map_from_elf(elf,n,1,&map));C(gh_native_input_init(&input,&config,map));gh_native_input_focus(&input,1);gh_native_input_key(&input,'Z',1);C(gh_native_input_tick(&input,0,&s)&&s.pressed==UINT64_C(0x210000000));gh_native_input_key(&input,'Z',0);gh_native_input_key(&input,27,1);C(gh_native_input_tick(&input,0,&s)&&s.raw_pressed==0x10&&(s.pressed&UINT64_C(0x20000000)));
 C(s.connected);gh_boot_work work;memset(&work,0xa5,sizeof(work));C(gh_native_input_store_work(&work,&s));
 unsigned char*bytes=(unsigned char*)&work;C(bytes[0x14]==1&&bytes[0x13]==0xa5&&bytes[0x15]==0xa5&&bytes[0x16]==0xa5);
 C(!memcmp(bytes+8,&s.raw_pressed,4)&&!memcmp(bytes+0x1a0,&s.pressed,8));
 gh_native_input_focus(&input,0);C(gh_native_input_tick(&input,0,&s)&&s.connected&&!s.held);
 gh_native_input_focus(&input,1);gh_native_input_connected(&input,0);C(gh_native_input_tick(&input,0xffff,&s)&&!s.connected&&!s.held&&!s.pressed);
 C(gh_native_input_store_work(&work,&s)&&bytes[0x14]==0&&bytes[0x15]==0xa5);gh_native_input_connected(&input,1);
 C(gh_native_input_tick(&input,0x1000,&s)&&s.connected&&s.raw_pressed==0x40);C(gh_native_input_store_work(&work,&s)&&bytes[0x14]==1);
 s.connected=2;C(!gh_native_input_store_work(&work,&s)&&bytes[0x14]==1);
 config.repeat_multiplier=0;C(!gh_native_input_init(&input,&config,map));free(elf);gh_disc_close(d);puts("PASS actualELF inputtables0/1: physicalCross/Circle/dpad/Start, originaledge/repeatm1/m2 cadence, keyautorepeatignored, focusloss distinct fromsource disconnect, exactWork14 readiness/neighbor bytes");return 0;}
