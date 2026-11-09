#include "../../src/core/screen_fade.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x)do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(void){gh_screen_fade s={0},before;gh_screen_fade_context c={512,1824,0,0};gh_fade_draw d;
 C(gh_screen_fade_start_00307208(&s,&c,0,10,0,0xff000000,0xff000000,1)==GH_FADE_OK);C(s.x==0x7000&&s.y==0x7200&&s.w==0x2000&&s.h==0x1c00&&s.time==1&&s.state==11&&s.ot_min==7&&s.ot_max==1&&s.alpha==UINT64_C(0x8000000044));
 c.input_mask_5e0=0x20000;C(gh_screen_fade_step_00307388(&s,&c,&d)==GH_FADE_OK&&!d.visible&&!s.count);c.input_mask_5e0=0;C(gh_screen_fade_step_00307388(&s,&c,&d)==GH_FADE_OK&&d.visible&&s.count==1&&!(s.state&4)&&d.rgba==0xff000000&&d.test==0x30000);C(gh_screen_fade_step_00307388(&s,&c,&d)==GH_FADE_OK&&(s.state&5)==5&&s.count==1&&d.rgba==0xff000000);C(gh_screen_fade_step_00307388(&s,&c,&d)==GH_FADE_OK&&(s.state&5)==5&&d.visible);
 C(gh_screen_fade_start_00307208(&s,&c,0,10,0,0xff000000,0,15)==GH_FADE_OK);for(unsigned i=0;i<15;i++){C(gh_screen_fade_step_00307388(&s,&c,&d)==GH_FADE_OK);C(d.rgba>>24==255-i*17&&s.count==i+1&&!(s.state&4));}C(gh_screen_fade_step_00307388(&s,&c,&d)==GH_FADE_OK&&d.rgba==0&&(s.state&4));
 // Negativechanneldivision is signedtrunczero, notfloor orfloat interpolation.
 C(gh_screen_fade_start_00307208(&s,&c,0,10,0,0x02000002,0,3)==GH_FADE_OK);C(gh_screen_fade_step_00307388(&s,&c,&d)==GH_FADE_OK&&d.rgba==0x02000002);C(gh_screen_fade_step_00307388(&s,&c,&d)==GH_FADE_OK&&d.rgba==0x02000002);C(gh_screen_fade_step_00307388(&s,&c,&d)==GH_FADE_OK&&d.rgba==0x01000001);
 c.frame_shift=1;C(gh_screen_fade_start_00307208(&s,&c,3,8,0,1,2,2)==GH_FADE_OK&&s.time==4&&s.kind==3);gh_screen_fade_set_ot_00307760(&s,2,8);C(s.ot_min==2&&s.ot_max==8);c.frame_shift=0;C(gh_screen_fade_start_00307208(&s,&c,0,8,0,1,2,0)==GH_FADE_OK);C(gh_screen_fade_step_00307388(&s,&c,&d)==GH_FADE_OK&&d.visible&&d.rgba==2&&!s.state&&d.test==0x70000);C(gh_screen_fade_step_00307388(&s,&c,&d)==GH_FADE_OK&&!d.visible);gh_screen_fade_kill_003076e0(&s);C(!gh_screen_fade_status_00307718(&s));before=s;C(gh_screen_fade_start_00307208(&s,&c,0,10,3,1,2,3)==GH_FADE_UNSUPPORTED&&!memcmp(&s,&before,sizeof(s)));puts("PASS actualclassFADE0x28/start/status/kill/OT: R020oneframeblack requires2updates, hold/done,15stepalpha, signedbyteinterpolation, sourcegate, frameshift andfinaldraw");return 0;}
