#include "game_prepare.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(void){
 gh_game_prepare *s=malloc(sizeof(*s));uint8_t bytes[GH_CORE_SAVE_BYTES],expected[GH_CORE_SAVE_BYTES],state;
 gh_core_save_view save={bytes,sizeof(bytes)};CHECK(s);
 for(unsigned mode=0;mode<2;mode++){
  memset(s,0xa5,sizeof(*s));memset(bytes,0x6d,sizeof(bytes));memcpy(expected,bytes,sizeof(bytes));state=2;
  bytes[0x80]=expected[0x80]=0x12;bytes[0x81]=expected[0x81]=0x34;
  memset(expected+0x8c,0,12);memset(expected+0x12,0,2);
  for(unsigned i=0;i<128;i++)expected[0x28a+i*16]=expected[0x28b+i*16]=255;
  expected[0x288]=1;expected[0x289]=0;if(!mode)memcpy(expected+0x84,expected+0x80,4);
  CHECK(gh_game_prepare_002a5d40(s,save,mode?0x20:0,&state)==GH_INIT_OK && state==3);
  CHECK(!memcmp(bytes,expected,sizeof(bytes)) && s->global_0061a994==0);
  for(unsigned i=0;i<sizeof(s->fields);i++)CHECK(s->fields[i]==((i>=4&&i<8)||(i>=0x1658&&i<0x1660)?0xa5:0));
 }
 state=2;CHECK(gh_game_prepare_002a5d40(s,(gh_core_save_view){NULL,0},0,&state)==GH_INIT_OK&&state==3);
 state=2;CHECK(gh_game_prepare_002a5d40(s,(gh_core_save_view){bytes,1},0,&state)==GH_INIT_ARGUMENT&&state==2);
 free(s);puts("PASS originalcase2 save/item reset, room20 preservation, exact Work spans/gaps and state3");return 0;
}
