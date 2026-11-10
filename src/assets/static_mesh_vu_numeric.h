#ifndef GH_STATIC_MESH_VU_NUMERIC_H
#define GH_STATIC_MESH_VU_NUMERIC_H
#include <stdint.h>
#include <math.h>
#include <float.h>
#include <limits.h>
/* Shared finite VU numeric reference for recovered static programs10/14.
 * No instruction execution, Q/status owner or overflow-mode assumption.
 * Subnormal division inputs remain unsupported rather than silently flushed. */
static inline int gh_static_vu_reciprocal(float w,float *q){
 if(!isfinite(w)||fpclassify(w)==FP_SUBNORMAL)return 0;
 if(w==0)*q=copysignf(FLT_MAX,w);
 else{volatile float n=1.0f/w;*q=n;}
 return isfinite(*q)&&fpclassify(*q)!=FP_SUBNORMAL;
}
static inline int gh_static_vu_ftoi4(float x,int32_t *out){
 if(isnan(x))return 0;
 /* Overflowed projected +/-Inf and signed finite saturation lead to the
  * same integer output. Surviving cross arithmetic still requires finite. */
 if(x>=134217728.0f){*out=INT32_MAX;return 1;}
 if(x<=-134217728.0f){*out=INT32_MIN;return 1;}
 volatile float scaled=x*16.0f;*out=(int32_t)scaled;return 1;
}
static inline uint8_t gh_static_vu_clip_flags(const float p[4]){
 float w=fabsf(p[3]);int tiny=w==0||fpclassify(w)==FP_SUBNORMAL;uint8_t flags=0;
 for(unsigned k=0;k<3;k++){
  int outside=tiny?fpclassify(p[k])==FP_NORMAL:fabsf(p[k])>w;
  if(outside)flags|=(uint8_t)(1u<<(2*k+(signbit(p[k])?1:0)));
 }
 return flags;
}
#endif
