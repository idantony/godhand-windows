/*147A48/147728/147B60/147BF8/SDK30A650/700/7B8/870.*/
#include "game_actor_matrix.h"
#include "id_layout_rotation.h"
#include <string.h>
#include <math.h>
#include <float.h>
#include <fenv.h>
static float mul(float a,float b){volatile float n=a*b;return n;}static float add(float a,float b){volatile float n=a+b;return n;}
static int normal(float f){return isfinite(f)&&(f==0||fabsf(f)>=FLT_MIN);}
static void identity(float*m){memset(m,0,64);for(unsigned j=0;j<4;j++)m[j*5]=1;}
static void product(float*out,const float*a,const float*b){float m[16];for(unsigned col=0;col<4;col++)for(unsigned row=0;row<4;row++){float n=mul(a[row],b[col*4]);n=add(n,mul(a[4+row],b[col*4+1]));n=add(n,mul(a[8+row],b[col*4+2]));m[col*4+row]=add(n,mul(a[12+row],b[col*4+3]));}memcpy(out,m,64);}
static gh_actor_matrix_status rotation_body(const float*angle,unsigned mode,float rotation[16],uint32_t*f){identity(rotation);static const unsigned orders[6][3]={{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
 if(mode<6)for(unsigned step=0;step<3;step++){unsigned axis=orders[mode][step];float sine,cosine;if(gh_id_sincos_reference(angle[axis],&sine,&cosine)){*f=0x30a5d0;return GH_ACTOR_MATRIX_UNSUPPORTED;}float r[16];identity(r);if(axis==0){r[5]=r[10]=cosine;r[6]=sine;r[9]=add(0,-sine);}else if(axis==1){r[0]=r[10]=cosine;r[2]=add(0,-sine);r[8]=sine;}else{r[0]=r[5]=cosine;r[1]=sine;r[4]=add(0,-sine);}product(rotation,r,rotation);}return GH_ACTOR_MATRIX_OK;}
gh_actor_matrix_status gh_actor_matrix_compose_00147a48(const float*p,const float*angle,const float*scale,unsigned mode,float*out,gh_static_transform_policy policy,uint32_t*f){if(!p||!angle||!scale||!out||!f)return GH_ACTOR_MATRIX_ARGUMENT;*f=0;if(policy!=GH_STATIC_BINARY32_REFERENCE){*f=0x147a48;return GH_ACTOR_MATRIX_UNSUPPORTED;}for(unsigned j=0;j<3;j++)if(!normal(p[j])||!normal(scale[j])||(mode<6&&!normal(angle[j]))){*f=0x147728;return GH_ACTOR_MATRIX_UNSUPPORTED;}
 fenv_t saved;if(fegetenv(&saved)||fesetround(FE_TONEAREST)){*f=0x147a48;return GH_ACTOR_MATRIX_UNSUPPORTED;}float rotation[16];identity(rotation);gh_actor_matrix_status result=GH_ACTOR_MATRIX_OK;result=rotation_body(angle,mode,rotation,f);
 float m[16],translation[16],scaling[16];identity(translation);identity(scaling);memcpy(translation+12,p,12);for(unsigned j=0;j<3;j++)scaling[j*5]=scale[j];if(!result){product(m,translation,rotation);product(m,m,scaling);for(unsigned j=0;j<16;j++)if(!normal(m[j])){*f=0x30a2e0;result=GH_ACTOR_MATRIX_UNSUPPORTED;break;}}
 if(fesetenv(&saved)){*f=0x147a48;return GH_ACTOR_MATRIX_UNSUPPORTED;}if(!result)memcpy(out,m,64);return result;
}
gh_actor_matrix_status gh_actor_euler_adjust_001f7da0(const float*in,float*out,gh_static_transform_policy policy,uint32_t*f){if(!in||!out||!f)return GH_ACTOR_MATRIX_ARGUMENT;*f=0;if(policy!=GH_STATIC_BINARY32_REFERENCE){*f=0x1f7da0;return GH_ACTOR_MATRIX_UNSUPPORTED;}float n[4];memcpy(n,in,16);for(unsigned j=0;j<3;j++)if(gh_id_angle_reference_001f7d48(in[j],n+j)){*f=0x1f7da0;return GH_ACTOR_MATRIX_UNSUPPORTED;}memcpy(out,n,16);return GH_ACTOR_MATRIX_OK;}

gh_actor_matrix_status gh_actor_matrix_rotation_00147728(const float*angle,unsigned mode,float*out,gh_static_transform_policy policy,uint32_t*f){if(!angle||!out||!f)return GH_ACTOR_MATRIX_ARGUMENT;*f=0;if(policy!=GH_STATIC_BINARY32_REFERENCE){*f=0x147728;return GH_ACTOR_MATRIX_UNSUPPORTED;}if(mode<6)for(unsigned k=0;k<3;k++)if(!normal(angle[k])){*f=0x147728;return GH_ACTOR_MATRIX_UNSUPPORTED;}fenv_t saved;if(fegetenv(&saved)||fesetround(FE_TONEAREST)){*f=0x147728;return GH_ACTOR_MATRIX_UNSUPPORTED;}float m[16];gh_actor_matrix_status result=rotation_body(angle,mode,m,f);if(fesetenv(&saved)){*f=0x147728;return GH_ACTOR_MATRIX_UNSUPPORTED;}if(!result)memcpy(out,m,64);return result;}
