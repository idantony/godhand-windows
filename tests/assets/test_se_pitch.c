#include "se_pitch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint64_t add(uint64_t h,uint32_t v){for(unsigned i=0;i<4;i++)h=(h^((v>>(i*8))&255))*UINT64_C(1099511628211);return h;}
int main(int argc,char**argv){C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));uint64_t n=99;size_t resources=gh_disc_count(d);C(!gh_disc_iso_file_size(d,"modules\\irx.ims",&n)&&n==314304);uint8_t header[16];C(!gh_disc_read_iso_file(d,"MODULES/IRX.IMS",0,header,16)&&!memcmp(header,"IMS\0",4));C(gh_disc_read_iso_file(d,"MODULES/IRX.IMS",n,header,1)==GH_DISC_BOUNDS);C(gh_disc_read_iso_file(d,"MODULES/IRX.IMS",UINT64_MAX,NULL,0)==GH_DISC_BOUNDS);C(!gh_disc_read_iso_file(d,"MODULES/IRX.IMS",n,NULL,0));C(gh_disc_iso_file_size(d,"../MODULES/IRX.IMS",&n)==GH_DISC_UNSAFE_PATH&&n==314304);C(gh_disc_iso_file_size(d,"/MODULES/IRX.IMS",&n)==GH_DISC_UNSAFE_PATH);C(gh_disc_iso_file_size(d,"MODULES",&n)==GH_DISC_NOT_FOUND);C(gh_disc_iso_file_size(d,"MODULES/missing.irx",&n)==GH_DISC_NOT_FOUND);C(!gh_disc_iso_file_size(d,"SLUS_215.03",&n)&&n==gh_disc_boot_elf_size(d));C(gh_disc_count(d)==resources);
 gh_se_pitch_reference pitch={0};C(!gh_se_pitch_load_reference(&pitch,d)&&pitch.loaded&&pitch.random_state==0xd37);gh_se_pitch_reference before=pitch;C(gh_se_pitch_load_reference(&pitch,d)==GH_VOICE_UNSUPPORTED&&!memcmp(&pitch,&before,sizeof(pitch)));
 const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};for(unsigned mode=0;mode<4;mode++){
  C(!fesetround(modes[mode])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));uint64_t hash=UINT64_C(14695981039346656037);
  for(unsigned state=0;state<65536;state++){pitch.random_state=(uint16_t)state;int16_t chosen;C(!gh_se_pitch_choose_0037d760(&pitch,-128,128,&chosen));hash=add(add(hash,pitch.random_state),(uint32_t)(int32_t)chosen);}C(hash==UINT64_C(0x295a0be5a23c64ab));
  hash=UINT64_C(14695981039346656037);const uint32_t rates[]={1000,14000,16000,18000,48000};for(int offset=-512;offset<=512;offset++)for(unsigned i=0;i<5;i++){uint16_t q;C(!gh_se_pitch_q12_000038e4(&pitch,(int16_t)offset,rates[i],&q));hash=add(hash,q);}C(hash==UINT64_C(0x573b26bb3a30291c));C(fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);
 }
 C(!fesetround(FE_TONEAREST));pitch.random_state=0xd37;int16_t same;C(!gh_se_pitch_choose_0037d760(&pitch,17,17,&same)&&same==17&&pitch.random_state==0xd37);C(gh_se_pitch_choose_0037d760(&pitch,18,17,&same)==GH_VOICE_UNSUPPORTED&&pitch.random_state==0xd37);uint16_t q;C(!gh_se_pitch_q12_000038e4(&pitch,0,48000,&q)&&q==4096);gh_disc_close(d);puts("PASS SAMEdisc readonlyISO paths/bounds, real libsd ordinal0D tables+bootseed, 65536 SDE RNG states +5125 actual instruction pitch oracle cases x4FEnv; no audio output");return 0;}
