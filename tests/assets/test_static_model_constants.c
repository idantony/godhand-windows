#include "static_model_constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#include <math.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}static void value(uint8_t*p,float f){memcpy(p,&f,4);}static uint32_t bits(float f){uint32_t n;memcpy(&n,&f,4);return n;}
int main(void){
 /* Typed arithmetic fixture only, NOT a fabricated livegame camera/object.
  * ActualcanonicalSCPconstruction and camera projection have separate tests. */
 uint8_t model[0x360]={0},node[0x480]={0};gh_scene_mesh_object object={0};object.allocation.pointer=model;object.node=node;object.constructor_complete=1;object.fog_rgb=0xaabbcc;
 gh_game_pl_camera camera={0};camera.constructed=1;camera.policy=GH_CAMERA_NATIVE_BINARY32_REFERENCE;
 const float object_matrix[16]={0,2,0,0,-3,0,0,0,0,0,4,0,5,6,7,1};const float camera_matrix[16]={2,0,0,0,0,3,0,0,0,0,4,1,10,20,30,2};
 memcpy(model+0x80,object_matrix,64);memcpy(camera.fields,camera_matrix,64);gh_game_render_projection projection={.camera_clip={.25f,0,0,0,0,.25f,0,0,0,0,.25f,0,0,0,0,1},.ready=1};
 for(unsigned i=0;i<16;i++)projection.projection[i]=(float)(i+1);
 const float expected_screen[16]={0,6,0,0,-6,0,0,0,0,0,16,4,20,38,58,9};const float expected_clip[16]={0,.5f,0,0,-.75f,0,0,0,0,0,1,0,1.25f,1.5f,1.75f,1};
 gh_static_model_constants constants;uint32_t frontier;const int modes[4]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
 for(unsigned m=0;m<4;m++){
  C(!fesetround(modes[m])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));
  C(!gh_static_model_constants_0014d0e8(&object,&camera,&projection,GH_STATIC_BINARY32_REFERENCE,&constants,&frontier)&&!frontier&&fegetround()==modes[m]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);
  C(!memcmp(constants.screen,expected_screen,64)&&!memcmp(constants.clip,expected_clip,64)&&!memcmp(constants.projection,projection.projection,64));for(unsigned i=0;i<16;i++)C(constants.identity[i]==((i%5)==0?1.0f:0.0f));
 }
 C(!fesetround(FE_TONEAREST));gh_static_model_constants saved=constants;
 put(model+0x250,0x80000);C(gh_static_model_constants_0014d0e8(&object,&camera,&projection,GH_STATIC_BINARY32_REFERENCE,&constants,&frontier)==GH_STATIC_UNSUPPORTED&&frontier==0x14d0e8&&!memcmp(&constants,&saved,sizeof(saved)));put(model+0x250,0);
 object.descriptor.mesh.flags=0x200000;value(model+0x80,NAN);C(!gh_static_model_constants_0014d0e8(&object,&camera,&projection,GH_STATIC_BINARY32_REFERENCE,&constants,&frontier)&&!memcmp(constants.screen,camera_matrix,64)&&!memcmp(constants.clip,projection.camera_clip,64)); /* originalspecialbranch doesnotreadmodelmatrix */
 object.descriptor.mesh.flags=0;memcpy(model+0x80,object_matrix,64);constants=saved;
 gh_game_world_light*light=calloc(1,sizeof(*light));C(light);light->constructed=light->ready=1;const float ambient[4]={1.25f,.75f,.5f,1};memcpy(light->scalar+0x30,ambient,16);
 const float model_color[4]={.8f,.6f,.4f,.9f},node_color[4]={1,.5f,2,.75f};memcpy(model+0x240,model_color,16);memcpy(node+0x3a0,node_color,16);model[0x2b6]=128;model[0x2b7]=64;model[0x2bc]=192;model[0x2be]=0xfe;
 value(node+0x38c,.125f);value(node+0x390,-.25f);value(light->scalar+0x16150,10);value(light->scalar+0x16154,110);value(light->scalar+0x16158,100.9f);value(light->scalar+0x1615c,20.2f);value(light->scalar+0x16160,-3);
 gh_boot_work work={0};put((uint8_t*)&work+0x588,0x1000);put((uint8_t*)&work+0x594,0x800);gh_static_node_packet packet;
 for(unsigned m=0;m<4;m++){
  C(!fesetround(modes[m])&&!gh_static_node_constants_00153b00(&object,&work,light,&constants,GH_STATIC_BINARY32_REFERENCE,&packet,&frontier)&&fegetround()==modes[m]);
  C(!memcmp(packet.inputs.screen_matrix,expected_screen,64)&&!memcmp(packet.inputs.clip_matrix,expected_clip,64)&&!memcmp(packet.projection,projection.projection,64));
  C(bits(packet.inputs.color_scale[0])==0x3f800000&&bits(packet.inputs.color_scale[1])==0x3e666667&&bits(packet.inputs.color_scale[2])==0x3ecccccd&&bits(packet.inputs.color_scale[3])==0x3d832176);
  C(packet.inputs.uv_offset[0]==.125f&&packet.inputs.uv_offset[1]==-.25f&&packet.inputs.fog[0]==280.5f&&bits(packet.inputs.fog[1])==0xc0233333&&packet.fog_rgb==0x1464&&packet.fog_written);
 }
 C(!fesetround(FE_TONEAREST));value(light->scalar+0x16150,1);value(light->scalar+0x16154,15);C(!gh_static_node_constants_00153b00(&object,&work,light,&constants,GH_STATIC_BINARY32_REFERENCE,&packet,&frontier)&&bits(packet.inputs.fog[0])==0x43889b6d);
 put(node+0x380,0x20000);value(light->scalar+0x64,1);value(light->scalar+0x68,15);put(light->scalar+0x60,0x123456);C(!gh_static_node_constants_00153b00(&object,&work,light,&constants,GH_STATIC_BINARY32_REFERENCE,&packet,&frontier)&&bits(packet.inputs.fog[0])==0x43889b6e&&packet.fog_rgb==0x123456); /* originalalternateorder differs1ULP */
 put(node+0x380,0);put((uint8_t*)&work+0x58c,0x1000);C(!gh_static_node_constants_00153b00(&object,&work,light,&constants,GH_STATIC_BINARY32_REFERENCE,&packet,&frontier)&&!packet.fog_written&&packet.fog_rgb==0xaabbcc&&bits(packet.inputs.fog[0])==0xc43f3fff&&bits(packet.inputs.fog[1])==0x4f3663e6);
 put((uint8_t*)&work+0x58c,0);put((uint8_t*)&work+0x598,0x800);value(light->scalar+0x30,NAN);C(!gh_static_node_constants_00153b00(&object,&work,light,&constants,GH_STATIC_BINARY32_REFERENCE,&packet,&frontier)&&bits(packet.inputs.color_scale[0])==0x3f4ccccd); /* ignoredambientnotread */
 value(light->scalar+0x30,1.25f);put((uint8_t*)&work+0x598,0);gh_static_node_packet before=packet;put(model+0x250,0x800000);C(gh_static_node_constants_00153b00(&object,&work,light,&constants,GH_STATIC_BINARY32_REFERENCE,&packet,&frontier)==GH_STATIC_UNSUPPORTED&&frontier==0x2d7e50&&!memcmp(&before,&packet,sizeof(packet)));put(model+0x250,0);
 value(light->scalar+0x16154,1);C(gh_static_node_constants_00153b00(&object,&work,light,&constants,GH_STATIC_BINARY32_REFERENCE,&packet,&frontier)==GH_STATIC_UNSUPPORTED&&frontier==0x2d81a0&&!memcmp(&before,&packet,sizeof(packet)));
 free(light);puts("PASS originalmatrixcolumnorder/translation andnormalstaticnode tint/alpha/UV/fog/order/Workoffset gates,4FEnv; explicitnumericfixtures notlivegamecamera");return 0;
}
