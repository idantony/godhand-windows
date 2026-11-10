#include "game_terrain_closest.h"
#include <math.h>
#include <string.h>
static float add(float a,float b){volatile float x=a+b;return x;}
static float sub(float a,float b){volatile float x=a-b;return x;}
static float mul(float a,float b){volatile float x=a*b;return x;}
static float divf(float a,float b){volatile float x=a/b;return x;}
static float dot(const float*a,const float*b){return add(add(mul(a[0],b[0]),mul(a[1],b[1])),mul(a[2],b[2]));}
static void delta(const float*a,const float*b,float*out){for(unsigned k=0;k<3;k++)out[k]=sub(a[k],b[k]);}
static void cross(const float*a,const float*b,float*out){out[0]=sub(mul(a[1],b[2]),mul(a[2],b[1]));out[1]=sub(mul(a[2],b[0]),mul(a[0],b[2]));out[2]=sub(mul(a[0],b[1]),mul(a[1],b[0]));}
static int edge(const float*a,const float*delta,float x,float y,float*out){float denominator=add(x,y);if(!(denominator>0)||!isfinite(denominator))return 0;float t=divf(x,denominator);for(unsigned k=0;k<3;k++)out[k]=add(a[k],mul(delta[k],t));return 1;}
int gh_terrain_closest_00135348(const float*p,const float*a,const float*b,const float*c,float*out){
 if(!p||!a||!b||!c||!out)return 0;for(unsigned k=0;k<3;k++)if(!isfinite(p[k])||!isfinite(a[k])||!isfinite(b[k])||!isfinite(c[k]))return 0;
 float ab[3],ac[3],bc[3],pa[3],pb[3],pc[3],negative[3];delta(b,a,ab);delta(c,a,ac);delta(c,b,bc);delta(p,a,pa);delta(p,b,pb);delta(p,c,pc);
 float d1=dot(pa,ab);for(unsigned k=0;k<3;k++)negative[k]=-ab[k];float d2=dot(pb,negative),d3=dot(pa,ac);for(unsigned k=0;k<3;k++)negative[k]=-ac[k];float d4=dot(pc,negative);
 if(d1<=0&&d3<=0){memcpy(out,a,12);return 1;}
 float d5=dot(pb,bc);for(unsigned k=0;k<3;k++)negative[k]=-bc[k];float d6=dot(pc,negative);
 if(d2<=0&&d5<=0){memcpy(out,b,12);return 1;}if(d4<=0&&d6<=0){memcpy(out,c,12);return 1;}
 float n[3],ap[3],bp[3],cp[3],crossed[3];cross(ab,ac,n);delta(a,p,ap);delta(b,p,bp);delta(c,p,cp);cross(ap,bp,crossed);float area_c=dot(n,crossed);
 if(area_c<=0&&d1>=0&&d2>=0)return edge(a,ab,d1,d2,out);
 cross(bp,cp,crossed);float area_a=dot(n,crossed);if(area_a<=0&&d5>=0&&d6>=0)return edge(b,bc,d5,d6,out);
 cross(cp,ap,crossed);float area_b=dot(n,crossed);if(area_b<=0&&d3>=0&&d4>=0)return edge(a,ac,d3,d4,out);
 float total=add(add(area_a,area_b),area_c);if(!isfinite(total)||!(total>0))return 0;float x=divf(area_a,total),y=divf(area_b,total),z=sub(sub(1,x),y);
 for(unsigned k=0;k<3;k++){out[k]=add(add(mul(a[k],x),mul(b[k],y)),mul(c[k],z));if(!isfinite(out[k]))return 0;}return 1;
}
