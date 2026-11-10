#include "game_player_init.h"
#include <string.h>
static uint32_t w(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static void ww(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
static gh_player_init_status fail(gh_game_player_init*s,uint32_t pc,gh_player_init_status e){s->frontier=pc;return e;}
static gh_player_init_status lookup(gh_game_player_init*s,const gh_bundle*b,const char*tag,gh_bundle_entry*out){gh_bundle_error e=gh_bundle_find(b,tag,0,out);if(e==GH_BUNDLE_NOT_FOUND){*out=(gh_bundle_entry){0};return GH_PLAYER_INIT_OK;}return e?fail(s,0x31ccf0,GH_PLAYER_INIT_BOUNDS):GH_PLAYER_INIT_OK;}
gh_player_init_status gh_game_player_initialize_00107f58(gh_game_player_init*s,const gh_player_init_bindings*b){
 if(!s||!b||!b->factory||!b->factory->complete||!b->work||!b->factory->b.player||b->current_player!=b->factory->b.player||!b->current_player->view.bytes||b->current_player->view.size<GH_TITLE_PLAYER_BYTES)return GH_PLAYER_INIT_ARGUMENT;
 if(s->stage&&(s->factory!=b->factory||s->work!=b->work))return GH_PLAYER_INIT_ARGUMENT;
 gh_game_title_player*player=b->current_player;uint8_t*raw=player->view.bytes;
 if(!s->stage){s->factory=b->factory;s->work=b->work;/*100320 isactualreturn1, ignored. */s->stage=1;}
 if(s->stage==1){gh_bundle bundle;if(gh_bundle_open(b->factory->resource.bytes,b->factory->file_bytes,&bundle))return fail(s,0x31ccf0,GH_PLAYER_INIT_BOUNDS);gh_player_init_status e=lookup(s,&bundle,"MD",&s->geometry);if(e)return e;
  e=lookup(s,&bundle,"TM3",&s->texture);if(e)return e;s->texture_mode=0;if(!s->texture.bytes){e=lookup(s,&bundle,"T3L",&s->texture);if(e)return e;s->texture_mode=1;if(!s->texture.bytes){e=lookup(s,&bundle,"T32",&s->texture);if(e)return e;s->texture_mode=0;}}
  if(s->geometry.bytes&&gh_model_open_scr(s->geometry.bytes,s->geometry.size,&s->geometry_scripts))return fail(s,0x149738,GH_PLAYER_INIT_BOUNDS);s->stage=2;
 }
 if(s->stage==2){
  if(!s->geometry.bytes){/* Actual149738 NULL-MD branch, beforetextureaccess. */if(!player->view.alias_f0)return GH_PLAYER_INIT_ARGUMENT;memset(player->view.alias_f0,0,12);uint8_t*work=(uint8_t*)b->work;ww(work+0x584,w(work+0x584)&0xff1fffff);ww(raw+0x2c4,1);s->model_original_return=0;}
  else {if(!b->model_initialize_00149738)return fail(s,0x149738,GH_PLAYER_INIT_UNSUPPORTED);uint32_t f=0;int original=0;gh_player_init_status e=b->model_initialize_00149738(b->model_context,player,b->work,&s->geometry,&s->texture,s->texture_mode,&original,&f);if(e)return fail(s,f?f:0x149738,e);s->model_original_return=original;}
  s->model_call_complete=1;s->stage=3;
 }
 if(s->stage==3){/* Originalglobalplayer=self isidempotent for thisMain-only
  * path: current_player identitywasrequired, not a newshadow global. */ww(raw+0x254,w(raw+0x254)|0x8000000);s->stage=4;}
 s->pending_child_id=0x101;return fail(s,0x30fa98,GH_PLAYER_INIT_UNSUPPORTED);
}
