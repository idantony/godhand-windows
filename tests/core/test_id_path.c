#include "id_path.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#include "../platform/disc.h"
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static void h(unsigned char *p,unsigned n){p[0]=(unsigned char)n;p[1]=(unsigned char)(n>>8);}
static void f(unsigned char *p,float n){memcpy(p,&n,4);}
static uint32_t bits(float n){uint32_t b;memcpy(&b,&n,4);return b;}
static void run(void){unsigned char src[0x2e0]={0};gh_id_entry e={0};e.source=src;e.parts[0]=src+0x130;e.jitter[1]=42;
 h(src+0x6c,4);h(src+0x6e,6);f(src+0x13c,0);f(src+0x144,15);f(src+0x14c,5);f(src+0x154,67);
 C(gh_id_path_reset_002abc30(&e)==GH_ID_OK && e.path_length==87 && !e.jitter[1] && !(e.flags&0x2000));
 f(src+0x138,3);f(src+0x140,0);f(src+0x13c,2);f(src+0x144,0);
 C(gh_id_path_reset_002abc30(&e)==GH_ID_UNSUPPORTED && e.path_length==0);
 C(gh_id_path_reset_reference_nearest(&e)==GH_ID_OK);
 h(src+0x6e,2);C(gh_id_path_reset_002abc30(&e)==GH_ID_OK && e.path_length==0);
 h(src+0x6c,1);C(gh_id_path_reset_002abc30(&e)==GH_ID_OK && (e.flags&0x2000));
 src[0x5d]=1;h(src+0x6c,4);C(gh_id_path_reset_002abc30(&e)==GH_ID_OK && (e.flags&0x2000));
 h(src+0x6e,18);C(gh_id_path_reset_002abc30(&e)==GH_ID_BOUNDS);
}
static void actual(const char *iso)
{
 gh_disc *d;gh_disc_entry de;size_t index;gh_bundle bundle;gh_bundle_entry idd;
 gh_id_state *m=calloc(1,sizeof(*m));unsigned char *data;
 unsigned ids[]={1,14,18,21},success=0,unsupported=0;
 C(m && gh_disc_open(iso,&d)==GH_DISC_OK && gh_disc_find(d,"etc/core_usa.bin",&index)==GH_DISC_OK);
 C(gh_disc_entry_at(d,index,&de)==GH_DISC_OK);data=malloc((size_t)de.size);
 C(data && gh_disc_read(d,index,0,data,(size_t)de.size)==GH_DISC_OK);
 C(gh_bundle_open(data,(size_t)de.size,&bundle)==GH_BUNDLE_OK);
 C(gh_id_find_language_data(&bundle,&idd)==GH_MENU_OK && gh_id_system_init_002acc70(m,&idd)==GH_MENU_OK);
 for(unsigned i=0;i<4;++i){
  gh_id_view v;C(gh_id_lookup_002acd78(m,1,ids[i]+1,&v)==GH_ID_OK);
  unsigned count=v.bytes[4]|(unsigned)v.bytes[5]<<8;
  for(unsigned j=0;j<count;++j){
   gh_id_entry e={0};e.source=v.bytes+16+j*0x2e0;e.parts[0]=e.source+0x130;
   gh_id_error status=gh_id_path_reset_002abc30(&e);
   if(status==GH_ID_UNSUPPORTED){
    static const uint32_t expected[]={0x41953f95,0x41baae4d,0x41a262f8};
    ++unsupported;C(ids[i]==1 && j>=18 && j<=20);
    int old=fegetround();int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
    for(unsigned k=0;k<4;++k)if(!fesetround(modes[k])){
     C(gh_id_path_reset_reference_nearest(&e)==GH_ID_OK && bits(e.path_length)==expected[j-18]);
    }
    if(old!=-1)C(!fesetround(old));
   }
   else {C(status==GH_ID_OK);++success;
    if(ids[i]==14 && j==2)C(e.path_length==16);
    if(ids[i]==18 && j==0)C(e.path_length==87);
    if(ids[i]==21 && j==2)C(e.path_length==16);
   }
  }
 }
 C(success==40 && unsupported==3);free(m);free(data);gh_disc_close(d);
}
int main(int argc,char **argv){int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};size_t i;int old=fegetround();for(i=0;i<4;++i)if(!fesetround(modes[i]))run();if(old!=-1)C(!fesetround(old));if(argc==2)actual(argv[1]);else C(argc==1);puts("PASS real path primitive exact integer domain across host rounding modes; non-square roots explicitly unsupported");return 0;}
