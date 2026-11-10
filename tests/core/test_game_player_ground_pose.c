#include "game_player_ground_pose.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static void scalar(uint8_t*p,float n){memcpy(p,&n,4);}
static float number(const uint8_t*p){float n;memcpy(&n,p,4);return n;}
int main(int argc,char**argv){
 C(argc==2);FILE*f=fopen(argv[1],"rb");C(f&&!fseek(f,0,SEEK_END));long size=ftell(f);C(size>52&&!fseek(f,0,SEEK_SET));uint8_t*elf=malloc((size_t)size);C(elf&&fread(elf,1,(size_t)size,f)==(size_t)size);fclose(f);
 gh_player_ground_globals original={0};C(!gh_player_ground_globals_from_elf(&original,elf,(size_t)size)&&original.loaded&&original.tilt==0);
 original.tilt=.125f;C(gh_player_ground_globals_from_elf(&original,elf,(size_t)size)==GH_GROUND_BUSY&&original.tilt==.125f);
 gh_player_ground_globals bad={0};C(gh_player_ground_globals_from_elf(&bad,elf,51)==GH_GROUND_ARGUMENT&&!bad.loaded);
 uint8_t raw[GH_TITLE_PLAYER_BYTES]={0},part[0x1c0]={0};gh_title_player_view actor={.bytes=raw,.size=sizeof(raw)};
 gh_actor_model model={.root=actor,.part_count=1,.parts_ready=1};model.parts[0].bytes=part;model.parts[0].alias_d0=part+0x30;raw[0x2b4]=1;
 const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
 for(unsigned mode=0;mode<4;mode++){
  gh_player_ground_globals globals={.loaded=1,.tilt=.02f};long double expected=.02f;
  /* A long-double oracle accumulates independently over changing normals,
   * while each original invocation borrows the same mutable data owner. */
  for(unsigned i=0;i<256;i++){
   C(!fesetround(FE_TONEAREST));float n[3]={(float)((int)(i%9)-4),(float)((int)(i%7)-3),(float)((int)(i%5)-2)};if(i%17==0)memset(n,0,12);memcpy(raw+0x480,n,12);
   uint16_t seq=i%3==0?4:(i%3==1?0x400:0);memcpy(raw+0x3ac,&seq,2);uint32_t state=(i&1?0x80:0)|(i&2?0x40000:0);put(raw+0x15f4,state);put(raw+0x258,0x3abcdef1);scalar(part+0x34,10);put(part+0x3c,0xfedcba98);put(raw+0x48c,0x1234abcd);
   long double len=sqrtl((long double)n[0]*n[0]+(long double)n[1]*n[1]+(long double)n[2]*n[2]);long double ny=len?n[1]/len:0,target=(1-ny)*1.2L*.5L;if(target>.06L)target=.06L;if(seq&12)target=0;expected=expected*.85L+target*.15L;if(expected<0)expected=0;
   gh_player_ground_pose_call call={0};C(!fesetround(modes[mode])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));int exceptions=fetestexcept(FE_ALL_EXCEPT);
   C(!gh_player_ground_pose_00108ab0(&call,&actor,&model,&globals)&&call.complete&&!call.frontier&&fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==exceptions);
   C(fabsl((long double)globals.tilt-expected)<.0000001L&&fabsl((long double)number(part+0x34)-(10-expected))<.000001L);
   for(unsigned k=0;k<3;k++)C(fabsl((long double)number(raw+0x480+k*4)-(len?n[k]/len:0))<.0000002L);
   C(word(raw+0x48c)==0x1234abcd&&word(part+0x3c)==0xfedcba98);
   uint32_t flags=0x3abcdef1;if((seq&0x404)||(state&0x80))flags|=0x80000000;if(state&0x40000)flags|=0x40000000;C(word(raw+0x258)==flags);
   float saved=globals.tilt,y=number(part+0x34);C(!gh_player_ground_pose_00108ab0(&call,&actor,&model,&globals)&&globals.tilt==saved&&number(part+0x34)==y);
  }
 }
 C(!fesetround(FE_TONEAREST));memset(raw,0,sizeof(raw));scalar(raw+0x480,NAN);put(raw+0x258,UINT32_MAX);put(raw+0x15f4,0);
 gh_player_ground_pose_call empty={0};C(!gh_player_ground_pose_00108ab0(&empty,&actor,NULL,NULL)&&empty.complete&&word(raw+0x258)==0x3fffffff&&isnan(number(raw+0x480)));
 /* Late missing globals retain completed normalization and allow repair of
  * that SAME owner, without repeating normalization/filter/part write. */
 raw[0x2b4]=1;scalar(raw+0x480,0);scalar(raw+0x484,3);scalar(raw+0x488,4);scalar(part+0x34,10);gh_player_ground_globals repaired={0};gh_player_ground_pose_call call={0};
 C(gh_player_ground_pose_00108ab0(&call,&actor,&model,&repaired)==GH_GROUND_UNSUPPORTED&&call.stage==3&&call.frontier==0x109778&&number(part+0x34)==10);
 scalar(raw+0x480,NAN);repaired.loaded=1;C(!gh_player_ground_pose_00108ab0(&call,&actor,&model,&repaired)&&call.complete&&isnan(number(raw+0x480))&&repaired.tilt>0&&number(part+0x34)<10);
 gh_player_ground_globals foreign={.loaded=1};C(gh_player_ground_pose_00108ab0(&call,&actor,&model,&foreign)==GH_GROUND_BUSY);
 free(elf);puts("PASS inline108AB0 ground pose: actual ELF3BD470, 1024 frame long-double oracle, flags/zero-parts/read gates, shared filter, late retry, 4 FEnv");return 0;
}
