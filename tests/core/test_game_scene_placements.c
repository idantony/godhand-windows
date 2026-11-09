#include "game_scene_placements.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t w(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
int main(int argc,char**argv){gh_disc*d;CHECK(argc==2&&gh_disc_open(argv[1],&d)==GH_DISC_OK);size_t idx;gh_disc_entry de;CHECK(gh_disc_find(d,"st0/r020.dat",&idx)==GH_DISC_OK&&gh_disc_entry_at(d,idx,&de)==GH_DISC_OK);uint8_t*r=malloc((size_t)de.size);CHECK(r&&gh_disc_read(d,idx,0,r,(size_t)de.size)==GH_DISC_OK);gh_bundle b;gh_bundle_entry ems,oms,acd;CHECK(gh_bundle_open(r,(size_t)de.size,&b)==GH_BUNDLE_OK&&gh_bundle_find(&b,"EMS",0,&ems)==GH_BUNDLE_OK&&gh_bundle_find(&b,"OMS",0,&oms)==GH_BUNDLE_OK&&gh_bundle_find(&b,"ACD",0,&acd)==GH_BUNDLE_OK);
 uint8_t stop=0,active[0x1650];memset(active,0x5a,sizeof(active));memset(active,0,4);gh_room_state room={.active=active,.size=sizeof(active)};gh_boot_work work={0};gh_em_scene_placements em={.stop_flag=&stop,.room04=7};memset(em.scratch10,0xa5,sizeof(em.scratch10));
 CHECK(gh_em_scene_room_init_00294b38(&em,&work,1)==GH_PLACE_OK&&em.room04==7&&em.scratch10[0]==0xa5);CHECK(gh_em_scene_room_init_00294b38(&em,&work,0)==GH_PLACE_OK&&em.room04==0xffff&&!em.scratch10[47]);int original;
 CHECK(gh_em_scene_bind_00294c68(&em,&room,&ems,&original)==GH_PLACE_OK&&!original&&!em.file.bytes&&w(active)==0x80000000u&&!active[8]);for(unsigned i=4;i<sizeof(active);i++)if(i!=8)CHECK(active[i]==0x5a);
 CHECK(gh_em_scene_bind_00294c68(&em,&room,&ems,&original)==GH_PLACE_UNSUPPORTED&&em.frontier==0x294ca0);stop=1;CHECK(gh_em_scene_bind_00294c68(&em,&room,0,&original)==GH_PLACE_OK&&original==1&&!stop);
 gh_om_scene_placements om;gh_om_scene_construct_002a7308(&om);CHECK(gh_om_scene_bind_001c3cb8(&om,&oms,&original)==GH_PLACE_OK&&!original&&!om.file.bytes&&!om.entries.bytes);
 gh_acd_scene_binding a={.field30=0x1234};CHECK(gh_acd_scene_bind_00137ae8(&a,&acd,&original)==GH_PLACE_OK&&original==1&&a.records==acd.bytes+0x18&&a.record_bytes==acd.size-0x18&&a.field30==0x1234&&w(a.scalar04)==0x444341&&w(a.scalar04+0x18)==0x3f87ae14);
 CHECK(!memcmp(a.scalar04,acd.bytes,0x18)&&!memcmp(a.scalar04+0x1c,acd.bytes+0x1c,0x10));CHECK(gh_acd_scene_bind_00137ae8(&a,0,&original)==GH_PLACE_OK&&!original&&!a.records&&!a.field30&&w(a.scalar04+0x18)==0x3f87ae14);
 free(r);gh_disc_close(d);puts("PASS actualR020 emptyEMS/OMS activeflags andACD boundedview/scalars/resetgates");return 0;}
