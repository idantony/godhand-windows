#include "game_movie_pool.h"
#include <stdlib.h>
#include <string.h>
struct gh_movie_pool {
 struct {void *player;uint32_t cached,clip_flags;uint8_t scalar[0xac];} track[3];
 uint32_t counter,busy_mask,movie_active_3c3cd4,frontier;uint8_t busy;
 gh_movie_provider provider;void *context;int updating;
};
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void w32(uint8_t*p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static void reset(gh_movie_pool*p,unsigned i){memset(p->track[i].scalar,0,0xac);w32(p->track[i].scalar+0x58,UINT32_MAX);w32(p->track[i].scalar+0x5c,UINT32_MAX);p->track[i].scalar[0x40]=31;p->track[i].scalar[0x42]=100;p->track[i].clip_flags=0;}
gh_movie_status gh_movie_pool_create_002b4960(gh_movie_pool**out){
 if(!out)return GH_MOVIE_ARGUMENT;*out=calloc(1,sizeof(gh_movie_pool));if(!*out)return GH_MOVIE_MEMORY;
 for(unsigned i=0;i<3;i++)reset(*out,i);return GH_MOVIE_OK;
}
gh_movie_status gh_movie_pool_set_provider(gh_movie_pool*p,gh_movie_provider f,void*context){
 if(!p)return GH_MOVIE_ARGUMENT;if(p->updating)return GH_MOVIE_BUSY;
 if(!f)for(unsigned i=0;i<3;i++)if(p->track[i].player)return GH_MOVIE_BUSY;p->provider=f;p->context=context;return GH_MOVIE_OK;
}
gh_movie_status gh_movie_pool_first_free_002b4f38(const gh_movie_pool*p,int*slot){if(!p||!slot)return GH_MOVIE_ARGUMENT;*slot=-1;for(unsigned i=0;i<3;i++)if(!p->track[i].player){*slot=(int)i;break;}return GH_MOVIE_OK;}
gh_movie_status gh_movie_pool_commit_started(gh_movie_pool*p,unsigned slot,void*player,uint32_t flags,gh_boot_work*work,uint32_t*handle){
 int free_slot;if(!p||!player||!work||!handle||slot>=3)return GH_MOVIE_ARGUMENT;*handle=0;if(p->updating)return GH_MOVIE_BUSY;
 gh_movie_pool_first_free_002b4f38(p,&free_slot);if(free_slot!=(int)slot)return GH_MOVIE_BUSY;
 p->track[slot].player=player;p->track[slot].clip_flags=flags;if(!++p->counter)p->counter=1;p->track[slot].cached=p->counter;*handle=p->counter;
 uint8_t*raw=(uint8_t*)work;if(flags&0x20){p->busy_mask|=1u<<slot;w32(raw+0x5e0,u32(raw+0x5e0)|0xfd800000u);}
 p->movie_active_3c3cd4=1;w32(raw+0x5e4,u32(raw+0x5e4)|0x2000);return GH_MOVIE_OK;
}
gh_movie_status gh_movie_pool_track_closed(gh_movie_pool*p,unsigned slot,void*player){
 if(!p||slot>=3||!player||p->track[slot].player!=player)return GH_MOVIE_ARGUMENT;
 p->track[slot].player=NULL;reset(p,slot);return GH_MOVIE_OK;
}
static gh_movie_status invoke(gh_movie_pool*p,unsigned slot,gh_movie_pool_operation op){
 uint32_t frontier=0x2d3750;
 if(!p->provider){p->frontier=frontier;return GH_MOVIE_UNSUPPORTED;}
 gh_movie_status e=p->provider(p->context,p,slot,p->track[slot].player,op,&frontier);if(e)p->frontier=frontier?frontier:0x2d3750;return e;
}
gh_movie_status gh_movie_pool_update_002b4a50(gh_movie_pool*p,gh_boot_work*work){
 if(!p||!work)return GH_MOVIE_ARGUMENT;if(p->updating)return GH_MOVIE_BUSY;p->frontier=0;p->updating=1;p->busy=0;
 for(unsigned i=0;i<3;i++){
  if(!p->track[i].player){if(p->track[i].cached){p->track[i].cached=0;if(p->busy_mask){p->busy_mask&=~(1u<<i);if(!p->busy_mask){uint8_t*raw=(uint8_t*)work;w32(raw+0x5e0,u32(raw+0x5e0)&0x027fffff);}}}}
  else {gh_movie_status e=invoke(p,i,u32(p->track[i].scalar+0x60)==1?GH_MOVIE_STOP_002D42E0:GH_MOVIE_UPDATE_002D3750);if(e){p->updating=0;return e;}p->busy=1;}
 }
 p->updating=0;return GH_MOVIE_OK;
}
gh_movie_status gh_movie_pool_is_idle_002b4e48(gh_movie_pool*p,gh_boot_work*work,int*idle){
 if(!idle)return GH_MOVIE_ARGUMENT;*idle=0;gh_movie_status e=gh_movie_pool_update_002b4a50(p,work);if(e)return e;
 *idle=1;for(unsigned i=0;i<3;i++)if(p->track[i].player){*idle=0;break;}return GH_MOVIE_OK;
}
gh_movie_status gh_movie_pool_stop_002b4d68(gh_movie_pool*p,gh_boot_work*work,uint32_t handle){
 if(!p||!work)return GH_MOVIE_ARGUMENT;if(p->updating)return GH_MOVIE_BUSY;p->frontier=0;
 for(unsigned i=0;i<3;i++)if(p->track[i].cached==handle&&handle&&p->track[i].player){w32(p->track[i].scalar+0x60,1);break;}
 uint8_t*raw=(uint8_t*)work;w32(raw+0x5e4,u32(raw+0x5e4)&~0x2000u);return GH_MOVIE_OK;
}
gh_movie_status gh_movie_pool_track_at(const gh_movie_pool*p,unsigned i,gh_movie_track_view*out){if(!p||!out||i>=3)return GH_MOVIE_ARGUMENT;*out=(gh_movie_track_view){p->track[i].player,p->track[i].cached,p->track[i].clip_flags,p->track[i].scalar,0xac};return GH_MOVIE_OK;}
void gh_movie_pool_inspect(const gh_movie_pool*p,gh_movie_pool_view*out){if(!out)return;memset(out,0,sizeof(*out));if(!p)return;uint32_t fps=0x426fc28f;memcpy(&out->default_fps,&fps,4);out->counter=p->counter;out->busy_mask=p->busy_mask;out->movie_active_3c3cd4=p->movie_active_3c3cd4;out->busy=p->busy;out->frontier=p->frontier;}
gh_movie_status gh_movie_pool_destroy(gh_movie_pool*p){if(!p)return GH_MOVIE_ARGUMENT;if(p->updating)return GH_MOVIE_BUSY;for(unsigned i=0;i<3;i++)if(p->track[i].player)return GH_MOVIE_BUSY;free(p);return GH_MOVIE_OK;}
