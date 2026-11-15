#include "game_enemy_model.h"
#include "game_enemy_factory.h"
#include "disc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t*load(gh_disc*d,const char*path,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,path,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*p=malloc(*n);C(p&&!gh_disc_read(d,i,0,p,*n));return p;}
static void w(uint8_t*p,size_t o,uint32_t v){memcpy(p+o,&v,4);}
int main(int argc,char**argv){C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t n,m;uint8_t*room=load(d,"st1/r100.dat",&n),*em=load(d,"em/em02.dat",&m);gh_bundle bundle;C(!gh_bundle_open(room,n,&bundle));const uint8_t*expectedMD=em+762752,*expectedTM3=em+673536;
 gh_heap*heap;C(!gh_heap_create(32*1024*1024,&heap));gh_room_pools pools={0};C(!gh_game_room_pools_initialize_002a5d80(&pools,&bundle,heap));gh_object_view resource={0};resource.bytes=em;resource.size=m;resource.kind=0x202;resource.file_extent=m;resource.file_extent_known=1;
 const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};uint32_t fog=0;for(unsigned mode=0;mode<4;mode++){
 _Alignas(16)uint8_t bytes[GH_EM202_BYTES]={0};gh_title_player_view root;C(!gh_enemy202_construct_00201fa0(bytes,sizeof(bytes),&root));uint16_t kind=0x202;memcpy(bytes+0x2fe,&kind,2);w(bytes,0x564,0x20a);gh_boot_work work={0};gh_enemy_model owner={0};int original=0;C(!fesetround(modes[mode]));
 gh_actor_model_status e=gh_enemy_model_initialize_00208268(&owner,&root,&resource,&work,&pools,heap,&fog,GH_STATIC_BINARY32_REFERENCE,&original);
 printf("mode%u status%d frontier%x parts%u nodes%u original%d\n",mode,e,owner.frontier,owner.model.part_count,owner.model.script.mesh_count,original);
 C(fegetround()==modes[mode]);C(owner.geometry.bytes==expectedMD&&owner.texture.bytes==expectedTM3&&owner.occurrence==2);
 C(!e&&original==1&&owner.model.ready);C(owner.model.parts_allocated==owner.model.part_count&&owner.model.part_count>0);
 uint32_t parts=pools.blocks[0].used_count;C(!gh_enemy_model_initialize_00208268(&owner,&root,&resource,&work,&pools,heap,&fog,GH_STATIC_BINARY32_REFERENCE,&original)&&original==1&&pools.blocks[0].used_count==parts);
 C(!gh_enemy_model_release_prefix(&owner)&&!pools.blocks[0].used_count&&!pools.blocks[1].used_count);
 }
 C(!fesetround(FE_TONEAREST));
 _Alignas(16) uint8_t exchanged_bytes[GH_EM202_BYTES]={0};
 gh_title_player_view exchanged;
 C(!gh_enemy202_construct_00201fa0(exchanged_bytes,sizeof(exchanged_bytes),&exchanged));
 const uint16_t kind=0x202;
 memcpy(exchanged_bytes+0x2fe,&kind,2);
 w(exchanged_bytes,0x564,0x200);w(exchanged_bytes,0x1648,0x200);w(exchanged_bytes,0x164c,1);
 exchanged_bytes[0x1651]=2;
 gh_boot_work r105={.room=0x105};
 gh_enemy_model model={0};
 int initialized;
 uint8_t auth=em[0x178500+8+12*8+2];
 C(auth==0);
 em[0x178500+8+12*8+2]=255;
 C(gh_enemy_model_initialize_00208268(&model,&exchanged,&resource,&r105,&pools,heap,&fog,
     GH_STATIC_BINARY32_REFERENCE,&initialized)==GH_ACTOR_MODEL_BOUNDS);
 C(!initialized&&model.frontier==0x149ad8&&!model.stage&&!pools.blocks[0].used_count);
 C(!memcmp(exchanged_bytes+0x564,"\x00\x02\x00\x00",4));
 em[0x178500+8+12*8+2]=auth;
 C(!gh_enemy_model_initialize_00208268(&model,&exchanged,&resource,&r105,&pools,heap,&fog,
     GH_STATIC_BINARY32_REFERENCE,&initialized)&&initialized);
 C(model.occurrence==0&&model.texture.bytes==em+0x7d00&&model.geometry.bytes==em+0x2d840);
 for(unsigned i=0;i<16;++i)C(exchanged_bytes[0x2d4+i]==(i<12?i+12:255));
 uint32_t dependency,flags;
 memcpy(&dependency,exchanged_bytes+0x564,4);
 memcpy(&flags,exchanged_bytes+0x254,4);
 C(dependency==0x227&&(flags&0x10000000u)&&model.model.ready);
 uint32_t used=pools.blocks[0].used_count;
 C(!gh_enemy_model_initialize_00208268(&model,&exchanged,&resource,&r105,&pools,heap,&fog,
     GH_STATIC_BINARY32_REFERENCE,&initialized)&&initialized&&pools.blocks[0].used_count==used);
 C(!gh_enemy_model_release_prefix(&model)&&!pools.blocks[0].used_count&&!pools.blocks[1].used_count);
 puts("PASS actual R105 EM202/227 TXE theme exchange, name table, malformed-index rejection and retry");
 C(!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));free(em);free(room);gh_disc_close(d);puts("PASS actualEM202/20A occurrence2 SAMEresource model initialize fourFEnv/retry/scopedrelease");return 0;
}
