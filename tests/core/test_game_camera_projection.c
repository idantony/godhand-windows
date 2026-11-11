#include "game_camera_projection.h"
#include "display_state.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t w(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static float value(const uint8_t*p){float n;memcpy(&n,p,4);return n;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static void apply(double out[4],const uint8_t*m,const double v[4]){for(unsigned r=0;r<4;r++){out[r]=0;for(unsigned k=0;k<4;k++)out[r]+=(double)value(m+4*(k*4+r))*v[k];}}
int main(int argc,char**argv){
 C(argc==3);FILE*fp=fopen(argv[2],"rb");C(fp&&fseek(fp,0,SEEK_END)==0);long n=ftell(fp);C(n>0&&fseek(fp,0,SEEK_SET)==0);uint8_t*elf=malloc((size_t)n);C(elf&&fread(elf,1,(size_t)n,fp)==(size_t)n&&fclose(fp)==0);
 gh_display_state display;C(gh_display_state_from_elf(&display,elf,(size_t)n)==GH_DISPLAY_OK);free(elf);
 gh_camera_projection_display config={&display.screen_width,&display.screen_height_bits,&display.pixel_aspect_bits,&display.initialized,GH_CAMERA_NATIVE_BINARY32_REFERENCE};
 static gh_game_init_state init;gh_game_init_state_initialize(&init);gh_game_pl_camera camera={0};
 C(gh_game_camera_construct_pl_00139bc8(&camera,&init.camera,GH_CAMERA_NATIVE_BINARY32_REFERENCE)==GH_CAMERA_OK);uint32_t frontier;
 C(gh_game_camera_project_00138740(&camera,&config,&frontier)==GH_CAMERA_UNSUPPORTED&&frontier==0x2a48a8);
 C(gh_display_static_initialize_002a48a8(&display,1,0xffff)==GH_DISPLAY_OK);
 uint8_t before[0x530];memcpy(before,camera.fields,sizeof(before));
 C(fesetround(FE_DOWNWARD)==0);C(gh_game_camera_project_00138740(&camera,&config,&frontier)==GH_CAMERA_OK&&!frontier);C(fegetround()==FE_DOWNWARD);C(fesetround(FE_TONEAREST)==0);
 C(!memcmp(before+0x180,camera.fields+0x180,0x530-0x180));
 /* Independently evaluate geometric invariants with binary64 arithmetic,
  * rather than duplicating the recovered polynomial/matrix implementation. */
 double eye[]={value(camera.fields+0x210),value(camera.fields+0x214),value(camera.fields+0x218),1},target[]={0,0,0,1},out[4];
 apply(out,camera.fields+0x140,eye);C(fabs(out[0])<1e-6&&fabs(out[1])<1e-6&&fabs(out[2])<1e-6&&out[3]==1);
 apply(out,camera.fields,target);C(out[3]>0&&fabs(out[0]/out[3]-2048)<0.001&&fabs(out[1]/out[3]-2048)<0.001);
 double fov=value(camera.fields+0x250),aspect;float af;memcpy(&af,&display.pixel_aspect_bits,4);aspect=af;
 double focal=(448/aspect*.5)/tan(fov*3.14159265358979323846/360);
 C(fabs(value(camera.fields+0x100)/(-focal*.8)-1)<2e-6&&fabs(value(camera.fields+0x114)/(-focal*aspect)-1)<2e-6);
 double near[]={0,0,.1,1},far[]={0,0,600,1};apply(out,camera.fields+0x100,near);C(fabs(out[2]/out[3]-65535)<.02);apply(out,camera.fields+0x100,far);C(fabs(out[2]/out[3]-1)<.001);
 C(!memcmp(camera.fields+0x40,camera.fields+0x80,64)&&memcmp(camera.fields+0x40,camera.fields+0xc0,64));
 /* ActualR020 ACD + actualInitialize5D8 signgate + samecanonicalcamera. */
 gh_disc*d;size_t idx;gh_disc_entry de;C(gh_disc_open(argv[1],&d)==GH_DISC_OK&&gh_disc_find(d,"st0/r020.dat",&idx)==GH_DISC_OK&&gh_disc_entry_at(d,idx,&de)==GH_DISC_OK);uint8_t*r=malloc((size_t)de.size);C(r&&gh_disc_read(d,idx,0,r,(size_t)de.size)==GH_DISC_OK);gh_bundle b;gh_bundle_entry acd;C(gh_bundle_open(r,(size_t)de.size,&b)==GH_BUNDLE_OK&&gh_bundle_find(&b,"ACD",0,&acd)==GH_BUNDLE_OK);
 gh_acd_scene_binding path;gh_game_camera_path_construct_00137a48(&path);int original;C(gh_acd_scene_bind_00137ae8(&path,&acd,&original)==GH_PLACE_OK&&original==1);
 uint8_t raw[GH_TITLE_PLAYER_BYTES]={0};gh_title_player_view player;C(gh_title_player_construct_00107e40(raw,sizeof(raw),&player)==GH_PLAYER_OK);player.initial_memory=GH_TITLE_MEMORY_ZERO_REFERENCE;gh_boot_work work={0};put((uint8_t*)&work+0x5d8,0xf00c6000);init.camera.mode=1;init.active_camera=&init.camera;
 C(gh_game_camera_step_projected_00138fc8(&camera,&init,&path,&player,&work,&config,&frontier)==GH_CAMERA_OK&&!frontier&&w(camera.fields+0x444)==0x3f87ae14);
 memcpy(before,camera.fields,sizeof(before));
 const unsigned amplitude[]={0x254,0x268},decay[]={0x25c,0x270},phase[]={0x260,0x274};
 for(unsigned i=0;i<2;i++){
  put(camera.fields+amplitude[i],i?0xbf800000:0x40000000);
  put(camera.fields+decay[i],0x3f000000);
  put(camera.fields+phase[i],0x3f000000);
 }
 camera.fields[0x264]=camera.fields[0x278]=2;
 C(gh_game_camera_step_projected_00138fc8(&camera,&init,&path,&player,&work,&config,&frontier)==GH_CAMERA_OK&&!frontier);
 C(camera.fields[0x264]==1&&camera.fields[0x278]==1);
 C(fabs(value(camera.fields+0x170)-value(before+0x170)-sin(.5))<1e-5);
 C(fabs(value(camera.fields+0x174)-value(before+0x174)+.5*sin(.5))<1e-5);
 C(!memcmp(camera.fields+0x100,before+0x100,64)&&memcmp(camera.fields,before,0x100));
 C(gh_game_camera_step_projected_00138fc8(&camera,&init,&path,&player,&work,&config,&frontier)==GH_CAMERA_OK&&!frontier);
 C(!camera.fields[0x264]&&!camera.fields[0x278]&&!memcmp(camera.fields,before,0x180));
 /* Unsupported large-angle reduction never writes stale/partial matrices. */
 put(camera.fields+0x250,0x42b40000);memcpy(before,camera.fields,sizeof(before));C(gh_game_camera_project_00138740(&camera,&config,&frontier)==GH_CAMERA_UNSUPPORTED&&frontier==0x3a4830&&!memcmp(before,camera.fields,sizeof(before)));
 free(r);gh_disc_close(d);puts("PASS actualR020 full camera late path + native projection/view/clip matrices; real displayELF, geometric near/far/center invariants, FEnv restoration");return 0;
}
