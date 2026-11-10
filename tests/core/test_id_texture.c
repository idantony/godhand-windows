#include "../../src/core/id_jobs.h"
#include "../../src/platform/disc.h"
#include "../../src/assets/tim2.h"
#include <stdio.h>
#include <stdlib.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(int argc,char **argv){gh_disc *d;gh_disc_entry entry;size_t i;unsigned char *b,*rgba;gh_bundle_entry idd={0};gh_id_state *manager=calloc(1,sizeof(*manager));gh_id_texture t;unsigned changed=0;
 C(argc==2||argc==3);C(manager);C(gh_disc_open(argv[1],&d)==GH_DISC_OK);C(gh_disc_find(d,"id/local/US/idsavecore_US.idd",&i)==GH_DISC_OK);C(gh_disc_entry_at(d,i,&entry)==GH_DISC_OK);b=malloc((size_t)entry.size);rgba=malloc(512*512*4);C(b&&rgba);C(gh_disc_read(d,i,0,b,(size_t)entry.size)==GH_DISC_OK);idd.bytes=b;idd.size=(size_t)entry.size;C(gh_id_set_data_002accf8(manager,11,&idd)==GH_MENU_OK);
 C(gh_id_texture_open(manager,11,1,&t)==GH_ID_NOT_FOUND);C(gh_id_texture_open(manager,11,2,&t)==GH_ID_OK&&t.width==512&&t.height==512);
 C(gh_id_texture_rgba(&t,rgba,512*512*4)==GH_ID_OK);
 for(i=0;i<512*512;i++)changed+=rgba[i*4]!=0||rgba[i*4+1]!=0||rgba[i*4+2]!=0;C(changed>1000);
 if(argc==3){FILE *f=fopen(argv[2],"wbx");C(f);C(fwrite(rgba,1,512*512*4,f)==512*512*4);C(fclose(f)==0);}
 C(gh_id_texture_rgba(&t,rgba,12)==GH_ID_CAPACITY);C(gh_id_texture_open(manager,11,3,&t)==GH_ID_OK&&t.height==256);C(gh_id_texture_rgba(&t,rgba,512*512*4)==GH_ID_OK);C(gh_id_texture_open(manager,11,4,&t)==GH_ID_NOT_FOUND);
 /* ReallanguagecursorTIM2 isnotanopaqueT32 packet. NativegenericTBL1lookup
  * mustdispatchtheoriginalTIM2 branchandretainactual16coloralpha domain. */
 {unsigned char*lang;gh_bundle bundle;gh_tim2_picture picture;uint64_t hash=UINT64_C(14695981039346656037);unsigned nonzero=0;
  C(gh_disc_find(d,"etc/core_usa.bin",&i)==GH_DISC_OK&&gh_disc_entry_at(d,i,&entry)==GH_DISC_OK);lang=malloc((size_t)entry.size);C(lang&&gh_disc_read(d,i,0,lang,(size_t)entry.size)==GH_DISC_OK&&gh_bundle_open(lang,(size_t)entry.size,&bundle)==GH_BUNDLE_OK&&gh_id_find_language_data(&bundle,&idd)==GH_MENU_OK&&gh_id_system_init_002acc70(manager,&idd)==GH_MENU_OK);
  C(gh_id_texture_open(manager,1,47,&t)==GH_ID_OK&&t.width==32&&t.height==32&&gh_tim2_first(t.source.bytes,t.source.size,&picture)==GH_TIM2_OK&&picture.colors==16&&picture.image_size==512&&picture.clut_size==64);
  for(i=0;i<16;i++)C(picture.clut[i*4+3]<=128);
  C(gh_id_texture_rgba(&t,rgba,4096)==GH_ID_OK);for(i=0;i<4096;i++)hash=(hash^rgba[i])*UINT64_C(1099511628211);for(i=0;i<1024;i++)nonzero+=rgba[i*4+3]!=0;C(hash==UINT64_C(0x4950639901636b2f)&&nonzero==194);
  {unsigned char*alpha=(unsigned char*)picture.clut+3;unsigned char saved=*alpha;*alpha=129;C(gh_id_texture_rgba(&t,rgba,4096)==GH_ID_UNSUPPORTED);*alpha=saved;}
  C(gh_id_texture_open(manager,1,31,&t)==GH_ID_OK&&t.width==32&&t.height==16&&t.tex0==UINT64_C(0x115408000));
  C(gh_tim2_first(t.source.bytes,t.source.size,&picture)==GH_TIM2_OK&&picture.image_size==256&&picture.clut_size==64&&picture.colors==16);
  C(gh_id_texture_rgba(&t,rgba,2048)==GH_ID_OK);unsigned loading_nonzero=0;
  for(unsigned pixel=0;pixel<512;pixel++){unsigned shift=pixel%2?4:0,index4=picture.image[pixel/2]>>shift&15;const unsigned char*color=picture.clut+index4*4;
   C(color[3]<=128);for(unsigned channel=0;channel<3;channel++)C(rgba[pixel*4+channel]==color[channel]);C(rgba[pixel*4+3]==(color[3]==128?255:color[3]*2));loading_nonzero+=rgba[pixel*4+3]!=0;
  }C(loading_nonzero>0);C(gh_id_texture_rgba(&t,rgba,2047)==GH_ID_CAPACITY);printf("actualSystemMenu32x16 item31 independentlymapped512pixels nontransparent%u\n",loading_nonzero);
  printf("actualcursor32x32 RGBAfnv%016llx nontransparent%u\n",(unsigned long long)hash,nonzero);free(lang);
 }
 free(rgba);free(b);free(manager);gh_disc_close(d);puts("PASS actual MC TBL1 T32/PSMT8 512x512 and512x256 native decoding; source bounds and empty slots");return 0;}

