/* Actual cIDBase_move2AAA40 / trans2AAC28 and calcEntries2AA6E0.
 * Reconstruction reference MIT upstream src/cod/id/cIDBase.c and002aa6e0.c;
 * notice LICENSE.upstream. No native draw success invented by collecting jobs. */
#include "id_jobs.h"
#include <stdint.h>
#include <math.h>
#include <string.h>
static uint32_t u32(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static int s16(const unsigned char *p){unsigned n=p[0]|(unsigned)p[1]<<8;return n>=32768?(int)n-65536:(int)n;}
static float f32(const unsigned char *p){uint32_t n=u32(p);float f;memcpy(&f,&n,4);return f;}
/* Keep original Hermite operation boundaries; no algebraic linear shortcut.
 * Host finite binary32 path, not a claim of a general EE FPU implementation. */
static float mul(float a,float b){volatile float result=a*b;return result;}
static float add(float a,float b){volatile float result=a+b;return result;}
static gh_id_error color_step(gh_id_entry *e){
 const unsigned char *src=e->source,*keys=src+0x27c;int n=s16(src+0x278),last,idx=-1,t;float sample=0;
 uint16_t tick=(uint16_t)e->jitter[4];uint32_t color=0;unsigned c;
 if(n<2||n>8)return GH_ID_BOUNDS;last=s16(keys+(size_t)(n-1)*12+8);
 if(last<(int)tick){if(u32(src+4)&0x10000)tick=0;else{e->flags|=0x400;return GH_ID_OK;}}
 t=tick>=32768?(int)tick-65536:(int)tick;
 if(s16(keys+8)<t){int j;idx=n;for(j=1;j<n;j++)if(!(s16(keys+(size_t)j*12+8)<t)){idx=j-1;break;}}
 if(idx>=0){
  const unsigned char *p,*q;int delta;float a,t2,t3,a3,b2,c1,h01,h10,ease,from;
  if(idx>=n-1)return GH_ID_BOUNDS;p=keys+(size_t)idx*12;q=p+12;delta=s16(q+8)-s16(p+8);if(!delta)return GH_ID_BOUNDS;
  a=(float)(t-s16(p+8))/(float)delta;t2=mul(a,a);t3=mul(t2,a);a3=mul(t2,3);b2=add(t3,t3);c1=add(t3,-t2);
  h01=add(-b2,a3);h10=add(add(c1,-t2),a);
  ease=add(add(h01,mul(mul((float)s16(p+6),1.0f/4096),h10)),mul(mul((float)s16(q+4),1.0f/4096),c1));
  from=f32(p);sample=add(from,mul(add(f32(q),-from),ease));
 }
 if(!isfinite(sample))return GH_ID_UNSUPPORTED;
 for(c=0;c<4;c++){
  float value=add(mul((float)src[0x50+c],add(1,-sample)),mul((float)src[0x2dc+c],sample));
  if(!isfinite(value)||value<0||value>=2147483648.0f)return GH_ID_UNSUPPORTED;
  color|=((uint32_t)value&255)<<(8*c);
 }
 e->color=color;if(!(e->flags&0x20))tick=(uint16_t)(tick+1);e->jitter[4]=(int16_t)(tick<32768?(int)tick:(int)tick-65536);return GH_ID_OK;
}
static int owned(const gh_id_base *o,const gh_id_entry *e){
 uintptr_t base=(uintptr_t)o->entries,p=(uintptr_t)e;size_t extent=(size_t)o->entry_count*sizeof(*e);
 return p>=base&&p-base<extent&&(p-base)%sizeof(*e)==0;
}
static gh_id_error frontier(gh_id_base *o,gh_id_frontier *f,uint32_t i,uint32_t address){
 o->first_unported=address;if(f){f->entry_index=i;f->address=address;f->kind=o->entries[i].kind;}return GH_ID_UNSUPPORTED;
}
static gh_id_error layout(gh_id_base *o,gh_id_frontier *f){
 uint32_t i;for(i=0;i<o->entry_count;i++){
  gh_id_entry *e=&o->entries[i],*p;float sx=e->scale_x,sy=e->scale_y,offx=e->off_x,offy=e->off_y;unsigned steps=0;
  e->draw_rotation=0;e->draw_scale_x=sx;e->draw_scale_y=sy;
  if(e->parent_ref>=0)for(p=e->parent;p;p=p->parent){
   if(!owned(o,p)||++steps>o->entry_count)return GH_ID_BOUNDS;
   if(e->flags&0x40000000U){e->draw_scale_x*=p->scale_x;e->draw_scale_y*=p->scale_y;}
   sx*=p->scale_x;sy*=p->scale_y;
   if(!(e->flags&0x80)&&p->rotation_speed!=0)return frontier(o,f,i,0x2aa6e0);
  }
  e->rotation_x=e->pos_x;e->rotation_y=e->pos_y;
  if(e->flags&0x04000000){e->rotation_x*=sx;e->rotation_y*=sy;}
  if(e->rotation_speed!=0)return frontier(o,f,i,0x2aa6e0);
  e->draw_x=e->rotation_x;e->draw_y=e->rotation_y;e->color=e->color_base;steps=0;
  if(e->parent_ref>=0)for(p=e->parent;p;p=p->parent){
   unsigned c;if(!owned(o,p)||++steps>o->entry_count)return GH_ID_BOUNDS;
   if(p->flags&0x10)e->kind=p->kind;
   e->draw_x+=p->rotation_x;e->draw_y+=p->rotation_y;offx+=p->off_x;offy+=p->off_y;
   if(!(p->flags&0x10000000)&&!(e->flags&0x10000000)){
    uint32_t color=0;for(c=0;c<4;c++){unsigned a=(e->color>>(c*8))&255,b=(p->color_base>>(c*8))&255;color|=((a*b/128)&255)<<(c*8);}e->color=color;
   }
  }
  e->draw_x+=offx;e->draw_y+=offy;
 }
 return GH_ID_OK;
}
gh_id_error gh_id_jobs_move(gh_id_base *o,gh_id_post_message post,void *ctx,gh_id_frontier *f){
 uint32_t i;gh_id_error error;static const uint32_t source_bits[]={0x02000000,0x01000000,0x00800000,0x00400000,0x00200000};
 static const uint32_t done_bits[]={0x4000,0x2000,0x1000,0x800,0x400};
 static const unsigned hold[]={0x5c,0x5d,0x5e,0x5f,0x64};
 static const uint32_t entry[]={0x2abdc0,0x2ac048,0x2ac1c0,0x2ac298,0x2ac378};
 if(!o)return GH_ID_ARGUMENT;if(f){f->address=0;f->entry_index=0;f->kind=0;}o->first_unported=0;
 if(o->stop||!o->entries)return GH_ID_OK;
 error=layout(o,f);if(error)return error;if(!o->playing)return GH_ID_OK;
 o->frame=(uint16_t)(o->frame+1);if(o->frame<0x8000&&o->frame>=10000)o->frame=0;
 for(i=0;i<o->entry_count;i++){
  gh_id_entry *e=&o->entries[i];unsigned j;uintptr_t src=(uintptr_t)e->source,base=(uintptr_t)o->source_records.bytes;
  if(!e->source||src<base||src-base>o->source_records.size||o->source_records.size-(size_t)(src-base)<0x2e0)return GH_ID_BOUNDS;
  for(j=0;j<5;j++)if((u32(e->source+4)&source_bits[j])&&!(e->flags&done_bits[j])&&!e->source[hold[j]]){
   if(j==4){error=color_step(e);if(error)return frontier(o,f,i,entry[j]);}
   else return frontier(o,f,i,entry[j]);
  }
  if(e->kind==4&&!(e->flags&2)&&e->message!=65535){
   if(!post)return frontier(o,f,i,0x2aeb68);
   error=post(ctx,e->message);if(error)return error;e->flags|=2;
  }
 }
 return GH_ID_OK;
}
gh_id_error gh_id_jobs_collect_visible(gh_id_base *o,gh_id_draw_job *jobs,size_t capacity,size_t *count,gh_id_frontier *f){
 int layer;uint32_t i;size_t n=0;if(!o||!count||(!jobs&&capacity))return GH_ID_ARGUMENT;*count=0;
 if(f){f->address=0;f->entry_index=0;f->kind=0;}
 if(o->hide||!o->entries)return GH_ID_OK;
 for(layer=4;layer>=-4;layer--)for(i=0;i<o->entry_count;i++){
  gh_id_entry *e=&o->entries[i],*p;unsigned steps=0;int hidden=0;
  if(e->layer!=layer||(e->flags&0x20000000))continue;
  if(e->parent_ref>=0)for(p=e->parent;p;p=p->parent){if(!owned(o,p)||++steps>o->entry_count)return GH_ID_BOUNDS;if((p->flags&8)&&(p->flags&0x08000000))hidden=1;}
  if(hidden||(e->flags&0x08000000))continue;
  if(n>=capacity)return GH_ID_CAPACITY;
  jobs[n].entry_index=i;jobs[n].entry=e;jobs[n].consumer_address=e->kind==1?0x2aaff0:e->kind==2?0x2ab0b8:e->kind==4?0x2ab2a0:e->kind==3?0x2ab360:0x2aaef0;n++;
 }
 *count=n;return GH_ID_OK;
}
