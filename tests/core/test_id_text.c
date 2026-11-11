#include "../../src/core/id_text.h"
#include "../../src/platform/disc.h"
#ifdef GH_ID_TEXT_QUEUE_TEST
#include "../../src/platform/mc_renderer.h"
#endif
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t little_word(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
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
 /* Actualnotice/choiceMSD contents, including2021 whoseexisting-save
  * Controlbranch stillstopsatOpen3350. ThisisassetconsumerQA, notfakedflow. */
 {static const uint16_t messages[]={0x2003,0x2004,0x201e,0x201f,0x2020,0x2021};
  static const size_t counts[]={3,2,37,188,157,226};
  static const size_t lines[]={1,1,1,5,5,7};
  C(gh_id_text_palette_from_elf(elf,elfsize,&context.palette)==GH_ID_OK);
  for(size_t k=0;k<sizeof(messages)/sizeof(messages[0]);k++){object.entries[0].message=messages[k];C(gh_id_text_prepare(&object,&object.entries[0],&menu,&packed,&font,&context,glyphs,512,&result)==GH_ID_OK);C(result.glyph_count==counts[k]&&result.line_count==lines[k]);}
  object.entries[0].message=0x2007;
 }
 C(gh_id_text_prepare(&object,&object.entries[0],&menu,&packed,&font,&context,glyphs,512,&result)==GH_ID_OK);
 printf("PASS actual firstMC text2007: %zu originalglyphs/%zulines, palette7, .9zoom, subpixelquad and capacity bounds\n",result.glyph_count,result.line_count);
 /* Original source57/ID0x62 is the third empty title-list row. */
 gh_id_entry *empty;
 C(!gh_id_base_get_work_002aa410(runtime,&object,0x62,&empty,NULL)&&empty->source_index==57);
 empty->message=0x2006;empty->draw_x=281;empty->draw_y=287;
 empty->draw_scale_x=1.19999981f;empty->draw_scale_y=1;
 C(empty->b8b==5);
 C(!gh_id_text_prepare(&object,empty,&menu,&packed,&font,&context,glyphs,512,&result));
 C(result.glyph_count==8&&result.line_count==1&&result.icon_count==0);
 const uint32_t empty_color=little_word(context.palette.bytes+7*20+4*4);
 C(glyphs[0].code==0x4d&&glyphs[7].code==0x40);
 for(size_t i=0;i<result.glyph_count;i++)C(glyphs[i].quad.rgba==empty_color);
 C(fabsf(glyphs[0].x-219.680008f)<.001f&&glyphs[0].y==277);
 C(fabsf(result.width-122.639977f)<.001f&&result.height==44);
 gh_id_entry *difficulty;
 C(!gh_id_base_get_work_002aa410(runtime,&object,0x30,&difficulty,NULL)&&difficulty->source_index==35);
 difficulty->draw_scale_x=difficulty->draw_scale_y=1;
 const unsigned indices[]={2,3,1};
 const size_t counts[]={4,6,4};
 unsigned group=difficulty->b8b%10;
 group=group>=1&&group<=7?group+2:2;
 for(unsigned i=0;i<3;i++){
  difficulty->message=(uint16_t)(0x2000+i);
  C(!gh_id_text_prepare(&object,difficulty,&menu,&packed,&font,&context,glyphs,512,&result));
  C(result.glyph_count==counts[i]);
  const uint32_t expected=little_word(context.palette.bytes+group*20+indices[i]*4);
  for(size_t j=0;j<result.glyph_count;j++)C(glyphs[j].quad.rgba==expected);
 }
 /* After B301, rendering resumes with the style's default palette index. */
 unsigned char *const body=(unsigned char *)packed.cursor.bytes+little_word(packed.cursor.bytes+4+6*8);
 C(body[20]==1&&body[21]==0xb3&&body[22]==1&&body[23]==0x80);
 C(body[18]==0x40&&body[19]==0);
 body[18]=1;body[19]=0xb3;body[20]=0x40;body[21]=0;
 C(!gh_id_text_prepare(&object,empty,&menu,&packed,&font,&context,glyphs,512,&result));
 C(result.glyph_count==8&&glyphs[6].quad.rgba==empty_color);
 C(glyphs[7].quad.rgba==little_word(context.palette.bytes+7*20));
 body[18]=0x40;body[19]=0;body[20]=1;body[21]=0xb3;
 body[2]=5;
 C(gh_id_text_prepare(&object,empty,&menu,&packed,&font,&context,glyphs,512,&result)==GH_ID_UNSUPPORTED);
 C(result.frontier_address==0x2b04c0&&result.opcode==0xb602&&!result.glyph_count);
 body[2]=4;
 puts("PASS actual R020 empty and difficulty rows: original B602/B301 palette commands");
 /* Real title DispInit(1) prompts. Source24 is original ID9. */
 gh_id_entry *prompt;
 C(!gh_id_base_get_work_002aa410(runtime,&object,9,&prompt,NULL)&&prompt->source_index==24);
 prompt->message=0x20a5;prompt->draw_x=370;prompt->draw_y=47;
 prompt->draw_scale_x=0.699999988f;prompt->draw_scale_y=0.699999928f;
 C(!gh_id_text_prepare(&object,prompt,&menu,&packed,&font,&context,glyphs,512,&result));
 C(result.glyph_count==16&&result.icon_count==1&&result.line_count==1&&glyphs[5].code==1);
 gh_id_texture_quad button;
 C(!gh_id_text_icon_quad(manager,&glyphs[5],&button));
 C(button.color==0x80808080&&button.color_mode==6&&button.font_mode==5);
 C(button.u0==.25f&&button.u1==.5f&&button.v0==0&&button.v1==.25f);
 C(fabsf(button.x0-395.9f)<.001f&&fabsf(button.y0-58.4f)<.001f);
 C(fabsf(button.x1-button.x0-18.2f)<.001f&&fabsf(button.y1-button.y0-18.2f)<.001f);
 C(button.texture.width==128&&button.texture.height==128&&button.texture.tex0==GH_ID_PAD_ICON_TEX0);
 C(button.texture.source.offset==0x2ff00);
 uint8_t rgba[128*128*4];
 C(!gh_id_texture_rgba(&button.texture,rgba,sizeof(rgba)));
 uint32_t hash=2166136261u;
 for(size_t i=0;i<sizeof(rgba);++i)hash=(hash^rgba[i])*16777619u;
 C(hash==0x3e7d1530u);
 C(!memcmp(rgba+(15*128+47)*4,"\x75\x75\x75\xff",4));
 C(!gh_id_base_get_work_002aa410(runtime,&object,2,&prompt,NULL)&&prompt->source_index==20);
 prompt->message=0x20a4;prompt->draw_x=40;prompt->draw_y=47;
 prompt->draw_scale_x=prompt->draw_scale_y=.7f;
 C(!gh_id_text_prepare(&object,prompt,&menu,&packed,&font,&context,glyphs,512,&result));
 C(result.glyph_count==14&&result.icon_count==2&&glyphs[0].code==3&&glyphs[2].code==0);
#ifdef GH_ID_TEXT_QUEUE_TEST
 gh_mc_renderer *renderer;
 C(!gh_mc_renderer_create(elf,elfsize,1,&renderer));
 gh_mc_renderer_begin(renderer);
 const gh_id_render_view view={&font,&menu,manager,&object,&packed};
 const gh_id_draw_job job={.entry_index=20,.entry=prompt,.consumer_address=0x2ab0b8};
 gh_ui_ot_tag tags[3];
 size_t groups=99;
 C(gh_mc_renderer_queue_view_groups(renderer,&view,&job,tags,2,&groups)==GH_ID_CAPACITY&&groups==99);
 C(!gh_mc_renderer_queue_view_groups(renderer,&view,&job,tags,3,&groups)&&groups==3);
 C(tags[0].bank==6&&tags[0].depth==2&&tags[0].index==0);
 C(tags[1].bank==6&&tags[1].depth==5&&tags[1].index==1);
 C(tags[2].bank==6&&tags[2].depth==5&&tags[2].index==2);
 gh_mc_renderer_destroy(renderer);
 puts("PASS original title mixed text/icon OT groups and atomic capacity");
#endif
 puts("PASS real ISO 20A4/20A5 button icons, exact atlas, UV and subpixel geometry");
 C(gh_id_base_release_002aa078(runtime,&object,&original,&reclaimed)==GH_ID_OK);C(gh_id_runtime_destroy(runtime)==GH_ID_OK);free(elf);free(save);free(language);free(manager);gh_disc_close(d);return 0;
}

