/* Actual normal14D0E8/153B00 producers. Native reference arithmetic only;
 * canonical inputs remain borrowed and their ownership is never duplicated. */
#include "static_model_constants.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static float scalar(const uint8_t*p){float f;memcpy(&f,p,4);return f;}
static float mul(float a,float b){volatile float r=a*b;return r;}
static float add(float a,float b){volatile float r=a+b;return r;}
static float sub(float a,float b){volatile float r=a-b;return r;}
static float divide(float a,float b){volatile float r=a/b;return r;}
static int finite(const float*p,size_t n){for(size_t i=0;i<n;i++)if(!isfinite(p[i]))return 0;return 1;}
static gh_static_mesh_status unsupported(uint32_t*f,uint32_t pc){if(f)*f=pc;return GH_STATIC_UNSUPPORTED;}
static void matrix(float*out,const float*a,const float*b){
 for(unsigned col=0;col<4;col++)for(unsigned row=0;row<4;row++){
  float v=mul(a[row],b[col*4]);v=add(v,mul(a[4+row],b[col*4+1]));
  v=add(v,mul(a[8+row],b[col*4+2]));out[col*4+row]=add(v,mul(a[12+row],b[col*4+3]));
 }
}

gh_static_mesh_status gh_static_model_constants_0014d0e8(const gh_scene_mesh_object*object,
 const gh_game_pl_camera*camera,const gh_game_render_projection*projection,gh_static_transform_policy policy,
 gh_static_model_constants*out,uint32_t*f){
 if(f)*f=0;if(!object||!camera||!projection||!out||!object->constructor_complete||!object->allocation.pointer||!object->node||!projection->ready||!camera->constructed)return GH_STATIC_ARGUMENT;
 if(policy!=GH_STATIC_BINARY32_REFERENCE)return unsupported(f,0x14d0e8);
 const uint8_t*model=object->allocation.pointer;
 if(word(model+0x250)&0x80000u)return unsupported(f,0x14d0e8);
 gh_static_model_constants result={0};float camera_matrix[16],object_matrix[16];
 memcpy(camera_matrix,camera->fields,sizeof(camera_matrix));
 int special=(object->descriptor.mesh.flags&0x200000u)!=0;
 if(!special){memcpy(object_matrix,model+0x80,sizeof(object_matrix));if(!finite(object_matrix,16))return unsupported(f,0x14d0e8);}
 if(!finite(camera_matrix,16)||!finite(projection->camera_clip,16))return unsupported(f,0x14d0e8);
 fenv_t saved;if(feholdexcept(&saved))return unsupported(f,0x14d0e8);if(fesetround(FE_TONEAREST)){fesetenv(&saved);return unsupported(f,0x14d0e8);}
 if(special){memcpy(result.screen,camera_matrix,64);memcpy(result.clip,projection->camera_clip,64);}
 else{matrix(result.screen,camera_matrix,object_matrix);matrix(result.clip,projection->camera_clip,object_matrix);}
 for(unsigned i=0;i<4;i++)result.identity[i*5]=1;
 memcpy(result.projection,projection->projection,64);
 int valid=finite(result.screen,16)&&finite(result.clip,16);int restore=fesetenv(&saved);
 if(!valid||restore)return unsupported(f,0x14d0e8);*out=result;return GH_STATIC_OK;
}

gh_static_mesh_status gh_static_node_constants_00153b00(const gh_scene_mesh_object*object,
 const gh_boot_work*work,const gh_game_world_light*light,const gh_static_model_constants*model_constants,
 gh_static_transform_policy policy,gh_static_node_packet*out,uint32_t*f){
 if(f)*f=0;if(!object||!work||!light||!model_constants||!out||!object->constructor_complete||!object->allocation.pointer||!object->node||!light->constructed||!light->ready)return GH_STATIC_ARGUMENT;
 if(policy!=GH_STATIC_BINARY32_REFERENCE)return unsupported(f,0x153b00);
 const uint8_t*model=object->allocation.pointer,*node=object->node,*w=(const uint8_t*)work,*l=light->scalar;
 uint32_t flags=word(node+0x380),model_flags=word(model+0x250);
 if(flags&1u)return unsupported(f,0x153b00); /* source leaves packet unchanged; no existing packet supplied here */
 if(object->descriptor.mesh.flags&0x200000u)return unsupported(f,0x153b00); /* specialcamera/normalmap consumer */
 if(model[0x2be]==0x18||model[0x2be]==0x1c)return unsupported(f,0x153b00); /* source preserves priorUV packet */
 if(model[0x2bd]>2)return unsupported(f,0x153b00);
 int direct=(flags&0x800000u)||(word(w+0x598)&0x800u);
 if(!direct&&(model_flags&0x800000u))return unsupported(f,0x2d7e50);
 gh_static_node_packet packet={0};packet.fog_rgb=object->fog_rgb;memcpy(packet.projection,model_constants->projection,64);gh_static_transform_inputs result={0};memcpy(result.screen_matrix,model_constants->screen,64);memcpy(result.clip_matrix,model_constants->clip,64);
 if(!finite(result.screen_matrix,16)||!finite(result.clip_matrix,16))return unsupported(f,0x153b00);
 fenv_t saved;if(feholdexcept(&saved))return unsupported(f,0x153b00);if(fesetround(FE_TONEAREST)){fesetenv(&saved);return unsupported(f,0x153b00);}
 gh_static_mesh_status status=GH_STATIC_OK;float tint[4];
 for(unsigned i=0;i<4;i++)tint[i]=mul(scalar(model+0x240+4*i),scalar(node+0x3a0+4*i));
 tint[3]=mul(tint[3],divide((float)model[0x2b6],255));tint[3]=mul(tint[3],divide((float)model[0x2b7],255));tint[3]=mul(tint[3],divide((float)model[0x2bc],255));
 for(unsigned i=0;i<4;i++)result.color_scale[i]=(direct||(model_flags&0x80000u))?tint[i]:mul(scalar(l+0x30+4*i),tint[i]); /*2D8008*/
 result.uv_offset[0]=scalar(node+0x38c);result.uv_offset[1]=scalar(node+0x390);
 float near_value,far_value;int alternative=1;
 if(model[0x2bd]){status=unsupported(f,0x153b00);goto done;} /* ELF3C3D20/24 fogowners not bound */
 if((flags&0x200000u)||(word(w+0x58c)&0x1000u)){
  uint32_t a=0xc43f3fffu,b=0x4f3663e6u;memcpy(result.fog,&a,4);memcpy(result.fog+1,&b,4);
 }else{
  if(flags&0x20000u){near_value=scalar(l+0x64);far_value=scalar(l+0x68);packet.fog_rgb=word(l+0x60)&0xffffffu;}
  else if(flags&2u){near_value=scalar(l+0x18);far_value=scalar(l+0x1c);packet.fog_rgb=word(l+8)&0xffffffu;}
  else if(flags&0x40000u){near_value=scalar(l+0x20);far_value=scalar(l+0x24);packet.fog_rgb=word(l+0xc)&0xffffffu;}
  else if(flags&0x10000000u){near_value=scalar(l+0x70);far_value=scalar(l+0x74);packet.fog_rgb=word(l+0x6c)&0xffffffu;}
  else{near_value=scalar(l+0x16150);far_value=scalar(l+0x16154);alternative=0;packet.fog_rgb=0;
   for(unsigned i=0;i<3;i++){float c=scalar(l+0x16158+4*i);if(!isfinite(c)||c>255){status=unsupported(f,0x2d81a0);goto done;}if(c>=0)packet.fog_rgb|=(uint32_t)c<<(8*i);}
  }
  float distance=sub(far_value,near_value);
  if(!isfinite(distance)||distance==0){status=unsupported(f,0x2d81a0);goto done;}
  packet.fog_written=1;
  result.fog[0]=alternative?add(mul(divide(near_value,distance),255),255):mul(add(divide(near_value,distance),1),255);result.fog[1]=divide(-255,distance);
 }
 if(!finite(tint,4)||!finite(result.color_scale,4)||!finite(result.uv_offset,2)||!finite(result.fog,2))status=unsupported(f,0x153b00);
done:
 if(fesetenv(&saved))return unsupported(f,0x153b00);if(status)return status;packet.inputs=result;*out=packet;return GH_STATIC_OK;
}
