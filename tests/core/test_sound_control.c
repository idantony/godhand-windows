#include "sound_control.h"
#include <stdio.h>
#include <stdlib.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(int argc,char **argv){gh_disc *d;gh_audio_registry *r;gh_sound_control s={0},copy;
 C(argc==2 && gh_disc_open(argv[1],&d)==GH_DISC_OK && gh_audio_registry_create(&r)==GH_AUDIO_OK);
 C(gh_sound_control_from_startup(&s,d,r)==GH_VOICE_UNSUPPORTED && !s.initialized);
 C(gh_audio_startup_load(r,d)==GH_AUDIO_OK);
 C(gh_sound_control_from_startup(&s,d,r)==GH_VOICE_OK && s.initialized && !s.flags_ac && !s.flags_b0 && !s.bgm_state_13c);
 C(s.event_mask50==0x80000000u && s.output_mode==1);
 for(unsigned i=0;i<6;i++)C(s.gain_bits[i]==(i==3||i==5?0x42b40000u:0x42fe0000u));
 C(gh_sound_control_from_startup(&s,d,r)==GH_VOICE_UNSUPPORTED);
 copy=s;copy.flags_ac=0xc4dfffff;copy.flags_b0=0x88401234;copy.bgm_state_13c=4;
 C(gh_sound_control_prepare_scene_002ca8c8(&copy)==GH_VOICE_OK && !copy.bgm_state_13c && copy.flags_ac==0xc49fffff && copy.flags_b0==0x88001234);
 C(copy.event_mask50==s.event_mask50 && copy.output_mode==s.output_mode);
 for(unsigned i=0;i<6;i++)C(copy.gain_bits[i]==s.gain_bits[i]);
 C(gh_sound_control_prepare_scene_002ca8c8(&s)==GH_VOICE_OK && !s.bgm_state_13c);
 gh_audio_registry_destroy(r);gh_disc_close(d);puts("PASS actualELF PT_LOAD/BSS controller origins plus realstartupready; sceneleaf preserves unrelated flags, no coldzero surrogate");return 0;}
