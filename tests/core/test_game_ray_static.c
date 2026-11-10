#include "game_ray_static.h"
#include "game_collision_scroll.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static unsigned half(const uint8_t*p){return (unsigned)p[0]|(unsigned)p[1]<<8;}
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static double value(const uint8_t*p){float n;memcpy(&n,p,4);return n;}
static uint64_t hash(const uint8_t*p,size_t n){uint64_t h=UINT64_C(14695981039346656037);for(size_t i=0;i<n;i++)h=(h^p[i])*UINT64_C(1099511628211);return h;}
static void point(gh_scene_table_view v,unsigned i,double*out){C((size_t)i*12+12<=v.size);for(unsigned j=0;j<3;j++)out[j]=value(v.bytes+(size_t)i*12+j*4);}
static double dot(const double*a,const double*b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
static void cross(double*out,const double*a,const double*b){out[0]=a[1]*b[2]-a[2]*b[1];out[1]=a[2]*b[0]-a[0]*b[2];out[2]=a[0]*b[1]-a[1]*b[0];}
/* Independent double oracle: globalpolygon scan instead ofgridtraversal;
 * analyticplane intersection, dominant-axis 2D crossing test instead ofthe
 * recovered normalizedradialvectors/crossdot epsilon implementation. */
static int polygon_inside(double v[4][3],unsigned n,const double*p,const double*normal){
 unsigned drop=0;if(fabs(normal[1])>fabs(normal[drop]))drop=1;if(fabs(normal[2])>fabs(normal[drop]))drop=2;unsigned x=(drop+1)%3,y=(drop+2)%3;int inside=0;
 for(unsigned i=0,j=n-1;i<n;j=i++){
  double ax=v[i][x],ay=v[i][y],bx=v[j][x],by=v[j][y];double area=(bx-ax)*(p[y]-ay)-(by-ay)*(p[x]-ax);
  if(fabs(area)<1e-9&&p[x]>=fmin(ax,bx)-1e-9&&p[x]<=fmax(ax,bx)+1e-9&&p[y]>=fmin(ay,by)-1e-9&&p[y]<=fmax(ay,by)+1e-9)return 1;
  if((ay>p[y])!=(by>p[y])){double at=ax+(p[y]-ay)*(bx-ax)/(by-ay);if(p[x]<at)inside=!inside;}
 }return inside;
}
typedef struct {int hit;double position[3],normal[3],t;unsigned id;} answer;
static answer oracle(const gh_game_scene_tables*s,const uint8_t*groups,const gh_scene_ray_request*r){
 answer best={0};best.t=2;double from[3],dir[3];for(unsigned j=0;j<3;j++){from[j]=r->from[j];dir[j]=(double)r->to[j]-from[j];}
 unsigned count=half(s->akg.bytes+0x28);
 for(unsigned i=0;i<count;i++){
  const uint8_t*p=s->actor_links[4].view.bytes+(size_t)i*20;uint32_t flags=word(p+16);if((flags&2)||!(groups[i]&r->mask))continue;
  unsigned attr=half(p+12);C((size_t)attr*16+16<=s->actor_links[1].view.size);if(word(s->actor_links[1].view.bytes+(size_t)attr*16+4)&r->flags)continue;
  double normal[3],v[4][3],plane[3];point(s->actor_links[3].view,half(p+8),normal);if(dot(normal,dir)>=0)continue;
  unsigned n=(flags&1)?4:3;for(unsigned k=0;k<n;k++)point(s->actor_links[2].view,half(p+2*k),v[k]);
  if(n==3){double a[3],b[3];for(unsigned j=0;j<3;j++){a[j]=v[1][j]-v[0][j];b[j]=v[2][j]-v[0][j];}cross(plane,a,b);if(dot(plane,dir)>=0)continue;}else memcpy(plane,normal,sizeof(plane));
  double den=dot(plane,dir);if(den==0)continue;double diff[3];for(unsigned j=0;j<3;j++)diff[j]=v[0][j]-from[j];double t=dot(plane,diff)/den;if(t<0||t>1||t>best.t)continue;
  double hit[3];for(unsigned j=0;j<3;j++)hit[j]=from[j]+dir[j]*t;if(!polygon_inside(v,n,hit,plane))continue;
  best.hit=1;best.t=t;best.id=i;memcpy(best.position,hit,sizeof(hit));double len=sqrt(dot(normal,normal));for(unsigned j=0;j<3;j++)best.normal[j]=len?normal[j]/len:0;
 }return best;
}
static uint8_t*group_masks(const gh_game_scene_tables*s){
 unsigned n=half(s->akg.bytes+0x28);uint8_t*groups=calloc(n,1);C(groups);size_t cells=(size_t)s->actor_count60*s->actor_count64*s->actor_count68;C(cells<=s->actor_links[5].view.size/8);
 for(size_t i=0;i<cells;i++){const uint8_t*c=s->actor_links[5].view.bytes+i*8;uint32_t start=word(c);if(start==UINT32_MAX)continue;
  /* Globalgroupmembershipunion is independent of thisquery's gridAABB. */
  unsigned prefix=0;for(unsigned g=0;g<3;g++){for(unsigned j=0;j<c[4+g];j++){C((uint64_t)start+prefix+j<s->actor_links[6].view.size/2);unsigned id=half(s->actor_links[6].view.bytes+((size_t)start+prefix+j)*2);C(id<n);groups[id]|=(uint8_t)(1u<<g);}prefix+=c[4+g];}
 }
 return groups;
}
static void request_for(const gh_game_scene_tables*s,unsigned id,gh_scene_ray_request*r){
 const uint8_t*p=s->actor_links[4].view.bytes+(size_t)id*20;unsigned n=(word(p+16)&1)?4:3;double center[3]={0},normal[3];for(unsigned i=0;i<n;i++){double v[3];point(s->actor_links[2].view,half(p+i*2),v);for(unsigned j=0;j<3;j++)center[j]+=v[j]/n;}
 point(s->actor_links[3].view,half(p+8),normal);double len=sqrt(dot(normal,normal));C(len>0);*r=(gh_scene_ray_request){.mask=7,.report_limit=8};for(unsigned j=0;j<3;j++){r->from[j]=(float)(center[j]+normal[j]/len*.125);r->to[j]=(float)(center[j]-normal[j]/len*.125);}r->from[3]=r->to[3]=1;
}
static unsigned checked=0,positive=0;
static void check(gh_game_scene_tables*s,const uint8_t*groups,gh_scene_ray_request request){
 fenv_t entry_env;C(!fegetenv(&entry_env));int entry_round=fegetround(),entry_flags=fetestexcept(FE_ALL_EXCEPT);C(!fesetround(FE_TONEAREST));
 answer expected=oracle(s,groups,&request);C(!fesetenv(&entry_env));float pos[4]={77,88,99,111},norm[4]={11,22,33,44};gh_scene_ray_report reports[9];memset(reports,0xa5,sizeof(reports));gh_scene_ray_report sentinel=reports[8];request.hit_position=pos;request.hit_normal=norm;request.native_reports=reports;request.report_limit=8;
 int hit=9;uint32_t frontier=9;gh_scene_ray_status e=gh_scene_ray_query_00131e10(s,&request,&hit,&frontier);C(fegetround()==entry_round&&fetestexcept(FE_ALL_EXCEPT)==entry_flags);
 if(e||hit!=expected.hit){fprintf(stderr,"query %u error%d hit%d/%d frontier%X ray%g,%g,%g ->%g,%g,%g\n",checked,e,hit,expected.hit,frontier,request.from[0],request.from[1],request.from[2],request.to[0],request.to[1],request.to[2]);}
 C(s->ray.reports18c==reports);
 C(e==GH_RAY_OK&&!frontier&&hit==expected.hit&&pos[3]==111&&norm[3]==44&&!memcmp(&reports[8],&sentinel,sizeof(sentinel)));
 if(hit){positive++;
  if(request.segmented){C(s->ray.mode181&&s->ray.triangle17c==1);const uint8_t*t=s->actor_links[4].view.bytes+(size_t)reports[0].triangle_index*20;double v[3],n[3],d[3];point(s->actor_links[2].view,half(t),v);point(s->actor_links[3].view,half(t+8),n);for(unsigned j=0;j<3;j++)d[j]=(double)pos[j]-v[j];C(fabs(dot(d,n))<.0003*sqrt(dot(n,n)));}
  for(unsigned j=0;j<3;j++){C(fabs((double)pos[j]-expected.position[j])<0.0003);C(fabs((double)norm[j]-expected.normal[j])<0.0003);}C(s->ray.triangle17c>0&&s->ray.triangle17c<=8);
  for(unsigned i=0;i<s->ray.triangle17c;i++){C(reports[i].attributes==s->actor_links[1].view.bytes+(size_t)reports[i].attribute_index*16&&reports[i].attribute_bytes==16);for(unsigned j=0;j<i;j++)C(reports[i].triangle_index!=reports[j].triangle_index);}
 }else{C(pos[0]==77&&pos[1]==88&&pos[2]==99&&norm[0]==11&&norm[1]==22&&norm[2]==33&&!s->ray.triangle17c);}
 checked++;C(!fesetenv(&entry_env));
}
int main(int argc,char**argv){C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t index;gh_disc_entry entry;C(!gh_disc_find(d,"st1/r100.dat",&index)&&!gh_disc_entry_at(d,index,&entry));uint8_t*room=malloc((size_t)entry.size);C(room&&!gh_disc_read(d,index,0,room,(size_t)entry.size));gh_disc_close(d);uint64_t original_hash=hash(room,(size_t)entry.size);gh_bundle bundle;C(!gh_bundle_open(room,(size_t)entry.size,&bundle));gh_game_scene_tables tables={0};C(!gh_scene_actors_0012e670(&tables,&bundle));uint8_t*groups=group_masks(&tables);unsigned count=half(tables.akg.bytes+0x28),kinds[2]={0},attribute_filters=0;
 printf("actualR100polygons%u grid%u,%u,%u\n",count,tables.actor_count60,tables.actor_count64,tables.actor_count68);
 gh_scene_ray_request first={0};unsigned first_id=0;
 for(unsigned i=0;i<count;i++){const uint8_t*p=tables.actor_links[4].view.bytes+(size_t)i*20;unsigned kind=word(p+16)&1;if((word(p+16)&2)||!groups[i]||kinds[kind]>=8)continue;gh_scene_ray_request r;request_for(&tables,i,&r);if(!oracle(&tables,groups,&r).hit)continue;
  if(!checked){first=r;first_id=i;}check(&tables,groups,r);kinds[kind]++;
  uint32_t exclude=word(tables.actor_links[1].view.bytes+(size_t)half(p+12)*16+4);if(exclude){r.flags=exclude;check(&tables,groups,r);r.flags=0;attribute_filters++;}
  for(unsigned mask=0;mask<8;mask++){r.mask=mask;check(&tables,groups,r);}r.mask=7;for(unsigned j=0;j<4;j++){float a=r.from[j];r.from[j]=r.to[j];r.to[j]=a;}check(&tables,groups,r);
 }
 C(kinds[0]>=4&&kinds[1]>=4&&positive>=8&&checked>=80);
 const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
 for(unsigned i=0;i<4;i++){C(!fesetround(modes[i])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));int expected=fetestexcept(FE_ALL_EXCEPT);check(&tables,groups,first);C(fegetround()==modes[i]&&fetestexcept(FE_ALL_EXCEPT)==expected);}C(!fesetround(FE_TONEAREST));
 uint32_t flags;C(!gh_scene_ray_triangle_flags(&tables,(uint16_t)first_id,&flags)&&(flags&4));gh_scene_ray_request zero=first;zero.mask=0;check(&tables,groups,zero);C(!gh_scene_ray_triangle_flags(&tables,(uint16_t)first_id,&flags)&&(flags&4));
 // Canonical collisionflag2 mutation must remainindependent ofnativevisited4.
 gh_collision_scroll_mutation mutation={0};unsigned name_count=half(tables.akg.bytes+0x20);int mutated=0;
 for(unsigned i=0;i<name_count&&!mutated;i++){
  const uint8_t*p=tables.actor_links[0].view.bytes+(size_t)i*20;unsigned begin=half(p+8),n=half(p+10);if(first_id<begin||first_id>=begin+n)continue;
  char name[9];memcpy(name,p+12,8);name[8]=0;uint32_t changed=0;
  C(!gh_collision_scroll_set_enabled_001332d0(&tables,&mutation,name,0,&changed)&&changed==n&&mutation.owned&&tables.actor_links[4].view.bytes==mutation.owned);
  check(&tables,groups,first);C(!gh_scene_ray_triangle_flags(&tables,(uint16_t)first_id,&flags)&&(flags&2));
  C(!gh_collision_scroll_set_enabled_001332d0(&tables,&mutation,name,1,&changed)&&changed==n);check(&tables,groups,first);
  C(!gh_collision_scroll_mutation_release(&mutation));check(&tables,groups,first);mutated=1;
 }
 C(mutated&&attribute_filters>0);
 // Zero-limit typed/opaque report arrays are neverwritten or rejected merely
 // because anactualhit exists; normalW andpositionW stayborroweduntouched.
 gh_scene_ray_request request=first;float pos[4]={77,88,99,111},normal[4]={11,22,33,44};gh_scene_ray_report reports[9],before[9];memset(reports,0xa5,sizeof(reports));memcpy(before,reports,sizeof(reports));request.hit_position=pos;request.hit_normal=normal;request.native_reports=reports;request.report_limit=0;int hit=9;uint32_t frontier=9;
 C(!gh_scene_ray_query_00131e10(&tables,&request,&hit,&frontier)&&hit&&!frontier&&!tables.ray.triangle17c&&!memcmp(before,reports,sizeof(reports))&&pos[3]==111&&normal[3]==44);
 request.native_reports=NULL;request.reports=reports;C(!gh_scene_ray_query_00131e10(&tables,&request,&hit,&frontier)&&hit&&!memcmp(before,reports,sizeof(reports)));
 // Nativeerrors cannot commit partialcallerreports or vectors, evenafterthe
 // staticqueryfoundrealhits butthecanonicaldynamicshapelist isunported.
 request=first;request.hit_position=pos;request.hit_normal=normal;request.native_reports=reports;request.report_limit=8;uint64_t dynamic_sentinel[4]={0};tables.pending_actor_head=dynamic_sentinel;tables.pending_actor_count=1;
 const float sentinel_pos[4]={77,88,99,111},sentinel_normal[4]={11,22,33,44};memcpy(pos,sentinel_pos,sizeof(pos));memcpy(normal,sentinel_normal,sizeof(normal));
 C(gh_scene_ray_query_00131e10(&tables,&request,&hit,&frontier)==GH_RAY_UNSUPPORTED&&frontier==0x130700&&!hit&&!memcmp(pos,sentinel_pos,sizeof(pos))&&!memcmp(normal,sentinel_normal,sizeof(normal))&&!memcmp(before,reports,sizeof(reports)));tables.pending_actor_head=NULL;tables.pending_actor_count=0;
 uint32_t dynamic_count=99;const void *identities[2]={(void*)1,(void*)2};gh_ray_dynamic_reports dynamic={identities,2,&dynamic_count};request.dynamic_reports=&dynamic;request.native_reports=NULL;request.report_limit=0;C(!gh_scene_ray_query_00131e10(&tables,&request,&hit,&frontier)&&hit&&dynamic_count==0&&identities[0]==(void*)1&&identities[1]==(void*)2);request.dynamic_reports=NULL;request.native_reports=reports;request.report_limit=8;
 memcpy(pos,sentinel_pos,sizeof(pos));memcpy(normal,sentinel_normal,sizeof(normal));memset(reports,0xa5,sizeof(reports));
 gh_scene_table_view list=tables.actor_links[6].view;tables.actor_links[6].view.size=1;
 C(gh_scene_ray_query_00131e10(&tables,&request,&hit,&frontier)==GH_RAY_BOUNDS&&!hit&&!memcmp(pos,sentinel_pos,sizeof(pos))&&!memcmp(normal,sentinel_normal,sizeof(normal))&&!memcmp(before,reports,sizeof(reports)));tables.actor_links[6].view=list;
 // Original131E10 nonzero-extra20 wrapper: shortsinglequery andlong
 // overlapping8-unitsegments share nearestgeometry with the globaloracle.
 gh_scene_ray_request segment=first;segment.segmented=1;check(&tables,groups,segment);
 gh_scene_ray_request longray=first;double center[3],direction[3],length=0;
 for(unsigned j=0;j<3;j++){center[j]=((double)first.from[j]+first.to[j])*.5;direction[j]=(double)first.from[j]-first.to[j];length+=direction[j]*direction[j];}length=sqrt(length);
 for(unsigned j=0;j<3;j++){longray.from[j]=(float)(center[j]+direction[j]/length*32);longray.to[j]=(float)(center[j]-direction[j]/length*2);}longray.segmented=1;
 uint64_t queries=tables.ray.native_queries;check(&tables,groups,longray);C(tables.ray.native_queries>queries+1);
 for(unsigned i=0;i<4;i++){C(!fesetround(modes[i])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));int exceptions=fetestexcept(FE_ALL_EXCEPT);check(&tables,groups,longray);C(fegetround()==modes[i]&&fetestexcept(FE_ALL_EXCEPT)==exceptions);}C(!fesetround(FE_TONEAREST));
 // Bothshort0/1 returntheactualsamepositivepoint; mode1reportsoneevenwhen
 // requestedlimit0, whereasmode0 limit0writesnothing (original181 branch).
 request=first;request.native_reports=reports;request.report_limit=0;request.hit_position=pos;request.hit_normal=normal;memset(reports,0xa5,sizeof(reports));
 C(!gh_scene_ray_query_00131e10(&tables,&request,&hit,&frontier)&&hit&&!tables.ray.mode181&&!tables.ray.triangle17c&&!memcmp(before,reports,sizeof(reports)));float directpos[3];memcpy(directpos,pos,12);
 request.segmented=1;C(!gh_scene_ray_query_00131e10(&tables,&request,&hit,&frontier)&&hit&&tables.ray.mode181&&tables.ray.triangle17c==1&&memcmp(before,reports,sizeof(reports))&&!memcmp(&before[1],&reports[1],sizeof(reports)-sizeof(reports[0])));for(unsigned j=0;j<3;j++)C(fabsf(pos[j]-directpos[j])<.0003f);
 // A24-unit completeoutside miss takesfour overlappedsegments, includingthe
 // finalshorttail. Zero-lengthwrapper makesNO12ED60 call/scratchmutation.
 gh_scene_ray_request outside={.segmented=1,.mask=7};for(unsigned j=0;j<3;j++)outside.from[j]=outside.to[j]=(float)value((const uint8_t*)&tables.actor_vec40[j])+100;outside.to[0]+=24;
 queries=tables.ray.native_queries;check(&tables,groups,outside);C(tables.ray.native_queries==queries+4);
 request=first;request.segmented=1;memcpy(request.to,request.from,sizeof(request.to));request.hit_position=pos;request.hit_normal=normal;request.native_reports=reports;request.report_limit=8;memcpy(pos,sentinel_pos,sizeof(pos));memcpy(normal,sentinel_normal,sizeof(normal));memset(reports,0xa5,sizeof(reports));queries=tables.ray.native_queries;
 C(!gh_scene_ray_query_00131e10(&tables,&request,&hit,&frontier)&&!hit&&!frontier&&queries==tables.ray.native_queries&&!memcmp(pos,sentinel_pos,sizeof(pos))&&!memcmp(normal,sentinel_normal,sizeof(normal))&&!memcmp(before,reports,sizeof(reports)));
 C(hash(room,(size_t)entry.size)==original_hash);gh_scene_ray_triangle_state_release(&tables);C(!tables.ray.triangle_state);C(!gh_scene_actors_reset_0012e800(&tables));free(groups);free(room);
 printf("PASS actualR100 staticrays%u positive%u independentdouble globalpolygon/2Dcoverage oracle,maskgroups/frontback/uniquevisited/reports/fullassetimmutable/FEnv; referencefloatnotEEcapture\n",checked,positive);return 0;
}
