/* Original2C5470/307B28/308BC0/308D40 native slice, MIT LICENSE.upstream,
 * eaae18d0dfab1f9a0df276ae5ca1581ad326faf3. Explicitreference math; actual
 * byteevidence inanalysis/game-scene-post-evidence.json. */
#include "game_scene_post.h"
#include "game_ray_query.h"
#include "id_layout_rotation.h"
#include <math.h>
#include <fenv.h>
#include <string.h>
static uint32_t word(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static float bits(uint32_t n){float x;memcpy(&x,&n,4);return x;}
static float readf(const uint8_t*p){return bits(word(p));}
static float add(float a,float b){volatile float r=a+b;return r;}
static float sub(float a,float b){volatile float r=a-b;return r;}
static float mul(float a,float b){volatile float r=a*b;return r;}
static float divide(float a,float b){volatile float r=a/b;return r;}
static gh_scene_post_status fail(uint32_t*f,uint32_t pc,gh_scene_post_status e){if(f)*f=pc;return e;}
static int normalize(float*v,float*length){float n=add(add(mul(v[0],v[0]),mul(v[1],v[1])),mul(v[2],v[2]));float len=sqrtf(n);if(!(len>0)||!isfinite(len))return 0;if(length)*length=len;float q=divide(1,len);for(unsigned i=0;i<3;i++)v[i]=mul(v[i],q);v[3]=1;return 1;}
static void side_cross(float*out,const float*v){out[0]=-v[2];out[1]=0;out[2]=v[0];out[3]=1;}
static void offset(float*out,const float*origin,const float*direction,float scale){for(unsigned i=0;i<3;i++)out[i]=add(origin[i],mul(direction[i],scale));out[3]=1;}
/* 3A19F8 smallangle→3A4728 iy0, y0. Actualimmediatecoefficients, notlibcsinf. */
static float sine_small(float x){float z=mul(x,x),v=mul(z,x),r=bits(0x2f2ec9d3);r=add(mul(z,r),bits(0xb2d72f34));r=add(mul(z,r),bits(0x3638ef1b));r=add(mul(z,r),bits(0xb9500d01));r=add(mul(z,r),bits(0x3c088889));return add(x,mul(v,add(mul(z,r),bits(0xbe2aaaab))));}
static void quat_rotate(float*out,const float*q,const float*v){
 float x=v[0],y=v[1],z=v[2],qx=q[0],qy=q[1],qz=q[2],qw=q[3];
 float a=add(add(sub(mul(qy,z),mul(qz,y)),mul(qx,0)),mul(qw,x));
 float b=add(add(sub(mul(qz,x),mul(qx,z)),mul(qy,0)),mul(qw,y));
 float c=add(add(sub(mul(qx,y),mul(qy,x)),mul(qz,0)),mul(qw,z));
 float d=sub(sub(sub(mul(qw,0),mul(qx,x)),mul(qy,y)),mul(qz,z));
 out[1]=add(add(sub(mul(c,-qx),mul(a,-qz)),mul(b,qw)),mul(d,-qy));
 out[2]=add(add(sub(mul(a,-qy),mul(b,-qx)),mul(c,qw)),mul(d,-qz));
 out[0]=add(add(sub(mul(b,-qz),mul(c,-qy)),mul(a,qw)),mul(d,-qx));out[3]=1;
}
gh_scene_post_status gh_scene_scroll_forward_002c5470(const gh_room_pools*p,const gh_boot_work*w,uint32_t*f){
 if(f)*f=0;if(!p||!w)return GH_POST_ARGUMENT;if(word((const uint8_t*)w+0x5d8)&0x08000000u)return GH_POST_OK;
 const gh_room_vector*v;if(gh_room_registry_view(p,0x754c50,&v)!=GH_POOL_OK)return fail(f,0x2c5470,GH_POST_POOLS);
 if(v->count)return fail(f,0x319520,GH_POST_UNSUPPORTED);return GH_POST_OK;
}
gh_scene_post_status gh_scene_hidden_reset_00308bc0(gh_game_scene_tables*t,const gh_room_pools*p,uint32_t*f){
 if(f)*f=0;if(!t||!p)return GH_POST_ARGUMENT;
 for(unsigned i=0;i<32;i++)if(t->scroll_link_words[i])return fail(f,0x14b6f8,GH_POST_UNSUPPORTED);
 const gh_room_vector*v;if(gh_room_registry_view(p,0x754c58,&v)!=GH_POOL_OK)return fail(f,0x308bc0,GH_POST_POOLS);
 if(v->count)return fail(f,0x308bc0,GH_POST_UNSUPPORTED);return GH_POST_OK;
}
void gh_scene_hidden_clear_00308d40(gh_game_scene_tables*t){if(t)for(unsigned i=32;i;i--)t->scroll_link_words[i-1]=0;}
gh_scene_post_status gh_scene_model_rays_00307b28(const gh_scene_post_bindings*b,gh_scene_model_ray rays[5],size_t*count,uint32_t*f){
 if(f)*f=0;if(!b||!rays||!count||!b->init||!b->camera||!b->player||!b->player->bytes||b->player->size<GH_TITLE_PLAYER_BYTES||b->player->alias_f0!=b->player->bytes+0xb0)return GH_POST_ARGUMENT;
 *count=0;if(b->policy!=GH_CAMERA_NATIVE_BINARY32_REFERENCE)return fail(f,0x307b28,GH_POST_UNSUPPORTED);
 if(!b->camera->constructed||!b->camera->identity||b->init->active_camera!=b->camera->identity)return fail(f,0x138f88,GH_POST_UNSUPPORTED);
 fenv_t env;if(feholdexcept(&env))return GH_POST_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_POST_NUMERIC;}
 gh_scene_model_ray temp[5]={0};float from[4],to[4],direction[4],side[4],length;int mode=(uint32_t)b->init->camera.mode&1;int good=1;uint32_t pc=0;
 for(unsigned i=0;i<3;i++){from[i]=readf(mode?b->player->alias_f0+i*4:b->camera->fields+0x200+i*4);to[i]=readf(b->camera->fields+0x210+i*4);if(!isfinite(from[i])||!isfinite(to[i]))good=0;}from[3]=to[3]=1;
 if(mode){from[1]=add(from[1],bits(0x3f99999a));to[1]=add(to[1],bits(0x3e4ccccd));}
 for(unsigned i=0;i<3;i++)direction[i]=sub(to[i],from[i]);direction[3]=1;
 if(!normalize(direction,&length))good=0;side_cross(side,direction);if(!normalize(side,NULL))good=0;
 if(good){memcpy(temp[0].from,from,16);memcpy(temp[0].to,to,16);
  if(!mode){offset(temp[1].from,from,side,.5f);offset(temp[1].to,to,side,.5f);offset(temp[2].from,from,side,-.5f);offset(temp[2].to,to,side,-.5f);}
  else{
   for(unsigned k=0;k<2;k++){float angle=bits(k?0x3ea0d97c:0xbea0d97c),half=mul(angle,.5f),sine=sine_small(half),sn,cs;
    if(gh_id_sincos_reference(half,&sn,&cs)!=GH_ID_OK){good=0;pc=0x1f7fb8;break;}
    float q[]={mul(0,sine),mul(1,sine),mul(0,sine),cs},rot[4];quat_rotate(rot,q,direction);
    memcpy(temp[1+k].from,from,16);for(unsigned i=0;i<3;i++)temp[1+k].to[i]=add(from[i],mul(mul(rot[i],length),bits(0x3f36db6e)));temp[1+k].to[3]=1;
   }
   offset(temp[3].from,from,side,bits(0x3eb33333));offset(temp[3].to,to,side,bits(0x3ee66666));offset(temp[4].from,from,side,-bits(0x3eb33333));offset(temp[4].to,to,side,-bits(0x3ee66666));
  }
 }
 unsigned n=mode?5:3;for(unsigned k=0;k<n;k++)for(unsigned i=0;i<4;i++)if(!isfinite(temp[k].from[i])||!isfinite(temp[k].to[i]))good=0;
 if(fesetenv(&env))return GH_POST_NUMERIC;if(!good)return fail(f,pc?pc:0x1f7e28,pc?GH_POST_UNSUPPORTED:GH_POST_NUMERIC);
 memcpy(rays,temp,n*sizeof(*rays));*count=n;return GH_POST_OK;
}
gh_scene_post_status gh_scene_model_hide_00307b28(const gh_scene_post_bindings*b,uint32_t*f){
 if(f)*f=0;if(!b||!b->work||!b->pools||!b->tables)return GH_POST_ARGUMENT;
 if(word((const uint8_t*)b->work+0x5d8)&0x200000u)return GH_POST_OK;
 gh_scene_post_status e=gh_scene_hidden_reset_00308bc0(b->tables,b->pools,f);if(e)return e;
 gh_scene_hidden_clear_00308d40(b->tables);
 gh_scene_model_ray rays[5];size_t count;e=gh_scene_model_rays_00307b28(b,rays,&count,f);if(e)return e;
 for(size_t i=0;i<count;i++)for(unsigned report=0;report<2;report++){
  gh_scene_ray_report storage[8];const void *objects[8]={0};uint32_t object_count=0;
  gh_ray_dynamic_reports dynamic={objects,8,&object_count};gh_scene_ray_request request={0};int hit=0;uint32_t pc=0;
  memcpy(request.from,rays[i].from,16);memcpy(request.to,rays[i].to,16);request.mask=7;
  request.report_limit=report?0:8;request.native_reports=report?NULL:storage;
  request.dynamic_reports=report?&dynamic:NULL;
  gh_scene_ray_status q=gh_scene_ray_query_00131e10(b->tables,&request,&hit,&pc);
  if(q!=GH_RAY_OK)return fail(f,pc?pc:0x131e10,q==GH_RAY_UNSUPPORTED?GH_POST_UNSUPPORTED:GH_POST_QUERY);
  if(hit)return fail(f,0x307b28,GH_POST_UNSUPPORTED);
 }
 /* Originalfinal754C58traversal isempty, provenbyreset'sSAMEregistryowner. */
 return GH_POST_OK;
}
