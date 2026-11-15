#include "game_enemy_base_init.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(int argc,char**argv){C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t index;gh_disc_entry e;C(!gh_disc_find(d,"etc/core_usa.bin",&index)&&!gh_disc_entry_at(d,index,&e));uint8_t*bytes=malloc((size_t)e.size);C(bytes&&!gh_disc_read(d,index,0,bytes,(size_t)e.size));gh_bundle bundle;gh_bundle_entry idd;C(!gh_bundle_open(bytes,(size_t)e.size,&bundle)&&!gh_id_find_language_data(&bundle,&idd));gh_id_state*id=calloc(1,sizeof(*id));C(id&&!gh_id_system_init_002acc70(id,&idd));gh_id_runtime*runtime;C(!gh_id_runtime_create(id,1,&runtime)&&!gh_id_runtime_set_path_policy(runtime,GH_ID_PATH_REFERENCE_NEAREST));gh_game_mt mt;gh_game_mt_seed_0030f2b0(&mt,7);gh_game_cockpit*c;C(!gh_game_cockpit_create(runtime,id,&mt,&c));uint8_t saved[GH_CORE_SAVE_BYTES];gh_core_save_view save={saved,sizeof(saved)};C(!gh_core_save_system_init(save)&&!gh_game_cockpit_initialize_00140c88(c,save));
 uint8_t raw[GH_EM202_BYTES]={0};gh_title_player_view root;C(!gh_enemy202_construct_00201fa0(raw,sizeof(raw),&root));uint16_t kind=0x202;memcpy(raw+0x2fe,&kind,2);gh_enemy_request request={.object_kind=0x202,.position_xyz={0x3f800000,0x40000000,0x40400000},.yaw_bits=0x3f000000};gh_enemy_base_initialize op={0};int original;
 C(gh_enemy_base_initialize_0028ebf0(&op,&root,&request,42,NULL,&mt,&original)==GH_ACTOR_MODEL_UNSUPPORTED&&op.frontier==0x2abbe8&&!original&&!memcmp(root.alias_f0,request.position_xyz,12));
 gh_id_base*base;C(!gh_game_cockpit_base(c,0xe00,&base));base->playing=0;base->frame=123;
 C(!gh_enemy_base_initialize_0028ebf0(&op,&root,&request,42,c,&mt,&original)&&original==1&&op.cockpit_base==base&&base->playing==1&&base->frame==0);
 C(!memcmp(raw+0x490,request.position_xyz,12)&&raw[0x531]==3&&raw[0x628]==42&&raw[0x570]==255&&raw[0x571]==255&&raw[0x574]==2);
 uint32_t random_index=mt.index;base->frame=10;C(!gh_enemy_base_initialize_0028ebf0(&op,&root,&request,42,c,&mt,&original)&&base->frame==10&&mt.index==random_index);
 C(!gh_game_cockpit_destroy(c));gh_id_runtime_destroy(runtime);free(id);free(bytes);gh_disc_close(d);puts("PASS actual28EBF0 sameCockpitE00 restart/playing/position/scalars/missingUIstrict/retry noreset");return 0;}
