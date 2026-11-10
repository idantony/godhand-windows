#include "game_collision_pair_queries.h"
#include <math.h>
#include <float.h>
#include <fenv.h>
#include <string.h>
static float val(uint32_t n){float f;memcpy(&f,&n,4);return f;}
static uint32_t bits(float f){uint32_t n;memcpy(&n,&f,4);return n;}
static float add(float a,float b){volatile float n=a+b;return n;}
static float sub(float a,float b){volatile float n=a-b;return n;}
static float mul(float a,float b){volatile float n=a*b;return n;}
static float divide(float a,float b){volatile float n=a/b;return n;}
static int normal(float a){return isfinite(a)&&(a==0||fabsf(a)>=FLT_MIN);}
static float dot(const float*a,const float*b){return add(add(mul(a[0],b[0]),mul(a[1],b[1])),mul(a[2],b[2]));}
static void cross(const float*a,const float*b,float*out){out[0]=sub(mul(a[1],b[2]),mul(a[2],b[1]));out[1]=sub(mul(a[2],b[0]),mul(a[0],b[2]));out[2]=sub(mul(a[0],b[1]),mul(a[1],b[0]));}
static gh_pair_query_status fail(uint32_t*f,uint32_t pc){if(f)*f=pc;return GH_PAIR_QUERY_UNSUPPORTED;}
gh_pair_query_status gh_collision_query_sphere_cylinder_002bb670(const uint8_t*point,uint32_t sr,const uint8_t*matrix,uint32_t cr,uint32_t height,gh_static_transform_policy policy,int*hit,uint32_t*distance,uint32_t*f){
 if(f)*f=0;if(!point||!matrix||!hit||!distance)return GH_PAIR_QUERY_ARGUMENT;if(policy!=GH_STATIC_BINARY32_REFERENCE)return fail(f,0x2bb670);
 float p[3],m[16],r1=val(sr),r2=val(cr),h=val(height);memcpy(p,point,12);memcpy(m,matrix,64);for(unsigned i=0;i<3;i++)if(!normal(p[i]))return fail(f,0x1566c0);for(unsigned i=0;i<16;i++)if(!normal(m[i]))return fail(f,0x30a2b0);if(!normal(r1)||!normal(r2)||!normal(h))return fail(f,0x2bb670);
 fenv_t env;if(feholdexcept(&env))return fail(f,0x1566c0);if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return fail(f,0x1566c0);}
 float a[3],b[3],ap[3],bp[3],dir[3],pa[3],first[3],n[3],unit[3]={0};
 for(unsigned k=0;k<3;k++){a[k]=m[12+k];float t=mul(m[k],0);t=add(t,mul(m[4+k],h));t=add(t,mul(m[8+k],0));b[k]=add(t,mul(m[12+k],1));ap[k]=sub(a[k],p[k]);bp[k]=sub(b[k],p[k]);dir[k]=sub(b[k],a[k]);pa[k]=sub(p[k],a[k]);}
 float d1=sqrtf(dot(ap,ap)),d2=sqrtf(dot(bp,bp)),nearest=d1<d2?d1:d2;
 float endpoint_test=sqrtf(add(dot(dir,dir),mul(nearest,nearest)));cross(dir,pa,first);cross(dir,first,n);float length=sqrtf(dot(n,n));if(length>0){float inverse=divide(1,length);for(unsigned k=0;k<3;k++)unit[k]=mul(n[k],inverse);}
 float answer=dot(unit,ap);if(endpoint_test<=d1||endpoint_test<=d2)answer=nearest;float sum=add(r1,r2);int result=answer<=sum;int valid=normal(d1)&&normal(d2)&&normal(nearest)&&normal(endpoint_test)&&normal(length)&&normal(answer)&&normal(sum);for(unsigned k=0;k<3;k++)valid=valid&&normal(a[k])&&normal(b[k])&&normal(ap[k])&&normal(bp[k])&&normal(dir[k])&&normal(pa[k])&&normal(first[k])&&normal(n[k])&&normal(unit[k]);
 if(fesetenv(&env))return fail(f,0x1566c0);if(!valid)return fail(f,0x1566c0);*hit=result;*distance=bits(answer);return GH_PAIR_QUERY_OK;
}
