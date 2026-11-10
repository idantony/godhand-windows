#include "game_ray_query.h"
#include "game_ray_static.h"
#include "../platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
static uint32_t bits(float f){uint32_t n;memcpy(&n,&f,4);return n;}
int main(int argc,char**argv){
 C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t i;gh_disc_entry e;
 C(!gh_disc_find(d,"st0/r020.dat",&i)&&!gh_disc_entry_at(d,i,&e));uint8_t*data=malloc((size_t)e.size);C(data&&!gh_disc_read(d,i,0,data,(size_t)e.size));gh_bundle room;
 C(!gh_bundle_open(data,(size_t)e.size,&room));gh_game_scene_tables s={0};C(!gh_scene_actors_0012e670(&s,&room)&&s.actors_ready&&s.akg.bytes);
 gh_scene_ray_request r={.from={0,.4f,0,1},.to={0,.4f,1,1},.mask=7,.flags=0x10};int hit=99;uint32_t frontier=99;
 float output[]={77,88,99,111};uint32_t reports[8];memset(reports,0xa5,sizeof(reports));r.hit_position=output;r.hit_normal=output;r.reports=reports;r.report_limit=8;
 fenv_t original;C(!fegetenv(&original));int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
 gh_scene_ray_state expected={0};
 for(unsigned m=0;m<4;++m){C(!fesetround(modes[m])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_INVALID|FE_INEXACT));int flags=fetestexcept(FE_ALL_EXCEPT);
  C(!gh_scene_ray_query_00131e10(&s,&r,&hit,&frontier)&&!hit&&!frontier);
  C(fegetround()==modes[m]&&fetestexcept(FE_ALL_EXCEPT)==flags);
  C(output[0]==77&&output[1]==88&&output[2]==99&&output[3]==111&&reports[0]==0xa5a5a5a5);
  C(s.ray.mask174==7&&s.ray.flags178==0x10&&!s.ray.hit_kind180&&!s.ray.triangle17c&&s.ray.distance184==bits(1.0f)&&s.ray.directiond0[2]==bits(1.0f));
  C(s.ray.report_limit140==8&&s.ray.reports18c==reports);
  gh_scene_ray_state q=s.ray;q.native_queries=0;if(!m)expected=q;else C(!memcmp(&q,&expected,sizeof(q)));
 }
 C(!fesetenv(&original));
 /* The actual overlapping R020 horizontal ray now traverses its real grid.
  * It still misses the polygons; independent positiveR100 checks are separate. */
 r.from[1]=0;r.to[1]=0;C(!gh_scene_ray_query_00131e10(&s,&r,&hit,&frontier)&&!hit&&!frontier&&reports[0]==0xa5a5a5a5);
 uint64_t count=s.ray.native_queries;r.segmented=1;C(!gh_scene_ray_query_00131e10(&s,&r,&hit,&frontier)&&!hit&&!frontier&&s.ray.native_queries==count+1&&s.ray.mode181==1);
 gh_game_scene_tables absent={0};r.segmented=0;C(!gh_scene_ray_query_00131e10(&absent,&r,&hit,&frontier)&&!hit&&!absent.ray.native_queries);
 gh_scene_ray_triangle_state_release(&s);free(data);gh_disc_close(d);puts("PASS actual R020 outside/inside-grid misses, canonical query prefix, untouched outputs/reports, segmented frontier and FEnv");return 0;
}
