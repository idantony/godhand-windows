#include "../../src/assets/adx.h"
#include "../../src/platform/disc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expr) do { \
    if (!(expr)) { fprintf(stderr,"line %d: %s\n",__LINE__,#expr); exit(1); } \
} while (0)

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}

int main(int argc,char **argv)
{
    CHECK(argc==3);
    gh_disc *disc;
    CHECK(!gh_disc_open(argv[1],&disc));
    size_t index;
    uint8_t directory[16];
    CHECK(!gh_disc_find(disc,"se/ev/ev01.afs",&index));
    CHECK(!gh_disc_read(disc,index,0,directory,sizeof(directory)));
    CHECK(!memcmp(directory,"AFS\0",4) && le32(directory+4)==1);
    size_t bytes=le32(directory+12);
    uint8_t *source=malloc(bytes);
    CHECK(source && !gh_disc_read(disc,index,le32(directory+8),source,bytes));
    size_t elf_bytes=(size_t)gh_disc_boot_elf_size(disc);
    uint8_t *elf=malloc(elf_bytes);
    gh_adx_key key;
    CHECK(elf && !gh_disc_read_boot_elf(disc,0,elf,elf_bytes) && !gh_adx_key_from_elf(elf,elf_bytes,&key));
    free(elf);
    gh_disc_close(disc);

    gh_adx_decoder *decoder=NULL;
    CHECK(gh_adx_open(source,39,&key,&decoder)==GH_ADX_FORMAT && !decoder);
    CHECK(gh_adx_open(source,40,&key,&decoder)==GH_ADX_BOUNDS && !decoder);
    uint8_t marker=source[34];source[34]^=1;
    CHECK(gh_adx_open(source,bytes,&key,&decoder)==GH_ADX_FORMAT && !decoder);
    source[34]=marker;
    CHECK(!gh_adx_open(source,bytes,&key,&decoder));
    gh_adx_info info;
    CHECK(!gh_adx_inspect(decoder,&info));
    CHECK(info.data_offset==40 && info.channels==2 && info.sample_rate==48000 && info.sample_count==1961984);
    CHECK(!info.loop_enabled && !info.loop_start && !info.loop_end);
    CHECK(!info.initial_history[0][0] && !info.initial_history[0][1] &&
        info.initial_history[1][0]==1 && info.initial_history[1][1]==1);

    FILE *reference=fopen(argv[2],"rb");
    uint8_t header[12];
    CHECK(reference && fread(header,1,12,reference)==12 && !memcmp(header,"RIFF",4) && !memcmp(header+8,"WAVE",4));
    for(;;){
        uint8_t chunk[8];
        CHECK(fread(chunk,1,8,reference)==8);
        uint32_t size=le32(chunk+4);
        if(!memcmp(chunk,"data",4)){
            CHECK(size==info.sample_count*4);
            break;
        }
        CHECK(!_fseeki64(reference,(int64_t)size+(size&1),SEEK_CUR));
    }
    int16_t expected[2048],actual[2048];
    size_t position=0,decoded;
    while(position<info.sample_count){
        size_t frames=1+position%1023;
        if(frames>info.sample_count-position)frames=info.sample_count-position;
        CHECK(fread(expected,4,frames,reference)==frames);
        CHECK(!gh_adx_decode(decoder,actual,frames,&decoded) && decoded==frames);
        CHECK(!memcmp(expected,actual,frames*4));
        position+=frames;
    }
    CHECK(!gh_adx_decode(decoder,actual,1,&decoded) && !decoded);
    CHECK(!gh_adx_seek(decoder,info.sample_count-1));
    CHECK(!gh_adx_decode(decoder,actual,1,&decoded) && decoded==1);
    CHECK(!_fseeki64(reference,-4,SEEK_CUR) && fread(expected,4,1,reference)==1 && !memcmp(actual,expected,4));
    CHECK(!fclose(reference));
    gh_adx_close(decoder);
    free(source);
    puts("PASS ev01 compact ADX, 1961984 stereo frames match vgmstream");
    return 0;
}
