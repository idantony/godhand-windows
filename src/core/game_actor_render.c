#include "game_actor_render.h"
#include <string.h>

static int same(const gh_actor_render_bindings *a,const gh_actor_render_bindings *b)
{
 return a->work==b->work&&a->camera==b->camera&&a->light==b->light&&
  a->depth_shift==b->depth_shift&&a->elf==b->elf&&a->elf_bytes==b->elf_bytes&&
  a->shadow==b->shadow&&a->actors==b->actors&&a->up_owner==b->up_owner&&
  a->pivot_owner==b->pivot_owner&&a->templates==b->templates&&a->textures==b->textures&&
  a->emit_shadow==b->emit_shadow&&a->emit_context==b->emit_context&&
  a->ray==b->ray&&a->ray_context==b->ray_context&&
  a->draw_callee==b->draw_callee&&a->draw_context==b->draw_context&&
  a->other_callee==b->other_callee&&a->other_context==b->other_context&&a->policy==b->policy&&
  a->actor_emit==b->actor_emit&&a->actor_emit_context==b->actor_emit_context&&a->light_globals==b->light_globals;
}

static gh_actor_model_status invoke(void *context,const gh_actor_trans_request *r,uint32_t *f)
{
 gh_actor_render_call *c=context;const gh_actor_render_bindings *b=&c->bindings;
 if(!r||r->model!=c->model||r->shadow!=b->shadow)return GH_ACTOR_MODEL_ARGUMENT;
 if(r->entry==0x14efa8){
  gh_shadow_bindings shadow={.model=c->model,.shadow=b->shadow,.work=b->work,
   .camera=b->camera,.world_light=b->light,.actors=b->actors,.up_owner=b->up_owner,
   .pivot_owner=b->pivot_owner,.templates=b->templates,.emit=b->emit_shadow,
   .emit_context=b->emit_context,.ray=b->ray,.ray_context=b->ray_context,.policy=b->policy};
  return gh_actor_shadow_draw_0014efa8(&c->shadow,&shadow,f);
 }
 if(r->entry==0x14ac90){
  if(r->depth<0||r->depth>UINT16_MAX){if(f)*f=r->entry;return GH_ACTOR_MODEL_UNSUPPORTED;}
  gh_actor_draw_bindings draw={.camera=b->camera,.light=b->light,
   .callee=b->draw_callee,.context=b->draw_context,.policy=b->policy,.textures=b->textures,
   .emit=b->actor_emit,.emit_context=b->actor_emit_context,
   .light_globals=b->light_globals,.pivot_owner=b->pivot_owner};
  return gh_actor_model_draw_0014ac90(&c->draw,c->model,(uint16_t)r->depth,r->far_mode,&draw,f);
 }
 if(!b->other_callee){if(f)*f=r->entry;return GH_ACTOR_MODEL_UNSUPPORTED;}
 return b->other_callee(b->other_context,r,f);
}

gh_actor_model_status gh_actor_render_0014a3e8(gh_actor_render_call *c,gh_actor_model *m,
 const gh_actor_render_bindings *b,uint32_t *f)
{
 if(f)*f=0;
 if(!c||!m||!b)return GH_ACTOR_MODEL_ARGUMENT;
 if(c->bound){if(c->identity!=c||c->model!=m||!same(&c->bindings,b))return GH_ACTOR_MODEL_BUSY;}
 else{memset(c,0,sizeof(*c));c->bindings=*b;c->model=m;c->identity=c;c->bound=1;}
 gh_actor_trans_bindings trans={.work=b->work,.camera=b->camera,.world_light=b->light,
  .depth_shift=b->depth_shift,.elf=b->elf,.elf_bytes=b->elf_bytes,.shadow=b->shadow,
  .callee=invoke,.context=c,.numeric_policy=b->policy};
 return gh_actor_model_trans_0014a3e8(&c->trans,m,&trans,f);
}

gh_actor_model_status gh_actor_render_next_call(gh_actor_render_call *c)
{
 if(!c||!c->bound||c->identity!=c||!c->trans.complete)return GH_ACTOR_MODEL_BUSY;
 memset(&c->trans,0,sizeof(c->trans));memset(&c->shadow,0,sizeof(c->shadow));
 memset(&c->draw,0,sizeof(c->draw));return GH_ACTOR_MODEL_OK;
}
