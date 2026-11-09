#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../../src/platform/window_host.h"
#include "../../src/platform/disc.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cstring>
#define C(x) do{if(!(x)){std::fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
int main(int argc,char**argv){C(argc==2);gh_disc*d;C(gh_disc_open(argv[1],&d)==GH_DISC_OK);std::vector<unsigned char>elf((size_t)gh_disc_boot_elf_size(d));C(gh_disc_read_boot_elf(d,0,elf.data(),elf.size())==GH_DISC_OK);gh_window_config c={};c.width=640;c.height=480;c.xinput_index=-1;gh_native_input_defaults(&c.input);C(gh_native_input_map_from_elf(elf.data(),elf.size(),0,&c.map));gh_window_host*w=nullptr;C(gh_window_create(&c,&w)==GH_WINDOW_OK);HWND hwnd=(HWND)gh_window_native_handle(w);C(hwnd&&!IsWindowVisible(hwnd));
 // Only our hidden HWND receives synthetictestmessages; not userinput evidence.
 gh_native_input_snapshot input;SendMessageW(hwnd,WM_SETFOCUS,0,0);PostMessageW(hwnd,WM_KEYDOWN,'X',1);C(gh_window_poll(w,&input)==GH_WINDOW_OK&&input.raw_pressed==0x20);PostMessageW(hwnd,WM_KEYDOWN,'X',1);C(gh_window_poll(w,&input)==GH_WINDOW_OK&&!input.raw_pressed);PostMessageW(hwnd,WM_KILLFOCUS,0,0);C(gh_window_poll(w,&input)==GH_WINDOW_OK&&!input.held&&!input.pressed&&!input.repeat_menu);
 const unsigned sw=37,sh=23;std::vector<unsigned char>source(sw*sh*4),result(640*480*4);for(unsigned y=0;y<sh;y++)for(unsigned x=0;x<sw;x++){size_t i=((size_t)y*sw+x)*4;source[i]=(unsigned char)(x*7);source[i+1]=(unsigned char)(y*11);source[i+2]=(unsigned char)(x+y);source[i+3]=37;}
 C(gh_window_present(w,source.data(),source.size(),sw,sh,16,9)==GH_WINDOW_OK);unsigned rw,rh;C(gh_window_readback(w,result.data(),result.size(),&rw,&rh)==GH_WINDOW_OK&&rw==640&&rh==480);for(unsigned y=0;y<rh;y++)for(unsigned x=0;x<rw;x++){auto*p=result.data()+((size_t)y*rw+x)*4;C(p[3]==255);if(y<60||y>=420)C(!p[0]&&!p[1]&&!p[2]);else{const auto*s=source.data()+(((size_t)(y-60)*sh/360)*sw+(size_t)x*sw/640)*4;C(!std::memcmp(p,s,3));}}
 gh_frame_clock clock;C(gh_frame_clock_init(&clock,30000,1001));ULONGLONG start=GetTickCount64();for(unsigned i=0;i<3;i++)C(gh_window_wait_frame(w,&clock)==GH_WINDOW_OK);C(GetTickCount64()-start>=95&&GetTickCount64()-start<1000);C(!IsWindowVisible(hwnd));PostMessageW(hwnd,WM_CLOSE,0,0);C(gh_window_poll(w,&input)==GH_WINDOW_CLOSED);gh_window_destroy(w);gh_disc_close(d);puts("PASS hidden HWND/nativeD3D11 swapchain307200pixels exact aspectfit/nearest/opaqueRGB, ownwindow syntheticinput/focusloss, QPC30Hz pacing and close");return 0;}
