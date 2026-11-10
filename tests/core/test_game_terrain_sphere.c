#define main original_ray_static_test
#include "test_game_ray_static.c"
#undef main
#include "game_terrain_sphere.h"
#include "game_terrain_closest.h"
#include "game_scene_dynamic_geometry.h"
/* Independent doubleoracle: project to plane plus closededge candidates;
 * chooses leastEuclidean distance, without the original sevenregion tree. */
static void closest_oracle(const float*p,const float a[3],const float b[3],const float c[3],double*out){
 double v[3][3],pt[3],ab[3],ac[3],normal[3];for(unsigned k=0;k<3;k++){v[0][k]=a[k];v[1][k]=b[k];v[2][k]=c[k];pt[k]=p[k];ab[k]=v[1][k]-v[0][k];ac[k]=v[2][k]-v[0][k];}cross(normal,ab,ac);double n2=dot(normal,normal),offset[3],projected[3];for(unsigned k=0;k<3;k++)offset[k]=pt[k]-v[0][k];double plane=dot(offset,normal)/n2;for(unsigned k=0;k<3;k++)projected[k]=pt[k]-plane*normal[k];
 double best=1e300;for(unsigned edge=0;edge<3;edge++){double delta[3],from[3];for(unsigned k=0;k<3;k++){delta[k]=v[(edge+1)%3][k]-v[edge][k];from[k]=pt[k]-v[edge][k];}double t=dot(from,delta)/dot(delta,delta);if(t<0)t=0;if(t>1)t=1;double candidate[3],d=0;for(unsigned k=0;k<3;k++){candidate[k]=v[edge][k]+t*delta[k];d+=(candidate[k]-pt[k])*(candidate[k]-pt[k]);}if(d<best){best=d;memcpy(out,candidate,24);}}
 int inside=1;for(unsigned edge=0;edge<3;edge++){double delta[3],from[3],side[3];for(unsigned k=0;k<3;k++){delta[k]=v[(edge+1)%3][k]-v[edge][k];from[k]=projected[k]-v[edge][k];}cross(side,delta,from);if(dot(side,normal)<-1e-8)inside=0;}if(inside)memcpy(out,projected,24);
}
static void check_closest(const gh_game_scene_tables*s){
 unsigned checked=0;for(unsigned i=0;i<half(s->akg.bytes+0x28)&&checked<400;i++){
  const uint8_t*t=s->actor_links[4].view.bytes+(size_t)i*20;float v[3][3];for(unsigned j=0;j<3;j++){double d[3];point(s->actor_links[2].view,half(t+j*2),d);for(unsigned k=0;k<3;k++)v[j][k]=(float)d[k];}
  double ab[3],ac[3],n[3];for(unsigned k=0;k<3;k++){ab[k]=(double)v[1][k]-v[0][k];ac[k]=(double)v[2][k]-v[0][k];}cross(n,ab,ac);if(dot(n,n)<1e-8)continue;
  for(unsigned j=0;j<8;j++){float p[3],actual[3];for(unsigned k=0;k<3;k++)p[k]=(v[0][k]+v[1][k]+v[2][k])/3+(float)((int)((j+1)*(k+3)%7)-3)*.63f;double expected[3];closest_oracle(p,v[0],v[1],v[2],expected);C(gh_terrain_closest_00135348(p,v[0],v[1],v[2],actual));for(unsigned k=0;k<3;k++)C(fabs(actual[k]-expected[k])<.0003);checked++;}
 }C(checked>=400);printf("closest actualtriangle independentprojection/edges %u PASS\n",checked);
}
static void check_single_wall(const gh_game_scene_tables*original,const uint8_t*groups,const gh_boot_work*work){
 for(unsigned i=0;i<half(original->akg.bytes+0x28);i++){
  const uint8_t*t=original->actor_links[4].view.bytes+(size_t)i*20;if(!(groups[i]&1)||!(word(t+16)&1)||word(t+16)&2)continue;double n[3];point(original->actor_links[3].view,half(t+8),n);if(fabs(n[1])>.0001||n[0]*n[0]+n[2]*n[2]<.99)continue;
  gh_game_scene_tables one=*original;memset(&one.ray,0,sizeof(one.ray));one.pending_actor_head=NULL;one.pending_actor_count=0;one.dynamic_native_owner=NULL;
  uint8_t grid[8]={0,0,0,0,1,0,0,0},list[4]={(uint8_t)i,(uint8_t)(i>>8),0,0};one.actor_links[5].view=(gh_scene_table_view){grid,8};one.actor_links[6].view=(gh_scene_table_view){list,2};one.actor_count60=one.actor_count64=one.actor_count68=1;
  for(unsigned k=0;k<3;k++){float step=(float)(value((const uint8_t*)&one.actor_vec40[k])-value((const uint8_t*)&one.actor_vec30[k])+1);memcpy(&one.actor_vec50[k],&step,4);}
  gh_scene_ray_request ray;request_for(original,i,&ray);gh_scene_ray_report reports[2];memset(reports,0xa5,sizeof(reports));gh_scene_ray_report sentinel=reports[1];uint32_t count=999;gh_terrain_sphere_request r={.radius=.4f,.mask=1,.flags=0x200,.reports=reports,.report_capacity=1,.report_count=&count};for(unsigned k=0;k<3;k++)r.center[k]=ray.from[k];gh_terrain_sphere_call c={.work=work};float out[3];int hit;uint32_t pc;
  C(!gh_terrain_sphere_query_001321d8(&c,&one,&r,out,&hit,&pc)&&hit&&c.complete&&count==1&&reports[0].triangle_index==i&&!memcmp(&reports[1],&sentinel,sizeof(sentinel)));
  double distance=0;for(unsigned k=0;k<3;k++)distance+=((double)out[k]-((double)ray.from[k]+ray.to[k])*.5)*n[k];double len=sqrt(dot(n,n));C(fabs(distance/len-.4)<.0004&&out[1]==r.center[1]);
  /* Completecursor retains outputs evenifanotherrealquery changes sharedscratch. */
  memset(one.ray.hit_position120,0,12);one.ray.hit_kind180=0;float again[3];C(!gh_terrain_sphere_query_001321d8(&c,&one,&r,again,&hit,&pc)&&hit&&!memcmp(out,again,12));
  /* Latebadlist afterthefirstrealpush preserves its cursor and report. */
  unsigned invalid=(i+1)%half(original->akg.bytes+0x28);uint8_t*poison=malloc(one.actor_links[4].view.size);C(poison);memcpy(poison,one.actor_links[4].view.bytes,one.actor_links[4].view.size);poison[invalid*20+12]=255;poison[invalid*20+13]=255;one.actor_links[4].view.bytes=poison;list[2]=(uint8_t)invalid;list[3]=(uint8_t)(invalid>>8);grid[4]=2;one.actor_links[6].view.size=4;gh_terrain_sphere_call retry={.work=work};count=999;
  C(gh_terrain_sphere_query_001321d8(&retry,&one,&r,out,&hit,&pc)==GH_RAY_BOUNDS&&!hit&&count==999&&one.ray.hit_kind180);uint32_t captured[3];memcpy(captured,one.ray.hit_position120,12);
  for(unsigned repeat=0;repeat<3;repeat++)C(gh_terrain_sphere_query_001321d8(&retry,&one,&r,out,&hit,&pc)==GH_RAY_BOUNDS&&!memcmp(captured,one.ray.hit_position120,12)&&one.ray.triangle17c==1);
  memcpy(poison+invalid*20+12,original->actor_links[4].view.bytes+invalid*20+12,2);C(!gh_terrain_sphere_query_001321d8(&retry,&one,&r,out,&hit,&pc)&&hit&&count==1);
  gh_scene_ray_triangle_state_release(&one);free(poison);puts("single actualwall fixture independent plane-distance .4/report bounds PASS");return;
 }C(0);
}
static void check_dynamic(gh_game_scene_tables*s,const uint8_t*elf,size_t elfsize,const gh_boot_work*work){
 gh_dynamic_corner_tables corners={0};C(!gh_dynamic_corners_from_elf(&corners,elf,elfsize)&&!gh_dynamic_corners_initialize_00133670(&corners,1,0xffff));gh_heap*heap;C(!gh_heap_create(4096,&heap));gh_scene_dynamic_owner owner={0};C(!gh_scene_dynamic_bind(&owner,s,heap));
 uint8_t raw[0x8f0]={0};gh_title_player_view object;C(!gh_game_object_construct_002a7338(raw,sizeof(raw),&object));float mid[3];for(unsigned k=0;k<3;k++)mid[k]=(float)((value((const uint8_t*)&s->actor_vec30[k])+value((const uint8_t*)&s->actor_vec40[k]))*.5);
 memset(raw+0x80,0,64);for(unsigned k=0;k<4;k++){float one=1;memcpy(raw+0x80+k*20,&one,4);}memcpy(raw+0xb0,mid,12);uint8_t attributes[96]={0};for(unsigned k=0;k<6;k++)attributes[k*16+2]=8;float zero[3]={0},size[3]={2,2,2};struct gh_scene_dynamic_box*box;int nonnull;C(!gh_scene_dynamic_box_create_0012ead8(&owner,&object,raw+0x80,zero,zero,size,attributes,&box,&nonnull)&&nonnull);uint32_t pc;C(!gh_scene_dynamic_box_update_0012dcb8(&owner,box,&corners,GH_STATIC_BINARY32_REFERENCE,&pc));
 uint8_t sources[9][0x10]={{0}};gh_title_player_view actors[9];for(unsigned i=0;i<9;i++)actors[i]=(gh_title_player_view){.bytes=sources[i],.size=sizeof(sources[i])};
 gh_scene_ray_report reports[2];memset(reports,0xa5,sizeof(reports));gh_scene_ray_report sentinel=reports[1];uint32_t count=99;gh_terrain_sphere_request request={.actor=&actors[0],.radius=.4f,.mask=8,.reports=reports,.report_capacity=1,.report_count=&count};memcpy(request.center,mid,12);request.center[0]-=1.2f;
 int roundings[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};for(unsigned repeat=0;repeat<4;repeat++){C(!fesetround(roundings[repeat])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));int flags=fetestexcept(FE_ALL_EXCEPT);gh_terrain_sphere_call c={.work=work};float out[3];int hit;gh_scene_ray_status status=gh_terrain_sphere_query_001321d8(&c,s,&request,out,&hit,&pc);C(fegetround()==roundings[repeat]&&fetestexcept(FE_ALL_EXCEPT)==flags);C(!status&&hit&&count==1&&fabsf(out[0]-(mid[0]-1.4f))<.0004f&&out[1]==mid[1]&&out[2]==mid[2]&&!memcmp(&reports[1],&sentinel,sizeof(sentinel)));const gh_title_player_view*contact=NULL;C(!gh_scene_dynamic_sphere_contact_at(&owner,box,0,&contact)&&contact==&actors[0]);gh_scene_dynamic_box_view view;C(!gh_scene_dynamic_box_view_at(&owner,box,&view)&&word(view.bytes+0x54)==1);}
 C(!fesetround(FE_TONEAREST));
 /* Lateforeignvirtual onsecondbox cannot reapply firstbox push/contact. */
 struct gh_scene_dynamic_box*second;C(!gh_scene_dynamic_box_create_0012ead8(&owner,&object,raw+0x80,zero,zero,size,attributes,&second,&nonnull)&&nonnull&&!gh_scene_dynamic_box_update_0012dcb8(&owner,second,&corners,GH_STATIC_BINARY32_REFERENCE,&pc));gh_scene_dynamic_box_view secondview;C(!gh_scene_dynamic_box_view_at(&owner,second,&secondview));uint32_t originalvt=word(secondview.bytes+0x5c),badvt=0xdeadbeef;memcpy(secondview.bytes+0x5c,&badvt,4);
 gh_terrain_sphere_call retry={.work=work};float out[3];int hit;C(gh_terrain_sphere_query_001321d8(&retry,s,&request,out,&hit,&pc)==GH_RAY_UNSUPPORTED&&pc==0x1311a0&&!hit);uint32_t captured[3];memcpy(captured,s->ray.hit_position120,12);C(retry.dynamic_cursor==second);
 for(unsigned repeat=0;repeat<3;repeat++)C(gh_terrain_sphere_query_001321d8(&retry,s,&request,out,&hit,&pc)==GH_RAY_UNSUPPORTED&&pc==0x1311a0&&!memcmp(captured,s->ray.hit_position120,12));
 memcpy(secondview.bytes+0x5c,&originalvt,4);C(!gh_terrain_sphere_query_001321d8(&retry,s,&request,out,&hit,&pc)&&hit&&word(secondview.bytes+0x54)<=1);C(!gh_scene_dynamic_box_abort(&owner,second));
 for(unsigned i=1;i<9;i++){int inserted=99;C(!gh_scene_dynamic_sphere_contact_001311a0(&owner,box,&actors[i],&inserted)&&inserted==(i<8));}
 gh_scene_dynamic_box_view view;C(!gh_scene_dynamic_box_view_at(&owner,box,&view)&&word(view.bytes+0x54)==8);const gh_title_player_view*contact=NULL;C(!gh_scene_dynamic_sphere_contact_at(&owner,box,7,&contact)&&contact==&actors[7]);
 gh_scene_dynamic_forget_sphere_actor(&owner,&actors[0]);C(word(view.bytes+0x54)==8&&gh_scene_dynamic_sphere_contact_at(&owner,box,0,&contact)==GH_DYNAMIC_UNSUPPORTED);
 C(!gh_scene_dynamic_box_update_0012dcb8(&owner,box,&corners,GH_STATIC_BINARY32_REFERENCE,&pc)&&word(view.bytes+0x54)==8&&!gh_scene_dynamic_sphere_contact_at(&owner,box,7,&contact)&&contact==&actors[7]);
 uint32_t one=1;memcpy(view.bytes+0x54,&one,4);int inserted=99;C(gh_scene_dynamic_sphere_contact_001311a0(&owner,box,&actors[8],&inserted)==GH_DYNAMIC_UNSUPPORTED&&!inserted&&word(view.bytes+0x54)==1);
 C(!gh_scene_dynamic_release(&owner)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));puts("dynamic actualELFcorners/constructor/update positiveXYZ/report/dedup/full8/detach/no invented reset PASS");
}
int main(int argc,char**argv){
 C(argc==2);gh_disc*d;size_t ix;gh_disc_entry en;C(!gh_disc_open(argv[1],&d)&&!gh_disc_find(d,"st1/r100.dat",&ix)&&!gh_disc_entry_at(d,ix,&en));uint8_t*room=malloc((size_t)en.size);C(room&&!gh_disc_read(d,ix,0,room,(size_t)en.size));size_t elfsize=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(elfsize);C(elf&&!gh_disc_read_boot_elf(d,0,elf,elfsize));gh_disc_close(d);
 gh_bundle b;gh_game_scene_tables s={0};C(!gh_bundle_open(room,(size_t)en.size,&b)&&!gh_scene_actors_0012e670(&s,&b));uint8_t*g=group_masks(&s);gh_boot_work work={0};check_closest(&s);check_single_wall(&s,g,&work);
 gh_terrain_sphere_request r={.center={100000,100000,100000},.radius=.4f,.mask=1,.flags=0x200};gh_terrain_sphere_call call={.work=&work};float out[3]={77,88,99};int hit;uint32_t pc;
 C(!gh_terrain_sphere_query_001321d8(&call,&s,&r,out,&hit,&pc)&&!hit&&call.complete&&out[0]==77);
 unsigned misses=0,positive=0;
 for(unsigned i=0;i<half(s.akg.bytes+0x28)&&(!positive||misses<8);i++){
  if(!(g[i]&1)||word(s.actor_links[4].view.bytes+(size_t)i*20+16)&2)continue;
  gh_scene_ray_request ray;request_for(&s,i,&ray);double n[3];const uint8_t*t=s.actor_links[4].view.bytes+(size_t)i*20;point(s.actor_links[3].view,half(t+8),n);double len=sqrt(dot(n,n));
  gh_terrain_sphere_request q={.radius=.4f,.mask=1,.flags=0x200};for(unsigned k=0;k<3;k++)q.center[k]=ray.from[k];gh_terrain_sphere_call p={.work=&work};
  gh_scene_ray_status e=gh_terrain_sphere_query_001321d8(&p,&s,&q,out,&hit,&pc);
  if(e==GH_RAY_OK&&hit){C(p.complete&&isfinite(out[0])&&isfinite(out[1])&&isfinite(out[2]));positive++;}
  if(e==GH_RAY_UNSUPPORTED&&pc==0x30a468){
   C(p.pending_triangle&&!hit);uint16_t id=p.triangle;uint32_t flags;C(!gh_scene_ray_triangle_flags(&s,id,&flags)&&(flags&4));gh_scene_ray_state before=s.ray;
   for(unsigned j=0;j<3;j++)C(gh_terrain_sphere_query_001321d8(&p,&s,&q,out,&hit,&pc)==GH_RAY_UNSUPPORTED&&pc==0x30a468&&p.triangle==id&&p.pending_triangle&&!memcmp(before.hit_position120,s.ray.hit_position120,12));positive++;
  }
  /* Radius-bbox fullyabove a floor is an actualearlymiss, not a centerline test. */
  if(misses<8){for(unsigned k=0;k<3;k++)q.center[k]=(float)(((double)ray.from[k]+ray.to[k])*.5+n[k]/len*.8);gh_terrain_sphere_call m={.work=&work};e=gh_terrain_sphere_query_001321d8(&m,&s,&q,out,&hit,&pc);if(!e){C(!hit&&m.complete);misses++;}}
 }
 printf("sphere observed prefixes: misses%u positives%u\n",misses,positive);C(positive&&misses);
 /* Realwall caller→same sphereowner; zero-displacement outsideAKG completes
  * only because sphereAABB itself misses, not because first ray did. */
 uint8_t bytes[GH_TITLE_PLAYER_BYTES]={0};gh_title_player_view actor={.bytes=bytes,.size=sizeof(bytes),.alias_f0=bytes+0xb0};for(unsigned k=0;k<3;k++){float x=100000;memcpy(bytes+0x490+k*4,&x,4);memcpy(actor.alias_f0+k*4,&x,4);}gh_terrain_sphere_call sphere={.work=&work};gh_player_terrain_bindings bindings={&s,&actor,gh_terrain_sphere_query_001321d8,&sphere};gh_player_terrain_call wall={0};C(!gh_player_terrain_wall_00132da0(&wall,&bindings,.4f,0x200,out,NULL,0,NULL,&hit,&pc)&&!hit&&wall.complete&&sphere.complete);
 const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};for(unsigned k=0;k<4;k++){C(!fesetround(modes[k])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));int ex=fetestexcept(FE_ALL_EXCEPT);gh_terrain_sphere_call a={.work=&work};C(!gh_terrain_sphere_query_001321d8(&a,&s,&r,out,&hit,&pc)&&!hit&&fegetround()==modes[k]&&fetestexcept(FE_ALL_EXCEPT)==ex);}
 C(!fesetround(FE_TONEAREST));gh_game_scene_tables empty={0};empty.ray.hit_kind180=1;gh_terrain_sphere_call nullcall={0};C(!gh_terrain_sphere_query_001321d8(&nullcall,&empty,&r,out,&hit,&pc)&&!hit);
 check_dynamic(&s,elf,elfsize,&work);free(elf);gh_scene_ray_triangle_state_release(&s);free(g);free(room);printf("sphere actualR100 miss%u positive%u sharedvisited/retry/FEnv PASS\n",misses,positive);return 0;
}
