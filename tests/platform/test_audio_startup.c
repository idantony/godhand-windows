#include "audio_startup.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t le(const uint8_t *b){return (uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;}
static uint8_t *read_file(gh_disc *disc,const char *path,size_t *size){
 gh_disc_entry e;size_t i;uint8_t *b;
 CHECK(gh_disc_find(disc,path,&i)==GH_DISC_OK);
 CHECK(gh_disc_entry_at(disc,i,&e)==GH_DISC_OK && e.size<4000000);
 *size=(size_t)e.size;b=malloc(*size);CHECK(b);
 CHECK(gh_disc_read(disc,i,0,b,*size)==GH_DISC_OK);return b;
}
int main(int argc,char **argv){
 gh_disc *disc;gh_audio_registry *r;gh_audio_token tokens[2];
 CHECK(argc==2 && gh_disc_open(argv[1],&disc)==GH_DISC_OK);
 CHECK(gh_audio_registry_create(&r)==GH_AUDIO_OK);
 for(int n=0;n<2;n++){
  size_t size;uint8_t *source=read_file(disc,n?"se/pl00.ses":"se/core.ses",&size);
  gh_sound_bank original;gh_sound_span programs,sys,data,afd,desc,tail;
  gh_audio_token failed,next;uint8_t record[48],saved[48];int16_t id=n?3:0;
  CHECK(gh_sound_bank_open(source,size,&original)==GH_SOUND_OK);
  CHECK(gh_audio_commit_se(r,id,source,size,&tokens[n])==GH_AUDIO_OK);
  CHECK(gh_audio_token_complete(r,tokens[n]));
  {gh_audio_registry *foreign;CHECK(gh_audio_registry_create(&foreign)==GH_AUDIO_OK);
   CHECK(!gh_audio_token_complete(foreign,tokens[n]));gh_audio_registry_destroy(foreign);}
  CHECK(gh_audio_bank_views(r,id,&programs,&sys,&data,&afd)==GH_AUDIO_OK);
  CHECK(programs.data!=original.sections[0].data && sys.data!=original.sections[2].data);
  CHECK(data.data!=original.sections[3].data && afd.data!=original.sections[1].data);
  CHECK(!memcmp(programs.data,original.sections[0].data,programs.size));
  CHECK(!memcmp(sys.data,original.sections[2].data,sys.size));
  CHECK(!memcmp(data.data,original.sections[3].data,data.size));
  CHECK(!memcmp(afd.data,original.sections[1].data,afd.size));
  CHECK(le(programs.data)==(n?68u:358u));
  for(uint32_t i=0;i<le(programs.data);i++){
   const uint8_t *raw=original.sections[0].data+le(programs.data+4)+(size_t)i*48;
   gh_audio_status status=gh_audio_get_program(r,id,(int32_t)i,0,record);
   if(raw[15]&0x80){CHECK(status==GH_AUDIO_DISABLED);continue;}
   CHECK(status==GH_AUDIO_OK && record[0]==id && record[1]==0);
   CHECK(record[2]==(uint8_t)i && record[3]==(uint8_t)(i>>8));
   CHECK(!memcmp(record+4,raw+4,44));
  }
  CHECK(gh_audio_get_program(r,id,0,0,saved)==GH_AUDIO_OK);
  CHECK(gh_audio_get_program(r,id,-1,0,record)==GH_AUDIO_BOUNDS);
  CHECK(gh_audio_get_program(r,id,0,1,record)==GH_AUDIO_BOUNDS);
  for(uint32_t i=0;i<sys.size/16;i++){
   CHECK(gh_audio_sample_at(r,id,i,&desc,&tail)==GH_AUDIO_OK);
   CHECK(desc.size==16 && tail.data==data.data+le(desc.data));
   CHECK(!memcmp(desc.data,original.sections[2].data+(size_t)i*16,16));
   CHECK(!memcmp(tail.data,original.sections[3].data+le(desc.data),tail.size));
  }
  CHECK(gh_audio_sample_at(r,id,(uint32_t)(sys.size/16),&desc,&tail)==GH_AUDIO_BOUNDS);
  /* A failed replacement never creates a completion token or loses old bank. */
  source[0]='X';CHECK(gh_audio_commit_se(r,id,source,size,&failed)==GH_AUDIO_FORMAT);
  CHECK(!gh_audio_token_complete(r,failed) && gh_audio_token_complete(r,tokens[n]));
  source[0]='S';
  {uint32_t off=le(source+0x10);uint8_t saved_byte=source[off+7];
   source[off+7]=255;CHECK(gh_audio_commit_se(r,id,source,size,&failed)==GH_AUDIO_BOUNDS);
   CHECK(!gh_audio_token_complete(r,failed));source[off+7]=saved_byte;}
  /* Source overwrite and free cannot affect any committed table or descriptor. */
  memset(source,0,size);free(source);
  CHECK(gh_audio_get_program(r,id,0,0,record)==GH_AUDIO_OK && !memcmp(saved,record,48));
  source=read_file(disc,n?"se/pl00.ses":"se/core.ses",&size);
  CHECK(gh_audio_commit_se(r,id,source,size,&next)==GH_AUDIO_OK);free(source);
  CHECK(!gh_audio_token_complete(r,tokens[n]) && gh_audio_token_complete(r,next));
  tokens[n]=next;
 }
 /* Actual room020 has no AFD at all. Registry owns all remaining sections;
  * corrupt nonempty AFD must reject atomically and preserve this generation. */
 {size_t size;uint8_t *source=read_file(disc,"se/r020.ses",&size);
  gh_audio_token token,bad;gh_sound_span prog,sys,data,afd;gh_sound_bank original;
  CHECK(gh_sound_bank_open(source,size,&original)==GH_SOUND_OK);
  CHECK(!le(source+0x18)&&!le(source+0x1c)&&!original.external_afs);
  CHECK(gh_audio_commit_se(r,2,source,size,&token)==GH_AUDIO_OK);
  CHECK(gh_audio_token_complete(r,token));
  CHECK(gh_audio_bank_views(r,2,&prog,&sys,&data,&afd)==GH_AUDIO_OK);
  CHECK(!afd.data&&!afd.size&&prog.size==original.sections[0].size&&sys.size==256);
  CHECK(!memcmp(prog.data,original.sections[0].data,prog.size));
  CHECK(!memcmp(data.data,original.sections[3].data,data.size));
  /* Pretend its program bytes are a short AFD: neither absent nor valid. */
  source[0x18]=0x40;source[0x1c]=1;
  CHECK(gh_audio_commit_se(r,2,source,size,&bad)==GH_AUDIO_BOUNDS);
  CHECK(gh_audio_token_complete(r,token)&&!gh_audio_token_complete(r,bad));
  source[0x18]=0;source[0x1c]=0;source[0x30]=0x40;
  CHECK(gh_audio_commit_se(r,2,source,size,&bad)==GH_AUDIO_BOUNDS);
  CHECK(gh_audio_token_complete(r,token));free(source);
  CHECK(gh_audio_release_bank(r,2)==GH_AUDIO_OK&&!gh_audio_token_complete(r,token));
 }
 CHECK(gh_audio_release_bank(r,0)==GH_AUDIO_OK && !gh_audio_token_complete(r,tokens[0]));
 CHECK(gh_audio_token_complete(r,tokens[1]));
 CHECK(gh_audio_release_bank(r,3)==GH_AUDIO_OK && !gh_audio_token_complete(r,tokens[1]));
 {uint32_t f0,f3,bgm;gh_sound_span table,row;uint64_t off,size;
  CHECK(!gh_audio_startup_ready(r,&f0,&f3,&bgm));
  CHECK(gh_audio_startup_load(r,disc)==GH_AUDIO_OK);
  CHECK(gh_audio_startup_ready(r,&f0,&f3,&bgm));
  CHECK(f0==0x7f && f3==0x7f && bgm==0xf);
  CHECK(gh_audio_bgm0_table(r,&table)==GH_AUDIO_OK && le(table.data)==56);
  for(unsigned n=0;n<3;n++){
   int16_t id=n==2?-1:(n?3:0);unsigned count=n==2?56:(n?16:42);
   const char *path=n==2?"bgm/bgm00.afs":(n?"se/pl00.afs":"se/core.afs");
   size_t entry;uint8_t pair[8];
   CHECK(gh_disc_find(disc,path,&entry)==GH_DISC_OK);
   for(unsigned i=0;i<count;i++){
    CHECK(gh_audio_external_sample(r,id,i,&row,&off,&size)==GH_AUDIO_OK);
    unsigned member=(unsigned)row.data[2]|(unsigned)row.data[3]<<8;
    CHECK(gh_disc_read(disc,entry,8+(uint64_t)member*8,pair,8)==GH_DISC_OK);
    CHECK(off==le(pair) && size==le(pair+4));
   }
   CHECK(gh_audio_external_sample(r,id,count,&row,&off,&size)==GH_AUDIO_BOUNDS);
  }
  /* Repeat startup performs actual fresh reads and atomically replaces banks. */
  CHECK(gh_audio_startup_load(r,disc)==GH_AUDIO_OK && gh_audio_startup_ready(r,0,0,0));
  CHECK(gh_audio_release_bank(r,3)==GH_AUDIO_OK && !gh_audio_startup_ready(r,0,0,0));
 }
 {size_t n;uint8_t *ses=read_file(disc,"se/r100.ses",&n);gh_audio_token first,next,bad;
  CHECK(gh_audio_commit_se_with_external_index(r,2,ses,n,disc,&first)==GH_AUDIO_OK);
  gh_sound_span original,after,x,y,z;CHECK(gh_audio_bank_views(r,2,&original,&x,&y,&z)==GH_AUDIO_OK);
  gh_sound_bank view;CHECK(gh_sound_bank_open(ses,n,&view)==GH_SOUND_OK);
  size_t offset=(size_t)(view.sections[1].data-ses)+18;
  uint8_t saved[2]={ses[offset],ses[offset+1]};ses[offset]=255;ses[offset+1]=255;
  CHECK(gh_audio_commit_se_with_external_index(r,2,ses,n,disc,&bad)==GH_AUDIO_BOUNDS);
  CHECK(!bad.owner&&gh_audio_token_complete(r,first));
  CHECK(gh_audio_bank_views(r,2,&after,&x,&y,&z)==GH_AUDIO_OK&&after.data==original.data);
  ses[offset]=saved[0];ses[offset+1]=saved[1];
  size_t path_offset=(size_t)((const uint8_t*)view.external_afs-ses);uint8_t path_first=ses[path_offset];ses[path_offset]='x';
  CHECK(gh_audio_commit_se_with_external_index(r,2,ses,n,disc,&bad)!=GH_AUDIO_OK);
  CHECK(!bad.owner&&gh_audio_token_complete(r,first));ses[path_offset]=path_first;
  CHECK(gh_audio_release_bank(r,2)==GH_AUDIO_OK&&!gh_audio_token_complete(r,first));
  CHECK(gh_audio_commit_se_with_external_index(r,2,ses,n,disc,&next)==GH_AUDIO_OK);
  CHECK(next.generation==first.generation+1&&next.owner==first.owner);free(ses);
 }
 gh_audio_registry_destroy(r);gh_disc_close(disc);
 puts("PASS actual core/pl00 native bank commit, program queries, owned sample copies, generation tokens and failed replacement");return 0;
}
