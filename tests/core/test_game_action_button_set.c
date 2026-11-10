#include "game_action_button_set.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(void){gh_game_action_buttons s={0};gh_game_action_buttons_reset_001f72c0(&s);gh_action_link l;C(!gh_action_button_first(&s,&l));unsigned headers=0;while(l.kind!=GH_ACTION_LINK_END){C(l.kind==GH_ACTION_LINK_HEADER&&l.index==15-headers++);C(!gh_action_button_next(&s,l,&l));}C(headers==16);
 gh_action_set_request r={.priority=4,.life=8,.callback_id=0x2dca58};int emitted;unsigned index;uint32_t pc;C(!gh_action_button_set_001f7858(&s,&r,&emitted,&index,&pc)&&emitted&&index==0&&!pc&&s.entries[0].priority==4&&s.entries[0].life==8&&s.entries[0].callback_id10==0x2dca58);
 r.life=9;r.callback_id=0x2dccd0;C(!gh_action_button_set_001f7858(&s,&r,&emitted,&index,&pc)&&emitted&&index==1);
 C(s.heads[4].kind==GH_ACTION_LINK_PROMPT&&s.heads[4].index==1&&s.prompt_next[1].kind==GH_ACTION_LINK_PROMPT&&s.prompt_next[1].index==0&&s.prompt_next[0].kind==GH_ACTION_LINK_HEADER&&s.prompt_next[0].index==3);
 C(!gh_action_button_first(&s,&l));unsigned visited=0,prompts=0;while(l.kind!=GH_ACTION_LINK_END){C(++visited<=24);if(l.kind==GH_ACTION_LINK_PROMPT){const gh_action_prompt*p;C(!gh_action_button_prompt(&s,l,&p)&&p==s.entries+(prompts?0:1));prompts++;}C(!gh_action_button_next(&s,l,&l));}C(visited==18&&prompts==2);
 for(unsigned i=2;i<8;i++){r.priority=(int32_t)i;r.callback_id=0xfeed0000+i;C(!gh_action_button_set_001f7858(&s,&r,&emitted,&index,&pc)&&emitted&&index==i);}gh_game_action_buttons before=s;C(!gh_action_button_set_001f7858(&s,&r,&emitted,&index,&pc)&&!emitted&&index==UINT_MAX&&!memcmp(&s,&before,sizeof(s)));
 r.priority=-1;C(gh_action_button_set_001f7858(&s,&r,&emitted,&index,&pc)==GH_ACTION_SET_BOUNDS&&pc==0x1f7800&&!memcmp(&s,&before,sizeof(s)));r.priority=16;C(gh_action_button_set_001f7858(&s,&r,&emitted,&index,&pc)==GH_ACTION_SET_BOUNDS);
 gh_game_action_buttons copy=s;C(gh_action_button_first(&copy,&l)==GH_ACTION_SET_ARGUMENT);gh_game_action_buttons_reset_001f72c0(&s);C(!gh_action_button_first(&s,&l));headers=0;while(l.kind!=GH_ACTION_LINK_END){C(l.kind==GH_ACTION_LINK_HEADER);headers++;C(!gh_action_button_next(&s,l,&l));}C(headers==16);
 /*201 is genuinely free byallocator predicate, andsetdoesNOTclear200. */
 s.entries[0].flags=0x201;r.priority=4;r.life=8;r.kind=7;r.unknown20=123;r.argument=&s;r.callback_id=0xdeadbeef;C(!gh_action_button_set_001f7858(&s,&r,&emitted,&index,&pc)&&emitted&&index==0&&s.entries[0].flags==0x201&&s.entries[0].kind==7&&s.entries[0].unknown20==123&&s.entries[0].callback_argument==&s);
 s.heads[4]=(gh_action_link){GH_ACTION_LINK_PROMPT,9};C(gh_action_button_next(&s,(gh_action_link){GH_ACTION_LINK_HEADER,4},&l)==GH_ACTION_SET_BOUNDS);C(gh_action_button_set_001f7858(&s,&r,&emitted,&index,&pc)==GH_ACTION_SET_BOUNDS&&pc==0x1f7800);
 puts("PASS canonical8prompt/16header mixedlinks/headinsertLIFO/realDoor scalarrequests/fullpoolvoidnoop/201retained/reset/foreignowner/bounds; no callbackinvocation");return 0;}
