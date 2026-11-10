#include "game_cloth_templates.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint64_t hash(const void*bytes,size_t n){const uint8_t*p=bytes;uint64_t h=UINT64_C(14695981039346656037);for(size_t i=0;i<n;i++)h=(h^p[i])*UINT64_C(1099511628211);return h;}
int main(int argc,char**argv){C(argc==2);FILE*f=fopen(argv[1],"rb");C(f);C(!fseek(f,0,SEEK_END));long size=ftell(f);C(size>0&&!fseek(f,0,SEEK_SET));uint8_t*elf=malloc((size_t)size);C(elf&&fread(elf,1,(size_t)size,f)==(size_t)size&&!fclose(f));
 gh_cloth_templates owner={0};C(!gh_cloth_templates_from_elf(&owner,elf,(size_t)size));for(size_t i=0;i<sizeof(owner.records);i++)C(!((uint8_t*)owner.records)[i]);C(gh_cloth_templates_from_elf(&owner,elf,(size_t)size)==GH_CLOTH_ARGUMENT);
 memset(owner.records,0x5a,sizeof(owner.records));gh_cloth_templates before=owner;C(!gh_cloth_templates_initialize_00100fb8(&owner,0,0xffff)&&!memcmp(&owner,&before,sizeof(owner)));C(!gh_cloth_templates_initialize_00100fb8(&owner,1,0)&&!memcmp(&owner,&before,sizeof(owner)));
 const int modes[4]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};const uint64_t expected[4]={UINT64_C(0x20f6173729004a89),UINT64_C(0x491f59bb851c8255),UINT64_C(0xf3a1014002999413),UINT64_C(0x6bc69eeb3548f956)};const size_t counts[4]={12,11,7,12};
 for(unsigned m=0;m<4;m++){C(!fesetround(modes[m]));C(!gh_cloth_templates_initialize_00100fb8(&owner,-1,0xffff)&&fegetround()==modes[m]);
  for(unsigned costume=0;costume<8;costume++){gh_cloth_sphere*table;size_t count;C(!gh_cloth_templates_for_costume(&owner,costume,&table,&count)&&count==counts[costume>>1]&&hash(table,count*sizeof(*table))==expected[costume>>1]);for(size_t i=0;i<count;i++){C(!table[i].flags&&!table[i].reserved&&table[i].offset_a[3]==1&&table[i].offset_b[3]==1);C(!memcmp(table[i].offset_a,table[i].offset_b,16));}}
 }
 gh_cloth_sphere*first;size_t n;C(!gh_cloth_templates_for_costume(&owner,0,&first,&n));first[0].radius=2;gh_cloth_sphere*alias;C(!gh_cloth_templates_for_costume(&owner,1,&alias,&n)&&alias==first&&alias[0].radius==2);C(!gh_cloth_templates_initialize_00100fb8(&owner,1,0xffff)&&hash(owner.records,12*48)==expected[0]);
 gh_cloth_sphere*sentinel=(gh_cloth_sphere*)(uintptr_t)1;n=999;C(gh_cloth_templates_for_costume(&owner,8,&sentinel,&n)==GH_CLOTH_UNSUPPORTED&&sentinel==(gh_cloth_sphere*)(uintptr_t)1&&n==999);gh_cloth_templates invalid={0},copy=invalid;C(gh_cloth_templates_from_elf(&invalid,elf,51)==GH_CLOTH_BOUNDS&&!memcmp(&invalid,&copy,sizeof(copy)));C(!fesetround(FE_TONEAREST));free(elf);puts("PASS actualELF BSS identity;42 originalcloth records match independent actualinstruction writeoracle;4FEnv/guards/mutablepairalias/reinitialize/boundsatomic");return 0;}
