#include "menu_state.h"
#include "dvd_jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t word(const unsigned char *p)
{return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
int main(int argc,char **argv)
{
    gh_disc *disc;gh_heap *heap;gh_dvd_jobs dvd;gh_r207_allocation allocation;
    gh_bundle language,nested;gh_bundle_entry idd,msd0,msd1,tbl0,tbl1;
    gh_menu_state menu;gh_id_state *id,*global=0;gh_disc_entry source;
    size_t index,i;uint32_t job;
    static const unsigned char one_table[]={1,0,0,0,12,0,0,0,'T','B','L',0,7,0,0,0};
    gh_bundle_entry broken;
    CHECK(argc==2);
    CHECK(gh_disc_open(argv[1],&disc)==GH_DISC_OK);
    CHECK(gh_heap_create(4*1024*1024,&heap)==GH_MEM_OK);
    gh_dvd_jobs_init(&dvd,disc);
    job=gh_dvd_read_alloc(&dvd,"etc/core_usa.bin",&allocation,heap,0,0,0,0);
    CHECK(job && gh_dvd_check(&dvd,job)==2);
    CHECK(gh_dvd_pump(&dvd)==GH_DVD_OK && !gh_dvd_check(&dvd,job));
    CHECK(gh_disc_find(disc,"etc/core_usa.bin",&index)==GH_DISC_OK);
    CHECK(gh_disc_entry_at(disc,index,&source)==GH_DISC_OK);
    CHECK(gh_bundle_open(allocation.pointer,(size_t)source.size,&language)==GH_BUNDLE_OK);
    CHECK(gh_bundle_find(&language,"MSD",0,&msd0)==GH_BUNDLE_OK);
    CHECK(gh_bundle_find(&language,"MSD",1,&msd1)==GH_BUNDLE_OK);
    memset(&menu,0xa5,sizeof(menu));
    CHECK(gh_menu_load_tables_002ae990(&menu,&language)==GH_MENU_OK);
    CHECK(menu.tables_bound && menu.timer==900 && menu.state==0);
    CHECK(menu.data.bytes==msd0.bytes && menu.texture_data.bytes==msd1.bytes);
    CHECK(menu.data.offset==287984 && menu.texture_data.offset==355984);
    CHECK(word(menu.data.bytes)==10 && word(menu.texture_data.bytes)==258);
    CHECK(menu.data.bytes!=menu.texture_data.bytes);
    CHECK(gh_id_find_language_data(&language,&idd)==GH_MENU_OK);
    CHECK(idd.offset==365264 && gh_bundle_open(idd.bytes,idd.size,&nested)==GH_BUNDLE_OK);
    CHECK(gh_bundle_find(&nested,"TBL",0,&tbl0)==GH_BUNDLE_OK);
    CHECK(gh_bundle_find(&nested,"TBL",1,&tbl1)==GH_BUNDLE_OK);
    id=malloc(sizeof(*id));CHECK(id);memset(id,0xa5,sizeof(*id));
    /* Startup binds global identity before systemInit, not an invented ready flag. */
    CHECK(gh_id_bind_global(&global,id)==GH_MENU_OK && global==id);
    CHECK(gh_id_system_init_002acc70(id,&idd)==GH_MENU_OK);
    CHECK(global->initialized && global->pool_used==0 && global->job_count==0);
    for(i=0;i<sizeof(id->jobs);++i)CHECK(id->jobs[i]==0);
    for(i=0;i<sizeof(id->pool);++i)CHECK(id->pool[i]==0xa5);
    for(i=0;i<32;++i)if(i!=1)CHECK(!id->pair[i].bound && !id->pair[i].data.bytes && !id->pair[i].texture_data.bytes);
    CHECK(id->pair[1].bound && id->pair[1].data.bytes==tbl0.bytes);
    CHECK(id->pair[1].texture_data.bytes==tbl1.bytes);
    CHECK(tbl0.offset==32 && tbl1.offset==219424);
    CHECK(word(id->pair[1].data.bytes)==33);
    /* Required second table absent is an error; earlier clears remain visible,
     * untouched pool bytes survive. No successful empty consumer is invented. */
    broken=idd;broken.bytes=one_table;broken.size=sizeof(one_table);
    id->job_count=17;id->pool_used=5;memset(id->jobs,0x7f,sizeof(id->jobs));
    CHECK(gh_id_system_init_002acc70(id,&broken)==GH_MENU_MISSING);
    CHECK(!id->initialized && !id->pair[1].bound && !id->job_count && !id->pool_used);
    for(i=0;i<sizeof(id->jobs);++i)CHECK(id->jobs[i]==0);
    for(i=0;i<sizeof(id->pool);++i)CHECK(id->pool[i]==0xa5);
    broken.size=3;
    CHECK(gh_id_system_init_002acc70(id,&broken)==GH_MENU_BOUNDS && !id->initialized);
    CHECK(gh_id_find_language_data(&language,0)==GH_MENU_ARGUMENT);
    CHECK(gh_id_bind_global(0,id)==GH_MENU_ARGUMENT);
    CHECK(gh_menu_load_tables_002ae990(0,&language)==GH_MENU_ARGUMENT);
    free(id);CHECK(gh_heap_release(heap,allocation)==GH_MEM_OK);
    CHECK(gh_heap_used(heap)==0 && gh_heap_destroy(heap)==GH_MEM_OK);gh_disc_close(disc);
    puts("PASS real language MSD0/MSD1, IDD/TBL binding, ID reset/preserved pool and global identity");
    return 0;
}
