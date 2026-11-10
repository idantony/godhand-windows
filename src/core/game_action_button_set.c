#include "game_action_button_set.h"
#include <limits.h>
static int owner(const gh_game_action_buttons*s){return s&&s->links_ready&&s->native_identity==s;}
static int valid(gh_action_link l){return l.kind==GH_ACTION_LINK_END||(l.kind==GH_ACTION_LINK_HEADER&&l.index<16)||(l.kind==GH_ACTION_LINK_PROMPT&&l.index<8);}
gh_action_set_status gh_action_button_first(const gh_game_action_buttons*s,gh_action_link*out){if(!owner(s)||!out)return GH_ACTION_SET_ARGUMENT;*out=(gh_action_link){GH_ACTION_LINK_HEADER,15};return GH_ACTION_SET_OK;}
gh_action_set_status gh_action_button_next(const gh_game_action_buttons*s,gh_action_link current,gh_action_link*out){
 if(!owner(s)||!out||!valid(current)||current.kind==GH_ACTION_LINK_END)return GH_ACTION_SET_ARGUMENT;
 gh_action_link next=current.kind==GH_ACTION_LINK_HEADER?s->heads[current.index]:s->prompt_next[current.index];if(!valid(next))return GH_ACTION_SET_BOUNDS;*out=next;return GH_ACTION_SET_OK;
}
gh_action_set_status gh_action_button_prompt(const gh_game_action_buttons*s,gh_action_link l,const gh_action_prompt**out){if(!owner(s)||!out||l.kind!=GH_ACTION_LINK_PROMPT||l.index>=8)return GH_ACTION_SET_ARGUMENT;*out=s->entries+l.index;return GH_ACTION_SET_OK;}
gh_action_set_status gh_action_button_set_001f7858(gh_game_action_buttons*s,const gh_action_set_request*r,int*emitted,unsigned*index,uint32_t*f){
 if(!emitted||!index||!f)return GH_ACTION_SET_ARGUMENT;*emitted=0;*index=UINT_MAX;*f=0;if(!owner(s)||!r)return GH_ACTION_SET_ARGUMENT;
 if(r->priority<0||r->priority>=16){*f=0x1f7800;return GH_ACTION_SET_BOUNDS;}
 unsigned selected=0;while(selected<8&&(s->entries[selected].flags&0x201)==1)selected++;if(selected==8)return GH_ACTION_SET_OK;
 gh_action_link next=s->heads[(unsigned)r->priority];if(!valid(next)){*f=0x1f7800;return GH_ACTION_SET_BOUNDS;}
 gh_action_prompt*p=s->entries+selected;p->life=r->life;p->unknown20=r->unknown20;p->flags|=1u;p->callback_id10=r->callback_id;p->callback_argument=r->argument;p->kind=r->kind;p->priority=r->priority;
 s->heads[(unsigned)r->priority]=(gh_action_link){GH_ACTION_LINK_PROMPT,selected};s->prompt_next[selected]=next;
 /* These cannot represent a heterogeneous target; leave compatibility-only
  * fields untouched rather than fabricate cross-typed pointer casts. */
 *emitted=1;*index=selected;return GH_ACTION_SET_OK;
}
