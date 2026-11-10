#include "game_camera_step.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t w(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
int main(int argc,char**argv){
 gh_disc*d;C(argc==2&&gh_disc_open(argv[1],&d)==GH_DISC_OK);size_t idx;gh_disc_entry de;
 C(gh_disc_find(d,"st0/r020.dat",&idx)==GH_DISC_OK&&gh_disc_entry_at(d,idx,&de)==GH_DISC_OK);
 uint8_t*r=malloc((size_t)de.size);C(r&&gh_disc_read(d,idx,0,r,(size_t)de.size)==GH_DISC_OK);
 gh_bundle b;gh_bundle_entry acd;C(gh_bundle_open(r,(size_t)de.size,&b)==GH_BUNDLE_OK&&gh_bundle_find(&b,"ACD",0,&acd)==GH_BUNDLE_OK);
 gh_acd_scene_binding path;memset(&path,0xa5,sizeof(path));gh_game_camera_path_construct_00137a48(&path);
 C(!path.records&&!path.field30&&!path.record_bytes&&w(path.scalar04+0x18)==0x3f87ae14);
 for(unsigned i=0;i<0x2c;i++)if(i<0x18||i>=0x1c)C(!path.scalar04[i]);
 int original;C(gh_acd_scene_bind_00137ae8(&path,&acd,&original)==GH_PLACE_OK&&original==1&&w(path.scalar04+8)==0);
 uint8_t raw[GH_TITLE_PLAYER_BYTES]={0};gh_title_player_view player;
 C(gh_title_player_construct_00107e40(raw,sizeof(raw),&player)==GH_PLAYER_OK);player.initial_memory=GH_TITLE_MEMORY_ZERO_REFERENCE;
 static gh_game_init_state init;gh_game_init_state_initialize(&init);gh_game_pl_camera camera={0};
 C(gh_game_camera_construct_pl_00139bc8(&camera,&init.camera,GH_CAMERA_NATIVE_BINARY32_REFERENCE)==GH_CAMERA_OK);
 init.active_camera=&init.camera;init.camera.mode=4;gh_boot_work work={0};uint32_t frontier;
 put(camera.fields+0x450,0x40000000);put(camera.fields+0x460,0x41000000);uint8_t before[0x530];memcpy(before,camera.fields,sizeof(before));
 path.field30=0xdeadbeef; /* actualnonnullcount0 selection clears old slot */
 C(fesetround(FE_UPWARD)==0);
 C(gh_game_camera_step_00138fc8(&camera,&init,&path,&player,&work,&frontier)==GH_CAMERA_OK&&!frontier);
 C(fegetround()==FE_UPWARD);C(fesetround(FE_TONEAREST)==0);
 C(!path.field30&&w(camera.fields+0x450)==0x3d8f5c30&&w(camera.fields+0x460)==0x40800000&&w(camera.fields+0x444)==0x3f87ae14);
 for(unsigned i=0;i<sizeof(before);i++)if(!(i>=0x444&&i<0x448)&&!(i>=0x450&&i<0x454)&&!(i>=0x460&&i<0x464))C(camera.fields[i]==before[i]);
 /* Path still runs before mode gate; unsupported movement preserves its writes. */
 init.camera.mode=1;
 C(gh_game_camera_step_00138fc8(&camera,&init,&path,&player,&work,&frontier)==GH_CAMERA_UNSUPPORTED&&frontier==0x139ff8);
 put(raw+0x5d8,4);C(gh_game_camera_step_00138fc8(&camera,&init,&path,&player,&work,&frontier)==GH_CAMERA_UNSUPPORTED&&frontier==0x13ad90);
 work.flags=0x00800000;C(gh_game_camera_step_00138fc8(&camera,&init,&path,&player,&work,&frontier)==GH_CAMERA_UNSUPPORTED&&frontier==0x13ad90);
 put((uint8_t*)&work+0x5d8,0xf00c6000);C(gh_game_camera_step_00138fc8(&camera,&init,&path,&player,&work,&frontier)==GH_CAMERA_UNSUPPORTED&&frontier==0x138740);
 camera.fields[0x264]=1;C(gh_game_camera_step_00138fc8(&camera,&init,&path,&player,&work,&frontier)==GH_CAMERA_UNSUPPORTED&&frontier==0x138558);camera.fields[0x264]=0;
 /* Positive region cannot be silently treated as no region. No persistent
  * scalar/selection mutation occurs before the unported geometry consumer. */
 put(path.scalar04+8,1);memcpy(before,camera.fields,sizeof(before));
 C(gh_game_camera_path_step_00137f38(&camera,&path,&player,&frontier)==GH_CAMERA_UNSUPPORTED&&frontier==0x1f83e8&&!memcmp(before,camera.fields,sizeof(before)));
 put(path.scalar04+8,0);path.records=NULL;path.field30=123;
 C(gh_game_camera_path_select_00137bb8(&path,&player,&original,&frontier)==GH_CAMERA_OK&&!original&&path.field30==123);
 C(gh_game_camera_path_step_00137f38(&camera,&path,&player,&frontier)==GH_CAMERA_UNSUPPORTED&&frontier==0x137f38);
 free(r);gh_disc_close(d);puts("PASS actualR020 ACD camera path/ordered scalar blend, canonical mode gate, bounded frontiers, host rounding restoration");return 0;
}
