#include "game_camera.h"
#include <fenv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void w32(uint8_t*p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static const uint8_t*at(const uint8_t*b,size_t size,uint32_t a,size_t n){uint32_t ph=u32(b+28);unsigned count=b[44]|(unsigned)b[45]<<8;
 for(unsigned i=0;i<count;i++){CHECK((uint64_t)ph+i*32+32<=size);const uint8_t*p=b+ph+i*32;uint32_t v=u32(p+8),z=u32(p+16),o=u32(p+4);if(u32(p)==1&&a>=v&&a-v<=z&&n<=z-(a-v)){CHECK((uint64_t)o+a-v+n<=size);return b+o+a-v;}}CHECK(0);return NULL;}
int main(int argc,char**argv){
 CHECK(argc==2);FILE*f=fopen(argv[1],"rb");CHECK(f&&!fseek(f,0,SEEK_END));long n=ftell(f);CHECK(n>0);rewind(f);uint8_t *elf=malloc((size_t)n);CHECK(elf&&fread(elf,1,(size_t)n,f)==(size_t)n);fclose(f);
 CHECK(u32(at(elf,(size_t)n,0x41dbb8,8))==0&&u32(at(elf,(size_t)n,0x41dbbc,4))==0x139090);
 static const uint32_t atan[]={0x3f7ffff5,0xbeaaa61c,0x3e4c40a6,0xbe0e6c63,0x3dc577df,0xbd6501c4,0x3cb31652,0xbb84d7e7,0x3f490fdb};
 static const uint32_t trig[]={0x362e9c14,0xb94fb21f,0x3c08873e,0xbe2aaaa4};
 for(unsigned i=0;i<9;i++)CHECK(u32(at(elf,(size_t)n,0x42c350+i*4,4))==atan[i]);
 for(unsigned i=0;i<4;i++)CHECK(u32(at(elf,(size_t)n,0x3c3c90+i*4,4))==trig[i]);
 gh_game_camera other={0},identity={1,16,&other},before=identity;
 gh_game_pl_camera cold={0};CHECK(gh_game_camera_construct_pl_00139bc8(&cold,&identity,GH_CAMERA_EXACT_UNAVAILABLE)==GH_CAMERA_UNSUPPORTED&&cold.failed_entry==0x1f8180&&!cold.constructed);
 int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
 for(unsigned mode=0;mode<4;mode++){
  gh_game_pl_camera c={0};CHECK(!fesetround(modes[mode]));CHECK(!feclearexcept(FE_ALL_EXCEPT));CHECK(!feraiseexcept(FE_INVALID|FE_INEXACT));int flags=fetestexcept(FE_ALL_EXCEPT);
  CHECK(gh_game_camera_construct_pl_00139bc8(&c,&identity,GH_CAMERA_NATIVE_BINARY32_REFERENCE)==GH_CAMERA_OK&&c.constructed);
  CHECK(fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==flags&&!memcmp(&identity,&before,sizeof(identity)));
  CHECK(u32(c.fields+0x220)==0xbef9a8d6&&u32(c.fields+0x224)==0x3f490fdb&&u32(c.fields+0x22c)==0x3f800000);
  for(unsigned matrix=0;matrix<7;matrix++)for(unsigned i=0;i<16;i++)CHECK(u32(c.fields+matrix*0x40+i*4)==(i%5==0?0x3f800000:0));
  CHECK(u32(c.fields+0x50c)==0x3f800000&&u32(c.fields+0x528)==0&&u32(c.fields+0x35c)==0x41dbb0);
  CHECK(gh_game_camera_construct_pl_00139bc8(&c,&identity,GH_CAMERA_NATIVE_BINARY32_REFERENCE)==GH_CAMERA_BUSY);
  memset(c.fields,0xa5,sizeof(c.fields));w32(c.fields+0x35c,0x41dbb0);uint8_t expected[sizeof(c.fields)];memcpy(expected,c.fields,sizeof(expected));
  unsigned zero[]={0x200,0x204,0x208,0x228,0x240,0x248,0x1c0,0x274,0x268,0x270,0x26c,0x260,0x254,0x25c,0x258};
  for(unsigned i=0;i<sizeof(zero)/sizeof(*zero);i++)w32(expected+zero[i],0);
  w32(expected+0x210,0x40333333);w32(expected+0x214,0x40066666);w32(expected+0x218,0x40333333);
  w32(expected+0x220,0xbef9a8d6);w32(expected+0x224,0x3f490fdb);w32(expected+0x244,0x3f800000);w32(expected+0x250,0x41f80000);expected[0x264]=expected[0x278]=0;
  CHECK(gh_game_camera_reset_00138210(&c,&identity)==GH_CAMERA_OK&&!memcmp(c.fields,expected,sizeof(expected)));
  CHECK(fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==flags&&!memcmp(&identity,&before,sizeof(identity)));
  CHECK(gh_game_camera_reset_00138210(&c,&other)==GH_CAMERA_UNSUPPORTED&&c.failed_entry==0x138210&&!memcmp(c.fields,expected,sizeof(expected)));
 }
 {
  gh_game_init_state *s=calloc(1,sizeof(*s));CHECK(s);uint32_t frontier=99;
  s->camera.mode=1;s->active_camera=&other;gh_game_camera original=s->camera;
  CHECK(gh_game_camera_before_move_00138f98(s,&frontier)==GH_CAMERA_OK&&!frontier&&!memcmp(&s->camera,&original,sizeof(original))&&s->active_camera==&other);
  s->camera.mode=4;CHECK(gh_game_camera_before_move_00138f98(s,&frontier)==GH_CAMERA_UNSUPPORTED&&frontier==0x138600);
  s->camera.mode=0x80;CHECK(gh_game_camera_before_move_00138f98(s,&frontier)==GH_CAMERA_OK&&!frontier);
  CHECK(gh_game_camera_before_move_00138f98(NULL,&frontier)==GH_CAMERA_ARGUMENT&&!frontier);free(s);
 }
 CHECK(!fesetround(FE_TONEAREST));free(elf);puts("PASS actual camera vtable/coefficients, reset identity, four host FP modes and canonical early-mode gate");return 0;
}
