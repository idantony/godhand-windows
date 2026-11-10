#include "se_voice.h"
#include <stdio.h>
#include <stdlib.h>
#include <fenv.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(int argc,char**argv){
 gh_disc*d;gh_audio_registry*r;gh_audio_output*out;int32_t error;gh_se_voice_engine*s;gh_se_mix mix;
 C(argc==2&&!gh_disc_open(argv[1],&d)&&!gh_audio_registry_create(&r)&&!gh_audio_startup_load(r,d)&&!gh_audio_output_create(&out,&error)&&!gh_audio_output_set_gain(out,0));
 C(!gh_se_mix_boot_defaults_002c9e78(&mix,0,GH_SE_DSP_NATIVE_LINEAR_REFERENCE)&&!gh_se_voice_create(r,out,&mix,&s)&&!gh_se_voice_bind_origin_seh(s,NULL,0));
 /* Original queue bound counts commands even when no owner matches. */
 for(unsigned i=0;i<48;i++){int accepted=-1;C(!gh_se_voice_request_stop_00375050(s,0x70000000+i,10,&accepted)&&accepted);}
 int accepted=-1;C(!gh_se_voice_request_stop_00375050(s,0x60000000,10,&accepted)&&!accepted&&!gh_se_voice_update(s));
 uint32_t handle;gh_se_voice_info v;C(!gh_se_voice_request_origin(s,0,0x2d,-1,-1,0,0,&handle)&&handle&&!gh_se_voice_update(s)&&!gh_se_voice_query(s,handle,&v)&&v.level_q14==8163&&v.stop_current==32512);
 uint32_t native=v.native_voice;int active;uint64_t played=0;
 for(unsigned i=0;i<100&&!played;i++){Sleep(5);C(!gh_audio_output_active(out,native,&active,&played));}C(active&&played>0);
 C(!gh_se_voice_request_stop_00375050(s,handle,10,&accepted)&&accepted&&!gh_se_voice_query(s,handle,&v)&&!v.stopping&&v.native_voice==native); /* queued, not immediate */
 const int modes[4]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
 for(unsigned i=1;i<=11;i++){C(!fesetround(modes[i%4]));
  C(!gh_se_voice_update(s)&&!gh_se_voice_query(s,handle,&v));int cur=32512-3251*(int)i;if(cur<0)cur=0;
  unsigned level=(16327u*(unsigned)cur/32512)/2;float left,right;
  C(v.stopping&&!v.retiring&&v.native_voice==native&&v.stop_current==cur&&v.stop_step==-3251&&v.stop_count==i&&v.level_q14==level);
  C(fegetround()==modes[i%4]&&!gh_audio_output_voice_gain_query(out,native,&left,&right)&&left==(float)level/16384.0f&&right==left);
 }
 C(!fesetround(FE_TONEAREST));
 C(!gh_se_voice_update(s)&&!gh_se_voice_query(s,handle,&v)&&v.retiring&&!v.native_voice&&!v.stopping&&gh_se_voice_active_nodes(s)==1);
 C(gh_audio_output_active(out,native,&active,&played)==GH_VOICE_BOUNDS); /* real source destroyed at12 */
 C(!gh_se_voice_update(s)&&gh_se_voice_query(s,handle,&v)==GH_VOICE_BOUNDS&&!gh_se_voice_active_nodes(s)); /* EE retirement nextvisit */
 C(gh_audio_output_voice_gain(out,native,0,0)==GH_VOICE_BOUNDS&&gh_audio_output_voice_gain(out,native,-1,0)==GH_VOICE_ARGUMENT);
 C(!gh_se_voice_request_origin(s,0,0x2d,-1,-1,0,0,&handle)&&handle&&!gh_se_voice_update(s)&&!gh_se_voice_query(s,handle,&v)&&v.native_voice);
 C(gh_se_voice_request_stop_00375050(s,handle,-2,&accepted)==GH_VOICE_UNSUPPORTED&&!accepted&&gh_se_voice_frontier(s)==0x378020);
 C(!gh_se_voice_request_stop_00375050(s,handle,0,&accepted)&&accepted&&!gh_se_voice_update(s)&&!gh_se_voice_query(s,handle,&v)&&v.retiring&&!v.native_voice&&!v.stop_count);
 C(!gh_se_voice_update(s)&&!gh_se_voice_active_nodes(s));
 gh_se_voice_destroy(s);gh_audio_output_destroy(out);gh_audio_registry_destroy(r);gh_disc_close(d);
 puts("PASS real muted native source Stop10 queue48, actual integer step/count/gain, zero at11/stop12/EE retirement13");return 0;
}
