#include "game_actor_two_bone.h"
#include "id_layout_rotation.h"
#include "game_actor_matrix.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t rd(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static float rf(const uint8_t*p){float x;memcpy(&x,p,4);return x;}
static float bits(uint32_t n){float x;memcpy(&x,&n,4);return x;}
static float add(float a,float b){volatile float x=a+b;return x;}
static float sub(float a,float b){volatile float x=a-b;return x;}
static float mul(float a,float b){volatile float x=a*b;return x;}
static float divide(float a,float b){volatile float x=a/b;return x;}
static float length3(const float*v){volatile float x=sqrtf(add(add(mul(v[0],v[0]),mul(v[1],v[1])),mul(v[2],v[2])));return x;}
static void normalize(float*v){float l=length3(v);if(l>0){float q=divide(1,l);for(unsigned i=0;i<3;i++)v[i]=mul(v[i],q);}else v[0]=v[1]=v[2]=0;v[3]=1;}
static void cross(float*out,const float*a,const float*b){out[0]=sub(mul(a[1],b[2]),mul(a[2],b[1]));out[1]=sub(mul(a[2],b[0]),mul(a[0],b[2]));out[2]=sub(mul(a[0],b[1]),mul(a[1],b[0]));out[3]=1;}
static void matrix_quat(float*q,const float*m,const unsigned*cycle){float tr=add(add(m[0],m[5]),m[10]);if(tr>0){float r=sqrtf(add(tr,1)),s=divide(.5f,r);q[3]=mul(r,.5f);q[0]=mul(sub(m[6],m[9]),s);q[1]=mul(sub(m[8],m[2]),s);q[2]=mul(sub(m[1],m[4]),s);}else{unsigned i=m[0]<m[5];if(m[i*5]<m[10])i=2;unsigned j=cycle[i],k=cycle[j];float r=sqrtf(add(sub(sub(m[i*5],m[j*5]),m[k*5]),1)),s=divide(.5f,r);q[i]=mul(r,.5f);q[3]=mul(sub(m[j*4+k],m[k*4+j]),s);q[j]=mul(add(m[i*4+j],m[j*4+i]),s);q[k]=mul(add(m[i*4+k],m[k*4+i]),s);}}
/*15ABA0 differs from15A9A0: columns(up,side,target). */
static void basis(float*q,const float*axis,const float*target,const unsigned*cycle){float side[4],up[4],m[16]={0};cross(side,target,axis);normalize(side);cross(up,side,target);normalize(up);for(unsigned i=0;i<3;i++){m[i]=up[i];m[4+i]=side[i];m[8+i]=target[i];}m[15]=1;matrix_quat(q,m,cycle);}
static void qmul(float*out,const float*a,const float*b){float q[4];q[0]=add(add(sub(mul(a[1],b[2]),mul(a[2],b[1])),mul(a[0],b[3])),mul(a[3],b[0]));q[1]=add(add(sub(mul(a[2],b[0]),mul(a[0],b[2])),mul(a[1],b[3])),mul(a[3],b[1]));q[2]=add(add(sub(mul(a[0],b[1]),mul(a[1],b[0])),mul(a[2],b[3])),mul(a[3],b[2]));q[3]=sub(sub(sub(mul(a[3],b[3]),mul(a[0],b[0])),mul(a[1],b[1])),mul(a[2],b[2]));memcpy(out,q,16);}
static void rotate(float*out,const float*q,const float*v){float vq[]={v[0],v[1],v[2],0},inv[]={-q[0],-q[1],-q[2],q[3]},t[4];qmul(t,q,vq);qmul(out,t,inv);out[3]=1;}
static float atan_original(float y,float x,const float*c){float ay=fabsf(y),ax=fabsf(x),pi=bits(0x40490fdb);if(ax<bits(0x3727c5ac))return ay<bits(0x3727c5ac)?0:(y<0?bits(0xbfc90fdb):bits(0x3fc90fdb));if(ay<bits(0x3727c5ac))return x<0?(y<0?-pi:pi):0;float r=divide(sub(ay,ax),add(ay,ax)),r2=mul(r,r),p=r,result=mul(r,c[0]);for(unsigned i=1;i<8;i++){p=mul(p,r2);result=add(result,mul(p,c[i]));}result=add(result,bits(0x3f490fdb));return x<0?(y<0?sub(result,pi):sub(pi,result)):(y<0?-result:result);}
static float acos_original(float x,const float*c){volatile float root=sqrtf(sub(1,mul(x,x)));return atan_original(root,x,c);}
static float sine_fast(float x){float z=mul(x,x),v=mul(z,x),r=bits(0x2f2ec9d3);r=add(mul(z,r),bits(0xb2d72f34));r=add(mul(z,r),bits(0x3638ef1b));r=add(mul(z,r),bits(0xb9500d01));r=add(mul(z,r),bits(0x3c088889));return add(x,mul(v,add(mul(z,r),bits(0xbe2aaaab))));}
static int axis_angle(float*q,float angle,const float*axis){float half=mul(angle,.5f);if(fabsf(half)>bits(0x3f490fd8))return 0;float sine=sine_fast(half),unused,cosine;if(gh_id_sincos_reference(half,&unused,&cosine))return 0;for(unsigned i=0;i<3;i++)q[i]=mul(axis[i],sine);q[3]=cosine;return 1;}
static void quat_matrix(float*m,const float*q){float x=q[0],y=q[1],z=q[2],w=q[3];float xx=add(mul(x,x),mul(x,x)),yy=add(mul(y,y),mul(y,y)),zz=add(mul(z,z),mul(z,z)),xy=add(mul(x,y),mul(x,y)),yz=add(mul(y,z),mul(y,z)),zx=add(mul(z,x),mul(z,x)),xw=add(mul(x,w),mul(x,w)),yw=add(mul(y,w),mul(y,w)),zw=add(mul(z,w),mul(z,w));memset(m,0,64);m[0]=sub(sub(1,yy),zz);m[5]=sub(sub(1,zz),xx);m[10]=sub(sub(1,xx),yy);m[1]=add(xy,zw);m[4]=sub(xy,zw);m[2]=sub(zx,yw);m[8]=add(zx,yw);m[6]=add(yz,xw);m[9]=sub(yz,xw);m[15]=1;}
gh_actor_motion_status gh_actor_two_bone_00153158(gh_actor_model*m,unsigned index,const gh_boot_work*work,const gh_motion_ik_globals*g,const float*coeff,gh_static_transform_policy policy,uint32_t*f){
 if(!m||!m->parts_ready||!work||!g||!g->initialized||!coeff||!f||index>=m->part_count||m->part_count-index<3)return GH_ACTOR_MOTION_ARGUMENT;*f=0;
 if(policy!=GH_STATIC_BINARY32_REFERENCE||(rd((const uint8_t*)work+0x58c)&1)){*f=0x153158;return GH_ACTOR_MOTION_UNSUPPORTED;}
 for(unsigned i=0;i<3;i++)if(g->cycle[i]>=3)return GH_ACTOR_MOTION_ARGUMENT;
 gh_actor_part*a=&m->parts[index],*b=&m->parts[index+1],*c=&m->parts[index+2];if(!a->bytes||!b->bytes||!a->alias_f0||!b->alias_f0||!b->alias_d0||!c->alias_f0)return GH_ACTOR_MOTION_ARGUMENT;
 for(unsigned i=0;i<8;i++)if(!isfinite(coeff[i]))return GH_ACTOR_MOTION_ARGUMENT;
 fenv_t env;if(fegetenv(&env)||fesetround(FE_TONEAREST)){*f=0x153158;return GH_ACTOR_MOTION_UNSUPPORTED;}
 gh_actor_motion_status result=GH_ACTOR_MOTION_OK;float delta[4];for(unsigned i=0;i<3;i++)delta[i]=sub(rf(a->alias_f0+i*4),rf(c->alias_f0+i*4));delta[3]=1;float distance=length3(delta),la=rf(a->bytes+0x158),lb=rf(b->bytes+0x158),angle_a=0,angle_b=0;
 if(!isfinite(distance)||!isfinite(la)||!isfinite(lb)||la<=0||lb<=0||distance<=0){*f=0x153158;result=GH_ACTOR_MOTION_UNSUPPORTED;goto finish;}
 if(distance<add(la,lb)){float ca=divide(sub(add(mul(la,la),mul(distance,distance)),mul(lb,lb)),mul(add(la,la),distance));float cb=divide(sub(add(mul(distance,distance),mul(lb,lb)),mul(la,la)),mul(add(distance,distance),lb));if(ca< -1)ca=-1;if(ca>1)ca=1;if(cb< -1)cb=-1;if(cb>1)cb=1;angle_a=acos_original(ca,coeff);angle_b=acos_original(cb,coeff);}
 float inverse_bind[4],local_axis[4],world_axis[4],target_direction[4],base[4],turn[4],q[4],world_a[16],world_b[16];memcpy(inverse_bind,a->bytes+0x160,16);rotate(local_axis,inverse_bind,g->xaxis);
 for(unsigned row=0;row<4;row++)world_axis[row]=add(add(mul(rf(a->bytes+0x80+row*4),local_axis[0]),mul(rf(a->bytes+0x90+row*4),local_axis[1])),mul(rf(a->bytes+0xa0+row*4),local_axis[2]));normalize(world_axis);
 for(unsigned i=0;i<3;i++)target_direction[i]=sub(rf(c->alias_f0+i*4),rf(a->alias_f0+i*4));target_direction[3]=1;normalize(target_direction);basis(base,world_axis,target_direction,g->cycle);qmul(base,base,inverse_bind);
 if(!axis_angle(turn,-angle_a,local_axis)){*f=0x3a19f8;result=GH_ACTOR_MOTION_UNSUPPORTED;goto finish;}qmul(q,base,turn);quat_matrix(world_a,q);for(unsigned i=0;i<3;i++)world_a[12+i]=rf(a->alias_f0+i*4);
 if(!axis_angle(turn,angle_b,local_axis)){*f=0x3a19f8;result=GH_ACTOR_MOTION_UNSUPPORTED;goto finish;}qmul(q,base,turn);quat_matrix(world_b,q);
 for(unsigned row=0;row<3;row++)world_b[12+row]=add(add(add(mul(world_a[row],rf(b->alias_d0)),mul(world_a[4+row],rf(b->alias_d0+4))),mul(world_a[8+row],rf(b->alias_d0+8))),world_a[12+row]);
 for(unsigned i=0;i<16;i++)if(!isfinite(world_a[i])||!isfinite(world_b[i])){*f=0x153158;result=GH_ACTOR_MOTION_UNSUPPORTED;goto finish;}
 memcpy(a->bytes+0x80,world_a,64);memcpy(b->bytes+0x80,world_b,64);
finish:if(fesetenv(&env)){*f=0x153158;return GH_ACTOR_MOTION_UNSUPPORTED;}return result;
}
/* Original3A2558 rational, allcoefficients read from actualimmediates. */
static float asin_original(float x){
 uint32_t raw;memcpy(&raw,&x,4);uint32_t magnitude=raw&0x7fffffffu;float ax=bits(magnitude),hi=bits(0x3fc90fda),lo=bits(0x33a22168);
 if(magnitude==0x3f800000u)return add(mul(x,hi),mul(x,lo));if(magnitude<0x32000000u)return x;
 float t=magnitude<0x3f000000u?mul(x,x):mul(sub(1,ax),.5f);
 float p=bits(0x3811ef08);p=add(mul(t,p),bits(0x3a4f7f04));p=add(mul(t,p),bits(0xbd241146));p=add(mul(t,p),bits(0x3e4e0aa8));p=add(mul(t,p),bits(0xbea6b090));p=mul(t,add(mul(t,p),bits(0x3e2aaaab)));
 float q=bits(0x3d9dc62e);q=add(mul(t,q),bits(0xbf303361));q=add(mul(t,q),bits(0x4001572d));q=add(mul(t,q),bits(0xc019d139));q=add(mul(t,q),1);
 if(magnitude<0x3f000000u)return add(x,mul(x,divide(p,q)));
 float root=sqrtf(t),result;if(magnitude>=0x3f79999au){float value=add(root,mul(root,divide(p,q)));result=sub(hi,sub(add(value,value),lo));}
 else{uint32_t bitsroot;memcpy(&bitsroot,&root,4);float truncated=bits(bitsroot&0xfffff000u),correction=divide(sub(t,mul(truncated,truncated)),add(root,truncated));float v=sub(mul(add(root,root),divide(p,q)),sub(lo,add(correction,correction)));float w=sub(bits(0x3f490fdb),add(truncated,truncated));result=sub(bits(0x3f490fdb),sub(v,w));}
 return raw&0x80000000u?-result:result;
}
static gh_actor_motion_status rebuild_body(gh_actor_model*m,unsigned i,gh_static_transform_policy policy,uint32_t*f){
 if(!m||!f||i>=m->part_count||!m->parts[i].bytes||!m->parts[i].alias_d0)return GH_ACTOR_MOTION_ARGUMENT;
 uint8_t*p=m->parts[i].bytes;float angle[4],wrapped[4],scale[3],position[3],matrix[16];for(unsigned j=0;j<3;j++)angle[j]=add(rf(p+0x100+j*4),rf(p+0x130+j*4));angle[3]=1;memcpy(scale,p+0x110,12);memcpy(position,m->parts[i].alias_d0,12);
 if(gh_actor_euler_adjust_001f7da0(angle,wrapped,policy,f)||gh_actor_matrix_compose_00147a48(position,wrapped,scale,0,matrix,policy,f))return GH_ACTOR_MOTION_UNSUPPORTED;memcpy(p,matrix,64);return GH_ACTOR_MOTION_OK;
}
static gh_actor_motion_status extract_angles(const uint8_t*p,const float*coeff,float angle[3],gh_static_transform_policy policy,uint32_t*f){
 *f=0;if(policy!=GH_STATIC_BINARY32_REFERENCE){*f=0x147c88;return GH_ACTOR_MOTION_UNSUPPORTED;}
 float scale[3];for(unsigned col=0;col<3;col++){float v[]={rf(p+col*16),rf(p+col*16+4),rf(p+col*16+8),1};scale[col]=length3(v);if(!(scale[col]>0)||!isfinite(scale[col])){*f=0x148160;return GH_ACTOR_MOTION_UNSUPPORTED;}}
 float y=divide(-rf(p+8),scale[0]);angle[0]=atan_original(divide(rf(p+24),scale[1]),divide(rf(p+40),scale[2]),coeff);angle[1]=y<= -1?bits(0xbfc90fdb):y>=1?bits(0x3fc90fdb):asin_original(y);angle[2]=atan_original(divide(rf(p+4),scale[0]),divide(rf(p),scale[0]),coeff);
 for(unsigned j=0;j<3;j++)if(!isfinite(angle[j])){*f=0x3a1b78;return GH_ACTOR_MOTION_UNSUPPORTED;}return GH_ACTOR_MOTION_OK;
}
static gh_actor_motion_status extract_body(gh_actor_model*m,unsigned index,const float*coeff,gh_static_transform_policy policy,uint32_t*f){
 if(!m||!coeff||!f||index>=m->part_count||!m->parts[index].bytes)return GH_ACTOR_MOTION_ARGUMENT;float angle[3];gh_actor_motion_status e=extract_angles(m->parts[index].bytes,coeff,angle,policy,f);if(e)return e;memcpy(m->parts[index].bytes+0x100,angle,12);return rebuild_body(m,index,policy,f);
}

gh_actor_motion_status gh_actor_matrix_extract_euler_00147c88(const float*m,const float*c,float*out,gh_static_transform_policy policy,uint32_t*f){if(!m||!c||!out||!f)return GH_ACTOR_MOTION_ARGUMENT;*f=0;fenv_t env;if(feholdexcept(&env))return GH_ACTOR_MOTION_UNSUPPORTED;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_ACTOR_MOTION_UNSUPPORTED;}float angle[3];gh_actor_motion_status e=extract_angles((const uint8_t*)m,c,angle,policy,f);if(fesetenv(&env))return GH_ACTOR_MOTION_UNSUPPORTED;if(!e)memcpy(out,angle,12);return e;}
gh_actor_motion_status gh_actor_part_rebuild_00150138(gh_actor_model*m,unsigned i,gh_static_transform_policy policy,uint32_t*f){if(!f)return GH_ACTOR_MOTION_ARGUMENT;fenv_t env;if(fegetenv(&env)||fesetround(FE_TONEAREST))return GH_ACTOR_MOTION_UNSUPPORTED;gh_actor_motion_status e=rebuild_body(m,i,policy,f);if(fesetenv(&env))return GH_ACTOR_MOTION_UNSUPPORTED;return e;}
gh_actor_motion_status gh_actor_part_extract_rebuild_00147c88(gh_actor_model*m,unsigned i,const float*c,gh_static_transform_policy policy,uint32_t*f){if(!f)return GH_ACTOR_MOTION_ARGUMENT;fenv_t env;if(fegetenv(&env)||fesetround(FE_TONEAREST))return GH_ACTOR_MOTION_UNSUPPORTED;gh_actor_motion_status e=extract_body(m,i,c,policy,f);if(fesetenv(&env))return GH_ACTOR_MOTION_UNSUPPORTED;return e;}

gh_actor_motion_status gh_actor_sine_small_003a19f8(float x,float*out,gh_static_transform_policy policy,uint32_t*f){if(!out||!f)return GH_ACTOR_MOTION_ARGUMENT;*f=0;if(policy!=GH_STATIC_BINARY32_REFERENCE||!isfinite(x)||fabsf(x)>bits(0x3f490fd8)){*f=0x3a19f8;return GH_ACTOR_MOTION_UNSUPPORTED;}fenv_t env;if(feholdexcept(&env))return GH_ACTOR_MOTION_UNSUPPORTED;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_ACTOR_MOTION_UNSUPPORTED;}uint32_t raw;memcpy(&raw,&x,4);float value=(raw&0x7fffffffu)<0x32000000u?x:sine_fast(x);int valid=isfinite(value);if(fesetenv(&env))valid=0;if(!valid){*f=0x3a19f8;return GH_ACTOR_MOTION_UNSUPPORTED;}*out=value;return GH_ACTOR_MOTION_OK;}
gh_actor_motion_status gh_actor_quaternion_matrix_001478a8(const float*q,float*out,gh_static_transform_policy policy,uint32_t*f){if(!q||!out||!f)return GH_ACTOR_MOTION_ARGUMENT;*f=0;if(policy!=GH_STATIC_BINARY32_REFERENCE){*f=0x1478a8;return GH_ACTOR_MOTION_UNSUPPORTED;}for(unsigned i=0;i<4;i++)if(!isfinite(q[i]))return GH_ACTOR_MOTION_BOUNDS;fenv_t env;if(feholdexcept(&env))return GH_ACTOR_MOTION_UNSUPPORTED;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_ACTOR_MOTION_UNSUPPORTED;}float m[16];quat_matrix(m,q);int valid=1;for(unsigned i=0;i<16;i++)if(!isfinite(m[i]))valid=0;if(fesetenv(&env))valid=0;if(!valid){*f=0x1478a8;return GH_ACTOR_MOTION_UNSUPPORTED;}memcpy(out,m,64);return GH_ACTOR_MOTION_OK;}

gh_actor_motion_status gh_actor_matrix_extract_scale_00148160(const float*m,float*out,gh_static_transform_policy policy,uint32_t*f){if(!m||!out||!f)return GH_ACTOR_MOTION_ARGUMENT;*f=0;if(policy!=GH_STATIC_BINARY32_REFERENCE){*f=0x148160;return GH_ACTOR_MOTION_UNSUPPORTED;}for(unsigned col=0;col<3;col++)for(unsigned row=0;row<3;row++)if(!isfinite(m[col*4+row])){*f=0x148160;return GH_ACTOR_MOTION_UNSUPPORTED;}fenv_t env;if(feholdexcept(&env))return GH_ACTOR_MOTION_UNSUPPORTED;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_ACTOR_MOTION_UNSUPPORTED;}float scale[3];int valid=1;for(unsigned col=0;col<3;col++){scale[col]=length3(m+col*4);if(!isfinite(scale[col]))valid=0;}if(fesetenv(&env))return GH_ACTOR_MOTION_UNSUPPORTED;if(!valid){*f=0x148160;return GH_ACTOR_MOTION_UNSUPPORTED;}memcpy(out,scale,12);return GH_ACTOR_MOTION_OK;}
