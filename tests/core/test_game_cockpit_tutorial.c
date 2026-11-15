#include "id_text.h"
#include "id_animation.h"
#include <math.h>
#include "game_cockpit_tutorial.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t*read_file(gh_disc*d,const char*path,size_t*size){size_t i;gh_disc_entry e;C(!gh_disc_find(d,path,&i)&&!gh_disc_entry_at(d,i,&e)&&e.size<=SIZE_MAX);*size=(size_t)e.size;uint8_t*p=malloc(*size);C(p&&!gh_disc_read(d,i,0,p,*size));return p;}
static uint32_t scalar_word(const uint8_t *bytes)
{
 uint32_t result;
 memcpy(&result,bytes,4);
 return result;
}
static void test_scaled_layout(gh_disc *disc,const gh_bundle *language,gh_font_state *font,
 const gh_packed_message_state *packed,gh_id_base *base,const gh_id_state *id,
 const gh_id_entry *entry,unsigned selector)
{
 const size_t elf_size=(size_t)gh_disc_boot_elf_size(disc);
 uint8_t *elf=malloc(elf_size);
 C(elf&&!gh_disc_read_boot_elf(disc,0,elf,elf_size));
 gh_id_text_context context={.language=1,.screen_width=512,.screen_height=448,.gs_y_origin=1824};
 C(!gh_id_text_palette_from_elf(elf,elf_size,&context.palette));
 gh_menu_state menu={0};
 C(!gh_menu_load_tables_002ae990(&menu,language));
 const gh_id_animation_profile reference={1,0};
 gh_id_frontier frontier={0};
 C(!gh_id_jobs_move_reference(base,NULL,NULL,&reference,&frontier));
 const uint8_t *table=packed->cursor.bytes;
 const size_t offset=scalar_word(table+4+selector*8);
 const uint8_t *body=table+offset;
 unsigned lines=1,icon_count=0,run=100;
 size_t token_offset=0,glyph_count=0,icon_indices[4]={0};
 double width=0,total_height=0,line_width=0,line_height=0;
 const double x=entry->draw_scale_x,y=entry->draw_scale_y,spacing=2*x;
 for(;offset+token_offset+2<=packed->cursor.size;token_offset+=2){
  const unsigned code=body[token_offset]|(unsigned)body[token_offset+1]<<8;
  if(code==0x8001)break;
  if(code==0xb002){
   C(offset+token_offset+4<=packed->cursor.size);
   run=body[token_offset+2]|(unsigned)body[token_offset+3]<<8;
   C(run==100||run==150);
   token_offset+=2;
   continue;
  }
  if(code==0xa301)continue;
  if(code==0xa001){
   if(line_width-spacing>width)width=line_width-spacing;
   total_height+=line_height+(-26*y+8)*y;
   line_width=line_height=0;
   ++lines;
   continue;
  }
  ++glyph_count;
  const double factor=run*.01;
  double h;
  if(code<14){
   C(run==150&&icon_count<4);
   icon_indices[icon_count++]=glyph_count-1;
   line_width+=spacing+32*.9*x*factor;
   h=32*y*factor+12*y;
  }else{
   gh_font_glyph metric;
   C(run==100&&!gh_font_glyph_at(font,(uint16_t)code,&metric));
   line_width+=spacing+(double)metric.width*.9*x;
   h=(font->slots[code>>12].cell_height+12)*y;
  }
  if(h>line_height)line_height=h;
 }
 C(offset+token_offset+2<=packed->cursor.size&&run==100&&lines==4);
 if(line_width-spacing>width)width=line_width-spacing;
 total_height+=line_height;
 gh_id_text_result result;
 gh_id_text_glyph glyphs[128];
 C(!gh_id_text_prepare(base,entry,&menu,packed,font,&context,glyphs,128,&result)&&
   result.glyph_count==glyph_count&&result.line_count==lines&&result.icon_count==icon_count&&
   glyph_count==(selector==1?94u:79u)&&icon_count==(selector==1?4u:1u)&&
   fabs(result.width-width)<.003&&fabs(result.height-total_height)<.003);
 for(unsigned i=0;i<icon_count;i++){
  const size_t j=icon_indices[i];
  gh_id_texture_quad packet;
  C(glyphs[j].code<14&&fabs(glyphs[j].icon_extent-26*1.5*y)<.001&&
    !gh_id_text_icon_quad(id,&glyphs[j],&packet)&&packet.texture.tex0==GH_ID_PAD_ICON_TEX0&&
    packet.color_mode==6&&packet.font_mode==5&&fabs(packet.x1-packet.x0-26*1.5*y)<.001);
  if(glyphs[j+1].code<0x8000)
   C(fabs(glyphs[j+1].x-glyphs[j].x-32*1.5*y)<.003);
 }
 free(elf);
 printf("Actual US tutorial%u: %zu glyphs, %u scaled buttons, layout %.4f x %.4f\n",
    selector,glyph_count,icon_count,result.width,result.height);
}
static void test_plaque(const gh_id_state *id,gh_id_base *base,unsigned selector,int moved)
{
 const gh_id_animation_profile reference={1,0};
 gh_id_frontier frontier={0};
 if(!moved)C(!gh_id_jobs_move_reference(base,NULL,NULL,&reference,&frontier));
 gh_id_draw_job jobs[64];
 size_t count=0;
 C(!gh_id_jobs_collect_visible(base,jobs,64,&count,&frontier));
 C(count==(selector==0?16u:15u));
 unsigned frequencies[4]={0},texts=0;
 for(size_t i=0;i<count;i++){
  const gh_id_entry *entry=jobs[i].entry;
  if(entry->kind==2){++texts;continue;}
  gh_id_texture_quad quad;
  C(jobs[i].consumer_address==0x2aaef0&&!gh_id_texture_quad_prepare(id,base,entry,&quad));
  C(quad.color_mode==6&&quad.font_mode==5&&quad.x0<quad.x1&&quad.y0<quad.y1&&
    quad.u0==entry->f70[0]&&quad.v0==entry->f70[1]&&quad.u1==entry->f70[2]&&quad.v1==entry->f70[3]);
  const unsigned item=entry->b8b+1;
  const unsigned profile=entry->b8a==1?3:item-1;
  static const uint64_t tex0[]={GH_ID_TIM2_128X64_PSMT4_TEX0,GH_ID_TIM2_64X64_PSMT4_TEX0,
      GH_ID_TIM2_128X64_PSMT8_TEX0,GH_ID_TIM2_64X64_PSMT8_TEX0};
  C((entry->b8a==1&&item==2)||(entry->b8a==29&&item>=1&&item<=3));
  C(quad.texture.tex0==tex0[profile]&&quad.texture.height==64&&
    quad.texture.width==(profile==0||profile==2?128:64));
  if(!frequencies[profile]){
   uint8_t rgba[128*64*4];
   C(!gh_id_texture_rgba(&quad.texture,rgba,sizeof(rgba)));
   unsigned pixels=0;
   for(size_t k=3;k<(size_t)quad.texture.width*64*4;k+=4){
    C(!(rgba[k]&1)||rgba[k]==255);
    pixels+=rgba[k]!=0;
   }
   C(pixels>0);
  }
  frequencies[profile]++;
  if(profile==1){
   const double left=(double)entry->draw_x+1-entry->f68*entry->draw_scale_x*.5;
   const double right=(double)entry->draw_x+1+entry->f68*entry->draw_scale_x*.5;
   C(entry->b89==0&&entry->f80!=1&&fabs(quad.x0-(256+(left-256)*entry->f80))<.001&&
     fabs(quad.x1-(256+(right-256)*entry->f80))<.001);
  }
 }
 C(texts==4&&frequencies[0]==9&&frequencies[1]==1&&frequencies[2]==(selector==0)&&
   frequencies[3]==1);
 printf("Actual US tutorial%u all %zu visible jobs, native plaque %zu sprites, exact TBL1 RGBA\n",
    selector,count,count-texts);
}
static void test_action(gh_game_cockpit *cockpit,gh_boot_work *work,gh_cockpit_tutorial_view *view,
 gh_id_entry *refs[7],gh_disc *disc,const gh_bundle *language,gh_font_state *font,
 gh_packed_message_state *packed,const gh_id_state *id,unsigned selector)
{
 gh_task_record32 records[16]={0};
 gh_task_manager manager={.records=records,.capacity=16,.count=16,.current_index=-1};
 gh_task_threads *threads;
 C(!gh_task_threads_create(&manager,&threads));
 uint8_t scenario_bytes[0x114]={0},actor_bytes[0x604]={0};
 gh_scene_scenario_view scenario={scenario_bytes,sizeof(scenario_bytes),&manager};
 gh_title_player_view actor={.bytes=actor_bytes,.size=sizeof(actor_bytes)};
 actor_bytes[0x600]=(uint8_t)selector;
 uint8_t *raw=(uint8_t *)work;
 const uint32_t pressed=0x10000000;
 memcpy(raw+0x1a0,&pressed,4);
 gh_cockpit_tutorial_action action={0};
 uint32_t pc;
 C(!gh_cockpit_tutorial_action_001bf7d8(&action,scenario,threads,work,cockpit,&actor,&pc)&&!pc);
 C(actor_bytes[0x2f5]==1&&action.slot==1&&action.scheduled&&records[1].running==2&&
   (records[1].attributes&1)&&(manager.flags&1)&&scalar_word(raw+0x5d8)==0xfed00000&&
   scalar_word(raw+0x5e0)==0x100000&&(scalar_word(raw+0x1a0)&0x30000000)==pressed);
 C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK&&!action.status);
 C(!gh_game_cockpit_get_tutorial(cockpit,view)&&view->current_message==0x2000+selector&&view->state_b8==1);
 memcpy(raw+0x1a0,&pressed,4);
 for(unsigned tick=0;tick<9;tick++){
  C(!gh_game_cockpit_tutorial_move_001471f0(cockpit));
  C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK&&action.scheduled);
 }
 memset(raw+0x1a0,0,4);
 for(unsigned tick=0;tick<20&&view->state_b8!=3;tick++){
  C(!gh_game_cockpit_tutorial_move_001471f0(cockpit));
  C(!gh_game_cockpit_get_tutorial(cockpit,view));
 }
 C(view->state_b8==3);
 C(!gh_game_cockpit_tutorial_move_001471f0(cockpit)&&
   refs[1]->message==0x2000+selector&&refs[2]->message==0x2000+selector&&
   refs[3]->message==0x2006+selector&&refs[4]->message==0x2006+selector&&
   ((selector==1||selector==2||selector==3)||!(refs[selector==0?5:6]->flags&0x08000000)));
 if(selector==1||selector==2)test_scaled_layout(disc,language,font,packed,view->base,id,refs[1],selector);
 if(work->language==1)test_plaque(id,view->base,selector,selector==1||selector==2);
 C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK&&action.scheduled&&
   !gh_game_cockpit_get_tutorial(cockpit,view)&&view->state_b8==3);
 memcpy(raw+0x1a0,&pressed,4);
 C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK&&action.scheduled&&
   !gh_game_cockpit_get_tutorial(cockpit,view)&&view->state_b8==4);
 unsigned closing=0;
 while(action.scheduled&&closing<20){
  C(!gh_game_cockpit_tutorial_move_001471f0(cockpit));
  C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);
  ++closing;
 }
 C(!action.scheduled&&!action.status&&!action.frontier&&!records[1].running&&
   !(manager.flags&1)&&!(records[1].attributes&1)&&!scalar_word(raw+0x5d8)&&
   !scalar_word(raw+0x5dc)&&!scalar_word(raw+0x5e0)&&actor_bytes[0x602]==10&&actor_bytes[0x2f5]==2);
 C(!gh_game_cockpit_get_tutorial(cockpit,view)&&!view->state_b8&&view->current_message==0xffff);
 C(!gh_task_threads_destroy(threads));
 printf("Tutorial selector%u real IDD header/body and 10-tick input gate, closed in %u updates\n",selector,closing);
}
int main(int argc,char**argv){C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t n;uint8_t*language=read_file(d,"etc/core_usa.bin",&n);gh_bundle bundle;gh_bundle_entry idd;C(!gh_bundle_open(language,n,&bundle)&&!gh_id_find_language_data(&bundle,&idd));gh_id_state*id=calloc(1,sizeof(*id));C(id&&!gh_id_system_init_002acc70(id,&idd));gh_id_runtime*runtime;C(!gh_id_runtime_create(id,1,&runtime)&&!gh_id_runtime_set_path_policy(runtime,GH_ID_PATH_REFERENCE_NEAREST));gh_game_mt mt={.index=625};gh_game_cockpit*c;uint8_t savebytes[GH_CORE_SAVE_BYTES]={0};gh_core_save_view save={savebytes,sizeof(savebytes)};C(!gh_core_save_system_init(save)&&!gh_game_cockpit_create(runtime,id,&mt,&c)&&!gh_game_cockpit_initialize_00140c88(c,save));C(id->pool_used==405&&id->job_count==39);gh_heap*heap;C(!gh_heap_create(2*1024*1024,&heap));gh_dvd_jobs dvd;gh_dvd_jobs_init(&dvd,d);gh_font_state fonts={0};C(!gh_font_initialize(&fonts,&bundle,1));gh_packed_message_state packed={0};gh_boot_work work={0};work.language=1;gh_tutorial_bindings b={c,&dvd,heap,id,&fonts,&packed,&bundle,&work};gh_cockpit_tutorial t={0};
 gh_heap*tiny;C(!gh_heap_create(16,&tiny));gh_tutorial_bindings small=b;small.room_heap=tiny;gh_cockpit_tutorial oom={0};C(gh_cockpit_tutorial_room_init_00147028(&oom,&small)==GH_TUTORIAL_MEMORY&&oom.frontier==0x200cb0&&!oom.resource.pointer&&!id->pair[0x1d].bound&&id->pool_used==405);C(!gh_cockpit_tutorial_release(&oom)&&!gh_heap_destroy(tiny));
 for(unsigned round=0;round<2;round++){
  C(!gh_cockpit_tutorial_room_init_00147028(&t,&b));gh_cockpit_tutorial_view v;C(!gh_game_cockpit_get_tutorial(c,&v));C(t.complete&&v.data_present&&v.read_id==t.job&&v.base->initialized&&v.base->resource_number==0x1d&&v.base->id==0&&v.base->playing&&t.packed_return==1&&packed.initialized&&v.current_message==0xffff&&!v.state_b8);C(t.image.size==(round?47104:29712));gh_id_entry*refs[7];for(unsigned i=0;i<7;i++){C(!gh_game_cockpit_entry_at(c,0x1a20+0x90+i*4,&refs[i]));C(refs[i]>=v.base->entries&&refs[i]<v.base->entries+v.base->entry_count);}C(!refs[0]->scale_x&&!refs[0]->scale_y&&(refs[1]->flags&0x8000000)&&(refs[2]->flags&0x8000000)&&refs[3]->message==0x200c&&refs[4]->message==0x200c);
  uint32_t pool=id->pool_used,jobs=id->job_count,job=t.job;void*source=t.resource.pointer;gh_id_entry*entries=v.base->entries;
  gh_cockpit_tutorial foreign={0};C(gh_cockpit_tutorial_room_init_00147028(&foreign,&b)==GH_TUTORIAL_BUSY&&!foreign.resource.pointer);C(!gh_cockpit_tutorial_release(&foreign)&&v.base->entries==entries&&v.base->owns_run&&id->pool_used==pool&&id->pair[0x1d].bound);
  refs[0]->scale_x=4;refs[1]->flags&=~0x8000000u;refs[3]->message=0;v.base->frame=37;C(!gh_cockpit_tutorial_next_call(&t)&&!gh_cockpit_tutorial_room_init_00147028(&t,&b));C(t.completed_calls==2&&t.job==job&&t.resource.pointer==source&&id->pool_used==pool&&id->job_count==jobs&&v.base->entries==entries&&v.base->frame==0&&!refs[0]->scale_x&&(refs[1]->flags&0x8000000)&&refs[3]->message==0x200c);printf("actual tutorial path=%s bytes=%zu entries=%u pool=%u jobs=%u repeat-source-preserved\n",t.path,t.image.size,v.base->entry_count,pool,jobs);
  if(!round){for(unsigned i=0;i<4;i++)test_action(c,&work,&v,refs,d,&bundle,&fonts,&packed,id,i);}
  else test_action(c,&work,&v,refs,d,&bundle,&fonts,&packed,id,4);
  /* Actual scene exit invokes 147188 before original 2AD108. The
   * source allocation survives both, until the room heap cleanup. */
  int compacted=99;
  C(gh_id_pool_compact_002ad108(runtime,&compacted)==GH_ID_BUSY&&!compacted);
  C(id->pool_used==pool&&id->job_count==jobs&&v.base->entries==entries);
  gh_id_job_view tutorial_job;
  C(!gh_id_runtime_job(runtime,jobs-1,&tutorial_job)&&tutorial_job.tag==0x1d&&tutorial_job.start==405);
  uint8_t heat[0x18]={0};uint32_t max=0x42f00000;memcpy(heat,&max,4);
  C(!gh_game_cockpit_room_quit_00141628(c,save,heat));
  C(!gh_game_cockpit_get_tutorial(c,&v)&&!v.data_present&&!v.base->owns_run&&!v.base->entries);
  C(id->pool_used==pool&&id->job_count==jobs&&t.resource.pointer==source&&id->pair[0x1d].bound);
  C(!gh_id_runtime_job(runtime,jobs-1,&tutorial_job)&&tutorial_job.tag==0x1d&&tutorial_job.active);
  gh_id_entry*unused;
  C(gh_game_cockpit_entry_at(c,0x1a20+0x90,&unused)==GH_ID_NOT_FOUND);
  C(!gh_id_pool_compact_002ad108(runtime,&compacted)&&compacted&&id->pool_used==405&&id->job_count==39);
  C(gh_id_runtime_job(runtime,39,&tutorial_job)==GH_ID_BOUNDS&&t.resource.pointer==source);
  C(!gh_cockpit_tutorial_release(&t)&&!gh_heap_used(heap)&&!id->pair[0x1d].bound);
  C(id->pool_used==405&&id->job_count==39);
  work.language=-1;
 }
 C(!gh_game_cockpit_destroy(c)&&!id->pool_used&&!id->job_count&&!gh_id_runtime_destroy(runtime)&&!gh_heap_destroy(heap));free(id);free(language);gh_disc_close(d);puts("PASS actual USA/fallback tutorial IDD through DVD201160, sameCockpit ownedrun/7refs/packed/font state, explicit repeat-call restart, roomQuit preserves counts, original 2AD108 compacts, then safe resource lifecycle");return 0;}
