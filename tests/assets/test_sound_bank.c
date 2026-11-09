#include "sound_bank.h"
#include "../platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t *load(gh_disc *d,const char *path,size_t *size){
 size_t i;gh_disc_entry e;uint8_t *b;
 CHECK(gh_disc_find(d,path,&i)==GH_DISC_OK);
 CHECK(gh_disc_entry_at(d,i,&e)==GH_DISC_OK && e.size<=4000000);
 *size=(size_t)e.size;b=malloc(*size);CHECK(b);
 CHECK(gh_disc_read(d,i,0,b,*size)==GH_DISC_OK);return b;
}
int main(int argc,char **argv){gh_disc *d;CHECK(argc==2);
 CHECK(gh_disc_open(argv[1],&d)==GH_DISC_OK);
 for(int n=0;n<2;n++){
  size_t size,afs_size;gh_sound_bank bank;gh_sound_span sample;gh_adx_header h;
  const char *ses=n?"se/pl00.ses":"se/core.ses";
  const char *afs=n?"se/pl00.afs":"se/core.afs";
  unsigned count=n?15:11;
  uint8_t *b=load(d,ses,&size),*a=load(d,afs,&afs_size);
  CHECK(gh_sound_bank_open(b,size,&bank)==GH_SOUND_OK);
  CHECK(!strcmp(bank.external_afs,afs));
  CHECK(bank.sections[3].size==(n?0x26cd0u:0xd1410u));
  for(unsigned i=0;i<count;i++){
   CHECK(gh_sound_afs_entry(a,afs_size,i,&sample)==GH_SOUND_OK);
   CHECK(gh_sound_adx_header(sample,&h)==GH_SOUND_OK);
   CHECK(h.encoding==3 && h.block_bytes==18 && h.bits==4);
   CHECK(h.version==4 && h.flags==8 && h.rate>0 && h.samples>0);
   CHECK(gh_sound_adx_decode_supported(&h)==GH_SOUND_UNSUPPORTED);
   if(!i) CHECK(h.rate==(n?48000u:24000u) && h.channels==(n?1:2));
  }
  CHECK(gh_sound_afs_entry(a,afs_size,count,&sample)==GH_SOUND_BOUNDS);
  CHECK(gh_sound_bank_open(b,0x3f,&bank)==GH_SOUND_BOUNDS);
  {uint8_t old=b[0xc];b[0xc]=1;CHECK(gh_sound_bank_open(b,size,&bank)==GH_SOUND_UNSUPPORTED);b[0xc]=old;}
  {uint8_t old[4];memcpy(old,b+0x28,4);memset(b+0x28,255,4);
   CHECK(gh_sound_bank_open(b,size,&bank)==GH_SOUND_BOUNDS);memcpy(b+0x28,old,4);}
  free(a);free(b);
 }
 gh_disc_close(d);puts("PASS resident actual core/pl00 SES + 26 AFS ADX header associations; decoding and sound readiness unsupported");return 0;
}
