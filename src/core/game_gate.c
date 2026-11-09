/* Actual201960,2A6CA8 and2A6DA8 paths, not a new loading/pause policy. */
#include "game_gate.h"
#include <string.h>
uint8_t gh_game_disc_gate_step(gh_game_disc_gate *s,gh_game_disc_inputs i){
 if(!s)return 0;
 if(i.changed==1){s->state=1;s->timer=90;}
 else if(s->state==1 && i.media_ready==1 && i.queue_idle && i.identity_matches){
  if(s->timer==0)s->state=0;
  else {uint32_t n;memcpy(&n,&s->timer,4);--n;memcpy(&s->timer,&n,4);}
 }
 return s->state;
}
static uint32_t word(const gh_boot_work *w,size_t off){uint32_t v;memcpy(&v,(const uint8_t *)w+off,4);return v;}
static void put(gh_boot_work *w,size_t off,uint32_t v){memcpy((uint8_t *)w+off,&v,4);}
static gh_game_status invoke(gh_game_gate *g,gh_game_gate_operation op,uint32_t entry){
 g->failed_operation=op;g->failed_entry=entry;
 if(!g->call)return GH_GAME_UNAVAILABLE;
 g->detail=g->call(g->context,g,op);return g->detail?GH_GAME_UNAVAILABLE:GH_GAME_OK;
}
static gh_game_status sleep_one(gh_game_gate *g){
 if(!g->threads){g->failed_entry=0x2d5aa0;return GH_GAME_UNAVAILABLE;}
 g->detail=(int)gh_task_threads_sleep(g->threads,1);
 if(g->detail){g->failed_entry=0x2d5aa0;return GH_GAME_THREAD;}
 return GH_GAME_OK;
}
#define DO(x) do{gh_game_status e=(x);if(e)return e;}while(0)
gh_game_status gh_game_gate_002a6ca8(gh_game_gate *g){
 if(!g || !g->dvd || !g->dvd->disc)return GH_GAME_ARGUMENT;
 g->failed_entry=0;g->detail=0;
 if(!gh_disc_count(g->dvd->disc))return GH_GAME_DISC;
 for(;;){
  gh_game_disc_inputs input={0,1,g->dvd->current==-1,1};
  /* Source is an immutable open ISO handle, with no optical eject operation.
   * Never clear an actual read failure by pretending original disc identity
   * RPC succeeded. The caller receives that unsupported recovery boundary. */
  if(g->dvd->last_error==GH_DVD_IO){g->failed_entry=0x201960;g->detail=GH_DVD_IO;return GH_GAME_DISC;}
  if(gh_game_disc_gate_step(&g->disc_gate,input)!=1){g->failed_entry=0;return GH_GAME_OK;}
  DO(invoke(g,GH_GATE_COVER_QUAD,0x15b880));DO(sleep_one(g));
 }
}
gh_game_status gh_game_gate_002a6da8(gh_game_gate *g){
 uint32_t saved;
 if(!g || !g->work || !g->game_state)return GH_GAME_ARGUMENT;
 g->failed_entry=0;g->detail=0;saved=word(g->work,0x5e0);
 if((g->work->flags&8) || (word(g->work,0x5e4)&0x6a000000u) ||
    (word(g->work,0x5e8)&0x40000000u) || g->work->room==0x20 ||
    g->game_state[0]!=4 || ((const uint8_t *)g->work)[0x14]!=0)return GH_GAME_OK;
 DO(invoke(g,GH_GATE_MESSAGE_BEGIN,0x13f238));
 put(g->work,0x5e0,word(g->work,0x5e0)|0x2020000u);
 while(!(word(g->work,8)&(g->work->language==0?0x20u:0x40u))){
  DO(invoke(g,GH_GATE_MESSAGE_UPDATE,0x140e58));
  DO(invoke(g,GH_GATE_MESSAGE_DRAW,0x1410a8));
  DO(invoke(g,GH_GATE_COVER_QUAD,0x15b880));DO(sleep_one(g));
 }
 put(g->work,0x5e0,saved);
 DO(invoke(g,GH_GATE_MESSAGE_FINISH,0x2a6da8));
 DO(sleep_one(g));g->failed_entry=0;return GH_GAME_OK;
}
