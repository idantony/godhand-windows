#include "../../src/core/id_text.h"
#include "../../src/platform/mc_renderer.h"
#include "../../src/platform/disc.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)

static unsigned char *asset(gh_disc *disc,const char *path,size_t *size)
{
    gh_disc_entry entry;
    size_t index;
    C(gh_disc_find(disc,path,&index)==GH_DISC_OK&&gh_disc_entry_at(disc,index,&entry)==GH_DISC_OK);
    *size=(size_t)entry.size;
    unsigned char *bytes=malloc(*size);
    C(bytes&&gh_disc_read(disc,index,0,bytes,*size)==GH_DISC_OK);
    return bytes;
}

int main(int argc,char **argv)
{
    gh_disc *disc;
    C(argc==2&&gh_disc_open(argv[1],&disc)==GH_DISC_OK);
    size_t size;
    unsigned char *language_bytes=asset(disc,"etc/core_usa.bin",&size);
    gh_bundle language;
    gh_bundle_entry idd;
    gh_font_state font={0};
    gh_menu_state menu={0};
    gh_packed_message_state packed={0};
    gh_id_state *manager=calloc(1,sizeof(*manager));
    gh_id_runtime *runtime;
    gh_id_base base={0};
    C(manager&&gh_bundle_open(language_bytes,size,&language)==GH_BUNDLE_OK);
    C(gh_font_initialize(&font,&language,1)==GH_FONT_OK&&gh_menu_load_tables_002ae990(&menu,&language)==GH_MENU_OK);
    C(gh_id_find_language_data(&language,&idd)==GH_MENU_OK&&gh_id_system_init_002acc70(manager,&idd)==GH_MENU_OK);
    unsigned char *map=asset(disc,"id/local/US/idsubscrmap_US.idd",&size);
    idd.bytes=map;idd.size=size;
    C(gh_id_set_data_002accf8(manager,7,&idd)==GH_MENU_OK&&gh_id_runtime_create(manager,1,&runtime)==GH_ID_OK);
    int original,reclaimed;
    C(gh_id_base_initialize_002a9ea0(runtime,&base,7,0,NULL,NULL,&original)==GH_ID_OK&&original==1);
    C(gh_packed_message_bind(&base,manager,&font,&packed,&language,1,7,1,&original)==GH_ID_OK&&original==1);
    gh_id_entry *entry;
    C(gh_id_base_get_work_002aa410(runtime,&base,25,&entry,NULL)==GH_ID_OK&&entry->kind==2);
    entry->message=0x2022;
    C(gh_id_base_restart_anim(&base,0,0)==GH_ID_OK);
    gh_id_frontier move_frontier;
    C(gh_id_jobs_move_reference_rotation(&base,NULL,NULL,&move_frontier)==GH_ID_OK);
    const size_t elf_size=(size_t)gh_disc_boot_elf_size(disc);
    unsigned char *elf=malloc(elf_size);
    gh_id_text_context context={1,512,448,1824,{0}};
    C(elf&&gh_disc_read_boot_elf(disc,0,elf,elf_size)==GH_DISC_OK);
    C(gh_id_text_palette_from_elf(elf,elf_size,&context.palette)==GH_ID_OK);
    gh_id_text_glyph glyphs[128];
    gh_id_text_result result;
    C(gh_id_text_prepare(&base,entry,&menu,&packed,&font,&context,glyphs,128,&result)==GH_ID_UNSUPPORTED);
    C(result.opcode==0x9401&&result.frontier_address==0x2b0028&&!result.glyph_count);
    C(gh_id_text_prepare_with_chapter(&base,entry,&menu,&packed,&font,&context,0x2022,glyphs,128,&result)==GH_ID_UNSUPPORTED);
    const gh_id_error status=gh_id_text_prepare_with_chapter(&base,entry,&menu,&packed,&font,&context,0x2024,glyphs,128,&result);
    if(status)fprintf(stderr,"prepare %d opcode %04x frontier %06x at %zu\n",status,result.opcode,result.frontier_address,result.source_offset);
    C(status==GH_ID_OK&&result.glyph_count==42&&result.line_count==2&&!result.icon_count);
    C(fabsf(result.width-231.419922f)<0.0001f&&fabsf(result.height-54.4599915f)<0.0001f);
    C(glyphs[0].code==0x4d&&glyphs[0].quad.x0==28912&&glyphs[0].quad.y0==35174);
    C(glyphs[10].code==0x52&&glyphs[10].quad.x0==29160&&glyphs[10].quad.y0==35552);
    C(glyphs[41].code==0x23&&glyphs[41].quad.x0==32596&&glyphs[41].quad.y0==35552);
    C(glyphs[10].quad.rgba==0x805a6a6e&&glyphs[41].quad.rgba==0x805a6a6e);
    C(gh_id_text_prepare_with_chapter(&base,entry,&menu,&packed,&font,&context,0x2024,glyphs,1,&result)==GH_ID_CAPACITY);
    gh_mc_renderer *renderer;
    C(gh_mc_renderer_create(elf,elf_size,1,&renderer)==GH_ID_OK);
    gh_mc_renderer_begin(renderer);
    const gh_id_render_view view={&font,&menu,manager,&base,&packed};
    const gh_id_draw_job job={.entry_index=(uint32_t)(entry-base.entries),.entry=entry,.consumer_address=0x2ab0b8};
    gh_ui_ot_tag tag={UINT32_MAX,-1,SIZE_MAX};
    size_t count=123;
    C(gh_mc_renderer_queue_view_groups_with_chapter(renderer,&view,&job,0x2024,&tag,0,&count)==GH_ID_CAPACITY);
    C(tag.index==SIZE_MAX&&count==123);
    C(gh_mc_renderer_queue_view_groups_with_chapter(renderer,&view,&job,0x2024,&tag,1,&count)==GH_ID_OK);
    C(count==1&&tag.bank==6&&tag.depth==2&&tag.index==0);
    gh_mc_renderer_destroy(renderer);
    C(gh_id_base_release_002aa078(runtime,&base,&original,&reclaimed)==GH_ID_OK&&gh_id_runtime_destroy(runtime)==GH_ID_OK);
    gh_disc_close(disc);free(elf);free(map);free(language_bytes);free(manager);
    puts("PASS actual USA Door map 9401 chapter and captured ordinary IDBase OT group");
    return 0;
}
