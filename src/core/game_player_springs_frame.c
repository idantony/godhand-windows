#include "game_player_springs_frame.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static float value(const uint8_t*p){float n;memcpy(&n,p,4);return n;}
static float add(float a,float b){volatile float n=a+b;return n;}
static float mul(float a,float b){volatile float n=a*b;return n;}
static gh_player_springs_status fail(gh_game_player_springs*s,gh_player_springs_status e){s->frontier=0x107c68;return e;}
gh_player_springs_status gh_spring_records_reset_00107c68(gh_actor_model *m,uint8_t *record_source,
 const uint8_t *indices,unsigned count,gh_static_transform_policy policy){
 if(!m||!m->parts_ready||!record_source||!indices||count>44)return GH_SPRINGS_ARGUMENT;
 if(policy!=GH_STATIC_BINARY32_REFERENCE)return GH_SPRINGS_NUMERIC;
 uint8_t records[44*0xa0];memcpy(records,record_source,sizeof(records));
 fenv_t saved;if(feholdexcept(&saved))return GH_SPRINGS_NUMERIC;
 if(fesetround(FE_TONEAREST)){(void)fesetenv(&saved);return GH_SPRINGS_NUMERIC;}
 gh_player_springs_status status=GH_SPRINGS_OK;
 for(unsigned i=0;i<count;i++){
  unsigned id=indices[i];const gh_actor_part*p;
  if(id>=44||gh_actor_model_part_at(m,id,&p)||!p->bytes){status=GH_SPRINGS_BOUNDS;break;}
  uint8_t*r=records+(size_t)id*0xa0;float out[3];
  for(unsigned row=0;row<3;row++){
   for(unsigned col=0;col<4;col++)if(!isfinite(value(p->bytes+0x80+col*16+row*4))||!isfinite(value(r+col*4)))status=GH_SPRINGS_NUMERIC;
   if(status)break;
   float n=mul(value(p->bytes+0x80+row*4),value(r));n=add(n,mul(value(p->bytes+0x90+row*4),value(r+4)));
   n=add(n,mul(value(p->bytes+0xa0+row*4),value(r+8)));out[row]=add(n,mul(value(p->bytes+0xb0+row*4),value(r+12)));
   if(!isfinite(out[row])){status=GH_SPRINGS_NUMERIC;break;}
  }
  if(status)break;memcpy(r+0x40,out,12);memcpy(r+0x50,out,12);memset(r+0x30,0,12);
 }
 if(fesetenv(&saved))return GH_SPRINGS_NUMERIC;
 if(status)return status;memcpy(record_source,records,sizeof(records));return GH_SPRINGS_OK;
}

gh_player_springs_status gh_player_springs_reset_00107c68(gh_game_player_springs*s,gh_static_transform_policy policy){
 if(!s||!s->child||!s->configured||!s->initialized)return GH_SPRINGS_ARGUMENT;
 gh_game_player_child*c=s->child;gh_actor_model*m=&c->model;
 if(!c->object.view.bytes||c->object.view.size<GH_PLAYER_CHILD_BYTES||m->root.bytes!=c->object.view.bytes||!m->parts_ready)return fail(s,GH_SPRINGS_ARGUMENT);
 uint8_t*raw=c->object.view.bytes;
 if(s->count>44||word(raw+0x2050)!=s->count)return fail(s,GH_SPRINGS_FORMAT);
 /* Original2054 is index-table pointer and20A4 optional part-array override.
    The recovered constructor published the typed index table and NULL override;
    changed raw tokens cannot silently keep those previous identities. */
 if(word(raw+0x2054)||word(raw+0x20a4))return fail(s,GH_SPRINGS_FORMAT);
 if(policy!=GH_STATIC_BINARY32_REFERENCE)return fail(s,GH_SPRINGS_NUMERIC);
 gh_player_springs_status status=gh_spring_records_reset_00107c68(m,raw+0x4d0,s->indices[0],s->count,policy);
 if(status)return fail(s,status);s->frontier=0;return GH_SPRINGS_OK;
}
