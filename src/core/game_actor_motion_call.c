#include "game_actor_motion_call.h"
#include "game_actor_motion_ik.h"
#include <string.h>
static uint32_t rd(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static unsigned half(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static void wr(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static int span(size_t n,size_t o,size_t z){return o<=n&&z<=n-o;}
static gh_actor_motion_status fail(gh_actor_motion_call*s,uint32_t pc,gh_actor_motion_status e){s->frontier=pc;return e;}
gh_actor_motion_status gh_actor_motion_call_open_parameters_002a8578(gh_actor_motion_call*s,gh_actor_motion*current,const gh_actor_motion_bindings*b,size_t motion,size_t candidate,uint32_t start_frame_bits,uint32_t blend_frames,uint16_t flags,int release_previous){
 if(!s||!current||!b||!b->model||!b->work||!b->resource)return GH_ACTOR_MOTION_ARGUMENT;
 if((s->active&&!s->complete)||!current->bound||!current->complete||current->step_stage)return GH_ACTOR_MOTION_BUSY;
 if(current->b.model!=b->model||current->b.work!=b->work)return GH_ACTOR_MOTION_ARGUMENT;
 if(release_previous!=0&&release_previous!=1)return GH_ACTOR_MOTION_ARGUMENT;
 if(flags||start_frame_bits)return fail(s,0x157568,GH_ACTOR_MOTION_UNSUPPORTED);
 if(blend_frames&&b->model->native_identity!=b->model)return GH_ACTOR_MOTION_ARGUMENT;
 if(blend_frames&&(!b->ik_globals||!b->ik_globals->initialized||b->ik_globals!=current->b.ik_globals||(b->model->blend_globals&&b->model->blend_globals!=b->ik_globals)))return fail(s,0x156d70,GH_ACTOR_MOTION_UNSUPPORTED);
 gh_motion_view view;if(gh_motion_open_mtb3(b->resource,b->resource_bytes,motion,&view)|| (candidate!=SIZE_MAX&&!span(b->resource_bytes,candidate,4)))return GH_ACTOR_MOTION_BOUNDS;
 *s=(gh_actor_motion_call){.current=current,.next=*b,.motion_offset=motion,.candidate_offset=candidate,.flags=flags,.active=1,.start_frame_bits=start_frame_bits,.blend_frames=blend_frames,.release_previous=(uint8_t)release_previous};return GH_ACTOR_MOTION_OK;
}
static gh_actor_motion_status invoke(gh_actor_motion_call*s,int32_t kind,uint32_t mode){
 gh_actor_motion*m=s->current;if(!s->pending){s->pending_kind=kind;s->pending_mode=mode;s->pending=1;}
 if(!s->next.effect)return fail(s,0x306200,GH_ACTOR_MOTION_UNSUPPORTED);
 gh_actor_motion_status e=s->next.effect(s->next.context,&m->b.model->root,s->pending_kind,s->pending_mode,&s->frontier);if(e){if(!s->frontier)s->frontier=0x306200;return e;}s->pending=0;return GH_ACTOR_MOTION_OK;
}
gh_actor_motion_status gh_actor_motion_call_step_002a8578(gh_actor_motion_call*s){
 if(!s||!s->active||!s->current)return GH_ACTOR_MOTION_ARGUMENT;if(s->complete)return GH_ACTOR_MOTION_OK;s->frontier=0;gh_actor_motion*m=s->current;
 if(!m->b.model||!m->b.model->root.bytes||m->b.model->root.size<0x578)return GH_ACTOR_MOTION_ARGUMENT;uint8_t*p=m->b.model->root.bytes;
 if(!s->stage&&!s->release_previous)s->stage=2;
 if(!s->stage){
  if(m->sequence_valid){size_t base=m->sequence_offset;if(!span(m->b.resource_bytes,base,0x18))return fail(s,0x2a84a8,GH_ACTOR_MOTION_BOUNDS);const uint8_t*seq=m->b.resource+base;
   if(memcmp(seq,"SEQ\0",4))return fail(s,0x2a84a8,GH_ACTOR_MOTION_BOUNDS);unsigned count=half(seq+0x16),offset=half(seq+0x14);size_t table=base+offset;
   if(count&&(table<base||!span(m->b.resource_bytes,table,(size_t)count*12)))return fail(s,0x2a84a8,GH_ACTOR_MOTION_BOUNDS);
   while(s->index<count){const uint8_t*r=m->b.resource+table+s->index*12;uint32_t bits=rd(r);int32_t kind=bits<=INT32_MAX?(int32_t)bits:(int32_t)((int64_t)bits-INT64_C(4294967296));
    if(kind!=-1||s->pending){gh_actor_motion_status e=invoke(s,kind,(r[10]&2)?2:0);if(e)return e;}s->index++;
   }
  }s->stage=1;
 }
 if(s->stage==1){unsigned raw=half(p+0x570);int32_t kind=raw<32768?(int32_t)raw:(int32_t)raw-65536;
  if(kind!=-1||s->pending){gh_actor_motion_status e=invoke(s,kind,rd(p+0x574));if(e)return e;}p[0x570]=p[0x571]=255;wr(p+0x574,2);s->stage=2;
 }
 if(s->stage==2){
  /* ONLY after genuine2A84A8 completed, prepare the SAME owner for157568.
   * stage1 skips the alreadycompleted570 release in constructor-onlybegin;
   * oldSEQ was not forgotten/reset before itsrequired306200 calls. */
  m->bound=0;m->complete=0;m->stage=1;m->frontier=0;m->detail=0;m->release_index=0;m->sequence_valid=0;m->step_stage=0;memset(m->caches,0,sizeof(m->caches));s->stage=3;
 }
 gh_actor_motion_status e=gh_actor_motion_begin_parameters_002a8578(m,&s->next,s->motion_offset,s->candidate_offset,s->start_frame_bits,s->blend_frames,s->flags);if(e){s->frontier=m->frontier;return e;}
 s->complete=1;s->frontier=0;return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_actor_motion_call_open_002a8578(gh_actor_motion_call*s,gh_actor_motion*m,const gh_actor_motion_bindings*b,size_t motion,size_t candidate,uint16_t flags){return gh_actor_motion_call_open_parameters_002a8578(s,m,b,motion,candidate,0,0,flags,1);}
