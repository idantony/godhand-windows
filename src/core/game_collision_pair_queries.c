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
static int endpoints(const float*m,float height,float*a,float*b){for(unsigned k=0;k<3;k++){a[k]=m[12+k];float t=mul(m[k],0);t=add(t,mul(m[4+k],height));t=add(t,mul(m[8+k],0));b[k]=add(t,mul(m[12+k],1));if(!normal(a[k])||!normal(b[k]))return 0;}return 1;}
static int normalize(const float*v,float*out){float length=sqrtf(dot(v,v));if(!normal(length))return 0;for(unsigned k=0;k<3;k++)out[k]=0;if(length>0){float inverse=divide(1,length);if(!normal(inverse))return 0;for(unsigned k=0;k<3;k++){out[k]=mul(v[k],inverse);if(!normal(out[k]))return 0;}}return 1;}
/* Actual1566C0 order, shared rather than a second segment-distance algorithm. */
static int point_segment_distance(const float*p,const float*a,const float*b,float*out){
 float ap[3],bp[3],dir[3],pa[3],first[3],n[3],unit[3];for(unsigned k=0;k<3;k++){ap[k]=sub(a[k],p[k]);bp[k]=sub(b[k],p[k]);dir[k]=sub(b[k],a[k]);pa[k]=sub(p[k],a[k]);}
 float d1=sqrtf(dot(ap,ap)),d2=sqrtf(dot(bp,bp)),nearest=d1<d2?d1:d2;float endpoint_test=sqrtf(add(dot(dir,dir),mul(nearest,nearest)));cross(dir,pa,first);cross(dir,first,n);if(!normalize(n,unit))return 0;float answer=dot(unit,ap);if(endpoint_test<=d1||endpoint_test<=d2)answer=nearest;
 int valid=normal(d1)&&normal(d2)&&normal(nearest)&&normal(endpoint_test)&&normal(answer);for(unsigned k=0;k<3;k++)valid=valid&&normal(ap[k])&&normal(bp[k])&&normal(dir[k])&&normal(pa[k])&&normal(first[k])&&normal(n[k]);if(valid)*out=answer;return valid;
}
gh_pair_query_status gh_collision_query_sphere_cylinder_002bb670(const uint8_t*point,uint32_t sr,const uint8_t*matrix,uint32_t cr,uint32_t height,gh_static_transform_policy policy,int*hit,uint32_t*distance,uint32_t*f){
 if(f)*f=0;if(!point||!matrix||!hit||!distance)return GH_PAIR_QUERY_ARGUMENT;if(policy!=GH_STATIC_BINARY32_REFERENCE)return fail(f,0x2bb670);
 float p[3],m[16],r1=val(sr),r2=val(cr),h=val(height);memcpy(p,point,12);memcpy(m,matrix,64);for(unsigned i=0;i<3;i++)if(!normal(p[i]))return fail(f,0x1566c0);for(unsigned i=0;i<16;i++)if(!normal(m[i]))return fail(f,0x30a2b0);if(!normal(r1)||!normal(r2)||!normal(h))return fail(f,0x2bb670);
 fenv_t env;if(feholdexcept(&env))return fail(f,0x1566c0);if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return fail(f,0x1566c0);}
 float a[3],b[3],answer=0,sum=add(r1,r2);int valid=endpoints(m,h,a,b)&&point_segment_distance(p,a,b,&answer)&&normal(sum),result=answer<=sum;
 if(fesetenv(&env))return fail(f,0x1566c0);if(!valid)return fail(f,0x1566c0);*hit=result;*distance=bits(answer);return GH_PAIR_QUERY_OK;
}

static int box_corners(const float*m,const float*size,float corners[8][3]){
 float half_x=mul(size[0],0.5f),half_z=mul(size[2],0.5f);int valid=normal(half_x)&&normal(half_z);
 for(unsigned i=0;i<8;i++){float v[3]={(i&1)?half_x:-half_x,(i&4)?size[1]:0,(i&2)?half_z:-half_z};for(unsigned k=0;k<3;k++){float n=mul(m[k],v[0]);n=add(n,mul(m[4+k],v[1]));n=add(n,mul(m[8+k],v[2]));corners[i][k]=add(n,mul(m[12+k],1));valid=valid&&normal(corners[i][k]);}}return valid;
}
/* Actual2BB868, also called by original cylinder and box dispatches. */
static int sphere_poly(float radius,const float*p,const float corners[8][3],int*hit){
 static const uint8_t faces[6][3]={{0,2,1},{4,5,6},{2,6,3},{0,1,4},{1,3,5},{2,0,6}};
 for(unsigned face=0;face<6;face++){const float*a=corners[faces[face][0]],*b=corners[faces[face][1]],*c=corners[faces[face][2]];float ca[3],ba[3],n[3],unit[3];for(unsigned k=0;k<3;k++){ca[k]=sub(c[k],a[k]);ba[k]=sub(b[k],a[k]);}cross(ca,ba,n);if(!normalize(n,unit))return 0;float plane_distance=sub(dot(unit,p),dot(unit,a));int valid=normal(plane_distance);for(unsigned k=0;k<3;k++)valid=valid&&normal(ca[k])&&normal(ba[k])&&normal(n[k]);if(!valid)return 0;if(radius<plane_distance){*hit=0;return 1;}}
 *hit=1;return 1;
}
gh_pair_query_status gh_collision_query_sphere_box_002bbba0(const uint8_t*point,uint32_t radius,const uint8_t*matrix,const uint32_t*full_size,gh_static_transform_policy policy,int*hit,uint32_t*f){
 if(f)*f=0;if(!point||!matrix||!full_size||!hit)return GH_PAIR_QUERY_ARGUMENT;if(policy!=GH_STATIC_BINARY32_REFERENCE)return fail(f,0x2bbba0);
 float p[3],m[16],size[3],r=val(radius);memcpy(p,point,12);memcpy(m,matrix,64);for(unsigned k=0;k<3;k++){size[k]=val(full_size[k]);if(!normal(size[k])||!normal(p[k]))return fail(f,0x2bbba0);}if(!normal(r))return fail(f,0x2bb868);for(unsigned k=0;k<16;k++)if(!normal(m[k]))return fail(f,0x30a2b0);
 fenv_t env;if(feholdexcept(&env))return fail(f,0x2bb868);if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return fail(f,0x2bb868);}float corners[8][3];int result=0,valid=box_corners(m,size,corners)&&sphere_poly(r,p,corners,&result);
 if(fesetenv(&env))return fail(f,0x2bb868);if(!valid)return fail(f,0x2bb868);*hit=result;return GH_PAIR_QUERY_OK;
}

static gh_pair_query_status cylinder_pair_body(const uint8_t*matrix_a,uint32_t radius_a,uint32_t height_a,const uint8_t*matrix_b,uint32_t radius_b,uint32_t height_b,int trim_first,uint32_t entry,gh_static_transform_policy policy,int*hit,uint32_t*f){
 if(f)*f=0;if(!matrix_a||!matrix_b||!hit)return GH_PAIR_QUERY_ARGUMENT;if(policy!=GH_STATIC_BINARY32_REFERENCE)return fail(f,entry);
 float ma[16],mb[16],ra=val(radius_a),rb=val(radius_b),ha=val(height_a),hb=val(height_b);memcpy(ma,matrix_a,64);memcpy(mb,matrix_b,64);for(unsigned k=0;k<16;k++)if(!normal(ma[k])||!normal(mb[k]))return fail(f,0x30a2b0);if(!normal(ra)||!normal(rb)||!normal(ha)||!normal(hb))return fail(f,entry);
 fenv_t env;if(feholdexcept(&env))return fail(f,0x2bc0f8);if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return fail(f,0x2bc0f8);}uint32_t pc=0;int result=0;
 float a[3],b[3],c[3],d[3],direction[3],unit[3];if(!endpoints(ma,ha,a,b)||!endpoints(mb,hb,c,d)){pc=0x30a2b0;goto done;}
 if(trim_first){for(unsigned k=0;k<3;k++)direction[k]=sub(b[k],a[k]);if(!normalize(direction,unit)){pc=0x101e28;goto done;}for(unsigned k=0;k<3;k++){float offset=mul(unit[k],ra);a[k]=add(a[k],offset);b[k]=sub(b[k],offset);if(!normal(a[k])||!normal(b[k])){pc=0x2bc470;goto done;}}ha=sub(ha,add(ra,ra));}
 /* 2BC470 trims first kind1 segment; 2BCF28 leaves both full. 2BC0F8 then samples the
  * larger-radius line (equal radii choose second), including raw short/negative
  * shortened height rather than a geometric clamp. */
 float *start=rb<ra?a:c,*end=rb<ra?b:d,*other_start=rb<ra?c:a,*other_end=rb<ra?d:b,large=rb<ra?ra:rb,length=rb<ra?ha:hb,sum=rb<ra?add(rb,ra):add(ra,rb),distance;
 if(!normal(ha)||!normal(sum)){pc=0x2bc0f8;goto done;}if(!point_segment_distance(start,other_start,other_end,&distance)){pc=0x1566c0;goto done;}if(distance<=sum){result=1;goto done;}if(!point_segment_distance(end,other_start,other_end,&distance)){pc=0x1566c0;goto done;}if(distance<=sum){result=1;goto done;}
 float ratio=divide(length,large);if(!normal(ratio)||ratio>4096){pc=0x2bc0f8;goto done;}uint32_t samples=ratio<0?0:(uint32_t)ratio;float step=divide(length,(float)(samples+1));if(!normal(step)){pc=0x2bc0f8;goto done;}for(unsigned k=0;k<3;k++)direction[k]=sub(end[k],start[k]);if(!normalize(direction,unit)){pc=0x101e28;goto done;}for(unsigned k=0;k<3;k++){unit[k]=mul(unit[k],step);if(!normal(unit[k])){pc=0x2bc0f8;goto done;}}
 for(uint32_t i=0;i<samples;i++){for(unsigned k=0;k<3;k++)start[k]=add(start[k],unit[k]);if(!point_segment_distance(start,other_start,other_end,&distance)){pc=0x1566c0;goto done;}if(distance<=sum){result=1;break;}}
 done:if(fesetenv(&env))return fail(f,0x2bc0f8);if(pc)return fail(f,pc);*hit=result;return GH_PAIR_QUERY_OK;
}

gh_pair_query_status gh_collision_query_cylinder_cylinder_002bc470(const uint8_t*a,uint32_t ra,uint32_t ha,const uint8_t*b,uint32_t rb,uint32_t hb,gh_static_transform_policy p,int*hit,uint32_t*f){return cylinder_pair_body(a,ra,ha,b,rb,hb,1,0x2bc470,p,hit,f);}
gh_pair_query_status gh_collision_query_kind2_pair_002bcf28(const uint8_t*a,uint32_t ra,uint32_t ha,const uint8_t*b,uint32_t rb,uint32_t hb,gh_static_transform_policy p,int*hit,uint32_t*f){return cylinder_pair_body(a,ra,ha,b,rb,hb,0,0x2bcf28,p,hit,f);}

static gh_pair_query_status cylinder_box_body(const uint8_t*cylinder_matrix,uint32_t radius,uint32_t height,const uint8_t*box_matrix,const uint32_t*full_size,int trim_first,uint32_t entry,gh_static_transform_policy policy,int*hit,uint32_t*f){
 if(f)*f=0;if(!cylinder_matrix||!box_matrix||!full_size||!hit)return GH_PAIR_QUERY_ARGUMENT;if(policy!=GH_STATIC_BINARY32_REFERENCE)return fail(f,entry);
 float cm[16],bm[16],r=val(radius),h=val(height),size[3];memcpy(cm,cylinder_matrix,64);memcpy(bm,box_matrix,64);for(unsigned k=0;k<16;k++)if(!normal(cm[k])||!normal(bm[k]))return fail(f,0x30a2b0);for(unsigned k=0;k<3;k++){size[k]=val(full_size[k]);if(!normal(size[k]))return fail(f,entry);}if(!normal(r)||!normal(h))return fail(f,trim_first?0x2bc6c8:0x2bd078);
 fenv_t env;if(feholdexcept(&env))return fail(f,0x2bc950);if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return fail(f,0x2bc950);}uint32_t pc=0;int result=0;float a[3],b[3],direction[3],unit[3],corners[8][3];
 if(!endpoints(cm,h,a,b)){pc=0x30a2b0;goto done;}if(trim_first){for(unsigned k=0;k<3;k++)direction[k]=sub(b[k],a[k]);if(!normalize(direction,unit)){pc=0x101e28;goto done;}for(unsigned k=0;k<3;k++){float offset=mul(unit[k],r);a[k]=add(a[k],offset);b[k]=sub(b[k],offset);if(!normal(a[k])||!normal(b[k])){pc=0x2bc6c8;goto done;}}h=sub(h,add(r,r));}if(!normal(h)){pc=trim_first?0x2bc6c8:0x2bd078;goto done;}if(!box_corners(bm,size,corners)){pc=0x30a2b0;goto done;}
 if(!sphere_poly(r,a,corners,&result)){pc=0x2bb868;goto done;}if(result)goto done;if(!sphere_poly(r,b,corners,&result)){pc=0x2bb868;goto done;}if(result)goto done;
 float ratio=divide(h,r);if(!normal(ratio)||ratio>4096){pc=0x2bc950;goto done;}uint32_t samples=ratio<0?0:(uint32_t)ratio;float step=divide(h,(float)(samples+1));if(!normal(step)){pc=0x2bc950;goto done;}for(unsigned k=0;k<3;k++)direction[k]=sub(b[k],a[k]);if(!normalize(direction,unit)){pc=0x101e28;goto done;}for(unsigned k=0;k<3;k++){unit[k]=mul(unit[k],step);if(!normal(unit[k])){pc=0x2bc950;goto done;}}
 for(uint32_t i=0;i<samples;i++){for(unsigned k=0;k<3;k++)a[k]=add(a[k],unit[k]);if(!sphere_poly(r,a,corners,&result)){pc=0x2bb868;goto done;}if(result)break;}
 done:if(fesetenv(&env))return fail(f,0x2bc950);if(pc)return fail(f,pc);*hit=result;return GH_PAIR_QUERY_OK;
}
gh_pair_query_status gh_collision_query_cylinder_box_002bcb80(const uint8_t*c,uint32_t r,uint32_t h,const uint8_t*b,const uint32_t*size,gh_static_transform_policy p,int*hit,uint32_t*f){return cylinder_box_body(c,r,h,b,size,1,0x2bcb80,p,hit,f);}
gh_pair_query_status gh_collision_query_kind2_box_002bd200(const uint8_t*c,uint32_t r,uint32_t h,const uint8_t*b,const uint32_t*size,gh_static_transform_policy p,int*hit,uint32_t*f){return cylinder_box_body(c,r,h,b,size,0,0x2bd200,p,hit,f);}

/* Original136A88 OBB15SAT. Keep source operation order and .001 padding,
 * including ascending-other-axis radius terms in the nine cross tests. */
static int obb_sat(const float ca[3],const float aa[3][3],const float ea[3],const float cb[3],const float ab[3][3],const float eb[3],int*hit,float*maximum){
 float r[3][3],absolute[3][3],delta[3],t[3];*maximum=0;float epsilon=val(0x3a83126f);
 for(unsigned i=0;i<3;i++)for(unsigned j=0;j<3;j++){r[i][j]=dot(aa[i],ab[j]);float v=r[i][j];if(v<0)v=-v;absolute[i][j]=add(v,epsilon);if(!normal(r[i][j])||!normal(absolute[i][j]))return 0;}
 for(unsigned k=0;k<3;k++){delta[k]=sub(cb[k],ca[k]);if(!normal(delta[k]))return 0;}for(unsigned i=0;i<3;i++){t[i]=dot(delta,aa[i]);if(!normal(t[i]))return 0;}
 for(unsigned i=0;i<3;i++){float projected=t[i];if(projected<0)projected=-projected;float radius=add(ea[i],mul(eb[0],absolute[i][0]));radius=add(radius,mul(eb[1],absolute[i][1]));radius=add(radius,mul(eb[2],absolute[i][2]));if(!normal(radius))return 0;if(radius<projected){*hit=0;return 1;}if(*maximum<radius)*maximum=radius;}
 for(unsigned j=0;j<3;j++){float projected=add(add(mul(t[0],r[0][j]),mul(t[1],r[1][j])),mul(t[2],r[2][j]));if(projected<0)projected=-projected;float radius=add(mul(ea[0],absolute[0][j]),mul(ea[1],absolute[1][j]));radius=add(radius,mul(ea[2],absolute[2][j]));radius=add(radius,eb[j]);if(!normal(projected)||!normal(radius))return 0;if(radius<projected){*hit=0;return 1;}if(*maximum<radius)*maximum=radius;}
 for(unsigned i=0;i<3;i++)for(unsigned j=0;j<3;j++){unsigned u=i==0?1:0,v=i==2?1:2,s=j==0?1:0,z=j==2?1:2;float projected=sub(mul(t[(i+2)%3],r[(i+1)%3][j]),mul(t[(i+1)%3],r[(i+2)%3][j]));if(projected<0)projected=-projected;float radius=add(mul(ea[u],absolute[v][j]),mul(ea[v],absolute[u][j]));radius=add(radius,mul(eb[s],absolute[i][z]));radius=add(radius,mul(eb[z],absolute[i][s]));if(!normal(projected)||!normal(radius))return 0;if(radius<projected){*hit=0;return 1;}if(*maximum<radius)*maximum=radius;}
 *hit=1;return 1;
}
gh_pair_query_status gh_collision_query_box_box_002bd828(const uint8_t*matrix_a,const uint32_t*size_a,const uint8_t*matrix_b,const uint32_t*size_b,gh_static_transform_policy policy,int*hit,uint32_t*f){
 if(f)*f=0;if(!matrix_a||!matrix_b||!size_a||!size_b||!hit)return GH_PAIR_QUERY_ARGUMENT;if(policy!=GH_STATIC_BINARY32_REFERENCE)return fail(f,0x2bd828);
 float a[16],b[16],sa[3],sb[3];memcpy(a,matrix_a,64);memcpy(b,matrix_b,64);for(unsigned k=0;k<16;k++)if(!normal(a[k])||!normal(b[k]))return fail(f,0x2bd828);for(unsigned k=0;k<3;k++){sa[k]=val(size_a[k]);sb[k]=val(size_b[k]);if(!normal(sa[k])||!normal(sb[k]))return fail(f,0x2bd828);}
 fenv_t env;if(feholdexcept(&env))return fail(f,0x136a88);if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return fail(f,0x136a88);}float ca[3],cb[3],aa[3][3],ab[3][3],ea[3],eb[3],scratch;int result=0,valid=1;
 for(unsigned k=0;k<3;k++){ea[k]=mul(sa[k],0.5f);eb[k]=mul(sb[k],0.5f);valid=valid&&normal(ea[k])&&normal(eb[k]);}for(unsigned k=0;k<3;k++){float v=mul(a[k],0);v=add(v,mul(a[4+k],ea[1]));v=add(v,mul(a[8+k],0));ca[k]=add(v,mul(a[12+k],1));v=mul(b[k],0);v=add(v,mul(b[4+k],eb[1]));v=add(v,mul(b[8+k],0));cb[k]=add(v,mul(b[12+k],1));valid=valid&&normal(ca[k])&&normal(cb[k]);}
 for(unsigned i=0;i<3&&valid;i++){valid=normalize(a+i*4,aa[i])&&normalize(b+i*4,ab[i]);}
 if(valid)valid=obb_sat(ca,aa,ea,cb,ab,eb,&result,&scratch);if(fesetenv(&env))return fail(f,0x136a88);if(!valid)return fail(f,0x136a88);*hit=result;return GH_PAIR_QUERY_OK;
}
