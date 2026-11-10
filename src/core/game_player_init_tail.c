#include "game_player_init_tail.h"
#include <string.h>
static uint32_t w(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static void ww(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
static void wh(uint8_t*p,uint16_t v){memcpy(p,&v,2);}
static gh_player_init_status fail(gh_game_player_init_tail*s,uint32_t pc,gh_player_init_status e){s->frontier=pc;return e;}
static gh_player_init_status player_error(gh_game_player_init_tail*s,uint32_t pc,gh_title_player_status e){return fail(s,pc,e==GH_PLAYER_UNSUPPORTED?GH_PLAYER_INIT_UNSUPPORTED:e==GH_PLAYER_BOUNDS?GH_PLAYER_INIT_BOUNDS:GH_PLAYER_INIT_CALLEE);}
static int same(const gh_player_init_tail_bindings*a,const gh_player_init_tail_bindings*b){return a->prefix==b->prefix&&a->parent_model==b->parent_model&&a->child==b->child&&a->attachment==b->attachment&&a->solids==b->solids&&a->default_heap==b->default_heap&&a->save.bytes==b->save.bytes&&a->save.size==b->save.size&&a->resources.dvd==b->resources.dvd&&a->resources.heap==b->resources.heap&&a->resources.threads==b->resources.threads&&a->resources.sound_check==b->resources.sound_check&&a->resources.sound_context==b->resources.sound_context&&a->initialize_child==b->initialize_child&&a->child_context==b->child_context;}
gh_player_init_status gh_game_player_initialize_tail_00107f58(gh_game_player_init_tail*s,const gh_player_init_tail_bindings*b){
 if(!s||!b||!b->prefix||!b->prefix->factory||!b->prefix->factory->complete||!b->prefix->model_call_complete||b->prefix->stage!=4||!b->child||!b->attachment||!b->solids||!b->default_heap)return GH_PLAYER_INIT_ARGUMENT;
 gh_game_player_factory*f=b->prefix->factory;gh_game_title_player*player=f->b.player;if(!player||!player->view.bytes||player->view.size<GH_TITLE_PLAYER_BYTES)return GH_PLAYER_INIT_ARGUMENT;
 if(b->parent_model&&b->parent_model->root.bytes!=player->view.bytes)return GH_PLAYER_INIT_ARGUMENT;
 if(s->bound&&!same(&s->b,b))return GH_PLAYER_INIT_ARGUMENT;if(!s->bound){s->b=*b;s->player=player;s->bound=1;}
 uint8_t*p=player->view.bytes;
 if(!s->stage){gh_player_child_bindings cb={0};cb.pools=f->b.pools;cb.global_objects=f->b.global_objects;cb.room_objects=f->b.room_objects;cb.work=b->prefix->work;cb.factory_serial=f->b.factory_serial;cb.resources=b->resources;int present=0;gh_title_player_status e=gh_game_player_child_create_0030fa98(b->child,&cb,&present);if(e)return player_error(s,b->child->frontier?b->child->frontier:0x30fa98,e);s->active_child=(uint8_t)present;ww(p+0x68c,0);s->stage=1;}
 if(s->stage==1){if(s->active_child){uint32_t pc=0;gh_title_player_status e=b->initialize_child?b->initialize_child(b->child_context,b->child,&s->child_original_return,&pc):gh_game_player_child_initialize_00100378(b->child,&s->child_original_return);if(e)return player_error(s,pc?pc:b->child->frontier?b->child->frontier:0x100378,e);if(!s->child_original_return){s->active_child=0;ww(p+0x68c,0);}}s->stage=2;}
 if(s->stage==2){if(s->active_child){if(b->parent_model){gh_title_player_status e=gh_game_player_child_attach_00100ed8(b->attachment,b->child,f,b->parent_model,17,b->save);if(e)return player_error(s,0x100ed8,e);}else if(p[0x2b4])return fail(s,0x100ed8,GH_PLAYER_INIT_UNSUPPORTED);}s->stage=3;}
 if(s->stage==3){p[0x140]=4;gh_title_player_status e=gh_game_player_shadow_initialize_0014ef50(&s->shadow,&player->view);if(e)return player_error(s,0x14ef50,e);s->stage=4;}
 if(s->stage==4){if(b->parent_model){gh_title_player_status e=gh_game_player_initial_part_flags(b->parent_model);if(e)return player_error(s,0x1080c0,e);}else if(p[0x2b4])return fail(s,0x1080c0,GH_PLAYER_INIT_UNSUPPORTED);s->stage=5;}
 if(s->stage==5){uint32_t pc=0;gh_solid_unit_status e=gh_collision_solid_create_unit_001346f8(b->solids,b->default_heap,&player->view,5,0x3e4ccccd,&s->solid_unit,&pc);if(e)return fail(s,pc?pc:0x1346f8,e==GH_SOLID_UNIT_UNSUPPORTED?GH_PLAYER_INIT_UNSUPPORTED:GH_PLAYER_INIT_CALLEE);s->stage=6;}
 if(s->stage==6){uint32_t pc=0,xyz[]={0,0x3f000000,0};gh_solid_unit_status e=gh_collision_solid_create_sphere_00134880(b->solids,&player->view,p+0x80,xyz,0x3f000000,&s->sphere,&pc);if(e)return fail(s,pc?pc:0x134880,e==GH_SOLID_UNIT_UNSUPPORTED?GH_PLAYER_INIT_UNSUPPORTED:GH_PLAYER_INIT_CALLEE);s->stage=7;}
 if(s->stage==7){uint16_t max=100,current=100;if(b->save.bytes){if(b->save.size<0x88)return fail(s,0x1fac80,GH_PLAYER_INIT_BOUNDS);max=(uint16_t)w(b->save.bytes+0x80);current=(uint16_t)w(b->save.bytes+0x84);}int a=max<32768?(int)max:(int)max-65536,c=current<32768?(int)current:(int)current-65536;wh(p+0x548,max);wh(p+0x54a,(uint16_t)(c<a?c:a));s->stage=8;}
 return fail(s,0x1fe218,GH_PLAYER_INIT_UNSUPPORTED);
}

