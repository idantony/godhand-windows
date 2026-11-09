#include "game_movie_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
typedef struct {unsigned calls[3],stop_calls;int close_now;} fixture;
/* Onlytestproviderforthescalarstatemachine; not a nativevideodecoderclaim. */
static gh_movie_status update(void*context,gh_movie_pool*p,unsigned slot,void*player,gh_movie_pool_operation op,uint32_t*frontier){
 fixture*c=context;(void)frontier;c->calls[slot]++;if(op==GH_MOVIE_STOP_002D42E0)c->stop_calls++;
 if(c->close_now||op==GH_MOVIE_STOP_002D42E0)return gh_movie_pool_track_closed(p,slot,player);return GH_MOVIE_OK;
}
int main(void){
 gh_movie_pool*p;gh_boot_work work={0};gh_movie_pool_view pool;gh_movie_track_view track;int idle,slot;uint32_t handles[3];int players[3]={0};
 CHECK(gh_movie_pool_create_002b4960(&p)==GH_MOVIE_OK);CHECK(gh_movie_pool_is_idle_002b4e48(p,&work,&idle)==GH_MOVIE_OK&&idle);
 for(unsigned i=0;i<3;i++){CHECK(gh_movie_pool_track_at(p,i,&track)==GH_MOVIE_OK&&!track.native_player&&!track.cached_handle);CHECK(u32(track.scalar_bytes+0x58)==UINT32_MAX&&u32(track.scalar_bytes+0x5c)==UINT32_MAX&&track.scalar_bytes[0x40]==31&&track.scalar_bytes[0x42]==100);}
 for(unsigned i=0;i<3;i++){CHECK(gh_movie_pool_first_free_002b4f38(p,&slot)==GH_MOVIE_OK&&slot==(int)i);CHECK(gh_movie_pool_commit_started(p,i,&players[i],i==1?0:0x20,&work,&handles[i])==GH_MOVIE_OK&&handles[i]==i+1);}
 CHECK(gh_movie_pool_first_free_002b4f38(p,&slot)==GH_MOVIE_OK&&slot==-1);
 CHECK(u32((const uint8_t*)&work+0x5e0)==0xfd800000&&(u32((const uint8_t*)&work+0x5e4)&0x2000));
 CHECK(gh_movie_pool_is_idle_002b4e48(p,&work,&idle)==GH_MOVIE_UNSUPPORTED&&!idle);gh_movie_pool_inspect(p,&pool);CHECK(pool.frontier==0x2d3750&&pool.busy_mask==5);
 CHECK(gh_movie_pool_destroy(p)==GH_MOVIE_BUSY);fixture f={0};CHECK(gh_movie_pool_set_provider(p,update,&f)==GH_MOVIE_OK);
 CHECK(gh_movie_pool_is_idle_002b4e48(p,&work,&idle)==GH_MOVIE_OK&&!idle&&f.calls[0]==1&&f.calls[1]==1&&f.calls[2]==1);
 CHECK(gh_movie_pool_stop_002b4d68(p,&work,handles[1])==GH_MOVIE_OK&&!(u32((const uint8_t*)&work+0x5e4)&0x2000));
 CHECK(f.stop_calls==0);CHECK(gh_movie_pool_is_idle_002b4e48(p,&work,&idle)==GH_MOVIE_OK&&!idle&&f.stop_calls==1);
 CHECK(gh_movie_pool_track_at(p,1,&track)==GH_MOVIE_OK&&!track.native_player&&track.cached_handle==handles[1]);
 f.close_now=1;CHECK(gh_movie_pool_is_idle_002b4e48(p,&work,&idle)==GH_MOVIE_OK&&idle);gh_movie_pool_inspect(p,&pool);CHECK(pool.busy&&pool.busy_mask==5);
 /* Cleanupcachedhandles/maskhappensonnextvisit, nottheEOFvisit. */
 CHECK(gh_movie_pool_is_idle_002b4e48(p,&work,&idle)==GH_MOVIE_OK&&idle);gh_movie_pool_inspect(p,&pool);CHECK(!pool.busy&&!pool.busy_mask&&u32((const uint8_t*)&work+0x5e0)==0);
 CHECK(gh_movie_pool_track_closed(p,0,&players[0])==GH_MOVIE_ARGUMENT);CHECK(gh_movie_pool_destroy(p)==GH_MOVIE_OK);
 puts("PASS original3track manager/idle/error, handleidentity, deferred stop and next-visit mask cleanup (fixture provider)");return 0;
}
