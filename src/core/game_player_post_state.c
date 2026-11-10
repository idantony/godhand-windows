#include "game_player_post_state.h"
#include <string.h>
static uint32_t word(const uint8_t *p){uint32_t v;memcpy(&v,p,4);return v;}
static void put(uint8_t *p,uint32_t v){memcpy(p,&v,4);}
static gh_player_post_status fail(gh_game_player_post_state *s,uint32_t pc,gh_player_post_status e){s->frontier=pc;return e;}
static gh_player_post_status bind(gh_game_player_post_state *s,const gh_title_player_view *v){
 if(!s||!v||!v->bytes||v->size<0x15f8)return GH_PLAYER_POST_ARGUMENT;
 if(s->player_bytes&&s->player_bytes!=v->bytes)return GH_PLAYER_POST_ARGUMENT;
 s->player_bytes=v->bytes;s->frontier=0;return GH_PLAYER_POST_OK;
}
gh_player_post_status gh_player_state_effect_0012bb38(gh_game_player_post_state *s,const gh_title_player_view *v,const gh_effect_release_bindings *effects){
 gh_player_post_status e=bind(s,v);if(e)return e;uint8_t *p=v->bytes;
 if(s->effect_phase==2)return GH_PLAYER_POST_OK;
 if(!s->effect_phase){uint32_t flags=word(p+0x15f4);unsigned state=p[0x2f4]|(unsigned)p[0x2f5]<<8;
  if(state==0x601){if(!(flags&0x40)){put(p+0x15f4,flags|0x40);s->effect_action=1;}}
  else if(flags&0x40){put(p+0x15f4,flags&~0x40u);s->effect_action=2;}
  s->effect_phase=1;
 }
 if(s->effect_action==1)return fail(s,0x306900,GH_PLAYER_POST_UNSUPPORTED);
 if(s->effect_action==2){if(!effects)return fail(s,0x306200,GH_PLAYER_POST_UNSUPPORTED);unsigned slot=0;uint32_t pc=0;
  gh_effect_status x=gh_game_effect_release_00306200(effects,v,4,2,&pc,&slot);
  if(x)return fail(s,pc?pc:0x306200,x==GH_EFFECT_UNSUPPORTED?GH_PLAYER_POST_UNSUPPORTED:GH_PLAYER_POST_CALLEE);
 }
 s->effect_phase=2;return GH_PLAYER_POST_OK;
}
static gh_player_post_status node_flag(gh_game_player_post_state *s,const gh_game_player_node_cache *cache,unsigned index,uint32_t and_mask,uint32_t or_mask){
 static const unsigned offsets[10]={0x1550,0x1574,0x1554,0x1558,0x155c,0x1560,0x1564,0x1568,0x156c,0x1570};
 if(word(cache->player->view.bytes+offsets[index]))return fail(s,0x1236e8,GH_PLAYER_POST_UNSUPPORTED);
 const gh_player_mesh_reference *r=&cache->nodes[index];if(!r->bytes)return r->owner?GH_PLAYER_POST_ARGUMENT:GH_PLAYER_POST_OK;
 const uint8_t *current=NULL;size_t mesh=0;
 if(r->owner!=cache->model||gh_actor_model_node_at(cache->model,(unsigned)r->mesh_index,&current,&mesh)||current!=r->bytes||mesh!=r->mesh_index)return fail(s,0x1236e8,GH_PLAYER_POST_ARGUMENT);
 gh_actor_model_status x=gh_actor_model_node_mask_flags(r->owner,(unsigned)mesh,0x380,and_mask,or_mask);
 return x?fail(s,0x1236e8,GH_PLAYER_POST_CALLEE):GH_PLAYER_POST_OK;
}
gh_player_post_status gh_player_mesh_visibility_001236e8(gh_game_player_post_state *s,const gh_game_player_node_cache *cache,gh_core_save_view save){
 if(!cache||!cache->ready||!cache->player||!cache->model||cache->model->root.bytes!=cache->player->view.bytes)return s?fail(s,0x1236e8,GH_PLAYER_POST_UNSUPPORTED):GH_PLAYER_POST_ARGUMENT;
 gh_player_post_status e=bind(s,&cache->player->view);if(e)return e;if(s->visibility_phase==2)return GH_PLAYER_POST_OK;
 uint8_t *p=cache->player->view.bytes;
 if(!s->visibility_phase){unsigned costume=0;uint32_t flags=word(p+0x15f4);
  if(flags&0x20){if(!save.bytes||save.size<GH_CORE_SAVE_BYTES)return fail(s,0x1fc0a8,GH_PLAYER_POST_ARGUMENT);costume=gh_core_save_costume(save);}
  if(!(flags&0x20)||costume==3||costume==5||costume==7){put(p+0x254,word(p+0x254)|0x10000000u);s->visibility_phase=1;}
  else{put(p+0x254,word(p+0x254)&~0x10000000u);e=node_flag(s,cache,0,~1u,0);if(e)return e;s->visibility_phase=3;}
 }
 if(s->visibility_phase==1)return fail(s,0x149ad8,GH_PLAYER_POST_UNSUPPORTED);
 /* All writes are idempotent. Late native validation errors may retry this
  * loop without releasing effects or executing the player's motion again. */
 for(unsigned i=2;i<10;i++){e=node_flag(s,cache,i,UINT32_MAX,1);if(e)return e;}
 uint16_t flags;memcpy(&flags,p+0x3ac,2);unsigned a=((flags&0x4000)?1u:0u)|((flags&0x1000)?2u:0u),b=((flags&0x8000)?1u:0u)|((flags&0x2000)?2u:0u);
 e=node_flag(s,cache,2+a,~1u,0);if(e)return e;e=node_flag(s,cache,6+b,~1u,0);if(e)return e;
 s->visibility_phase=2;s->frontier=0;return GH_PLAYER_POST_OK;
}
gh_player_post_status gh_player_camera_mode_00123908(const gh_game_init_state *init,uint32_t *out){
 if(!init||!out)return GH_PLAYER_POST_ARGUMENT;*out=(uint32_t)init->camera.mode&1u;return GH_PLAYER_POST_OK;
}
