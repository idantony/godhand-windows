#include "game_action_button_step.h"
#include <string.h>
static uint32_t w(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
static void ww(void*p,uint32_t n){memcpy(p,&n,4);}
static gh_action_step_status fail(gh_action_button_frame*s,uint32_t pc,gh_action_step_status e){s->frontier=pc;return e;}
static int member(unsigned n,const uint8_t*items,size_t count){for(size_t i=0;i<count;i++)if(n==items[i])return 1;return 0;}
static gh_action_step_status eligible(gh_action_button_frame*s,const gh_action_step_bindings*b,const gh_action_prompt*p,int*out){
 static const uint8_t idle[]={0,1,2,6,8,9,10,11,14,21,23,24,25,28,29,30,31,32,34,35,36,39,40,41,42,43,44,45,46,47};
 static const uint8_t clear[]={3,4,5,7,12,13,15,16,17,18,22,26,33,38};static const uint8_t always[]={19,20,27,37,50};
 unsigned life=(unsigned)p->life;*out=0;if(member(life,always,sizeof(always))){*out=1;return GH_ACTION_STEP_OK;}
 if(!member(life,idle,sizeof(idle))&&!member(life,clear,sizeof(clear)))return GH_ACTION_STEP_OK;
 const gh_title_player_view*player=b->current_player;if(!player||!player->bytes||player->size<0x15f8)return fail(s,0x2db6b0,GH_ACTION_STEP_ARGUMENT);
 if(member(life,idle,sizeof(idle))){*out=player->bytes[0x2f4]==0&&player->bytes[0x2f5]<4;return GH_ACTION_STEP_OK;}
 if(w(player->bytes+0x6a0)||b->aux_6a0)return GH_ACTION_STEP_OK;if(!b->auxiliary_binding_known)return fail(s,0x12bf50,GH_ACTION_STEP_UNSUPPORTED);int16_t hp;memcpy(&hp,player->bytes+0x54a,2);if(hp<=0||(w(player->bytes+0x15f4)&0x400000))return GH_ACTION_STEP_OK;if(!b->init)return fail(s,0x293100,GH_ACTION_STEP_ARGUMENT);*out=!b->init->enemies[0x53c]&&(player->bytes[0x15f5]&1);return GH_ACTION_STEP_OK;
}
static gh_action_step_status hud(gh_action_button_frame*s,const gh_action_step_bindings*b,const gh_action_prompt*p){
 gh_cockpit_action_view v;if(gh_game_cockpit_action_view_001f7680(b->cockpit,&v))return fail(s,0x1f7680,GH_ACTION_STEP_CALLEE);*v.flags|=1;uint8_t icon;uint32_t x=0x43480000,y=0x43a50000;
 switch(p->unknown20){case 1:icon=0;break;case 2:icon=3;break;case 3:icon=2;break;case 0x10:icon=4;x=0x43660000;y=0x43910000;break;case 0x11:icon=5;x=0x43660000;y=0x43910000;break;case 0x12:icon=6;x=0x43660000;y=0x43910000;break;default:icon=1;break;}
 *v.icon=icon;ww(v.screen_x,x);ww(v.screen_y,y);ww(v.message,(uint32_t)p->life+0x3011u);return GH_ACTION_STEP_OK;
}
static int pressed(const gh_boot_work*work,const gh_action_prompt*p){uint32_t buttons=w((const uint8_t*)work+0x1a4);switch(p->unknown20){case 0:return (buttons&8)!=0;case 1:return (buttons&1)!=0;case 2:return (buttons&2)!=0;case 3:return (buttons&4)!=0;default:return 0;}}
gh_action_step_status gh_action_button_frame_begin(gh_action_button_frame*s){if(!s||(s->active&&!s->complete))return GH_ACTION_STEP_ARGUMENT;*s=(gh_action_button_frame){.identity=s,.active=1};return GH_ACTION_STEP_OK;}
gh_action_step_status gh_action_button_step_001f7360(gh_action_button_frame*s,const gh_action_step_bindings*b){
 if(!s||!b||!s->active||s->identity!=s||!b->buttons||!b->work||b->buttons->native_identity!=b->buttons||!b->buttons->links_ready)return GH_ACTION_STEP_ARGUMENT;s->frontier=0;if(s->complete)return GH_ACTION_STEP_OK;if(s->bindings&&s->bindings!=b)return GH_ACTION_STEP_ARGUMENT;s->bindings=b;
 const uint8_t*work=(const uint8_t*)b->work;gh_game_action_buttons*a=b->buttons;
 if(s->stage==0){if(!(w(work+0x5e0)&0x100000)&&(w(work+0x5d8)&0x400000)){s->complete=1;return GH_ACTION_STEP_OK;}gh_cockpit_action_view v;if(gh_game_cockpit_action_view_001f7680(b->cockpit,&v))return fail(s,0x1f75b0,GH_ACTION_STEP_CALLEE);*v.flags&=0xfe;
  if(w(work+0x5e0)&0x100000)s->stage=4;else{if(gh_action_button_first(a,&s->link))return fail(s,0x1f7360,GH_ACTION_STEP_ARGUMENT);s->stage=1;}}
 if(s->stage==1){while(s->link.kind!=GH_ACTION_LINK_END){if(s->visited>=24)return fail(s,0x1f7360,GH_ACTION_STEP_BOUNDS);
   if(s->link.kind==GH_ACTION_LINK_HEADER){if(s->link.index>=16)return fail(s,0x1f7360,GH_ACTION_STEP_BOUNDS);if(a->pool[s->link.index].flags==1)return fail(s,0x1f7360,GH_ACTION_STEP_UNSUPPORTED);}
   else {const gh_action_prompt*p;if(gh_action_button_prompt(a,s->link,&p))return fail(s,0x1f7360,GH_ACTION_STEP_BOUNDS);if(p->state0c==1&&p->life!=0x31){int accept;gh_action_step_status e=eligible(s,b,p,&accept);if(e)return e;if(accept){s->selected=s->link.index;s->stage=2;break;}}}
   if(gh_action_button_next(a,s->link,&s->link))return fail(s,0x1f7360,GH_ACTION_STEP_BOUNDS);s->visited++;
  }if(s->stage==1)s->stage=4;}
 if(s->stage==2){gh_action_prompt*p=a->entries+s->selected;if(p->kind==3)return fail(s,0x1f70c8,GH_ACTION_STEP_UNSUPPORTED);if(p->kind>=0&&p->kind<=2){gh_action_step_status e=hud(s,b,p);if(e)return e;s->pressed=(uint8_t)pressed(b->work,p);s->callback_kind=p->kind;s->callback_entry=p->callback_id10;s->callback_argument=p->callback_argument;s->stage=3;}else s->stage=4;}
 if(s->stage==3){if(s->pressed&&s->callback_entry){if(s->callback_kind!=0&&(s->callback_kind!=2||s->callback_entry!=0x2c2138||!s->callback_argument||!b->callback))return fail(s,0x31b410,GH_ACTION_STEP_UNSUPPORTED);if(!b->callback)return fail(s,s->callback_entry,GH_ACTION_STEP_UNSUPPORTED);uint32_t pc=0;gh_action_step_status e=b->callback(b->context,s->callback_entry,s->callback_argument,&pc);if(e)return fail(s,pc?pc:s->callback_entry,e);}s->stage=4;}
 if(s->stage==4){while(s->cleanup_index<8){unsigned i=s->cleanup_index;gh_action_prompt*p=a->entries+i;if((p->flags&0x201)==1){if(p->kind==3&&p->unknown36!=1){s->cleanup_index++;continue;}if(p->kind==3&&p->unknown32>0){p->unknown32--;s->cleanup_index++;continue;}uint32_t pc=0;gh_action_set_status e=gh_action_button_reset_entry_001f7228(a,i,&pc);if(e)return fail(s,pc?pc:0x1f7228,e==GH_ACTION_SET_BOUNDS?GH_ACTION_STEP_BOUNDS:GH_ACTION_STEP_CALLEE);}s->cleanup_index++;}s->complete=1;}
 return GH_ACTION_STEP_OK;
}
