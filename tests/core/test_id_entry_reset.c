#include "id_entry_reset.h"
#include "disc.h"
#include "id_animation.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t*read_file(gh_disc*d,const char*p,size_t*n){size_t i;gh_disc_entry e;C(gh_disc_find(d,p,&i)==GH_DISC_OK&&gh_disc_entry_at(d,i,&e)==GH_DISC_OK);*n=(size_t)e.size;uint8_t*b=malloc(*n);C(b&&gh_disc_read(d,i,0,b,*n)==GH_DISC_OK);return b;}
int main(int argc,char**argv){
 /* Explicitfourentryfixture distinguishesdirectchildfromgrandchild; source
  * RAND40 mustnotcauseRNGcall. Nofakegame/renderresource is claimed here. */
 uint8_t src[4][0x2e0]={{0}};gh_id_entry es[4]={{0}};gh_id_base o={.entries=es,.entry_count=4,.source_records={&src[0][0],sizeof(src),0},.frame=99,.b1d=7,.b1e=8};gh_id_frontier f;
 for(unsigned i=0;i<4;i++){es[i].source=src[i];src[i][4]=0x40;es[i].flags=0x40;for(unsigned j=0;j<5;j++)es[i].jitter[j]=123;es[i].draw_x=17+i;}
 es[1].parent=es;es[2].parent=es+1;gh_id_entry grand=es[2],other=es[3];
 C(gh_id_entry_reset_family_002ac9b8(&o,es,1,&f)==GH_ID_OK&&!o.frame&&!o.b1d&&!o.b1e);
 C((es[0].flags&0x7c00)==0x7c00&&!(es[1].flags&0x7c00));for(unsigned i=0;i<5;i++)C(!es[0].jitter[i]&&!es[1].jitter[i]);
 C(!memcmp(es+2,&grand,sizeof(grand))&&!memcmp(es+3,&other,sizeof(other))&&es[0].draw_x==17&&es[1].draw_x==18);
 o.frame=91;o.b1d=2;o.b1e=3;C(gh_id_entry_reset_family_002ac9b8(&o,0,0,&f)==GH_ID_OK&&!o.frame&&!o.b1d&&!o.b1e);
 C(gh_id_jobs_calc_entry(&o,&grand,&f)==GH_ID_BOUNDS);
 gh_disc*d;C(argc==2&&gh_disc_open(argv[1],&d)==GH_DISC_OK);size_t n;uint8_t*lang=read_file(d,"etc/core_usa.bin",&n);gh_bundle b;gh_bundle_entry initial;C(gh_bundle_open(lang,n,&b)==GH_BUNDLE_OK&&gh_id_find_language_data(&b,&initial)==GH_MENU_OK);
 gh_id_state*m=calloc(1,sizeof(*m));C(m&&gh_id_system_init_002acc70(m,&initial)==GH_MENU_OK);uint8_t*idd=read_file(d,"id/local/US/idsavecore_US.idd",&n);gh_bundle_entry iddview={.bytes=idd,.size=n};C(gh_id_set_data_002accf8(m,11,&iddview)==GH_MENU_OK);gh_id_runtime*r;C(gh_id_runtime_create(m,1,&r)==GH_ID_OK);gh_id_base real={0};int original,reclaimed;C(gh_id_base_initialize_002a9ea0(r,&real,11,0,0,0,&original)==GH_ID_OK);
 gh_id_entry*color=real.entries+21;C(color->source_index==21);color->jitter[4]=30;real.frame=450;real.playing=0;real.stop=1;color->draw_x=12345;
 C(gh_id_jobs_calc_entry(&real,color,&f)==GH_ID_OK&&color->color==0x80444444u&&color->jitter[4]==31&&real.frame==450&&color->draw_x==12345);
 C(gh_id_entry_reset_family_002ac9b8(&real,color,1,&f)==GH_ID_OK&&color->jitter[4]==1&&(color->flags&0x7c00)==0x7c00&&!real.frame&&color->draw_x==12345);
 /* Mostimportantlynohiddenlayout/frameincrement/iconposting fromwholeMove. */
 C(real.stop==1&&real.playing==0);C(gh_id_base_release_002aa078(r,&real,&original,&reclaimed)==GH_ID_OK);
 C(gh_id_runtime_set_path_policy(r,GH_ID_PATH_REFERENCE_NEAREST)==GH_ID_OK);
 gh_id_base cockpit={0};C(gh_id_base_initialize_002a9ea0(r,&cockpit,1,14,0,0,&original)==GH_ID_OK);
 C(cockpit.entry_count>2);gh_id_animation_profile explicit_seed={1,0};
 gh_id_error default_error=gh_id_entry_reset_family_002ac9b8(&cockpit,cockpit.entries+2,0,&f);
 C(default_error==GH_ID_UNSUPPORTED&&f.address==0x2ac048);
 gh_id_animation_profile unresolved={0,0};
 C(gh_id_entry_reset_family_reference(&cockpit,cockpit.entries+2,0,&unresolved,&f)==GH_ID_UNSUPPORTED&&(f.address==0x2ac6e0||f.address==0x2ac878));
 C(gh_id_entry_reset_family_reference(&cockpit,cockpit.entries+2,0,&explicit_seed,&f)==GH_ID_OK&&!cockpit.frame);
 C(gh_id_base_release_002aa078(r,&cockpit,&original,&reclaimed)==GH_ID_OK&&gh_id_runtime_destroy(r)==GH_ID_OK);free(m);free(lang);free(idd);gh_disc_close(d);
 puts("PASS originalentry/directchildreset, noRAND/layout/frameadvance, actualMCcolorcalcleaf");return 0;}
