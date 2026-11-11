/* Read-only native originalIDD view; notclaimR020 Initialize/BGM finished. */
#include "../src/platform/disc.h"
#include "../src/core/id_jobs.h"
#include "../src/assets/font_texture.h"
#include "../src/assets/tim2.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static void *asset(gh_disc*d,const char*path,size_t*n){size_t i;gh_disc_entry e;void*b;if(gh_disc_find(d,path,&i)||gh_disc_entry_at(d,i,&e))return NULL;*n=(size_t)e.size;b=malloc(*n);if(!b||gh_disc_read(d,i,0,b,*n)){free(b);return NULL;}return b;}
static unsigned word(const unsigned char*p){return p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;}
static int index_value(const char *argument,unsigned limit,unsigned *result)
{
    char *end;
    const unsigned long value=strtoul(argument,&end,0);
    if(end==argument||*end||value>limit)return 0;
    *result=(unsigned)value;
    return 1;
}

/* Report raw original texture metadata even when the renderer rejects it. */
static int inspect_core_texture(const gh_id_state *manager,unsigned slot,unsigned item)
{
    const gh_bundle_entry *table=&manager->pair[slot].texture_data;
    if(!table->bytes||table->size<4)return 1;
    const unsigned count=word(table->bytes);
    if(!item||item>count||count>(table->size-4)/4)return 1;
    const size_t header=4+(size_t)count*4;
    const size_t offset=word(table->bytes+(size_t)item*4);
    if(offset<header||offset>=table->size)return 1;
    size_t end=table->size;
    for(unsigned i=1;i<=count;++i){
        const size_t next=word(table->bytes+(size_t)i*4);
        if(next&&(next<header||next>table->size))return 1;
        if(next>offset&&next<end)end=next;
    }
    const unsigned char *bytes=table->bytes+offset;
    const size_t size=end-offset;
    if(size<4)return 1;

    gh_id_texture texture={0};
    gh_id_error status=gh_id_texture_open(manager,slot,item,&texture);
    printf("{\"slot\":%u,\"item\":%u,\"bytes\":%zu,\"open_status\":%u",
           slot,item,size,(unsigned)status);
    if(!memcmp(bytes,"TIM2",4)){
        gh_tim2_picture picture;
        const gh_tim2_error error=gh_tim2_first(bytes,size,&picture);
        printf(",\"format\":\"TIM2\",\"parse_status\":%u",(unsigned)error);
        if(!error)printf(",\"width\":%u,\"height\":%u,\"colors\":%u,\"image_type\":%u,"
            "\"clut_type\":%u,\"image_bytes\":%zu,\"tex0\":\"%016llX\"",
            (unsigned)picture.width,(unsigned)picture.height,(unsigned)picture.colors,
            (unsigned)picture.image_type,(unsigned)picture.clut_type,picture.image_size,
            (unsigned long long)picture.tex0);
    }else if(size>=48){
        printf(",\"format\":\"T32 candidate\",\"tex0\":\"%08X%08X\"",
            word(bytes+36),word(bytes+32));
    }
    puts("}");
    return 0;
}

int main(int argc,char**argv){gh_disc*d=NULL;void*language=NULL,*title=NULL;size_t ln,tn;gh_bundle bundle;gh_bundle_entry idd;gh_id_state*m=calloc(1,sizeof(*m));gh_id_runtime*r=NULL;gh_id_base o={0};int original=0,reclaimed=0;gh_id_error init;int rc=1;if((argc!=2&&argc!=5)||!m||gh_disc_open(argv[1],&d))goto end;language=asset(d,"etc/core_usa.bin",&ln);
 if(!language||gh_bundle_open(language,ln,&bundle)||gh_id_find_language_data(&bundle,&idd)||gh_id_system_init_002acc70(m,&idd))goto end;
 if(argc==5){
    unsigned slot,item;
    if(strcmp(argv[2],"--texture")||!index_value(argv[3],31,&slot)||!index_value(argv[4],65535,&item))goto end;
    rc=inspect_core_texture(m,slot,item);
    goto end;
 }
 title=asset(d,"id/local/us/idtitlecore_us.idd",&tn);if(!title)goto end;idd.bytes=title;idd.size=tn;if(gh_id_set_data_002accf8(m,2,&idd)||gh_id_runtime_create(m,1,&r))goto end;init=gh_id_base_initialize_002a9ea0(r,&o,2,0,NULL,NULL,&original);printf("{\"scope\":\"originaltitleIDD view, notR020 Init/BGM completion\",\"initialize_error\":%u,\"frontier\":\"%08x\",\"entries\":%u,\"logos\":[",(unsigned)init,o.first_unported,o.entry_count);
 {unsigned seen=0;for(uint32_t i=0;i<o.entry_count;i++){gh_id_entry*e=o.entries+i;if(e->id!=0x10&&e->id!=0x11)continue;gh_id_texture t={0};gh_id_error error=gh_id_texture_open(m,e->b8a,(uint32_t)e->b8b+1,&t);printf("%s{\"entry\":%u,\"id\":%u,\"kind\":%u,\"flags\":\"%08x\",\"slot\":%u,\"item\":%u,\"texture_error\":%u,\"width\":%u,\"height\":%u,\"tex0\":\"%016llx\"",seen++?",":"",i,e->id,e->kind,e->flags,e->b8a,(unsigned)e->b8b+1,(unsigned)error,t.width,t.height,(unsigned long long)t.tex0);
 if(e->b8a>=32){printf("}");continue;}const gh_bundle_entry*table=&m->pair[e->b8a].texture_data;size_t index=((size_t)e->b8b+1)*4;if(table->bytes&&index+4<=table->size){size_t off=word(table->bytes+index);if(off+48<=table->size){const unsigned char*p=table->bytes+off;printf(",\"TBL1_offset\":%zu,\"magic\":\"%02x%02x%02x%02x\",\"rawTex0_at32\":\"%08x%08x\"",off,p[0],p[1],p[2],p[3],word(p+36),word(p+32));gh_font_slot slot={0};gh_font_transfer im={0},cl={0};slot.bound=1;slot.mst=p;slot.mst_size=table->size-off;for(unsigned j=0;j<2;j++){slot.table_offset[j]=word(p+8+j*4);slot.table_count[j]=(uint16_t)(p[16+j*2]|(unsigned)p[17+j*2]<<8);}gh_font_error a=gh_font_transfer_at(&slot,0,0,&im),b=gh_font_transfer_at(&slot,1,0,&cl);printf(",\"image_error\":%u,\"image_upload\":[%u,%u,%u,%u,%u],\"clut_error\":%u,\"clut_upload\":[%u,%u,%u,%u,%u]",(unsigned)a,im.width,im.height,im.psm,im.destination_base,im.buffer_width,(unsigned)b,cl.width,cl.height,cl.psm,cl.destination_base,cl.buffer_width);}}printf("}");}printf("]}\n");}rc=0;
end:if(r){if(o.entries)gh_id_base_release_002aa078(r,&o,&original,&reclaimed);if(gh_id_runtime_destroy(r))rc=1;}free(m);free(title);free(language);gh_disc_close(d);return rc;}
