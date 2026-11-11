#include "game_enemy_manager.h"
#include "game_title_player.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
static uint32_t word(const uint8_t*p,size_t o){uint32_t v;memcpy(&v,p+o,4);return v;}
static void put(uint8_t*p,size_t o,uint32_t v){memcpy(p+o,&v,4);}
static int index_of(const gh_game_init_state*s,const void*p){if(!p)return -1;for(unsigned i=0;i<64;i++)if(p==s->enemies+i*20)return (int)i;return -2;}
gh_init_status gh_enemy_manager_find_free_00290958(const gh_game_init_state*s,int*out){if(!s||!out)return GH_INIT_ARGUMENT;*out=-1;for(unsigned i=0;i<64;i++)if(!s->enemies[i*20+16]){*out=(int)i;break;}return GH_INIT_OK;}
gh_init_status gh_enemy_manager_insert_00290048(gh_game_init_state*s,unsigned slot,void*actor,int32_t kind,uint32_t*f){
 if(f)*f=0;if(!s||slot>=64||!actor||kind< -1)return GH_INIT_ARGUMENT;
 uint8_t*raw=s->enemies+slot*20;
 if(word(raw,8)||s->enemy_actors[slot]||word(raw,12)!=UINT32_MAX||raw[16]==1){if(f)*f=0x290048;return GH_INIT_LINKS;}
 int top=index_of(s,s->enemy_top),last=index_of(s,s->enemy_last);uint64_t visited=0;uint32_t count=0,matches=0;int previous=-1,current=top;
 if(top< -1||last< -1||(top==-1)!=(last==-1)){if(f)*f=0x2905d0;return GH_INIT_LINKS;}
 while(current>=0){unsigned i=(unsigned)current;if(i>=64||(visited&(UINT64_C(1)<<i))||i==slot||!s->enemy_actors[i]||s->enemies[i*20+16]!=1||s->enemy_prev[i]!=(previous<0?255:(unsigned)previous)){if(f)*f=0x290988;return GH_INIT_LINKS;}
  visited|=UINT64_C(1)<<i;count++;if(word(s->enemies,i*20+12)==(uint32_t)kind)matches++;previous=current;current=s->enemy_next[i]==255?-1:s->enemy_next[i];
 }
 if(previous!=last||count!=word(s->enemies,0x508)){if(f)*f=0x290988;return GH_INIT_LINKS;}
 /* Original290048, then EntryEm append, then290988 kind-count check. */
 s->enemy_actors[slot]=actor;put(raw,12,(uint32_t)kind);raw[16]=1;
 s->enemy_prev[slot]=last<0?255:(uint8_t)last;s->enemy_next[slot]=255;
 if(last>=0)s->enemy_next[last]=(uint8_t)slot;else s->enemy_top=raw;s->enemy_last=raw;
 put(s->enemies,0x508,count+1);if(!matches)put(s->enemies,0x50c,word(s->enemies,0x50c)+1);
 return GH_INIT_OK;
}

gh_init_status gh_enemy_manager_step_rates_00290250(gh_game_init_state *s,float world_rate,int *special)
{
 if(!s||!special)return GH_INIT_ARGUMENT;
 const int top=index_of(s,s->enemy_top),last=index_of(s,s->enemy_last);
 if(top< -1||last< -1||(top==-1)!=(last==-1))return GH_INIT_LINKS;

 uint8_t *actors[64];
 unsigned count=0;
 uint64_t visited=0;
 int previous=-1;
 int special_kind=0;
 for(int cursor=top;cursor>=0;){
  const unsigned index=(unsigned)cursor;
  if(index>=64||(visited&(UINT64_C(1)<<index))||s->enemies[index*20+16]!=1||
     s->enemy_prev[index]!=(previous<0?UINT8_MAX:(unsigned)previous))return GH_INIT_LINKS;
  const gh_game_title_player *actor=s->enemy_actors[index];
  if(!actor||!actor->view.bytes||actor->view.size<0x5ac)return GH_INIT_BOUNDS;
  visited|=UINT64_C(1)<<index;
  actors[count++]=actor->view.bytes;
  const uint32_t kind=word(actor->view.bytes,0x564);
  if(kind-0x220u<4||kind==0x209||kind==0x260||kind==0x263||kind==0x266)special_kind=1;
  previous=cursor;
  cursor=s->enemy_next[index]==UINT8_MAX?-1:s->enemy_next[index];
 }
 if(previous!=last||count!=word(s->enemies,0x508))return GH_INIT_LINKS;
 if(count){
  float enemy_rate;
  memcpy(&enemy_rate,s->enemies+0x548,4);
  if(!isfinite(world_rate)||!isfinite(enemy_rate))return GH_INIT_BOUNDS;
  fenv_t env;
  if(feholdexcept(&env))return GH_INIT_BOUNDS;
  if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_INIT_BOUNDS;}
  volatile float product=world_rate*enemy_rate;
  const float rate=product;
  if(fesetenv(&env)||!isfinite(rate))return GH_INIT_BOUNDS;
  uint32_t rate_bits;
  memcpy(&rate_bits,&rate,4);
  for(unsigned i=0;i<count;i++){
   uint8_t *actor=actors[i];
   put(actor,0x5a8,rate_bits);
   /* 158288 negates the animation rate when sequence flag 4 is set. */
   put(actor,0x444,rate_bits^((actor[0x434]&4)?UINT32_C(0x80000000):0));
  }
 }
 *special=special_kind;
 return GH_INIT_OK;
}
