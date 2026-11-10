#include "game_om_reaction.h"
#include <stdio.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
static void sh(uint8_t*p,uint16_t v){memcpy(p,&v,2);}static unsigned h(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
int main(int argc,char**argv){(void)argv;C(argc==1);uint8_t raw[0x620];gh_title_player_view a={0};a.bytes=raw;a.size=sizeof(raw);a.alias_f0=raw+0x30;const int modes[]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO};uint32_t pc=0;
 for(unsigned m=0;m<4;m++){C(!fesetround(modes[m])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));memset(raw,0xa5,sizeof(raw));raw[0x2f4]=1;raw[0x2f5]=0;memset(raw+0x5e4,0,4);sh(raw+0x5f4,0xffff);uint8_t expected[sizeof(raw)];memcpy(expected,raw,sizeof(raw));expected[0x2f5]=1;expected[0x2f6]=expected[0x2f7]=0;sh(expected+0x60e,2);C(!gh_om_reaction_001c2c40(&a,&pc)&&!pc&&!memcmp(raw,expected,sizeof(raw)));sh(expected+0x60e,1);C(!gh_om_reaction_001c2c40(&a,&pc)&&!memcmp(raw,expected,sizeof(raw)));sh(expected+0x60e,0);memset(expected+0x2f4,0,4);C(!gh_om_reaction_001c2c40(&a,&pc)&&!memcmp(raw,expected,sizeof(raw)));
 for(unsigned n=0;n<65536;n++){raw[0x2f4]=1;raw[0x2f5]=1;raw[0x2f6]=9;raw[0x2f7]=8;sh(raw+0x60e,(uint16_t)n);unsigned next=(n-1)&65535;C(!gh_om_reaction_001c2c40(&a,&pc)&&h(raw+0x60e)==next);C(raw[0x2f4]==(next&&next<32768?1:0));}
 raw[0x2f5]=0;raw[0x5e4]=1;memcpy(expected,raw,sizeof(raw));C(gh_om_reaction_001c2c40(&a,&pc)==GH_OBJSTEP_UNSUPPORTED&&pc==0x306838&&!memcmp(raw,expected,sizeof(raw)));raw[0x5e4]=0;sh(raw+0x5f4,0);memcpy(expected,raw,sizeof(raw));C(gh_om_reaction_001c2c40(&a,&pc)==GH_OBJSTEP_UNSUPPORTED&&pc==0x2cba48&&!memcmp(raw,expected,sizeof(raw)));raw[0x2f5]=2;memcpy(expected,raw,sizeof(raw));C(!gh_om_reaction_001c2c40(&a,&pc)&&!memcmp(raw,expected,sizeof(raw)));C(fegetround()==modes[m]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);}

 for(unsigned m=0;m<4;m++){
 C(!fesetround(modes[m])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));uint8_t expected[sizeof(raw)];memset(raw,0xa5,sizeof(raw));raw[0x2f4]=1;raw[0x2f5]=0;memcpy(expected,raw,sizeof(raw));expected[0x2f5]=1;expected[0x2f6]=expected[0x2f7]=0;sh(expected+0x600,14);C(!gh_om_reaction_00188190(&a,&pc)&&!pc&&!memcmp(raw,expected,sizeof(raw)));
 for(unsigned n=0;n<65536;n++){memset(raw,0xa5,sizeof(raw));raw[0x2f4]=1;raw[0x2f5]=1;sh(raw+0x600,(uint16_t)n);memcpy(expected,raw,sizeof(raw));unsigned next=(n-1)&65535;sh(expected+0x600,(uint16_t)next);if(!next||next>=32768)memset(expected+0x2f4,0,4);C(!gh_om_reaction_00188190(&a,&pc)&&!pc&&!memcmp(raw,expected,sizeof(raw)));}
 for(unsigned sub=2;sub<256;sub++){raw[0x2f5]=(uint8_t)sub;memcpy(expected,raw,sizeof(raw));C(!gh_om_reaction_00188190(&a,&pc)&&!memcmp(raw,expected,sizeof(raw)));}C(fegetround()==modes[m]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);
 }
 C(!fesetround(FE_TONEAREST));puts("PASS original188190 exhaustive poison/4FEnv; original1C2C40 three ticks/full65536 SH countdown/positive effect+sound strict/bytepoison/FEnv");return 0;}
