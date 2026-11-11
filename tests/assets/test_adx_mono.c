#include "adx.h"
#include "audio_startup.h"
#include "disc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expr) do { \
    if (!(expr)) { fprintf(stderr,"line %d: %s\n",__LINE__,#expr); return 1; } \
} while (0)

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}

static uint16_t le16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0]|(uint16_t)p[1]<<8);
}

static uint8_t *load_asset(gh_disc *disc,const char *name,size_t *bytes)
{
    size_t index;
    gh_disc_entry entry;
    if(gh_disc_find(disc,name,&index)||gh_disc_entry_at(disc,index,&entry))return NULL;
    uint8_t *data=malloc((size_t)entry.size);
    if(!data)return NULL;
    if(gh_disc_read(disc,index,0,data,(size_t)entry.size)){
        free(data);
        return NULL;
    }
    *bytes=(size_t)entry.size;
    return data;
}

typedef struct {
    int16_t program;
    uint16_t afd,member;
    uint32_t offset,bytes,frames;
    uint64_t fnv;
} expected_sound;

static int compare_reference(const char *path,const int16_t *pcm,size_t frames)
{
    if(!path)return 0;
    FILE *file=fopen(path,"rb");
    uint8_t header[12];
    CHECK(file&&fread(header,1,12,file)==12&&!memcmp(header,"RIFF",4)&&!memcmp(header+8,"WAVE",4));
    for(;;){
        uint8_t chunk[8];
        CHECK(fread(chunk,1,8,file)==8);
        const uint32_t bytes=le32(chunk+4);
        if(!memcmp(chunk,"data",4)){
            CHECK(bytes==frames*2);
            break;
        }
        CHECK(!_fseeki64(file,(int64_t)bytes+(bytes&1u),SEEK_CUR));
    }
    int16_t buffer[1024];
    for(size_t done=0;done<frames;){
        size_t count=frames-done;
        if(count>1024)count=1024;
        CHECK(fread(buffer,2,count,file)==count&&!memcmp(pcm+done,buffer,count*2));
        done+=count;
    }
    CHECK(!fclose(file));
    return 0;
}

static int verify(gh_disc *disc,const gh_audio_registry *registry,const gh_adx_key *key,
    size_t afs,const expected_sound *expected,const char *reference)
{
    uint8_t descriptor[48];
    CHECK(!gh_audio_get_program(registry,2,expected->program,0,descriptor));
    CHECK(descriptor[0x20]==4&&le16(descriptor+0x28)==expected->afd);
    gh_sound_span afd;
    uint64_t offset,bytes;
    CHECK(!gh_audio_external_sample(registry,2,expected->afd,&afd,&offset,&bytes));
    CHECK(afd.size==32&&le16(afd.data+2)==expected->member&&offset==expected->offset&&bytes==expected->bytes);
    uint8_t *source=malloc((size_t)bytes);
    CHECK(source&&!gh_disc_read(disc,afs,offset,source,(size_t)bytes));
    gh_adx_decoder *decoder=NULL;
    gh_adx_key bad=*key;
    bad.start^=0x2000;
    CHECK(gh_adx_open(source,(size_t)bytes,&bad,&decoder)==GH_ADX_KEY&&!decoder);
    const size_t truncated=40u+((expected->frames+31u)/32u)*18u-1u;
    CHECK(gh_adx_open(source,truncated,key,&decoder)==GH_ADX_BOUNDS&&!decoder);
    CHECK(!gh_adx_open(source,(size_t)bytes,key,&decoder));
    gh_adx_info info;
    CHECK(!gh_adx_inspect(decoder,&info));
    CHECK(info.channels==1&&info.sample_rate==48000&&info.sample_count==expected->frames&&
        info.data_offset==40&&!info.loop_enabled&&!info.loop_start&&!info.loop_end);
    int16_t *pcm=malloc((size_t)expected->frames*2);
    CHECK(pcm);
    size_t decoded;
    for(size_t at=0;at<expected->frames;){
        size_t count=1+at%1031;
        if(count>expected->frames-at)count=expected->frames-at;
        CHECK(!gh_adx_decode_mono(decoder,pcm+at,count,&decoded)&&decoded==count);
        at+=decoded;
    }
    CHECK(!gh_adx_decode_mono(decoder,NULL,0,&decoded)&&!decoded);
    CHECK(!gh_adx_decode_mono(decoder,pcm,1,&decoded)&&!decoded);
    CHECK(gh_adx_seek(decoder,expected->frames+1)==GH_ADX_BOUNDS&&gh_adx_tell(decoder)==expected->frames);
    CHECK(!compare_reference(reference,pcm,expected->frames));
    uint64_t fnv=UINT64_C(14695981039346656037);
    for(size_t i=0;i<(size_t)expected->frames*2;i++){
        fnv^=((const uint8_t *)pcm)[i];
        fnv*=UINT64_C(1099511628211);
    }
    CHECK(fnv==expected->fnv);
    int16_t stereo[70];
    CHECK(!gh_adx_seek(decoder,31)&&!gh_adx_decode(decoder,stereo,35,&decoded)&&decoded==35);
    for(size_t i=0;i<decoded;i++)CHECK(stereo[i*2]==pcm[31+i]&&stereo[i*2+1]==pcm[31+i]);
    CHECK(!gh_adx_seek(decoder,expected->frames-1));
    CHECK(!gh_adx_decode_mono(decoder,stereo,2,&decoded)&&decoded==1&&stereo[0]==pcm[expected->frames-1]);
    printf("PASS bank2 program %02x AFD%u member%u mono PCM %u frames FNV %016llx\n",
        expected->program,expected->afd,expected->member,expected->frames,(unsigned long long)fnv);
    gh_adx_close(decoder);
    free(pcm);
    free(source);
    return 0;
}

int main(int argc,char **argv)
{
    CHECK(argc==2||argc==4);
    gh_disc *disc=NULL;
    gh_audio_registry *registry=NULL;
    CHECK(!gh_disc_open(argv[1],&disc)&&!gh_audio_registry_create(&registry));
    size_t ses_bytes=0,afs;
    uint8_t *ses=load_asset(disc,"se/r100.ses",&ses_bytes);
    gh_audio_token token;
    CHECK(ses&&!gh_audio_commit_se_with_external_index(registry,2,ses,ses_bytes,disc,&token));
    CHECK(gh_audio_token_complete(registry,token)&&!gh_disc_find(disc,"se/r100.afs",&afs));
    free(ses);
    const size_t elf_bytes=(size_t)gh_disc_boot_elf_size(disc);
    uint8_t *elf=malloc(elf_bytes);
    gh_adx_key key;
    CHECK(elf&&!gh_disc_read_boot_elf(disc,0,elf,elf_bytes)&&!gh_adx_key_from_elf(elf,elf_bytes,&key));
    free(elf);
    const expected_sound cases[]={
        {0x32,4,2,268288,45454,80692,UINT64_C(0x212c969b8d1926a8)},
        {0x33,6,3,315392,30082,53375,UINT64_C(0x639e7618e6d5eaa4)}
    };
    for(unsigned i=0;i<sizeof(cases)/sizeof(cases[0]);i++)
        CHECK(!verify(disc,registry,&key,afs,cases+i,argc==4?argv[2+i]:NULL));
    /* The same room AFS contains stereo members. Mono output cannot downmix them. */
    gh_sound_span stereo_afd;
    uint64_t stereo_offset,stereo_bytes;
    CHECK(!gh_audio_external_sample(registry,2,0,&stereo_afd,&stereo_offset,&stereo_bytes));
    uint8_t *stereo=malloc((size_t)stereo_bytes);
    CHECK(stereo&&!gh_disc_read(disc,afs,stereo_offset,stereo,(size_t)stereo_bytes));
    gh_adx_decoder *stereo_decoder=NULL;
    CHECK(!gh_adx_open(stereo,(size_t)stereo_bytes,&key,&stereo_decoder));
    gh_adx_info stereo_info;
    CHECK(!gh_adx_inspect(stereo_decoder,&stereo_info)&&stereo_info.channels==2);
    int16_t untouched=123;
    size_t decoded=999;
    CHECK(gh_adx_decode_mono(stereo_decoder,&untouched,1,&decoded)==GH_ADX_UNSUPPORTED&&
        untouched==123&&!decoded&&!gh_adx_tell(stereo_decoder));
    gh_adx_close(stereo_decoder);
    free(stereo);
    gh_audio_registry_destroy(registry);
    gh_disc_close(disc);
    return 0;
}
