#include "../../src/assets/core_bundle.h"
#include "../../src/assets/tim2.h"
#include "../../src/platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static unsigned char *load(gh_disc *d,const char *path,size_t *size){size_t i;gh_disc_entry e;unsigned char *b;C(gh_disc_find(d,path,&i)==GH_DISC_OK);C(gh_disc_entry_at(d,i,&e)==GH_DISC_OK);*size=(size_t)e.size;b=malloc(*size);C(b);C(gh_disc_read(d,i,0,b,*size)==GH_DISC_OK);return b;}
int main(int argc,char **argv){gh_disc *d;unsigned char *b,*language,bad[16]={0};size_t n;gh_bundle v;gh_bundle_entry e;gh_tim2_picture image;
 C(argc==2);C(gh_disc_open(argv[1],&d)==GH_DISC_OK);
 b=load(d,"etc/core.bin",&n);C(gh_bundle_open(b,n,&v)==GH_BUNDLE_OK&&v.count==6);
 C(gh_bundle_find(&v,"SLD",1,&e)==GH_BUNDLE_OK&&e.offset==0x6140);
 C(gh_bundle_find(&v,"TM2",0,&e)==GH_BUNDLE_OK&&e.offset==0x1240);
 C(gh_tim2_first(e.bytes,e.size,&image)==GH_TIM2_OK);
 C(gh_bundle_find(&v,"SLD",2,&e)==GH_BUNDLE_NOT_FOUND);free(b);
 language=load(d,"etc/core_usa.bin",&n);C(gh_bundle_open(language,n,&v)==GH_BUNDLE_OK&&v.count==37);
 C(gh_bundle_find(&v,"IDD",0,&e)==GH_BUNDLE_OK&&e.offset==0x592d0);
 C(gh_bundle_find(&v,"TM2",30,&e)==GH_BUNDLE_OK&&e.offset==0x440b0);
 C(gh_tim2_first(e.bytes,e.size,&image)==GH_TIM2_OK);
 C(gh_bundle_at(&v,37,&e)==GH_BUNDLE_BOUNDS);bad[0]=255;C(gh_bundle_open(bad,16,&v)==GH_BUNDLE_BOUNDS);
 free(language);gh_disc_close(d);puts("PASS real core/language bundle SearchData and TIM2 view; IDD located without fake init");return 0;}
