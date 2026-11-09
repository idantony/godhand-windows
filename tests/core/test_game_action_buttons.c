#include "game_action_buttons.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(void){gh_game_action_buttons s={.unknown00=0x12345678};int32_t command=77;uint16_t id=88;uint8_t arg=99;float matrix[16]={0};
 for(unsigned i=0;i<16;i++){s.pool[i].unknown0=i+3;s.pool[i].unknown8=i+9;s.pool[i].previous=s.pool+15;s.pool[i].flags=0xffffffff;}
 for(unsigned i=0;i<8;i++){gh_action_prompt*p=s.entries+i;p->flags=0xa5a5a5a5;p->next=s.entries+7;p->unknown08=i+42;p->unknown37=0x5a;p->callback_id10=0x777;p->callback_argument=&command;p->matrix=matrix;p->command_table=&command;p->id_table=&id;p->argument_table=&arg;}
 gh_game_action_buttons_reset_001f72c0(&s);CHECK(s.unknown00==0x12345678);
 for(unsigned i=0;i<16;i++)CHECK(s.pool[i].previous==(i?s.pool+i-1:0)&&!s.pool[i].flags&&s.pool[i].unknown0==i+3&&s.pool[i].unknown8==i+9);
 for(unsigned i=0;i<8;i++){gh_action_prompt*p=s.entries+i;CHECK(p->flags==0xa5a5a5a4&&p->next==s.entries+7&&p->unknown08==i+42&&p->unknown37==0x5a);CHECK(p->state0c==1&&!p->callback_id10&&!p->callback_argument&&!p->priority&&p->life==49&&!p->kind&&!p->unknown20&&!p->matrix&&p->unknown2c==31);CHECK(!p->unknown30&&!p->unknown32&&!p->command_count&&!p->unknown35&&!p->unknown36&&!p->command_table&&!p->id_table&&!p->argument_table);}
 CHECK(command==77&&id==88&&arg==99);puts("PASS real16prevlinks/8actionpromptreset, preservedunknowns/next/foreigndata");return 0;}
