#include "game_collision_pair_queries.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static uint32_t bits(float f){uint32_t n;memcpy(&n,&f,4);return n;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static void identity(uint8_t*p){memset(p,0,64);for(unsigned i=0;i<4;i++)put(p+i*20,0x3f800000);}
static void verify_faces(const char*path){FILE*f=fopen(path,"rb");C(f&&!fseek(f,0,SEEK_END));long n=ftell(f);C(n>52&&!fseek(f,0,SEEK_SET));uint8_t*b=malloc((size_t)n);C(b&&fread(b,1,(size_t)n,f)==(size_t)n);fclose(f);C(!memcmp(b,"\177ELF\1\1\1",7));uint32_t table=word(b+28);unsigned stride=b[42]|(unsigned)b[43]<<8,count=b[44]|(unsigned)b[45]<<8;C(stride>=32&&table<=(size_t)n&&count<=((size_t)n-table)/stride);const uint8_t*data=NULL;for(unsigned i=0;i<count;i++){const uint8_t*h=b+table+(size_t)i*stride;uint32_t va=word(h+8),off=word(h+4),files=word(h+16);if(word(h)==1&&va<=0x3c2600&&0x3c2600u-va<=files&&72<=files-(0x3c2600u-va)){C(off<=(size_t)n&&files<=(size_t)n-off);data=b+off+(0x3c2600-va);break;}}C(data);static const uint32_t expected[]={0,2,1,4,5,6,2,6,3,0,1,4,1,3,5,2,0,6};for(unsigned i=0;i<18;i++)C(word(data+i*4)==expected[i]);free(b);}
int main(int argc,char**argv){C(argc==2);verify_faces(argv[1]);int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};for(unsigned mode=0;mode<4;mode++){C(!fesetround(modes[mode]));feclearexcept(FE_ALL_EXCEPT);feraiseexcept(FE_INVALID);int exceptions=fetestexcept(FE_ALL_EXCEPT);uint8_t m[64];identity(m);uint32_t size[]={0x40000000,0x40000000,0x40000000};
 for(int xi=-10;xi<=10;xi++)for(int yi=-8;yi<=16;yi++)for(int zi=-10;zi<=10;zi++){float x=xi*.25f,y=yi*.25f,z=zi*.25f;uint32_t p[]={bits(x),bits(y),bits(z)},pc;int hit=-1;C(!gh_collision_query_sphere_box_002bbba0((const uint8_t*)p,0x3f800000,m,size,GH_STATIC_BINARY32_REFERENCE,&hit,&pc)&&!pc&&hit==(x>=-2&&x<=2&&y>=-1&&y<=3&&z>=-2&&z<=2)&&fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==exceptions);}
 /* Original plane expansion includes this Euclidean-corner false positive. */
 uint32_t p[]={0x3fe00000,0x40300000,0},pc;int hit;C(!gh_collision_query_sphere_box_002bbba0((const uint8_t*)p,0x3f800000,m,size,GH_STATIC_BINARY32_REFERENCE,&hit,&pc)&&hit);
 /* Nonuniform source matrix requires normalized faces. */
 put(m,0x40000000);put(m+20,0x3f000000);p[0]=0x40400000;p[1]=0x3f000000;C(!gh_collision_query_sphere_box_002bbba0((const uint8_t*)p,0x3f800000,m,size,GH_STATIC_BINARY32_REFERENCE,&hit,&pc)&&hit);p[0]=0x40500000;C(!gh_collision_query_sphere_box_002bbba0((const uint8_t*)p,0x3f800000,m,size,GH_STATIC_BINARY32_REFERENCE,&hit,&pc)&&!hit);
 identity(m);put(m,0);put(m+4,0x3f800000);put(m+16,0xbf800000);put(m+20,0);put(m+48,0x40400000);put(m+52,0x40800000);put(m+56,0x40a00000);p[0]=0x40000000;p[1]=0x40c00000;p[2]=0x40a00000;C(!gh_collision_query_sphere_box_002bbba0((const uint8_t*)p,0x3f800000,m,size,GH_STATIC_BINARY32_REFERENCE,&hit,&pc)&&hit);p[1]=0x40c80000;C(!gh_collision_query_sphere_box_002bbba0((const uint8_t*)p,0x3f800000,m,size,GH_STATIC_BINARY32_REFERENCE,&hit,&pc)&&!hit);
 uint32_t zero[3]={0};identity(m);p[0]=0x42c80000;p[1]=0;p[2]=0;C(!gh_collision_query_sphere_box_002bbba0((const uint8_t*)p,0x3f800000,m,zero,GH_STATIC_BINARY32_REFERENCE,&hit,&pc)&&hit);p[0]=0x7fc00000;hit=77;C(gh_collision_query_sphere_box_002bbba0((const uint8_t*)p,0x3f800000,m,size,GH_STATIC_BINARY32_REFERENCE,&hit,&pc)==GH_PAIR_QUERY_UNSUPPORTED&&pc==0x2bbba0&&hit==77&&fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==exceptions);
 }puts("PASS actualELF6faces/8corners spherebox plane semantics:44100 independent axisgrid, equality/cornerquirk, normalizednonuniform+rotationtranslation, degenerate/unknown4FEnv");return 0;}
