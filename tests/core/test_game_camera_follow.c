#include "game_camera_follow.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#include <math.h>
#define C(x)do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t word(const void*p){uint32_t x;memcpy(&x,p,4);return x;}
static void put(void*p,uint32_t x){memcpy(p,&x,4);}
static float number(const void*p){float x;memcpy(&x,p,4);return x;}
static void store(void*p,float x){memcpy(p,&x,4);}
typedef struct{unsigned calls;int fail_first;} observer;
static gh_scene_ray_status ray(void*ctx,gh_game_scene_tables*t,const gh_scene_ray_request*r,int*hit,uint32_t*f){observer*o=ctx;o->calls++;if(o->fail_first){*f=0x131e10;return GH_RAY_UNSUPPORTED;}return gh_scene_ray_query_00131e10(t,r,hit,f);}
int main(int argc,char**argv){C(argc==2);gh_disc*d;size_t i;gh_disc_entry e;C(!gh_disc_open(argv[1],&d)&&!gh_disc_find(d,"st1/r100.dat",&i)&&!gh_disc_entry_at(d,i,&e));uint8_t*raw=malloc((size_t)e.size);size_t en=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(en);C(raw&&elf&&!gh_disc_read(d,i,0,raw,(size_t)e.size)&&!gh_disc_read_boot_elf(d,0,elf,en));gh_disc_close(d);gh_bundle room;C(!gh_bundle_open(raw,(size_t)e.size,&room));gh_game_scene_tables tables={0};C(!gh_scene_actors_0012e670(&tables,&room));
 gh_heap*heap;gh_room_pools pools={0};C(!gh_heap_create(32*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&room,heap));gh_boot_work work={0};work.room=0x100;gh_game_title_player player={0};int nonnull;C(!gh_game_player_construct_00312f00(&player,&pools,&work,&nonnull)&&nonnull&&player.view.initial_memory==GH_TITLE_MEMORY_ORIGINAL_FACTORY_ZERO);
 /* Explicit outside-grid pose fixture on actual312F00/107E40-owned player.
  * It is not the runtime spawn31C5C0 result or a successful R100 frame. */
 uint8_t*p=player.view.bytes;for(unsigned a=0;a<3;a++){store(player.view.alias_f0+4*a,10000.f+10*a);store(p+0x80+48+4*a,10000.f+10*a);}store(p+0x104,.5f);store(p+0x15f0,.25f);
 gh_game_init_state init;gh_game_init_state_initialize(&init);init.active_camera=&init.camera;gh_game_pl_camera c={0};C(!gh_game_camera_construct_pl_00139bc8(&c,&init.camera,GH_CAMERA_NATIVE_BINARY32_REFERENCE));gh_camera_follow follow={0};uint32_t f;put(c.fields+0x520,1);put(c.fields+0x3dc,0xaabbccdd);
 const int modes[4]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};uint8_t reference[0x530];for(unsigned mode=0;mode<4;mode++){put(c.fields+0x520,1);C(!fesetround(modes[mode])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));C(!gh_game_camera_bind_default_00139db8(&follow,&c,&init,&player.view,&work,elf,en,&f)&&!f&&follow.bound&&follow.player==&player.view&&follow.camera==&c&&fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);C(word(c.fields+0x524)==1&&word(c.fields+0x520)==0&&word(c.fields+0x3f0)==0&&word(c.fields+0x430)==0&&word(c.fields+0x250)==0x41f80000&&word(c.fields+0x3dc)==0xaabbccdd);if(!mode)memcpy(reference,c.fields,sizeof(reference));else C(!memcmp(reference,c.fields,sizeof(reference)));}
 C(!fesetround(FE_TONEAREST));
 // Independent double trigonometric geometry check for the initial target.
 double sx=sin(.5),cx=cos(.5),x=10000+cx*(-.2),y=10010+1.06,z=10020-sx*(-.2);C(fabs(number(c.fields+0x200)-x)<.003&&fabs(number(c.fields+0x204)-y)<.003&&fabs(number(c.fields+0x208)-z)<.003);C(fabs(number(c.fields+0x210)-number(c.fields+0x200))>1);
 put(c.fields+0x518,3);store(c.fields+0x200,9990);store(c.fields+0x204,9990);store(c.fields+0x208,9990);observer o={0,1};C(gh_game_camera_follow_00139ff8(&follow,&tables,ray,&o,&f)==GH_CAMERA_UNSUPPORTED&&f==0x131e10&&follow.stage==2&&word(c.fields+0x518)==2&&o.calls==1);uint8_t prefix[0x530];memcpy(prefix,c.fields,sizeof(prefix));C(gh_game_camera_follow_00139ff8(&follow,&tables,ray,&o,&f)==GH_CAMERA_UNSUPPORTED&&f==0x131e10&&!memcmp(prefix,c.fields,sizeof(prefix))&&o.calls==2);
 C(follow.ray.mask==1&&follow.ray.segmented==1&&follow.ray.report_limit==0&&fabsf(follow.ray.from[1]-10010.3f)<.001f&&follow.ray.to[1]==10011);
 o.fail_first=0;uint64_t queries=tables.ray.native_queries;C(!fesetround(FE_UPWARD)&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));C(gh_game_camera_follow_00139ff8(&follow,&tables,ray,&o,&f)==GH_CAMERA_UNSUPPORTED&&f==0x13a3c0&&follow.stage==6&&word(c.fields+0x518)==2&&tables.ray.native_queries==queries+2&&tables.ray.mask174==7&&tables.ray.mode181==1&&fegetround()==FE_UPWARD&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);C(!fesetround(FE_TONEAREST));memcpy(prefix,c.fields,sizeof(prefix));C(gh_game_camera_follow_00139ff8(&follow,&tables,NULL,NULL,&f)==GH_CAMERA_UNSUPPORTED&&f==0x13a3c0&&!memcmp(prefix,c.fields,sizeof(prefix))&&tables.ray.native_queries==queries+2);
 /* Extended caller retries must not re-run the earlier137F38 path blend. */
 C(!gh_game_camera_bind_default_00139db8(&follow,&c,&init,&player.view,&work,elf,en,&f));gh_acd_scene_binding path={0};gh_game_camera_path_construct_00137a48(&path);store(path.scalar04+0x14,.5f);store(path.scalar04+0x10,.25f);store(path.scalar04+0xc,8);o.calls=0;o.fail_first=1;
 C(gh_game_camera_step_follow_00138fc8(&c,&init,&path,&player.view,&work,&follow,&tables,ray,&o,&f)==GH_CAMERA_UNSUPPORTED&&f==0x131e10&&follow.stage==2);uint32_t path450=word(c.fields+0x450),path460=word(c.fields+0x460);C(gh_game_camera_step_follow_00138fc8(&c,&init,&path,&player.view,&work,&follow,&tables,ray,&o,&f)==GH_CAMERA_UNSUPPORTED&&f==0x131e10&&word(c.fields+0x450)==path450&&word(c.fields+0x460)==path460&&o.calls==2);
 work.room=0x104;C(!gh_game_camera_bind_default_00139db8(&follow,&c,&init,&player.view,&work,elf,en,&f)&&word(c.fields+0x444)==0x3f947ae1&&word(c.fields+0x450)==0x3d4ccccd);init.active_camera=NULL;memcpy(prefix,c.fields,sizeof(prefix));C(gh_game_camera_bind_default_00139db8(&follow,&c,&init,&player.view,&work,elf,en,&f)==GH_CAMERA_UNSUPPORTED&&f==0x13b9b8&&!memcmp(prefix,c.fields,sizeof(prefix)));C(!gh_scene_actors_reset_0012e800(&tables)&&!gh_game_title_player_release(&player)&&!gh_game_room_pools_release(&pools)&&!gh_heap_destroy(heap));free(raw);free(elf);puts("PASS actualELF default139DB8/13B9B8 pose +R100AKG-owned outside-grid139FF8 terrain rays; independentdouble pose geometry, sameplayer typedlinks, prefixretry/counter,4FEnv,104branch. Explicit fixture; remaining13A3C0 frontier, not completed camera/gameframe");return 0;}




