#include "../../src/core/id_animation.h"
#include "../../src/platform/disc.h"
#include <fenv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
static void w16(unsigned char*p,unsigned n){p[0]=(unsigned char)n;p[1]=(unsigned char)(n>>8);}
static void w32(unsigned char*p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(n>>(i*8));}
static void wf(unsigned char*p,float f){memcpy(p,&f,4);}
int main(int argc,char**argv){gh_id_animation_profile zero={1,0},four={1,4};gh_id_frontier frontier;C(argc==2);
 {unsigned char source[0x2e0]={0};gh_id_entry e={0};gh_id_base b={0};e.source=source;b.entries=&e;b.entry_count=1;b.source_records=(gh_id_view){source,sizeof(source),0};
  w32(source+4,0x800000);w16(source+0x1b0,2);e.parts[2]=source+0x1b4;wf(source+0x1b4,0);wf(source+0x1c0,1);w16(source+0x1bc,0);w16(source+0x1c8,10);e.jitter[2]=5;
  C(gh_id_jobs_calc_entry(&b,&e,&frontier)==GH_ID_UNSUPPORTED&&frontier.address==0x2ac1c0);
  C(gh_id_jobs_calc_entry_reference(&b,&e,&zero,&frontier)==GH_ID_OK&&e.scale_x==0.5f&&e.scale_y==0.5f&&e.jitter[2]==6);
  e.jitter[2]=11;C(gh_id_jobs_calc_entry_reference(&b,&e,&zero,&frontier)==GH_ID_OK&&(e.flags&0x1000)&&e.scale_x==0.5f);
  e.flags=0;w32(source+4,0x840000);C(gh_id_jobs_calc_entry_reference(&b,&e,&zero,&frontier)==GH_ID_OK&&e.scale_x==0&&e.jitter[2]==1);
  e.flags=0x20;e.jitter[2]=5;C(gh_id_jobs_calc_entry_reference(&b,&e,&zero,&frontier)==GH_ID_OK&&e.jitter[2]==5);
  w32(source+4,0x400000);e.flags=0;e.parts[3]=source+0x218;w16(source+0x214,2);wf(source+0x218,0);wf(source+0x224,1);w16(source+0x220,0);w16(source+0x22c,10);e.jitter[2]=5;e.jitter[3]=12;
  C(gh_id_jobs_calc_entry_reference(&b,&e,&zero,&frontier)==GH_ID_OK&&e.rotation_speed==180&&e.jitter[2]==5&&e.jitter[3]==13);
  w32(source+4,0x2000000);source[0x69]=1;source[0x6a]=source[0x6b]=2;e.jitter[0]=1;e.flags=0;b.b1d=1;
  C(gh_id_jobs_calc_entry_reference(&b,&e,&zero,&frontier)==GH_ID_OK&&e.f70[0]==0.5f&&e.f70[1]==0&&e.f70[2]==1&&e.f70[3]==0.5f&&e.jitter[0]==2&&!b.b1d);
  source[0x69]=0;C(gh_id_jobs_calc_entry_reference(&b,&e,&zero,&frontier)==GH_ID_UNSUPPORTED);
  memset(source,0,sizeof(source));w32(source+4,0x1000000);w16(source+0x6c,2);w16(source+0x6e,4);wf(source+8,10);wf(source+12,20);wf(source+0x70,0);wf(source+0x7c,1);w16(source+0x78,0);w16(source+0x84,10);
  for(unsigned i=0;i<4;i++){wf(source+0x130+i*8,240+(float)i*16);wf(source+0x134+i*8,224);}e.flags=0;e.parts[0]=source+0x130;e.parts[1]=source+0x70;e.jitter[1]=5;e.path_length=16;
  C(gh_id_jobs_calc_entry_reference(&b,&e,&zero,&frontier)==GH_ID_OK&&e.pos_x==18&&e.pos_y==20); /* Catmullstraightline midpoint */
  e.jitter[1]=5;source[0x67]=1;C(gh_id_jobs_calc_entry_reference(&b,&e,&four,&frontier)==GH_ID_OK&&e.pos_x==14&&e.pos_y==20);
  e.jitter[1]=10;C(gh_id_jobs_calc_entry_reference(&b,&e,&zero,&frontier)==GH_ID_UNSUPPORTED&&frontier.address==0x2ac878); /* unwrittenfinal cumulative slot */
 }
 {gh_disc*d;size_t index;gh_disc_entry asset;gh_bundle bundle;gh_bundle_entry idd;unsigned char*bytes;gh_id_state*m=calloc(1,sizeof(*m));gh_id_runtime*r;
  C(m&&gh_disc_open(argv[1],&d)==GH_DISC_OK&&gh_disc_find(d,"etc/core_usa.bin",&index)==GH_DISC_OK&&gh_disc_entry_at(d,index,&asset)==GH_DISC_OK);bytes=malloc((size_t)asset.size);C(bytes&&gh_disc_read(d,index,0,bytes,(size_t)asset.size)==GH_DISC_OK);gh_disc_close(d);
  C(gh_bundle_open(bytes,(size_t)asset.size,&bundle)==GH_BUNDLE_OK&&gh_id_find_language_data(&bundle,&idd)==GH_MENU_OK&&gh_id_system_init_002acc70(m,&idd)==GH_MENU_OK);C(gh_id_runtime_create(m,1,&r)==GH_ID_OK&&gh_id_runtime_set_path_policy(r,GH_ID_PATH_REFERENCE_NEAREST)==GH_ID_OK);
  for(unsigned id=14;id<=21;id+=7){gh_id_base b={0};gh_id_entry*e;int ret,reclaimed;fenv_t saved;float x,y;
   C(gh_id_base_initialize_002a9ea0(r,&b,1,(int)id,NULL,NULL,&ret)==GH_ID_OK&&ret==1);e=b.entries+2;x=e->pos_x;y=e->pos_y;C(e->path_length==16&&e->source[0x67]==1&&e->source[0x6e]==4);
   C(gh_id_jobs_calc_entry_reference(&b,e,NULL,&frontier)==GH_ID_UNSUPPORTED&&frontier.address==0x2ac878&&e->pos_x==x&&e->pos_y==y);
   C(fegetenv(&saved)==0&&fesetround(FE_DOWNWARD)==0&&feraiseexcept(FE_DIVBYZERO)==0);int flags=fetestexcept(FE_ALL_EXCEPT);
   C(gh_id_jobs_calc_entry_reference(&b,e,&zero,&frontier)==GH_ID_OK&&e->pos_x==x&&e->pos_y==y);C(fegetround()==FE_DOWNWARD&&fetestexcept(FE_ALL_EXCEPT)==flags);C(fesetenv(&saved)==0);
   C(gh_id_jobs_calc_entry_reference(&b,e,&four,&frontier)==GH_ID_OK&&e->pos_x==x&&e->pos_y==y+4); /* actual first segment vertical16, seed4 => t=-1/4 */
   e->pos_y=y;b.playing=1;C(gh_id_jobs_move_reference(&b,NULL,NULL,&zero,&frontier)==GH_ID_OK);
   printf("realID%u entry2 mode1/3keys/4points path16: no-seed accurate2AC878, seed0 keepsorigin, explicitseed4 shiftsY+4; Move PASS\n",id);
   C(gh_id_base_release_002aa078(r,&b,&ret,&reclaimed)==GH_ID_OK&&reclaimed);
  }C(gh_id_runtime_destroy(r)==GH_ID_OK);free(bytes);free(m);
 }
 puts("PASS explicit animation reference: real ID14/21 path, nonzero scratch seed, scale Hermite, original angle-counter quirk, UV, hold/done/loop and FEnv");return 0;
}
