#include "game_actor_motion.h"
#include "game_actor_motion_ik.h"
#include <string.h>
#include <math.h>
static uint32_t rd(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static unsigned half(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static void wr(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static float scalar(const uint8_t*p){float x;memcpy(&x,p,4);return x;}
static int span(size_t n,size_t o,size_t z){return o<=n&&z<=n-o;}
static gh_actor_motion_status fail(gh_actor_motion*s,uint32_t pc,uint32_t detail){s->frontier=pc;s->detail=detail;return GH_ACTOR_MOTION_UNSUPPORTED;}
static gh_actor_motion_status effect(gh_actor_motion*s,int32_t kind,uint32_t mode){
 if(!s->b.effect)return fail(s,0x306200,(uint32_t)kind);return s->b.effect(s->b.context,&s->b.model->root,kind,mode,&s->frontier);
}
static gh_actor_motion_status reset_pose(gh_actor_motion*s){
 gh_actor_model*m=s->b.model;gh_model_mesh mesh;if(gh_model_mesh_at(&m->script,0,&mesh))return GH_ACTOR_MOTION_BOUNDS;
 size_t base=mesh.data_offset;if(!span(m->script.size,base,12))return GH_ACTOR_MOTION_BOUNDS;
 uint32_t rel=rd(m->script.bytes+base+4);size_t positions=base+rel;
 if(positions<base||!span(m->script.size,positions,(size_t)m->part_count*16))return GH_ACTOR_MOTION_BOUNDS;
 for(unsigned i=0;i<m->part_count;i++){gh_actor_part*p=&m->parts[i];if(!p->bytes||!p->alias_d0)return GH_ACTOR_MOTION_ARGUMENT;uint32_t flags=rd(p->bytes+0x154);
  if(flags&1)continue;
  if(!(flags&0x200)){memcpy(p->bytes+0xe0,m->script.bytes+positions+(size_t)i*16,12);memcpy(p->alias_d0,p->bytes+0xe0,12);}
  if(!(flags&0x100))memset(p->bytes+0x100,0,12);
  if(!(flags&0x400))for(unsigned a=0;a<3;a++)wr(p->bytes+0x110+a*4,0x3f800000);
 }
 return GH_ACTOR_MOTION_OK;
}
gh_actor_motion_status gh_actor_motion_begin_002a8578(gh_actor_motion*s,const gh_actor_motion_bindings*b,size_t motion,size_t sequence,uint16_t flags){
 if(!s||!b||!b->model||!b->model->parts_ready||!b->model->root.bytes||b->model->root.size<0x5ac||b->model->part_count>255||!b->resource||!b->work)return GH_ACTOR_MOTION_ARGUMENT;
 uint8_t*p=b->model->root.bytes;
 if(s->bound&&(s->b.model!=b->model||s->b.resource!=b->resource||s->b.work!=b->work||s->motion.offset!=motion||s->sequence_offset!=sequence||s->flags!=flags))return GH_ACTOR_MOTION_BUSY;
 s->frontier=0;s->detail=0;
 if(!s->bound){
  if(flags)return fail(s,0x157568,flags);
  if(rd(p+0x428)||rd(p+0x42c))return fail(s,0x2a84a8,0x42c);
  if(!isfinite(scalar(p+0x5a8))||scalar(p+0x5a8)<=0||!isfinite(scalar(p+0x34c)))return fail(s,0x157568,0x5a8);
  gh_motion_view view;if(gh_motion_open_mtb3(b->resource,b->resource_bytes,motion,&view))return GH_ACTOR_MOTION_BOUNDS;
  if(sequence!=SIZE_MAX&&!span(b->resource_bytes,sequence,4))return GH_ACTOR_MOTION_BOUNDS;
  int sequence_valid=sequence!=SIZE_MAX&&!memcmp(b->resource+sequence,"SEQ\0",4);
  if(sequence_valid&&!span(b->resource_bytes,sequence,0x18))return GH_ACTOR_MOTION_BOUNDS;
  s->b=*b;s->motion=view;s->sequence_offset=sequence;s->flags=flags;s->sequence_valid=(uint8_t)sequence_valid;s->bound=1;
 }
 if(s->complete)return GH_ACTOR_MOTION_OK;
 if(!s->stage){int kind=(int16_t)half(p+0x570);if(kind!=-1){gh_actor_motion_status e=effect(s,kind,rd(p+0x574));if(e)return e;}
  p[0x570]=p[0x571]=0xff;wr(p+0x574,2);s->stage=1;}
 if(s->stage==1){
  /* Previous42C was NULL above, so originalSEQ release loop is genuinelyempty.
   * NewSEQ is validated and borrowed only AFTER that original2A84A8 call. */
  wr(p+0x428,0);wr(p+0x42c,0);wr(p+0x430,0);p[0x434]=p[0x435]=p[0x436]=0;wr(p+0x440,0x3f800000);wr(p+0x444,rd(p+0x5a8));wr(p+0x438,0);wr(p+0x43c,0);
  const unsigned vectors[]={0x310,0x320,0x330,0x340,0x350,0x360};for(unsigned i=0;i<6;i++)memset(p+vectors[i],0,12);wr(p+0x35c,rd(p+0x34c));wr(p+0x27c,0);s->stage=2;
 }
 if(s->stage==2){gh_actor_motion_status e=reset_pose(s);if(e)return e;s->stage=3;}
 if(s->stage==3){
  for(unsigned i=0;i<s->motion.count;i++){gh_motion_track track;if(gh_motion_track_at(&s->motion,i,&track))return GH_ACTOR_MOTION_BOUNDS;
   if(track.part==0x7f)break;float value;uint32_t f=0;gh_motion_status e=gh_motion_sample_reference(&s->motion,i,0,&value,&s->caches[i],&f);
   if(e)return fail(s,f?f:0x1595f8,i);
   if((track.method==1||track.method==3||track.method==5)&&track.keys<=1){s->caches[i].method=0;s->caches[i].first_value=0;}
  }s->stage=4;
 }
 if(s->stage==4){wr(p+0x448,0x3f800000);wr(p+0x250,rd(p+0x250)&~0x20u);
  for(unsigned i=0;i<b->model->part_count;i++){uint8_t*q=b->model->parts[i].bytes;wr(q+0x158,0);wr(q+0x154,rd(q+0x154)&0xffb01ff7u);}s->stage=5;}
 if(s->stage==5&&b->ik_globals){unsigned part=0;gh_actor_motion_status e=gh_actor_motion_ik_00152448(b->model,&s->motion,b->ik_globals,GH_STATIC_BINARY32_REFERENCE,&s->frontier,&part);s->detail=part;if(e)return e;s->complete=1;return GH_ACTOR_MOTION_OK;}
 if(s->stage==5){
  while(s->release_index<b->model->part_count){unsigned i=(unsigned)s->release_index;uint32_t control=0;
   for(unsigned k=0;k<s->motion.count;k++){gh_motion_track t;if(gh_motion_track_at(&s->motion,k,&t))return GH_ACTOR_MOTION_BOUNDS;if(t.part==0x7f||t.part==i){control=t.control;break;}}
   if((control&255)!=0)return fail(s,0x152448,i); /* actualIK dependency, not a sampled bindpose */
   s->release_index++;
  }s->complete=1;
 }
 return GH_ACTOR_MOTION_OK;
}
