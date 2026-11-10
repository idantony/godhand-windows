#define main original_static_ray_fixture
#include "test_game_ray_static.c"
#undef main
#include "game_player_frame.h"
static void store(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static void scalar(uint8_t*p,float n){memcpy(p,&n,4);}
int main(int argc,char**argv){
 C(argc==2);gh_disc*d;size_t ix;gh_disc_entry en;
 C(!gh_disc_open(argv[1],&d)&&!gh_disc_find(d,"st1/r100.dat",&ix)&&!gh_disc_entry_at(d,ix,&en));
 uint8_t*room=malloc((size_t)en.size);C(room&&!gh_disc_read(d,ix,0,room,(size_t)en.size));gh_disc_close(d);
 gh_bundle bundle;gh_game_scene_tables tables={0};C(!gh_bundle_open(room,(size_t)en.size,&bundle)&&!gh_scene_actors_0012e670(&tables,&bundle));uint8_t*groups=group_masks(&tables);
 gh_game_init_state*init=calloc(1,sizeof(*init));C(init);gh_boot_work work={0};uint8_t raw[GH_TITLE_PLAYER_BYTES]={0};
 gh_game_title_player player={.view={.bytes=raw,.size=sizeof(raw),.alias_f0=raw+0xb0}};
 gh_player_frame_bindings b={.player=&player,.work=&work,.init=init,.actors=&tables};
 const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};unsigned checked_floors=0;
 /* Test the original caller continuation, not just the query leaf: raw LHU
  * attribute width, velocity branches, XYZ/W ownership and late retry. */
 for(unsigned i=0;i<half(tables.akg.bytes+0x28)&&checked_floors<4;i++){
  if(!(groups[i]&2))continue;gh_scene_ray_request seed;request_for(&tables,i,&seed);
  double normal[3];point(tables.actor_links[3].view,half(tables.actor_links[4].view.bytes+(size_t)i*20+8),normal);if(normal[1]<.8)continue;
  gh_scene_ray_request ray={.mask=2,.flags=0x400};for(unsigned k=0;k<3;k++)ray.from[k]=ray.to[k]=seed.from[k];ray.from[1]+=1;ray.to[1]-=20;
  answer expected=oracle(&tables,groups,&ray);if(!expected.hit)continue;
  unsigned attr=half(tables.actor_links[4].view.bytes+(size_t)expected.id*20+12);uint32_t attribute=half(tables.actor_links[1].view.bytes+(size_t)attr*16);
  for(unsigned mode=0;mode<4;mode++)for(unsigned branch=0;branch<3;branch++){
   C(!fesetround(FE_TONEAREST));memset(raw,0,sizeof(raw));store(raw+0x15f4,0x3000);
   for(unsigned k=0;k<3;k++){scalar(raw+0xb0+k*4,seed.from[k]);scalar(raw+0x490+k*4,seed.from[k]);}
   float current_y=(float)expected.position[1]+.2f;scalar(raw+0xb4,current_y);scalar(raw+0x494,current_y);
   scalar(raw+0x1608,branch==0?0:(branch==1?1:-1));scalar(raw+0x1604,branch==0?NAN:.1f);
   store(raw+0xbc,0x1234abcd);store(raw+0x48c,0x87654321);
   gh_game_player_frame frame={.b=b,.bound=1,.stage=21};
   C(!fesetround(modes[mode])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));int flags=fetestexcept(FE_ALL_EXCEPT);
   C(gh_game_player_frame_00108ab0(&frame,&b)==GH_PLAYER_FRAME_UNSUPPORTED&&frame.stage==26&&frame.frontier==0x149eb8&&fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==flags);
   C(word(raw+0x474)==attribute&&word(raw+0xbc)==0x1234abcd&&word(raw+0x48c)==0x87654321);
   C(fabs(value(raw+0xb0)-seed.from[0])<.00001&&fabs(value(raw+0xb8)-seed.from[2])<.00001);
   double wanted_y=branch==1?(double)current_y+.9:expected.position[1];C(fabs(value(raw+0xb4)-wanted_y)<.0005&&fabs(value(raw+0x1608)-(branch==1?.9:0))<.00001);
   for(unsigned k=0;k<3;k++)C(fabs(value(raw+0x480+k*4)-expected.normal[k])<.0001);
   uint8_t saved[sizeof(raw)];memcpy(saved,raw,sizeof(raw));uint64_t count=tables.ray.native_queries;
   C(gh_game_player_frame_00108ab0(&frame,&b)==GH_PLAYER_FRAME_UNSUPPORTED&&!memcmp(saved,raw,sizeof(raw))&&count==tables.ray.native_queries);
  }
  checked_floors++;
 }
 C(checked_floors==4&&!fesetround(FE_TONEAREST));
 /* No terrain queries or unread NaN inputs on genuine 15F4 gates. */
 memset(raw,0,sizeof(raw));scalar(raw+0xb4,NAN);scalar(raw+0x1604,NAN);store(raw+0x15f4,0x3080);b.actors=NULL;
 gh_game_player_frame skip={.b=b,.bound=1,.stage=21};
 C(gh_game_player_frame_00108ab0(&skip,&b)==GH_PLAYER_FRAME_UNSUPPORTED&&skip.frontier==0x149eb8&&skip.stage==26&&!word(raw+0x474)&&!word(raw+0x480)&&word(raw+0x484)==0x3f800000&&!word(raw+0x488));
 free(groups);gh_scene_ray_triangle_state_release(&tables);free(room);free(init);
 puts("PASS original108AB0 terrain caller: real R100 floors, LHU attributes, gravity/snap/clamp, late retry, exact gates, 4 FEnv; missing same model remains149EB8");return 0;
}
