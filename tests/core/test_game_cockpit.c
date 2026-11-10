#include "game_cockpit.h"
#include "../platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static const uint8_t *elf_at(const uint8_t *b,size_t size,uint32_t a,size_t n){
 uint32_t ph=(uint32_t)b[28]|(uint32_t)b[29]<<8|(uint32_t)b[30]<<16|(uint32_t)b[31]<<24;
 unsigned count=b[44]|(unsigned)b[45]<<8;
 for(unsigned i=0;i<count;i++){
  const uint8_t *p=b+ph+i*32;uint32_t v=0,len=0,off=0;
  CHECK(ph+(size_t)i*32+32<=size);for(unsigned j=0;j<4;j++){v|=(uint32_t)p[8+j]<<(8*j);len|=(uint32_t)p[16+j]<<(8*j);off|=(uint32_t)p[4+j]<<(8*j);}
  if(p[0]==1 && a>=v && a-v<=len && n<=len-(a-v)){CHECK((uint64_t)off+a-v+n<=size);return b+off+a-v;}
 }CHECK(0);return NULL;
}
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
int main(int argc,char **argv){
 gh_disc*d;size_t i,n;gh_disc_entry e;void*b;gh_bundle bundle;gh_bundle_entry idd;
 gh_id_state*m=calloc(1,sizeof(*m));gh_id_runtime*r;gh_game_mt mt={.index=625};
 gh_game_cockpit *c;gh_id_base *base;gh_cockpit_widget w;uint8_t save[GH_CORE_SAVE_BYTES];
 gh_core_save_view sv={save,sizeof(save)};gh_id_entry *entry;const uint8_t *fields;
 CHECK(argc==3 && m);
 FILE *ef=fopen(argv[2],"rb");CHECK(ef && !fseek(ef,0,SEEK_END));long es=ftell(ef);CHECK(es>0);rewind(ef);
 uint8_t *elf=malloc((size_t)es);CHECK(elf && fread(elf,1,(size_t)es,ef)==(size_t)es);fclose(ef);
 CHECK(u32(elf_at(elf,(size_t)es,0x3bd930,4))==0x566e50 && u32(elf_at(elf,(size_t)es,0x3bd6e8,4))==0x464080);
CHECK(gh_disc_open(argv[1],&d)==GH_DISC_OK);
 CHECK(gh_disc_find(d,"etc/core_usa.bin",&i)==GH_DISC_OK);CHECK(gh_disc_entry_at(d,i,&e)==GH_DISC_OK);
 b=malloc((size_t)e.size);CHECK(b);CHECK(gh_disc_read(d,i,0,b,(size_t)e.size)==GH_DISC_OK);
 CHECK(gh_bundle_open(b,(size_t)e.size,&bundle)==GH_BUNDLE_OK);
 CHECK(gh_id_find_language_data(&bundle,&idd)==GH_MENU_OK);CHECK(gh_id_system_init_002acc70(m,&idd)==GH_MENU_OK);
 CHECK(gh_id_runtime_create(m,1,&r)==GH_ID_OK);
 CHECK(gh_id_runtime_set_path_policy(r,GH_ID_PATH_REFERENCE_NEAREST)==GH_ID_OK);CHECK(gh_core_save_system_init(sv)==GH_INIT_OK);
 /* A genuine exhausted native ID pool must fail after real earlier runs;
  * cleanup releases only those owned runs and preserves the prior allocation. */
 gh_id_pool_run prior[5];int reclaimed;
 for(unsigned j=0;j<5;j++)CHECK(gh_id_pool_allocate_002ace48(r,122,123456,(int)j,&prior[j])==GH_ID_OK);
 CHECK(gh_game_cockpit_create(r,m,&mt,&c)==GH_ID_OK);
 CHECK(gh_game_cockpit_initialize_00140c88(c,sv)==GH_ID_CAPACITY && !gh_game_cockpit_ready(c));
 CHECK(gh_game_cockpit_destroy(c)==GH_ID_OK && m->pool_used==610 && m->job_count==5);
 for(unsigned j=5;j>0;j--)CHECK(gh_id_pool_release_002acf30(r,prior[j-1],123456,&reclaimed)==GH_ID_OK && reclaimed);
 CHECK(gh_game_cockpit_create(r,m,&mt,&c)==GH_ID_OK);
 CHECK(gh_game_cockpit_base(c,0,&base)==GH_ID_OK && base->vector_a[3]==1 && base->mode==6);
 gh_id_error result=gh_game_cockpit_initialize_00140c88(c,sv);
 fields=gh_game_cockpit_fields(c,&n);CHECK(fields && n==0x23c0);
 CHECK(gh_game_cockpit_widget_at(c,0,&w)==GH_ID_OK && w.offset==0x6e0 && w.method==0x143b70);
 CHECK(gh_game_cockpit_widget_at(c,10,&w)==GH_ID_OK && w.kind==17 && w.method==0x143558);
 CHECK(gh_game_cockpit_base(c,0xd0,&base)==GH_ID_OK && base->initialized && base->id==32);
 CHECK(fields[0x3a0+0xbe]==255 && fields[0x3a0+0xbf]==255);
 if(result!=GH_ID_OK){fprintf(stderr,"cockpit blocked status%d frontier%08x pool%u jobs%u\n",result,gh_game_cockpit_frontier(c),m->pool_used,m->job_count);}
 CHECK(result==GH_ID_OK && gh_game_cockpit_ready(c));
 CHECK(m->pool_used>300 && m->job_count>30);
 printf("Cockpit actual pool=%u runs=%u cached entries/gauges verified\n",m->pool_used,m->job_count);
 for(unsigned j=0;j<11;j++){
  CHECK(gh_game_cockpit_widget_at(c,j,&w)==GH_ID_OK && w.initialized);
  const uint8_t *vt=elf_at(elf,(size_t)es,w.vtable+0x10,8);CHECK(vt[0]==0 && vt[1]==0 && u32(vt+4)==w.method);
  CHECK(u32(fields+w.offset+0x80)==w.vtable);
 }
 CHECK(gh_game_cockpit_base(c,0x460,&base)==GH_ID_OK && base->playing);
 CHECK(gh_game_cockpit_entry_at(c,0x460+0xc8,&entry)==GH_ID_OK && (entry->flags&0x400));
 CHECK(gh_game_cockpit_entry_at(c,0xea0+0x94,&entry)==GH_ID_OK && entry->scale_x==0 && entry->color_base==0x50808080);
 CHECK(gh_game_cockpit_entry_at(c,0xea0+0xe4,&entry)==GH_ID_OK && (entry->flags&8));
 CHECK(u32(fields+0xea0+0x368)==0x80808080 && fields[0xea0+0x4d0]==95 && fields[0xea0+0x41e]==0);
 CHECK(fields[0x460+0x17c]==1 && fields[0x460+0x26d]==1);
 CHECK(gh_game_cockpit_base(c,0x1650,&base)==GH_ID_OK && base->h44==1 && base->id==18);
 CHECK(gh_game_cockpit_base(c,0x1cb0,&base)==GH_ID_OK);
 CHECK(gh_id_base_get_work_002aa410(r,base,0,&entry,NULL)==GH_ID_OK && entry->pos_y==-50);
 CHECK(fields[0x1cb0+0x102]==90 && fields[0x1520+0x9c]==255 && fields[0x23bc]==0);
 CHECK(gh_game_cockpit_initialize_00140c88(c,sv)==GH_ID_BUSY);
 gh_boot_work work_state={0};uint8_t heat[0x18]={0};uint32_t maximum=0x42f00000;memcpy(heat,&maximum,4);
 uint32_t tick=0; /*actualcase3 clearsWork.playerObj beforethisgetter*/memcpy((uint8_t*)&work_state+0x5f4,&tick,4);
 result=gh_game_cockpit_room_reset_00140de0(c,sv,&work_state,heat);
 if(result)fprintf(stderr,"roomreset blocked%d frontier%08X\n",result,gh_game_cockpit_frontier(c));
 CHECK(result==GH_ID_OK&&m->pool_used==405&&m->job_count==39);
 CHECK(u32(fields+0x6e0+0x98)==tick&&u32(fields+0x6e0+0x5d0)==0x41a00000);
 for(unsigned j=0;j<11;j++){CHECK(!gh_game_cockpit_widget_at(c,j,&w));const uint8_t*vt=elf_at(elf,(size_t)es,w.vtable+0x18,8);CHECK(vt[0]==0&&vt[1]==0);CHECK(fields[w.offset+0x75]==1&&fields[w.offset+0x76]==0);}
 CHECK(gh_game_cockpit_base(c,0x460+0x190,&base)==GH_ID_OK&&!base->playing);
 CHECK(fields[0x460+0x17c]==0&&fields[0x15c0+0x33c]==255&&fields[0x15c0+0x343]==255);
 CHECK(gh_game_cockpit_entry_at(c,0x6e0+0x4a0,&entry)==GH_ID_OK&&entry);
 CHECK(gh_game_cockpit_room_reset_00140de0(c,sv,&work_state,heat)==GH_ID_BUSY);
 /* Same global owner survives TitleExit into R100 and later rooms. Original
  * fixed-offset pointer-cache stores overwrite fields; never allocate ID runs. */
 gh_id_entry *cached=entry;gh_id_base *global_base;CHECK(!gh_game_cockpit_base(c,0x6e0,&global_base));gh_id_entry *global_entries=global_base->entries;
 for(unsigned room=0;room<8;room++){
  CHECK(!gh_game_cockpit_room_quit_00141628(c,sv,heat));
  result=gh_game_cockpit_room_reset_00140de0(c,sv,&work_state,heat);
  if(result)fprintf(stderr,"repeat room%u reset%d frontier%08X\n",room,result,gh_game_cockpit_frontier(c));
  CHECK(!result&&m->pool_used==405&&m->job_count==39&&global_base->entries==global_entries);
  CHECK(!gh_game_cockpit_entry_at(c,0x6e0+0x4a0,&entry)&&entry==cached);
 }
 CHECK(gh_game_cockpit_destroy(c)==GH_ID_OK && m->pool_used==0 && m->job_count==0);
 CHECK(gh_id_runtime_destroy(r)==GH_ID_OK);free(m);free(b);free(elf);gh_disc_close(d);
 puts("PASS actual USA Cockpit11 widget init/reset, eight quit-reset room lifecycles preserve global405entries39runs/cacheidentity, exhausted-pool cleanup");return 0;
}
