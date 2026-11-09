#include "../../src/core/packed_message.h"
#include "../../src/platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static unsigned char *load(gh_disc *d,const char *path,size_t *n){size_t i;gh_disc_entry e;unsigned char *b;C(gh_disc_find(d,path,&i)==GH_DISC_OK);C(gh_disc_entry_at(d,i,&e)==GH_DISC_OK);*n=(size_t)e.size;b=malloc(*n);C(b);C(gh_disc_read(d,i,0,b,*n)==GH_DISC_OK);return b;}
static uint32_t u32(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
int main(int argc,char **argv){gh_disc *disc;size_t n;unsigned char *language,*idsave;gh_bundle lang,packed;gh_bundle_entry idd={0},mst,msa,msd;gh_id_view view;gh_id_state *manager=calloc(1,sizeof(*manager));gh_id_base self={0};gh_font_state font={0};gh_packed_message_state state;int original;
 C(argc==2);C(manager);C(gh_disc_open(argv[1],&disc)==GH_DISC_OK);language=load(disc,"etc/core_usa.bin",&n);C(gh_bundle_open(language,n,&lang)==GH_BUNDLE_OK);
 idsave=load(disc,"id/local/US/idsavecore_US.idd",&n);idd.bytes=idsave;idd.size=n;C(gh_id_set_data_002accf8(manager,11,&idd)==GH_MENU_OK);
 C(gh_id_lookup_002acd78(manager,11,2,&view)==GH_ID_OK);C(gh_bundle_open(view.bytes,view.size,&packed)==GH_BUNDLE_OK);
 C(gh_bundle_find(&lang,"MST",0,&mst)==GH_BUNDLE_OK);C(gh_bundle_find(&packed,"MSA",0,&msa)==GH_BUNDLE_OK);C(gh_bundle_find(&packed,"MSD",0,&msd)==GH_BUNDLE_OK);
 memset(&state,0xaa,sizeof(state));C(gh_packed_message_bind(&self,manager,&font,&state,&lang,1,11,1,&original)==GH_ID_OK&&original==1);
 C(self.packed.bytes==view.bytes&&state.cursor.bytes==msd.bytes&&state.initialized);
 /* Actual MC packed MSA has count0: original2A4CB0 skips binding, preserving
  * slot3. Do not fabricate a font by ignoring this real header condition. */
 C(msa.bytes[0]==0&&msa.bytes[1]==0&&!font.slots[3].bound&&!font.slots[3].mst);
 C(u32(state.environment)==1&&u32(state.environment+0x28)==2&&u32(state.environment+0x4c)==2&&u32(state.environment+0x50)==0);
 C(u32(state.environment+0x3c)==0x3f666666&&u32(state.environment+0x44)==0x3f666666);
 C(u32(state.environment+0x2c)==0x40000000&&u32(state.environment+0x30)==0xc1200000);
 C(state.environment[0x54]==255&&state.environment[0x55]==255&&state.environment[0x56]==0xaa&&state.environment[0x57]==0xaa);
 C(u32(state.environment+0x58)==6&&u32(state.environment+0x5c)==UINT32_MAX&&u32(state.environment+0x60)==0x477fff00);
 C(gh_packed_message_bind(&self,manager,&font,&state,&lang,0,11,1,&original)==GH_ID_OK&&original==1);
 C(u32(state.environment+0x3c)==0x3f800000&&u32(state.environment+0x44)==0x3f800000);
 C(gh_packed_message_bind(&self,manager,&font,&state,&lang,1,11,99999,&original)==GH_ID_OK&&original==0);
 C(gh_packed_message_bind(&self,manager,&font,&state,&lang,1,11,-1,&original)==GH_ID_BOUNDS);
 printf("PASS real MC B:2 packed, original zero-MSA skip, cursorMSD and exact language-dependent environment\n");free(language);free(idsave);free(manager);gh_disc_close(disc);return 0;}
