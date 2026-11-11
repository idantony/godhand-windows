#include "r100_watch_geometry.h"
#include "../core/game_player_targets.h"
#include <fenv.h>
#include <math.h>
#include <string.h>

static gh_r100_script_status fail(uint32_t *frontier,uint32_t address)
{
 *frontier=address;
 return GH_R100_SCRIPT_UNSUPPORTED;
}

static gh_r100_script_status geometry(const void *elf,size_t size,gh_game_scene_tables *tables,
 const float player[4],const float enemy[4],float heading,int *payload,uint32_t *frontier)
{
 if(!isfinite(heading))return fail(frontier,0x2da2d0);
 gh_scene_ray_request ray={.mask=7,.segmented=1};
 memcpy(ray.from,enemy,sizeof(ray.from));
 memcpy(ray.to,player,sizeof(ray.to));
 ray.from[1]+=1;
 ray.to[1]+=1;
 for(unsigned i=0;i<3;i++)
  if(!isfinite(ray.from[i])||!isfinite(ray.to[i]))return fail(frontier,0x2da2d0);
 const float pi=0x1.921fb6p+1f,limit=0x1.921fb6p-1f;
 float delta;
 if(ray.from[0]==ray.to[0]&&ray.from[2]==ray.to[2]){
  /* 2DA2D0 coincident XZ falls into the non-positive X branch. */
  delta=-pi*.5f-heading;
  if(delta>pi)delta-=pi*2;
  if(delta< -pi)delta+=pi*2;
 }else{
  /* Same SDK atan polynomial and wrap sequence as 2DA038; a clipped
   * magnitude equal to the limit is the original false result. */
  uint32_t pc=0;
  if(gh_player_target_angle_002da038(elf,size,ray.to,ray.from,heading,limit,&delta,&pc))
   return fail(frontier,pc?pc:0x2da2d0);
 }
 if(!(fabsf(delta)<limit)){*payload=1;return GH_R100_SCRIPT_OK;}
 int hit;
 uint32_t pc=0;
 if(gh_scene_ray_query_00131e10(tables,&ray,&hit,&pc))return fail(frontier,pc?pc:0x131e10);
 *payload=hit!=0;
 return GH_R100_SCRIPT_OK;
}

gh_r100_script_status gh_r100_at1_geometry(const void *elf,size_t size,gh_game_scene_tables *tables,
 const float player[4],const float enemy[4],float heading,int *payload,uint32_t *frontier)
{
 if(!elf||!tables||!player||!enemy||!payload||!frontier)return GH_R100_SCRIPT_ARGUMENT;
 *frontier=0;
 fenv_t saved;
 if(feholdexcept(&saved))return fail(frontier,0x2da2d0);
 if(fesetround(FE_TONEAREST)){
  (void)fesetenv(&saved);
  return fail(frontier,0x2da2d0);
 }
 int result;
 gh_r100_script_status status=geometry(elf,size,tables,player,enemy,heading,&result,frontier);
 if(fesetenv(&saved))return fail(frontier,0x2da2d0);
 if(!status)*payload=result;
 return status;
}
