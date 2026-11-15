#include "game_enemy_initialize.h"
#include "game_enemy_factory.h"
#include "disc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t*load(gh_disc*d,const char*path,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,path,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*p=malloc(*n);C(p&&!gh_disc_read(d,i,0,p,*n));return p;}
int main(int argc,char**argv){C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t n,m;uint8_t*room=load(d,"st1/r100.dat",&n),*em=load(d,"em/em02.dat",&m);gh_bundle bundle;C(!gh_bundle_open(room,n,&bundle));const uint8_t*expectedMD=em+762752,*expectedTM3=em+673536;
 gh_heap*heap;C(!gh_heap_create(32*1024*1024,&heap));gh_room_pools pools={0};C(!gh_game_room_pools_initialize_002a5d80(&pools,&bundle,heap));gh_object_view resource={0};resource.bytes=em;resource.size=m;resource.kind=0x202;resource.file_extent=m;resource.file_extent_known=1;
 const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};uint32_t fog=0;for(unsigned mode=0;mode<4;mode++){
 _Alignas(16)uint8_t bytes[GH_EM202_BYTES]={0};gh_title_player_view root;C(!gh_enemy202_construct_00201fa0(bytes,sizeof(bytes),&root));uint16_t kind=0x202;memcpy(bytes+0x2fe,&kind,2);gh_boot_work work={0};work.room=0x100;uint8_t saved[GH_CORE_SAVE_BYTES]={0},scenario=0;gh_core_save_view save={saved,sizeof(saved)};gh_enemy_request request={.object_kind=0x202,.position_xyz={0x3f800000,0x40000000,0x40400000},.dependency_kind=0x20a,.appearance=1,.entry_number=9};gh_enemy_initialize owner={0};C(!fesetround(modes[mode]));
 C(gh_enemy202_initialize_prefix_002022d0(&owner,&root,&request,&resource,&work,save,&scenario,NULL,&pools,heap,&fog,GH_STATIC_BINARY32_REFERENCE)==GH_ACTOR_MODEL_UNSUPPORTED&&owner.frontier==0x28ebf0&&owner.stage==3);
 C(fegetround()==modes[mode]&&owner.model.geometry.bytes==expectedMD&&owner.model.texture.bytes==expectedTM3&&owner.model.model.ready&&owner.model.model.part_count==31);
 C(!memcmp(bytes+0x1630,request.position_xyz,12)&&bytes[0x163c]==0&&bytes[0x163f]==0x3f&&bytes[0x186b]==1&&bytes[0x1650]==1&&bytes[0x1651]==9);
 uint32_t parts=pools.blocks[0].used_count;C(gh_enemy202_initialize_prefix_002022d0(&owner,&root,&request,&resource,&work,save,&scenario,NULL,&pools,heap,&fog,GH_STATIC_BINARY32_REFERENCE)==GH_ACTOR_MODEL_UNSUPPORTED&&owner.frontier==0x28ebf0&&pools.blocks[0].used_count==parts);
 C(!gh_enemy202_initialize_abort_prefix(&owner)&&!pools.blocks[0].used_count&&!pools.blocks[1].used_count);
 scenario=1;C(!gh_enemy202_construct_00201fa0(bytes,sizeof(bytes),&root));memcpy(bytes+0x2fe,&kind,2);
 C(gh_enemy202_initialize_prefix_002022d0(&owner,&root,&request,&resource,&work,save,&scenario,NULL,&pools,heap,&fog,GH_STATIC_BINARY32_REFERENCE)==GH_ACTOR_MODEL_UNSUPPORTED&&owner.frontier==0x202434&&!owner.model.model.parts_allocated);
 uint8_t alternate=7;C(gh_enemy202_initialize_prefix_002022d0(&owner,&root,&request,&resource,&work,save,&scenario,&alternate,&pools,heap,&fog,GH_STATIC_BINARY32_REFERENCE)==GH_ACTOR_MODEL_UNSUPPORTED&&owner.frontier==0x28ebf0&&bytes[0x186b]==7);
 C(!gh_enemy202_initialize_abort_prefix(&owner));
 }
 C(!fesetround(FE_TONEAREST));C(!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));free(em);free(room);gh_disc_close(d);puts("PASS actual2022D0 fields/roomoverride-strict/resume/model31parts/fourFEnv/retry/scopedrelease frontier28EBF0");return 0;
}
