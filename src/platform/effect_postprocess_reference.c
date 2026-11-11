#include "effect_postprocess_reference.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

static int64_t floor_div(int64_t a,int64_t b){int64_t q=a/b,r=a%b;return q-(r<0);}
static int surface_valid(const gh_effect_post_surface*s){
 if(!s||!s->rgba||!s->width||!s->height||s->width>2048||s->height>2048||s->stride<(size_t)s->width*4)return 0;
 return s->height-1<=SIZE_MAX/s->stride&&(size_t)(s->height-1)*s->stride<=s->bytes&&(size_t)s->width*4<=s->bytes-(size_t)(s->height-1)*s->stride;
}
static gh_effect_post_status sampler(const gh_effect_post_sampler*s,int*linear){
 if(!s||!s->known||s->policy!=GH_EFFECT_SAMPLE_NATIVE_SEPARABLE_4BIT||s->clamp)return GH_EFFECT_POST_UNSUPPORTED;
 unsigned mag=(unsigned)(s->tex1>>5)&1,min=(unsigned)(s->tex1>>6)&7;
 if(((s->tex1>>2)&7)||(mag?!(min==1||min==4||min==5):!(min==0||min==2||min==3)))return GH_EFFECT_POST_UNSUPPORTED;
 *linear=(int)mag;return GH_EFFECT_POST_OK;
}
static gh_effect_post_status texel(const gh_effect_post_surface*s,unsigned w,unsigned h,int64_t x,int64_t y,uint8_t c[4]){
 x%=w;y%=h;if(x<0)x+=w;if(y<0)y+=h;
 if((uint64_t)x>=s->width||(uint64_t)y>=s->height)return GH_EFFECT_POST_BOUNDS;
 memcpy(c,s->rgba+(size_t)y*s->stride+(size_t)x*4,4);return GH_EFFECT_POST_OK;
}
gh_effect_post_status gh_effect_post_sample(const gh_effect_post_surface*s,uint32_t w,uint32_t h,
 const gh_effect_post_sampler*state,int64_t u,int64_t v,uint8_t out[4]){
 if(!surface_valid(s)||!out||!w||!h||w>1024||h>1024||(w&(w-1))||(h&(h-1))||u<INT32_MIN||u>INT32_MAX||v<INT32_MIN||v>INT32_MAX)return GH_EFFECT_POST_ARGUMENT;
 int linear;gh_effect_post_status e=sampler(state,&linear);if(e)return e;
 uint8_t result[4];if(!linear){e=texel(s,w,h,floor_div(u,16),floor_div(v,16),result);if(e)return e;}
 else{
  u-=8;v-=8;int64_t x=floor_div(u,16),y=floor_div(v,16);unsigned fx=(unsigned)(u-x*16),fy=(unsigned)(v-y*16);
  uint8_t rows[2][4];for(unsigned row=0;row<(fy?2u:1u);row++){
   uint8_t left[4],right[4];e=texel(s,w,h,x,y+row,left);if(e)return e;
   if(fx){e=texel(s,w,h,x+1,y+row,right);if(e)return e;}
   for(unsigned k=0;k<4;k++)rows[row][k]=(uint8_t)(fx?((unsigned)left[k]*(16-fx)+(unsigned)right[k]*fx)/16:left[k]);
  }
  for(unsigned k=0;k<4;k++)result[k]=(uint8_t)(fy?((unsigned)rows[0][k]*(16-fy)+(unsigned)rows[1][k]*fy)/16:rows[0][k]);
 }
 memcpy(out,result,4);return GH_EFFECT_POST_OK;
}
typedef struct {int64_t x0,y0,x1,y1;int64_t u0,v0,u1,v1;uint32_t rgba;} rectangle;
static int rect(const gh_effect_trans_quad*q,int clear,rectangle*r){
 if(q->vertex_count!=(clear?2:4))return 0;
 const gh_effect_trans_vertex*a=q->vertices,*d=q->vertices+(clear?1:3);
 *r=(rectangle){a->x12_4,a->y12_4,d->x12_4,d->y12_4,a->u12_4,a->v12_4,d->u12_4,d->v12_4,a->rgba};
 if(r->x1<=r->x0||r->y1<=r->y0)return 0;
 for(unsigned i=0;i<q->vertex_count;i++){
  uint32_t qbits;memcpy(&qbits,&q->vertices[i].q,4);
  if(qbits!=0x3f800000u||q->vertices[i].rgba!=r->rgba||q->vertices[i].z!=(clear?0xffffu:0u))return 0;
 }
 if(!clear){const gh_effect_trans_vertex*b=a+1,*c=a+2;if(b->x12_4!=d->x12_4||b->y12_4!=a->y12_4||c->x12_4!=a->x12_4||c->y12_4!=d->y12_4||b->u12_4!=d->u12_4||b->v12_4!=a->v12_4||c->u12_4!=a->u12_4||c->v12_4!=d->v12_4)return 0;}
 return 1;
}
static int known_packet(const gh_effect_trans_packet*p){
 if(p->zbuf!=UINT64_C(0x13a0000f8)||!p->restores_scene_target||!p->texture_flush_after||p->restore_frame_mask||p->restore_zbuf!=UINT64_C(0x3a0000f8)||p->quad_count>25||!p->quad_count)return 0;
 if(p->kind==GH_EFFECT_PASS_CLEAR)return p->original_callee==0x2e9808&&p->prim==6&&p->quad_count==1&&!p->frame_mask&&!p->writes_tex0&&p->alpha==UINT64_C(0x8000000044)&&p->test==0x30000&&p->destination>=GH_EFFECT_SURFACE_3900&&p->destination<=GH_EFFECT_SURFACE_3B00;
 if(p->prim!=0x15c||!p->writes_tex0||!p->inherited_sampler)return 0;
 if(p->kind==GH_EFFECT_PASS_DOWNSAMPLE)return p->original_callee==0x2e8b38&&p->source==GH_EFFECT_SURFACE_SCENE_COLOR&&p->destination==GH_EFFECT_SURFACE_3900&&p->tex0==UINT64_C(0x2000000664020e00)&&p->alpha==UINT64_C(0x8000000048)&&p->test==0x3080d&&!p->frame_mask&&p->quad_count==1;
 if(p->kind==GH_EFFECT_PASS_GAUSSIAN)return p->original_callee==0x2e83d8&&p->alpha==UINT64_C(0x8000000068)&&p->test==0x30000&&!p->frame_mask&&p->quad_count==25&&((p->source==GH_EFFECT_SURFACE_3900&&p->destination==GH_EFFECT_SURFACE_3A00&&p->tex0==(UINT64_C(0x20000005dc008000)|0x3900u))||(p->source==GH_EFFECT_SURFACE_3A00&&p->destination==GH_EFFECT_SURFACE_3B00&&p->tex0==(UINT64_C(0x20000005dc008000)|0x3a00u)));
 if(p->kind==GH_EFFECT_PASS_COMPOSITE)return p->original_callee==0x2e8ee8&&p->source==GH_EFFECT_SURFACE_3B00&&p->destination==GH_EFFECT_SURFACE_SCENE_COLOR&&p->tex0==(UINT64_C(0x20000005dc008000)|0x3b00u)&&p->alpha==UINT64_C(0x8000000068)&&p->test==0x3000a&&p->frame_mask==0xff000000u&&p->quad_count==1;
 return 0;
}
static int alpha_pass(uint64_t t,unsigned a){if(!(t&1))return 1;unsigned r=(unsigned)(t>>4)&255;switch((t>>1)&7){case 0:return 0;case 1:return 1;case 2:return a<r;case 3:return a<=r;case 4:return a==r;case 5:return a>=r;case 6:return a>r;default:return a!=r;}}
static int select_color(unsigned selector,int s,int d){return selector==0?s:selector==1?d:0;}
static int overlap(const gh_effect_post_surface*a,const gh_effect_post_surface*b){uintptr_t x=(uintptr_t)a->rgba,y=(uintptr_t)b->rgba;return x<=y?y-x<a->bytes:x-y<b->bytes;}
gh_effect_post_status gh_effect_postprocess_apply(const gh_effect_trans_packet*p,const gh_effect_post_sampler*state,
 gh_effect_post_context*c,gh_effect_post_receipt*out,uint32_t*f){
 uint32_t local=0;if(!f)f=&local;*f=0;
 if(!p||!c||!out)return GH_EFFECT_POST_ARGUMENT;
 *f=p->original_callee;
 if(!known_packet(p)||c->frame_psm||c->colclamp!=1||c->dthe||c->fba||c->visible_width!=512||c->visible_height!=448||c->xy_offset_x16>65535||c->xy_offset_y16>65535)return GH_EFFECT_POST_UNSUPPORTED;
 if((unsigned)p->destination>=4)return GH_EFFECT_POST_ARGUMENT;
 gh_effect_post_surface*dst=c->surfaces+p->destination;const gh_effect_post_surface*src=NULL;
 if(!surface_valid(dst))return GH_EFFECT_POST_BOUNDS;
 unsigned tw=0,th=0;if(p->kind!=GH_EFFECT_PASS_CLEAR){
  if((unsigned)p->source>=4)return GH_EFFECT_POST_ARGUMENT;src=c->surfaces+p->source;
  if(!surface_valid(src))return GH_EFFECT_POST_BOUNDS;
  if(overlap(src,dst))return GH_EFFECT_POST_UNSUPPORTED;
  int ignored;gh_effect_post_status e=sampler(state,&ignored);if(e)return e;
  tw=1u<<((p->tex0>>26)&15);th=1u<<((p->tex0>>30)&15);
 }
 rectangle rectangles[25];for(unsigned q=0;q<p->quad_count;q++){
  rectangle*r=rectangles+q;if(!rect(p->quads+q,p->kind==GH_EFFECT_PASS_CLEAR,r))return GH_EFFECT_POST_UNSUPPORTED;
  int64_t width=r->x1-r->x0,height=r->y1-r->y0;
  if(p->kind==GH_EFFECT_PASS_CLEAR){if(width!=128*16||height!=128*16||r->rgba)return GH_EFFECT_POST_UNSUPPORTED;}
  else{
   if((r->rgba>>24)!=128||((r->rgba>>8)&255)!=(r->rgba&255)||((r->rgba>>16)&255)!=(r->rgba&255))return GH_EFFECT_POST_UNSUPPORTED;
   if(p->kind==GH_EFFECT_PASS_DOWNSAMPLE){if(width!=127*16||height!=112*16||r->u0!=8||r->v0!=8||r->u1!=512*16+8||r->v1!=448*16+8||r->rgba!=0x80808080u)return GH_EFFECT_POST_UNSUPPORTED;}
   else if(p->kind==GH_EFFECT_PASS_COMPOSITE){if(width!=512*16||height!=448*16||r->u0!=8||r->v0!=8||r->u1!=127*16+8||r->v1!=112*16+8)return GH_EFFECT_POST_UNSUPPORTED;}
   else if(width>127*16||height>112*16||r->u0<8||r->v0<8||r->u1>127*16+8||r->v1>112*16+8||r->u1-r->u0!=width||r->v1-r->v0!=height)return GH_EFFECT_POST_UNSUPPORTED;
  }
 }
 size_t row_bytes=(size_t)dst->width*4,n=row_bytes*dst->height;uint8_t*copy=malloc(n);if(!copy)return GH_EFFECT_POST_MEMORY;
 for(unsigned y=0;y<dst->height;y++)memcpy(copy+(size_t)y*row_bytes,dst->rgba+(size_t)y*dst->stride,row_bytes);
 gh_effect_post_receipt receipt={.quads=p->quad_count,.destination=p->destination,.restores_scene_target=p->restores_scene_target,.texture_flush_after=p->texture_flush_after,.restore_frame_mask=p->restore_frame_mask,.restore_zbuf=p->restore_zbuf};
 gh_effect_post_status status=GH_EFFECT_POST_OK;
 for(unsigned q=0;q<p->quad_count;q++){
  const rectangle*r=rectangles+q;
  for(unsigned y=0;y<dst->height;y++)for(unsigned x=0;x<dst->width;x++){
   int64_t px=(int64_t)x*16+c->xy_offset_x16,py=(int64_t)y*16+c->xy_offset_y16;
   if(px<r->x0||px>=r->x1||py<r->y0||py>=r->y1)continue;
   receipt.covered_fragments++;uint8_t source[4];
   if(src){
    int64_t u=floor_div(r->u0*(r->x1-r->x0)+(r->u1-r->u0)*(px-r->x0),r->x1-r->x0);
    int64_t v=floor_div(r->v0*(r->y1-r->y0)+(r->v1-r->v0)*(py-r->y0),r->y1-r->y0);
    status=gh_effect_post_sample(src,tw,th,state,u,v,source);if(status)goto done;
    for(unsigned k=0;k<4;k++){unsigned value=(unsigned)source[k]*((r->rgba>>(k*8))&255)/128;source[k]=(uint8_t)(value>255?255:value);}
   }else for(unsigned k=0;k<4;k++)source[k]=(uint8_t)(r->rgba>>(k*8));
   if(!alpha_pass(p->test,source[3]))continue; /* actual packets useAFAIL_KEEP */
   uint8_t*pixel=copy+(size_t)y*row_bytes+(size_t)x*4,new_color[4];memcpy(new_color,source,4);
   if(p->prim&64){
    unsigned a=(unsigned)p->alpha&3,b=(unsigned)(p->alpha>>2)&3,cs=(unsigned)(p->alpha>>4)&3,d=(unsigned)(p->alpha>>6)&3;
    int factor=cs==0?source[3]:cs==1?pixel[3]:(int)(p->alpha>>32)&255;
    for(unsigned k=0;k<3;k++){
     int64_t difference=select_color(a,source[k],pixel[k])-select_color(b,source[k],pixel[k]);
     int64_t color=floor_div(difference*factor,128)+select_color(d,source[k],pixel[k]);
     new_color[k]=(uint8_t)(color<0?0:color>255?255:color);
    }
   }
   for(unsigned k=0;k<4;k++){unsigned mask=(p->frame_mask>>(k*8))&255;pixel[k]=(uint8_t)((new_color[k]&~mask)|(pixel[k]&mask));}
   receipt.color_writes++;
  }
 }
done:
 if(!status){for(unsigned y=0;y<dst->height;y++)memcpy(dst->rgba+(size_t)y*dst->stride,copy+(size_t)y*row_bytes,row_bytes);*out=receipt;*f=0;}
 free(copy);return status;
}

gh_effect_post_status gh_effect_postprocess_capture_scene(const gh_effect_trans_packet *p,
 const gh_effect_post_sampler *state,const gh_effect_post_surface *scene,
 gh_effect_post_surface *capture,uint32_t x_origin,uint32_t y_origin,int *first_row_known)
{
 if(!p||!first_row_known||!surface_valid(scene)||!surface_valid(capture))return GH_EFFECT_POST_ARGUMENT;
 if(p->kind!=GH_EFFECT_PASS_SCENE_CAPTURE||p->original_callee!=0x2fa620||
    p->source!=GH_EFFECT_SURFACE_SCENE_COLOR||p->destination!=GH_EFFECT_SURFACE_3C00||
    p->quad_count!=8||p->prim!=0x116||p->test!=0x30000||
    p->tex0!=UINT64_C(0x2000000664020e00)||p->alpha!=UINT64_C(0x8000000044)||
    p->zbuf!=UINT64_C(0x13a0000f8)||p->frame_mask!=0xff000000u||
    !p->writes_tex0||p->writes_texa||p->writes_clamp||!p->inherited_sampler||!p->texture_flush_after||
    !p->restores_scene_target||p->restore_frame_mask||p->restore_zbuf!=0x3a0000f8||
    x_origin!=28672||y_origin!=29184||overlap(scene,capture))
  return GH_EFFECT_POST_UNSUPPORTED;
 if(scene->width!=512||scene->height!=448||capture->width!=256||capture->height!=448)
  return GH_EFFECT_POST_BOUNDS;
 for(unsigned strip=0;strip<8;strip++){
  const gh_effect_trans_quad *quad=p->quads+strip;
  if(quad->vertex_count!=2)return GH_EFFECT_POST_UNSUPPORTED;
  for(unsigned end=0;end<2;end++){
   const gh_effect_trans_vertex *vertex=quad->vertices+end;
   if(vertex->x12_4!=(int32_t)(x_origin+(strip+end)*512)||
      vertex->y12_4!=(int32_t)(y_origin+end*7168)||
      vertex->u12_4!=(strip+end)*1024+8||vertex->v12_4!=end*7168+4||
      vertex->rgba!=0x80808080u||vertex->z!=0xffffu||vertex->q!=1)
    return GH_EFFECT_POST_UNSUPPORTED;
  }
 }
 int linear;
 gh_effect_post_status status=sampler(state,&linear);
 if(status)return status;
 uint8_t boundary[4];
 unsigned first_y=linear?1u:0u;
 status=gh_effect_post_sample(scene,512,512,state,8,first_y*16+4,boundary);
 if(status)return status;
 status=gh_effect_post_sample(scene,512,512,state,255*32+8,447*16+4,boundary);
 if(status)return status;
 for(unsigned y=first_y;y<448;y++){
  for(unsigned x=0;x<256;x++){
   uint8_t rgba[4];
   status=gh_effect_post_sample(scene,512,512,state,x*32+8,y*16+4,rgba);
   if(status)return status;
   memcpy(capture->rgba+(size_t)y*capture->stride+x*4,rgba,3);
  }
 }
 *first_row_known=!first_y;
 return GH_EFFECT_POST_OK;
}
