#include "../../src/core/id_text.h"
#include "../../src/platform/disc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static unsigned char *asset(gh_disc*d,const char*path,size_t*n){size_t i;gh_disc_entry e;unsigned char*b;C(gh_disc_find(d,path,&i)==GH_DISC_OK);C(gh_disc_entry_at(d,i,&e)==GH_DISC_OK);*n=(size_t)e.size;b=malloc(*n);C(b);C(gh_disc_read(d,i,0,b,*n)==GH_DISC_OK);return b;}
int main(int argc,char**argv){
 gh_disc*d;size_t n,elfsize;unsigned char *language,*save,*elf;gh_bundle lang;gh_bundle_entry idd;gh_id_state*manager=calloc(1,sizeof(*manager));gh_id_runtime*runtime;gh_id_base object={0};gh_font_state font={0};gh_packed_message_state packed={0};gh_menu_state menu={0};gh_id_text_context context={1,512,448,1824,{0}};gh_id_text_glyph glyphs[512];gh_id_text_result result;int original,reclaimed;
 C(argc==2&&manager);C(gh_disc_open(argv[1],&d)==GH_DISC_OK);language=asset(d,"etc/core_usa.bin",&n);C(gh_bundle_open(language,n,&lang)==GH_BUNDLE_OK);C(gh_font_initialize(&font,&lang,1)==GH_FONT_OK);C(gh_menu_load_tables_002ae990(&menu,&lang)==GH_MENU_OK);C(gh_id_find_language_data(&lang,&idd)==GH_MENU_OK);C(gh_id_system_init_002acc70(manager,&idd)==GH_MENU_OK);
 save=asset(d,"id/local/US/idsavecore_US.idd",&n);idd.bytes=save;idd.size=n;C(gh_id_set_data_002accf8(manager,11,&idd)==GH_MENU_OK);C(gh_id_runtime_create(manager,1,&runtime)==GH_ID_OK);C(gh_id_base_initialize_002a9ea0(runtime,&object,11,0,NULL,NULL,&original)==GH_ID_OK);C(gh_packed_message_bind(&object,manager,&font,&packed,&lang,1,11,1,&original)==GH_ID_OK);
 elfsize=(size_t)gh_disc_boot_elf_size(d);C(elfsize>0&&elfsize<8*1024*1024);elf=malloc(elfsize);C(elf);C(gh_disc_read_boot_elf(d,0,elf,elfsize)==GH_DISC_OK);C(gh_id_text_palette_from_elf(elf,elfsize,&context.palette)==GH_ID_OK);
 object.entries[0].message=0x2007;object.entries[0].draw_x=39;object.entries[0].draw_y=213;object.entries[0].draw_scale_x=0.849999726f;object.entries[0].draw_scale_y=0.899999976f;
 C(gh_id_text_prepare(&object,&object.entries[0],&menu,&packed,&font,&context,glyphs,512,&result)==GH_ID_OK);
 {const unsigned char*t=packed.cursor.bytes;size_t message_offset=(size_t)t[60]|(size_t)t[61]<<8|(size_t)t[62]<<16|(size_t)t[63]<<24;unsigned char*p=(unsigned char*)t+message_offset;
 unsigned char a=p[0],b=p[1];p[0]=2;p[1]=0x93;
 C(gh_id_text_prepare(&object,&object.entries[0],&menu,&packed,&font,&context,glyphs,512,&result)==GH_ID_UNSUPPORTED&&result.opcode==0x9302&&result.frontier_address==0x2b0028&&result.glyph_count==0);p[0]=a;p[1]=b;}
 C(gh_id_text_prepare(&object,&object.entries[0],&menu,&packed,&font,&context,glyphs,512,&result)==GH_ID_OK);
 C(result.glyph_count>150&&result.line_count==5);C(glyphs[0].code==0x42&&glyphs[1].code==0x61);
 C(glyphs[0].quad.x0==29296&&glyphs[0].quad.y0==32764&&glyphs[0].quad.rgba==0xb232383cU);
 C(glyphs[0].quad.u0==536&&glyphs[0].quad.v0==1032&&glyphs[0].quad.palette_parity==0);
 C(gh_id_text_prepare(&object,&object.entries[0],&menu,&packed,&font,&context,glyphs,1,&result)==GH_ID_CAPACITY);
 C(gh_id_text_palette_from_elf(elf,40,&context.palette)==GH_ID_FORMAT);
 C(gh_id_text_prepare(&object,&object.entries[0],&menu,&packed,&font,&context,glyphs,512,&result)==GH_ID_OK);
 printf("PASS actual firstMC text2007: %zu originalglyphs/%zulines, palette7, .9zoom, subpixelquad and capacity bounds\n",result.glyph_count,result.line_count);
 C(gh_id_base_release_002aa078(runtime,&object,&original,&reclaimed)==GH_ID_OK);C(gh_id_runtime_destroy(runtime)==GH_ID_OK);free(elf);free(save);free(language);free(manager);gh_disc_close(d);return 0;
}

