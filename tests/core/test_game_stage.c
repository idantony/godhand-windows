#include "game_stage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t u32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint16_t u16(const uint8_t *p){return (uint16_t)((unsigned)p[0]|(unsigned)p[1]<<8);}
static uint8_t *payload(gh_disc *d,const char *path,size_t *size){size_t i;gh_disc_entry e;uint8_t *b;
 CHECK(gh_disc_find(d,path,&i)==GH_DISC_OK && gh_disc_entry_at(d,i,&e)==GH_DISC_OK);*size=(size_t)e.size;
 b=malloc(*size);CHECK(b && gh_disc_read(d,i,0,b,*size)==GH_DISC_OK);return b;}
static int test_children(void *ctx,gh_stage_case1 *s){(void)ctx;(void)s;return 0;}
int main(int argc,char **argv){
 gh_disc *disc;gh_disc_entry entry;size_t index;uint8_t *core,*bad,*rgba;
 gh_stage_textures *s;gh_stage_texture_cursor cursor;uint32_t pixel,channel;
 CHECK(argc==2 && gh_disc_open(argv[1],&disc)==GH_DISC_OK);
 CHECK(gh_disc_find(disc,"etc/core.dat",&index)==GH_DISC_OK && gh_disc_entry_at(disc,index,&entry)==GH_DISC_OK);
 core=malloc((size_t)entry.size);CHECK(core);
 CHECK(gh_disc_read(disc,index,0,core,(size_t)entry.size)==GH_DISC_OK);
 CHECK(gh_stage_textures_create(&s)==GH_STAGE_OK);
 CHECK(gh_stage_core_textures(s,core,(size_t)entry.size)==GH_STAGE_OK);
 CHECK(gh_stage_texture_cursor_get(s,&cursor)==GH_STAGE_OK);
 CHECK(cursor.count==2 && cursor.next_channel==2 && cursor.image_base==0x2660 && cursor.clut_index==2);
 for(unsigned i=0;i<2;i++){
  uint32_t off=u32(core+4+i*4),end=u32(core+8+i*4);gh_stage_channel c;
  gh_font_slot slot={0};gh_font_transfer t;
  CHECK(gh_stage_texture_channel(s,i,&c)==GH_STAGE_OK);
  CHECK(c.base==(i?0x2620:0x2600) && c.clut_base==(i?0x1c04:0x1c00));
  CHECK(c.psm==(i?19:20) && c.width==(i?256:64) && c.height==64);
  slot.bound=1;slot.mst=core+off;slot.mst_size=end-off;
  for(unsigned table=0;table<2;table++){slot.table_offset[table]=u32(slot.mst+8+table*4);slot.table_count[table]=u16(slot.mst+16+table*2);
   CHECK(gh_font_transfer_at(&slot,table,0,&t)==GH_FONT_OK);
   for(unsigned y=0;y<t.height;y++)for(unsigned x=0;x<t.width;x++){
    CHECK(gh_stage_vram_ct32(s,table?c.clut_base:c.base,t.buffer_width,x,y,&pixel)==GH_STAGE_OK);
    CHECK(pixel==u32(t.pixels+((size_t)y*t.width+x)*4));
   }
  }
  rgba=malloc((size_t)c.width*c.height*4);CHECK(rgba);
  CHECK(gh_stage_texture_rgba(s,i,rgba,(size_t)c.width*c.height*4)==GH_STAGE_OK);
  {uint32_t hash=2166136261u;for(size_t j=0;j<(size_t)c.width*c.height*4;j++)hash=(hash^rgba[j])*16777619u;
   printf("original core texture%u %ux%u FNV1a=%08x\n",i,c.width,c.height,hash);
   CHECK(hash==(i?0xc6871dc5u:0xdfadc5e3u));}
  free(rgba);
 }
 /* Source identity dedup matches150D78 and never increments actual cursor. */
 CHECK(gh_stage_core_textures(s,core,(size_t)entry.size)==GH_STAGE_OK);
 CHECK(gh_stage_texture_cursor_get(s,&cursor)==GH_STAGE_OK && cursor.count==2);
 {uint32_t off=u32(core+4),end=u32(core+8);bad=malloc(end-off);CHECK(bad);memcpy(bad,core+off,end-off);
  bad[0]=2;CHECK(gh_stage_texture_load(s,bad,end-off,&channel)==GH_STAGE_UNSUPPORTED);
  memcpy(bad,core+off,end-off);bad[8]=0xff;bad[9]=0xff;bad[10]=0xff;bad[11]=0xff;
  CHECK(gh_stage_texture_load(s,bad,end-off,&channel)==GH_STAGE_BOUNDS);
  CHECK(gh_stage_texture_cursor_get(s,&cursor)==GH_STAGE_OK && cursor.count==2);
  free(bad);}
 CHECK(gh_stage_texture_channel(s,256,&(gh_stage_channel){0})==GH_STAGE_BOUNDS);
 {
  size_t bin_size,language_size;uint8_t *bin=payload(disc,"etc/core.bin",&bin_size);
  uint8_t *language=payload(disc,"etc/core_usa.bin",&language_size);
  uint8_t save_bytes[GH_CORE_SAVE_BYTES];gh_core_save_view save={save_bytes,sizeof(save_bytes)};
  gh_boot_work work={0};gh_stage_skills skills={0};uint32_t original;const uint8_t *record;
  CHECK(gh_core_save_system_init(save)==GH_INIT_OK);work.language=1;
  CHECK(gh_stage_skills_initialize(&skills,bin,bin_size,&work,save)==GH_STAGE_OK);
  CHECK(skills.count==114 && skills.available==114);
  {int previous=-32769;uint8_t seen[114]={0};uint32_t source=u32(bin+24);
   for(unsigned i=0;i<114;i++){
    CHECK(gh_stage_skill_at(&skills,i,&original,&record)==GH_STAGE_OK && original<114 && !seen[original]);seen[original]=1;
    unsigned raw=u16(record+20);int priority=raw<32768?(int)raw:(int)raw-65536;
    CHECK(priority>=previous);previous=priority;
    CHECK(u32(record+12)==(u32(bin+source+12+(size_t)original*36+12)|7));
    CHECK(save_bytes[0xb0+original]==0);
   }
  }
  gh_stage_skills_clear(&skills);
  CHECK(gh_core_save_system_init(save)==GH_INIT_OK);((uint8_t *)&work)[0x596]=0x80; /*cheat800000*/
  CHECK(gh_stage_skills_initialize(&skills,bin,bin_size,&work,save)==GH_STAGE_OK && skills.available==114);
  CHECK(save_bytes[0xb0]==255);gh_stage_skills_clear(&skills);((uint8_t *)&work)[0x596]=0;
  /* Actual JPN and all-moves selection use different originalSLD blocks. */
  work.language=0;CHECK(gh_stage_skills_initialize(&skills,bin,bin_size,&work,save)==GH_STAGE_OK);
  CHECK(!memcmp(skills.image,bin+u32(bin+20),12));
  for(unsigned i=0;i<114;i++){const uint8_t *a=skills.image+12+i*36,*b=bin+u32(bin+20)+12+i*36;
   CHECK(!memcmp(a,b,12) && !memcmp(a+16,b+16,20));}gh_stage_skills_clear(&skills);
  ((uint8_t *)&work)[0x595]=8; /*cheat800*/
  CHECK(gh_stage_skills_initialize(&skills,bin,bin_size,&work,save)==GH_STAGE_OK);
  CHECK(!memcmp(skills.image,bin+u32(bin+24),12));
  for(unsigned i=0;i<114;i++){const uint8_t *a=skills.image+12+i*36,*b=bin+u32(bin+24)+12+i*36;
   CHECK(!memcmp(a,b,12) && !memcmp(a+16,b+16,20));}gh_stage_skills_clear(&skills);
  ((uint8_t *)&work)[0x595]=0;work.language=1;
  /* Actualcase1 reaches a genuinevirtualobjectinitfrontier afteractualtextures,
   * ID,font,menu,workarea andvibration data. It must not markstate2 there. */
  {gh_stage_case1 *stage=calloc(1,sizeof(*stage));gh_game_assets assets={0};gh_bundle bundle;
   gh_font_state font={0};gh_menu_state menu={0};gh_id_state *id=calloc(1,sizeof(*id)),*global=0;
   uint8_t state=1,slot_a[0xb8],slot_b[0x5c];int heap_handle;
   CHECK(stage && id && gh_bundle_open(language,language_size,&bundle)==GH_BUNDLE_OK);
   work.flags=0x2000;assets.work=&work;assets.core.pointer=core;assets.core_size=(size_t)entry.size;
   stage->assets=&assets;stage->core_bin=bin;stage->core_bin_bytes=bin_size;stage->language=&bundle;
   stage->textures=s;stage->font=&font;stage->menu=&menu;stage->id=id;stage->id_global=&global;
   stage->game_state=&state;stage->save=save;
   memset(stage->work_area,0xa5,sizeof(stage->work_area));
   CHECK(gh_game_stage_002a5bb8(stage)==GH_STAGE_UNAVAILABLE && stage->failed_entry==0x15fd98 && state==1);
   CHECK(stage->work_area[0]==0 && stage->work_area[0xb0ef]==0 && stage->vibration_flags==1);
   CHECK(stage->vibration_data==bin+u32(bin+4) && global==id && menu.tables_bound && font.slots[0].bound);
   /* Following callbacks are ONLYtesttraces, not completed originalmanagers.
    * Concrete later reset/SLD work is tested without shipping emptycallbacks. */
   stage->init_children_15fd98=test_children;stage->init_cockpit_140c88=test_children;
   memset(slot_a,0xa5,sizeof(slot_a));memset(slot_b,0xa5,sizeof(slot_b));
   stage->global_slots=(gh_stage_slots){slot_a,sizeof(slot_a),2,0,1};
   stage->room_slots=(gh_stage_slots){slot_b,sizeof(slot_b),1,0,1};stage->room_heap=&heap_handle;
   memset(stage->aux_state,0xa5,sizeof(stage->aux_state));
   CHECK(gh_game_stage_002a5bb8(stage)==GH_STAGE_OK && state==2);
   CHECK(stage->saved_block==save_bytes+0x180 && stage->room_slots.heap_handle==&heap_handle);
   CHECK(u32(slot_a+12)==0xffffffff && u32(slot_a+0x58)==65535 && u32(slot_a+0x5c+0x58)==65535);
   CHECK(stage->aux_state[8]==0xa5 && stage->aux_state[11]==0xa5 && stage->aux_state[13]==0);
   gh_stage_skills_clear(&stage->skills);free(stage);free(id);
  }
  free(language);free(bin);
 }
 gh_stage_textures_destroy(s);free(core);gh_disc_close(disc);
 puts("PASS actual core texture commits/pixels,114 SLD skills,case1 concrete resets and explicit virtual-init frontier");return 0;
}
