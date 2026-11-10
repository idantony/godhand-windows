#include "../../src/core/game_render_projection.h"
#include "../../src/core/display_state.h"
#include "../../src/platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(int argc,char**argv){
 C(argc==2);gh_disc*disc;C(gh_disc_open(argv[1],&disc)==GH_DISC_OK);
 size_t n=(size_t)gh_disc_boot_elf_size(disc);unsigned char*elf=malloc(n);C(elf&&gh_disc_read_boot_elf(disc,0,elf,n)==GH_DISC_OK);gh_disc_close(disc);
 gh_display_state display;C(gh_display_state_from_elf(&display,elf,n)==GH_DISPLAY_OK);free(elf);
 gh_camera_projection_display cfg={&display.screen_width,&display.screen_height_bits,&display.pixel_aspect_bits,&display.initialized,GH_CAMERA_NATIVE_BINARY32_REFERENCE};
 static gh_game_init_state init;gh_game_init_state_initialize(&init);gh_game_pl_camera camera={0};
 C(gh_game_camera_construct_pl_00139bc8(&camera,&init.camera,GH_CAMERA_NATIVE_BINARY32_REFERENCE)==GH_CAMERA_OK);
 gh_game_render_projection out={0},saved;uint32_t frontier;
 C(gh_game_render_projection_001538f0(&out,&camera,&cfg,&frontier)==GH_CAMERA_UNSUPPORTED&&frontier==0x2a48a8&&!out.ready);
 C(gh_display_static_initialize_002a48a8(&display,1,0xffff)==GH_DISPLAY_OK);
 C(gh_game_camera_project_00138740(&camera,&cfg,&frontier)==GH_CAMERA_OK);
 unsigned char before[sizeof(camera.fields)];memcpy(before,camera.fields,sizeof(before));
 C(fesetround(FE_DOWNWARD)==0&&feraiseexcept(FE_INEXACT)==0);
 C(gh_game_render_projection_001538f0(&out,&camera,&cfg,&frontier)==GH_CAMERA_OK&&!frontier&&out.ready);
 C(fegetround()==FE_DOWNWARD&&(fetestexcept(FE_INEXACT)&FE_INEXACT));C(fesetround(FE_TONEAREST)==0);
 C(!memcmp(before,camera.fields,sizeof(before))&&!memcmp(out.camera_clip,camera.fields+0x40,64));
 /* Independent binary64 geometry: focal/tan factors cancel in real arithmetic.
  * Check final coordinate mapping, not a second implementation of polynomial. */
 float fov,aspect;memcpy(&fov,camera.fields+0x250,4);memcpy(&aspect,&display.pixel_aspect_bits,4);
 double x=51.2*((double)display.screen_width/640)/.1,y=44.8*(double)aspect/.1;
 C(fabs(out.projection[0]/x-1)<3e-6&&fabs(out.projection[5]/y-1)<3e-6);
 double depth=2031585.0/(double)fov;
 C(fabs(out.projection[10]-((1-depth)*.5))<.01&&fabs(out.projection[14]-((depth+1)*.5))<.01);
 C(out.projection[12]==2048&&out.projection[13]==2048&&out.projection[15]==1);
 for(unsigned i=0;i<16;i++)if(i!=0&&i!=5&&i!=10&&i!=12&&i!=13&&i!=14&&i!=15)C(out.projection[i]==0);
 saved=out;float bad=180;memcpy(camera.fields+0x250,&bad,4);
 C(gh_game_render_projection_001538f0(&out,&camera,&cfg,&frontier)==GH_CAMERA_UNSUPPORTED&&frontier==0x3a4830&&!memcmp(&saved,&out,sizeof(out)));
 bad=NAN;memcpy(camera.fields+0x250,&bad,4);C(gh_game_render_projection_001538f0(&out,&camera,&cfg,&frontier)==GH_CAMERA_NUMERIC&&!memcmp(&saved,&out,sizeof(out)));
 puts("PASS actual boot display/canonical camera ->1538F0 copied clip +native projection; independent geometry, atomic failures, FEnv");return 0;
}
