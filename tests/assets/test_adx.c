#include "../../src/assets/adx.h"
#include "../../src/platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL line%d: %s\n",__LINE__,#x);return 1;}}while(0)
static unsigned le32(const unsigned char*p){return (unsigned)p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;}
int main(int argc,char**argv){gh_disc*disc=NULL;size_t index,elf_size,n,at=0;unsigned char *encoded,*elf;gh_adx_key key,bad;gh_adx_decoder*d=NULL,*other=NULL;gh_adx_info info;int16_t *pcm,buffer[4096];uint64_t fnv=UINT64_C(14695981039346656037);size_t i;FILE*f;
 C(argc==3);C(gh_disc_open(argv[1],&disc)==GH_DISC_OK);C(gh_disc_find(disc,"bgm/bgm00.afs",&index)==GH_DISC_OK);
 encoded=malloc(7168000);elf_size=(size_t)gh_disc_boot_elf_size(disc);elf=malloc(elf_size);C(encoded&&elf);
 C(gh_disc_read(disc,index,2048,encoded,7168000)==GH_DISC_OK);C(gh_disc_read_boot_elf(disc,0,elf,elf_size)==GH_DISC_OK);gh_disc_close(disc);
 C(gh_adx_key_from_elf(elf,elf_size,&key)==GH_ADX_OK);C(key.start==0x49e1&&key.mult==0x4a57&&key.add==0x553d);
 C(gh_adx_key_from_elf(elf,52,&bad)==GH_ADX_BOUNDS);bad=key;bad.start^=0x2000;C(gh_adx_open(encoded,7168000,&bad,&d)==GH_ADX_KEY&&d==NULL);
 C(gh_adx_open(encoded,64,&key,&d)==GH_ADX_FORMAT&&d==NULL);
 {unsigned char old=encoded[5];encoded[5]=17;C(gh_adx_open(encoded,7168000,&key,&d)==GH_ADX_UNSUPPORTED);encoded[5]=old;}
 {unsigned char old=encoded[47];encoded[47]^=1;C(gh_adx_open(encoded,7168000,&key,&d)==GH_ADX_FORMAT);encoded[47]=old;}
 C(gh_adx_open(encoded,7168000,&key,&d)==GH_ADX_OK);C(gh_adx_inspect(d,&info)==GH_ADX_OK);
 C(info.channels==2&&info.sample_rate==48000&&info.sample_count==6369200&&info.loop_enabled&&info.loop_start==471744&&info.loop_end==6163980);
 pcm=malloc((size_t)info.sample_count*4);C(pcm);
 while(at<info.sample_count){size_t take=1+(at%2017);if(take>info.sample_count-at)take=info.sample_count-at;C(gh_adx_decode(d,pcm+at*2,take,&n)==GH_ADX_OK&&n==take);at+=n;}
 C(gh_adx_decode(d,buffer,100,&n)==GH_ADX_OK&&!n);C(gh_adx_seek(d,info.sample_count+1)==GH_ADX_BOUNDS&&gh_adx_tell(d)==info.sample_count);
 C(gh_adx_seek(d,info.loop_start)==GH_ADX_OK);C(gh_adx_decode(d,buffer,2048,&n)==GH_ADX_OK&&n==2048);C(!memcmp(buffer,pcm+(size_t)info.loop_start*2,n*4));
 C(gh_adx_seek(d,31)==GH_ADX_OK);C(gh_adx_decode(d,buffer,2048,&n)==GH_ADX_OK&&n==2048);C(!memcmp(buffer,pcm+62,n*4));
 /* Independent official vgmstream -i output, RIFF chunk parser rather than
  * assuming a fixed 44B WAV header. Every signed sample must match. */
 f=fopen(argv[2],"rb");C(f);{unsigned char h[12];C(fread(h,1,12,f)==12&&!memcmp(h,"RIFF",4)&&!memcmp(h+8,"WAVE",4));for(;;){unsigned char ch[8];unsigned size;C(fread(ch,1,8,f)==8);size=le32(ch+4);if(!memcmp(ch,"data",4)){C(size==(unsigned)info.sample_count*4);at=0;while(at<size){size_t take=size-at;if(take>sizeof(buffer))take=sizeof(buffer);C(fread(buffer,1,take,f)==take);C(!memcmp(buffer,(unsigned char*)pcm+at,take));at+=take;}break;}C(_fseeki64(f,(int64_t)size+(size&1),SEEK_CUR)==0);}}C(fclose(f)==0);
 for(i=0;i<(size_t)info.sample_count*4;i++){fnv^=((unsigned char*)pcm)[i];fnv*=UINT64_C(1099511628211);}
 /* Nonzero v4 history must really feed the predictor. */
 encoded[24]=0x12;encoded[25]=0x34;C(gh_adx_open(encoded,7168000,&key,&other)==GH_ADX_OK);C(gh_adx_decode(other,buffer,32,&n)==GH_ADX_OK&&n==32);C(memcmp(buffer,pcm,128)!=0);gh_adx_close(other);
 printf("PASS actual encrypted stereo ADX: %u frames / %zu PCM bytes, all samples match independent vgmstream; key=%04X/%04X/%04X coefficients=%d/%d PCM FNV=%016llx; seek/history/invalid-key checks\n",info.sample_count,(size_t)info.sample_count*4,key.start,key.mult,key.add,info.coefficient[0],info.coefficient[1],(unsigned long long)fnv);
 gh_adx_close(d);free(pcm);free(encoded);free(elf);return 0;
}
