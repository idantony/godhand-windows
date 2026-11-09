#include "../../src/core/id_jobs.h"
#include "../../src/platform/disc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(int argc,char **argv){gh_disc *disc;gh_disc_entry resource;size_t index,count;gh_id_state *manager=calloc(1,sizeof(*manager));gh_id_runtime *runtime;gh_id_base object={0};gh_bundle_entry idd={0};unsigned char *data,*language;gh_id_frontier frontier;gh_id_draw_job jobs[67];int original,reclaimed;
 C(argc==2&&manager);C(gh_disc_open(argv[1],&disc)==GH_DISC_OK);C(gh_disc_find(disc,"id/local/US/idsavecore_US.idd",&index)==GH_DISC_OK);C(gh_disc_entry_at(disc,index,&resource)==GH_DISC_OK);data=malloc((size_t)resource.size);C(data);C(gh_disc_read(disc,index,0,data,(size_t)resource.size)==GH_DISC_OK);idd.bytes=data;idd.size=(size_t)resource.size;
 {gh_bundle pack;gh_bundle_entry initial;
 C(gh_disc_find(disc,"etc/core_usa.bin",&index)==GH_DISC_OK);C(gh_disc_entry_at(disc,index,&resource)==GH_DISC_OK);language=malloc((size_t)resource.size);C(language);C(gh_disc_read(disc,index,0,language,(size_t)resource.size)==GH_DISC_OK);C(gh_bundle_open(language,(size_t)resource.size,&pack)==GH_BUNDLE_OK);C(gh_id_find_language_data(&pack,&initial)==GH_MENU_OK);C(gh_id_system_init_002acc70(manager,&initial)==GH_MENU_OK);
 /* Keep global language table views alive until manager no longer needed. */
 }
 C(gh_id_set_data_002accf8(manager,11,&idd)==GH_MENU_OK);C(gh_id_runtime_create(manager,1,&runtime)==GH_ID_OK);C(gh_id_base_initialize_002a9ea0(runtime,&object,11,0,NULL,NULL,&original)==GH_ID_OK&&original==1&&object.entry_count==67);
 object.playing=1;C(gh_id_jobs_move(&object,NULL,NULL,&frontier)==GH_ID_OK&&object.frame==1&&frontier.address==0);
 C(gh_id_jobs_collect_visible(&object,jobs,67,&count,&frontier)==GH_ID_OK);C(count>0);
 {gh_id_texture_quad quad;C(gh_id_texture_quad_prepare(manager,&object,&object.entries[63],&quad)==GH_ID_OK);
 C(quad.x0==0&&quad.y0==0&&quad.x1==512&&quad.y1==448&&quad.texture.width==512&&quad.texture.height==512&&quad.clip==0);
 C(quad.u0==0&&quad.v0==0&&quad.u1==1&&quad.v1>0.869f&&quad.v1<0.871f);}
 for(size_t i=1;i<count;i++)C(jobs[i-1].entry->layer>=jobs[i].entry->layer);
 C(gh_id_jobs_collect_visible(&object,NULL,0,&count,&frontier)==GH_ID_CAPACITY);
 object.frame=9999;C(gh_id_jobs_move(&object,NULL,NULL,&frontier)==GH_ID_OK&&object.frame==0);
 {gh_id_entry *color=&object.entries[21];C(color->source_index==21);color->jitter[4]=30;
 C(gh_id_jobs_move(&object,NULL,NULL,&frontier)==GH_ID_OK);C(color->color==0x80444444U&&color->jitter[4]==31);
 color->jitter[4]=60;C(gh_id_jobs_move(&object,NULL,NULL,&frontier)==GH_ID_OK);C(color->color==0x80808080U);
 color->jitter[4]=61;C(gh_id_jobs_move(&object,NULL,NULL,&frontier)==GH_ID_OK);C(color->color==0x80808080U&&color->jitter[4]==1);}
 object.stop=1;object.frame=75;C(gh_id_jobs_move(&object,NULL,NULL,&frontier)==GH_ID_OK&&object.frame==75);object.stop=0;
 object.hide=1;C(gh_id_jobs_collect_visible(&object,jobs,67,&count,&frontier)==GH_ID_OK&&count==0);object.hide=0;
 {gh_id_entry entries[2]={{0}};gh_id_base fixture={0};fixture.entries=entries;fixture.entry_count=2;
 entries[0].parent_ref=-1;entries[0].scale_x=entries[0].scale_y=2;entries[0].pos_x=10;entries[0].pos_y=20;entries[0].off_x=3;entries[0].off_y=4;entries[0].color_base=0x40404040;
 entries[1].parent_ref=0;entries[1].parent=entries;entries[1].flags=0x44000000;entries[1].scale_x=entries[1].scale_y=1;entries[1].pos_x=5;entries[1].pos_y=6;entries[1].off_x=entries[1].off_y=1;entries[1].color_base=0x80808080;
 C(gh_id_jobs_move(&fixture,NULL,NULL,&frontier)==GH_ID_OK);C(entries[1].draw_x==24&&entries[1].draw_y==37&&entries[1].draw_scale_x==2&&entries[1].color==0x40404040);
 entries[0].rotation_speed=1;C(gh_id_jobs_move(&fixture,NULL,NULL,&frontier)==GH_ID_UNSUPPORTED&&frontier.address==0x2aa6e0);}
 C(gh_id_base_release_002aa078(runtime,&object,&original,&reclaimed)==GH_ID_OK);C(gh_id_runtime_destroy(runtime)==GH_ID_OK);free(data);free(language);free(manager);gh_disc_close(disc);
 puts("PASS actual67 ID move/layout/frame and Trans visibility jobs; rendering consumers remain explicit");return 0;}
