#include "../../src/platform/native_input.h"
#include "../../src/platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
static void device_input(gh_native_input_map map){
 gh_native_input_config c;gh_native_input input;gh_native_input_snapshot s;
 gh_native_input_defaults(&c);C(gh_native_input_init(&input,&c,map));gh_native_input_focus(&input,1);
 gh_boot_work work={0},different={0},before;
 uint8_t*b=(uint8_t*)&work;
 memset(b+0x64,0x5a,12);memset(b+0x74,0x5a,8);
 const gh_native_gamepad_sample full={0xf3ff,32767,32767,-32768,-32768,31,255,1};
 C(gh_native_input_tick_device(&input,&work,&full,&s));
 const uint32_t stick=0x800000|0x100000|0x40000|0x20000;
 C(s.analog_valid&&s.analog.digital_raw==0xffff&&s.raw_held==(0xffff|stick));
 C(s.raw_pressed==s.raw_held&&s.raw_repeat_fast==s.raw_held&&s.raw_repeat_menu==s.raw_held);
 C(s.analog.axes[0]==127&&s.analog.axes[1]==127&&s.analog.axes[2]==-128&&s.analog.axes[3]==-128);
 C(s.analog.wire[0]==0&&s.analog.wire[1]==255&&s.analog.wire[2]==255&&s.analog.wire[3]==0);
 C(!gh_native_input_store_work(&different,&s));
 C(gh_native_input_store_work(&work,&s));
 C(!memcmp(b+0x70,s.analog.axes,4)&&!memcmp(b+0x1f0,s.analog.axes,4));
 C(!memcmp(b+0x190,&s.held,8)&&!memcmp(b+0x180,&s.analog.directions,4));
 for(unsigned i=0x64;i<0x70;i++)C(b[i]==0x5a&&b[0x1e0+i-0x60]==0x5a);
 for(unsigned i=0x74;i<0x7c;i++)C(b[i]==0x5a);
 memcpy(&before,&work,sizeof(work));
 C(!gh_native_input_store_work(&work,&s)&&!memcmp(&work,&before,sizeof(work)));
 C(gh_native_input_tick_device(&input,&work,&full,&s)&&!s.raw_pressed&&!s.analog.pressed_directions);
 C(gh_native_input_store_work(&work,&s));

 gh_native_gamepad_sample pad={0,32767,32767,0,0,30,31,1};
 gh_native_input_key(&input,'A',1);gh_native_input_key(&input,'D',1);
 C(gh_native_input_tick_device(&input,&work,&pad,&s));
 C(s.analog.digital_raw==2&&s.analog.axes[0]==0&&s.analog.axes[1]==127);
 C(gh_native_input_store_work(&work,&s));
 pad.connected=0; // Ignore stale values supplied for a disconnected controller.
 gh_native_input_key(&input,'A',0);gh_native_input_key(&input,'W',1);gh_native_input_key(&input,16,1);
 gh_native_input_key(&input,'C',1);
 C(gh_native_input_tick_device(&input,&work,&pad,&s));
 C(s.connected&&s.raw_pressed==0x80&&s.raw_released==(0x100000|2)&&s.raw_held==0x80);
 C(s.analog.axes[0]==68&&s.analog.axes[1]==67);
 C(gh_native_input_store_work(&work,&s));
 b[0x10]=1; // Original prior-axis-to-dpad mode.
 gh_native_input_focus(&input,0);
 C(gh_native_input_tick_device(&input,&work,&full,&s));
 C(s.connected&&!s.focused&&!s.raw_held&&!s.pressed);
 C(s.analog.raw_held==0x3000&&!s.analog.directions&&s.analog.axes[0]==0&&s.analog.axes[1]==0);
 C(gh_native_input_store_work(&work,&s)&&b[0x14]==1&&b[0x1f0]==0&&b[0x1f1]==0);
 gh_native_input_focus(&input,1);
 C(gh_native_input_tick_device(&input,&work,&pad,&s)&&s.analog_valid&&!s.raw_held&&!s.analog.directions);
 C(gh_native_input_store_work(&work,&s));
}
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
 device_input(map);
 config.repeat_multiplier=0;C(!gh_native_input_init(&input,&config,map));free(elf);gh_disc_close(d);puts("PASS actualELF inputtables0/1: physicalCross/Circle/dpad/Start, originaledge/repeatm1/m2 cadence, keyautorepeatignored, focusloss distinct fromsource disconnect, exactWork14 readiness/neighbor bytes; full digital device, four original axes, Shift tilt, stale commit, disconnect and neutral focus");return 0;}
