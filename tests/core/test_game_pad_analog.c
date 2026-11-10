#include "game_pad_analog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static void put(void*p,uint32_t n){memcpy(p,&n,4);}
static uint32_t word(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
static int touched(unsigned i){return(i>=0x60&&i<0x64)||(i>=0x70&&i<0x74)||(i>=0x180&&i<0x18c)||(i>=0x1e0&&i<0x1f4);}
int main(void){
 static const struct {uint8_t wire[4];int8_t axes[4];uint32_t raw,dir;} golden[]={
  {{128,128,128,128},{0,0,0,0},0,0},{{127,127,127,127},{0,0,0,0},0,0},
  {{255,0,0,255},{-128,-128,127,127},0x690000,0x69},{{0,255,255,0},{127,127,-128,-128},0x960000,0x96},
  {{160,191,160,191},{0,-64,32,-64},0,0x2a},/* actualright-Y -64 asymmetry */
  {{128,128,255,128},{127,-1,0,0},0x800000,0xa0},{{255,128,128,128},{0,0,127,-1},0x80000,0xa},
  {{128,0,128,0},{0,127,0,127},0x110000,0x11},{{128,255,128,255},{0,-128,0,-128},0x220000,0x22},
  {{64,128,64,128},{-64,0,-64,0},0,0x44},{{63,128,63,128},{-65,-1,-65,-1},0,0x66}
 };
 const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};unsigned checks=0;
 for(unsigned m=0;m<4;m++){
  C(!fesetround(modes[m])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));
  for(unsigned n=0;n<256;n++){int original=(int)n-128,expected=original>=-63&&original<=63?0:original;C(gh_pad_center_deadzone_0031af70((uint8_t)n)==expected);}
  for(unsigned g=0;g<sizeof(golden)/sizeof(*golden);g++){
   gh_boot_work work,before;memset(&work,0xa5,sizeof(work));uint8_t*p=(uint8_t*)&work;put(p+0x10,0);put(p+0x180,0x11223344);before=work;gh_pad_analog_result result;
   C(!gh_pad_analog_evaluate_0031b560(&work,golden[g].wire,0x40,&result)&&!memcmp(&work,&before,sizeof(work)));C(!memcmp(result.axes,golden[g].axes,4)&&result.raw_held==(golden[g].raw|0x40)&&result.directions==golden[g].dir&&result.pressed_directions==((golden[g].dir^0x11223344)&golden[g].dir));
   C(!gh_pad_analog_commit_0031afe0(&work,&result));for(unsigned i=0;i<sizeof(work);i++)if(!touched(i))C(p[i]==((uint8_t*)&before)[i]);C(!memcmp(p+0x1e0,p+0x60,20)&&!memcmp(p+0x1f0,result.axes,4)&&word(p+0x180)==result.directions&&word(p+0x184)==0x11223344&&word(p+0x188)==result.pressed_directions);
   /* Unknownpressure bytes are copied fromSAME Work, never zero-filled. */
   for(unsigned i=4;i<16;i++)C(p[0x1e0+i]==0xa5);checks++;
  }
  /* Independent rectangular-set invariants over every possible hardware
   * pair. This is deliberately not a radial deadzone or normalizedvector. */
  for(unsigned x=0;x<256;x++)for(unsigned y=0;y<256;y++){
   gh_boot_work work={0};uint8_t wire[4]={(uint8_t)x,(uint8_t)y,(uint8_t)x,(uint8_t)y};gh_pad_analog_result result;C(!gh_pad_analog_evaluate_0031b560(&work,wire,0,&result));int rawx=(int)x-128,rawy=127-(int)y;
   for(unsigned k=0;k<4;k+=2){int a=result.axes[k],b=result.axes[k+1];C((a==0||a==rawx)&&(b==0||b==rawy));if(abs(rawx)<=63&&abs(rawy)<=63)C(!a&&!b);if(abs(rawx)>64&&abs(rawy)<=63)C(a==rawx&&b==rawy);if(abs(rawy)>64&&abs(rawx)<=63)C(a==rawx&&b==rawy);}
   if(x==208&&y==127)C(!(result.raw_held&0x880000));if(x==209&&y==127)C((result.raw_held&0x880000)==0x880000);if(x==48&&y==127)C(!(result.raw_held&0x440000));if(x==47&&y==127)C((result.raw_held&0x440000)==0x440000);checks++;
  }
  gh_boot_work work={0};uint8_t*p=(uint8_t*)&work;uint8_t neutral[]={128,128,128,128};gh_pad_analog_result r;
  put(p+0x10,1);p[0x62]=p[0x63]=177;C(!gh_pad_analog_evaluate_0031b560(&work,neutral,0,&r)&&r.raw_held==0x9000);/* prior sample, not currentneutral */
  C(!gh_pad_analog_evaluate_0031b560(&work,neutral,0x6000,&r)&&r.raw_held==0x6000);/* physicaldirection takesprecedence */
  p[0x62]=p[0x63]=79;C(!gh_pad_analog_evaluate_0031b560(&work,neutral,0,&r)&&r.raw_held==0x6000);p[0x62]=p[0x63]=80;C(!gh_pad_analog_evaluate_0031b560(&work,neutral,0,&r)&&!r.raw_held);p[0x62]=p[0x63]=176;C(!gh_pad_analog_evaluate_0031b560(&work,neutral,0,&r)&&!r.raw_held);
  gh_boot_work before=work,foreign=work;C(gh_pad_analog_commit_0031afe0(&foreign,&r)==GH_PAD_ANALOG_STALE&&!memcmp(&foreign,&before,sizeof(before)));p[0x62]++;before=work;C(gh_pad_analog_commit_0031afe0(&work,&r)==GH_PAD_ANALOG_STALE&&!memcmp(&work,&before,sizeof(before)));p[0x62]--;
  C(!gh_pad_analog_evaluate_0031b560(&work,neutral,0,&r));put(p+0x10,0);before=work;C(gh_pad_analog_commit_0031afe0(&work,&r)==GH_PAD_ANALOG_STALE&&!memcmp(&work,&before,sizeof(before)));put(p+0x10,1);put(p+0x180,1);before=work;C(gh_pad_analog_commit_0031afe0(&work,&r)==GH_PAD_ANALOG_STALE&&!memcmp(&work,&before,sizeof(before)));put(p+0x180,0);
  r.axes[0]=1;before=work;C(gh_pad_analog_commit_0031afe0(&work,&r)==GH_PAD_ANALOG_STALE&&!memcmp(&work,&before,sizeof(before)));memset(&r,0x5a,sizeof(r));gh_pad_analog_result saved=r;C(gh_pad_analog_evaluate_0031b560(&work,neutral,0x10000,&r)==GH_PAD_ANALOG_ARGUMENT&&!memcmp(&r,&saved,sizeof(r))&&!memcmp(&work,&before,sizeof(before)));
  C(fegetround()==modes[m]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);
 }
 C(!fesetround(FE_TONEAREST));printf("PASS originalanalog %u pair/golden checks,256deadzoneinputs,4FEnv/rightYasymmetry/previousaxisDpad/81thresholds/exactWorkbytes/pressurepreserved/staleatomic\n",checks);return 0;
}
