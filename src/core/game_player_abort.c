#include "game_player_abort.h"
#include <string.h>
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static gh_player_abort_status fail(gh_game_player_abort*s,uint32_t pc,gh_player_abort_status e){s->frontier=pc;return e;}
static gh_player_abort_status invoke(gh_game_player_abort*s,const gh_player_abort_bindings*b,uint32_t offset,gh_player_abort_consumer call,uint32_t entry){
 uint8_t*p=b->player->view.bytes;if(!s->pending){s->pending_handle=word(p+offset);s->pending=1;}
 /* 165C zero skips action; 2CBBA0 handle0 is genuine jrra early return. */
 if(s->pending_handle){if(!call)return fail(s,entry,GH_PLAYER_ABORT_UNSUPPORTED);uint32_t f=0;gh_player_abort_status e=call(b->context,s->pending_handle,&f);if(e)return fail(s,f?f:entry,e);}
 put(p+offset,0);s->pending=0;s->pending_handle=0;return GH_PLAYER_ABORT_OK;
}
gh_player_abort_status gh_game_player_abort_00126770(gh_game_player_abort*s,const gh_player_abort_bindings*b){
 if(!s||!b||!b->player||!b->player->view.bytes||b->player->view.size<0x1660||!b->sound||!b->sound->initialized)return GH_PLAYER_ABORT_ARGUMENT;
 if(s->active&&(s->player_bytes!=b->player->view.bytes||s->sound!=b->sound))return GH_PLAYER_ABORT_BUSY;
 if(!s->active){s->active=1;s->stage=0;s->pending=0;s->player_bytes=b->player->view.bytes;s->sound=b->sound;}s->frontier=0;
 if(s->stage==0){gh_player_abort_status e=invoke(s,b,0x165c,b->action_exit_001f6f10,0x1f6f10);if(e)return e;s->stage=1;}
 if(s->stage==1){gh_player_abort_status e=invoke(s,b,0x161c,b->se_stop_002cbba0,0x2cbba0);if(e)return e;s->stage=2;}
 if(s->stage==2){b->sound->flags_b0&=UINT32_C(0xffbfffff);b->sound->flags_ac&=UINT32_C(0xffbfffff);s->stage=3;}
 if(s->stage==3){gh_player_abort_status e=invoke(s,b,0x1620,b->se_stop_002cbba0,0x2cbba0);if(e)return e;s->stage=0;s->active=0;s->frontier=0;s->player_bytes=NULL;s->sound=NULL;return GH_PLAYER_ABORT_OK;}
 return fail(s,0x126770,GH_PLAYER_ABORT_ARGUMENT);
}
