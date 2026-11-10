#include "game_actor_motion.h"
#include "game_actor_motion_ik.h"
#include "game_actor_motion_step.h"
#include "game_actor_motion_solve.h"
#include "game_actor_two_bone.h"
#include "game_actor_finish_model.h"
#include "game_actor_external_parent.h"
#include "game_camera_envelope.h"
#include <math.h>
#include <fenv.h>
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t rd(const uint8_t*p){uint32_t x;memcpy(&x,p,4);return x;}
static void wr(uint8_t*p,uint32_t x){memcpy(p,&x,4);}
static uint8_t*load(gh_disc*d,const char*path,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,path,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*b=malloc(*n);C(b&&!gh_disc_read(d,i,0,b,*n));return b;}
int main(int argc,char**argv){C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t rn,n;uint8_t*r=load(d,"st1/r100.dat",&rn),*data=load(d,"pl/pl00.dat",&n);size_t elfsize=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(elfsize);C(elf&&!gh_disc_read_boot_elf(d,0,elf,elfsize));gh_disc_close(d);gh_bundle room,bundle;gh_bundle_entry md,texture;C(!gh_bundle_open(r,rn,&room)&&!gh_bundle_open(data,n,&bundle)&&!gh_bundle_find(&bundle,"MD",0,&md)&&!gh_bundle_find(&bundle,"TM3",0,&texture));
 gh_heap*heap;C(!gh_heap_create(16*1024*1024,&heap));gh_room_pools pools={0};C(!gh_game_room_pools_initialize_002a5d80(&pools,&room,heap));uint8_t raw[GH_TITLE_PLAYER_BYTES]={0};gh_title_player_view view;C(!gh_title_player_construct_00107e40(raw,sizeof(raw),&view));gh_boot_work work={0};gh_actor_model model={0};int original;
 C(gh_actor_model_initialize_00149738(&model,&view,&work,&pools,&md,&texture,0,0,GH_STATIC_BINARY32_REFERENCE,&original)==GH_ACTOR_MODEL_UNSUPPORTED&&model.parts_ready&&model.part_count==32); /* realmodel constructor prefix, not animation */
 gh_actor_motion_bindings b={.model=&model,.resource=data,.resource_bytes=n,.work=&work};gh_actor_motion state={0};size_t motion=rd(data+0x28),seq=rd(data+0x2c);
 C(gh_actor_motion_begin_002a8578(&state,&b,motion,seq,0)==GH_ACTOR_MOTION_UNSUPPORTED&&state.frontier==0x306200); /* cold570 zero genuinely needs effectservice */
 raw[0x570]=raw[0x571]=255;wr(model.parts[0].bytes+0x100,0x3f800000);wr(model.parts[0].bytes+0x154,0);wr(model.parts[0].alias_d0,0x42c80000);
 C(gh_actor_motion_begin_002a8578(&state,&b,motion,seq,0)==GH_ACTOR_MOTION_UNSUPPORTED&&state.frontier==0x152448&&state.detail==18&&state.stage==5&&!state.complete);
 C(!rd(model.parts[0].bytes+0x100)&&rd(model.parts[0].bytes+0x110)==0x3f800000&&rd(model.parts[0].alias_d0)!=0x42c80000&&rd(raw+0x444)==rd(raw+0x5a8)&&!rd(raw+0x428)&&!rd(raw+0x42c));
 C(state.caches[0].method==1&&state.caches[0].next_frame>0&&state.caches[0].first_value!=0);uint32_t before=rd(model.parts[0].alias_d0);wr(model.parts[0].alias_d0,0x42c80000);C(gh_actor_motion_begin_002a8578(&state,&b,motion,seq,0)==GH_ACTOR_MOTION_UNSUPPORTED&&state.detail==18&&rd(model.parts[0].alias_d0)==0x42c80000);wr(model.parts[0].alias_d0,before);
 gh_actor_motion_bindings foreign=b;foreign.resource=data+1;C(gh_actor_motion_begin_002a8578(&state,&foreign,motion,seq,0)==GH_ACTOR_MOTION_BUSY);
 gh_motion_ik_globals globals={0};C(!gh_motion_ik_globals_from_elf(&globals,elf,elfsize));C(gh_motion_ik_globals_from_elf(&globals,elf,elfsize)==GH_ACTOR_MOTION_ARGUMENT);
 uint32_t frontier;unsigned part;const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};uint8_t first_output[0x60];unsigned positive=0;
 for(unsigned round=0;round<4;round++){
  C(!fesetround(modes[round]));C(!gh_actor_motion_ik_00152448(&model,&state.motion,&globals,GH_STATIC_BINARY32_REFERENCE,&frontier,&part)&&fegetround()==modes[round]&&!frontier);
  if(!round)memcpy(first_output,model.parts[18].bytes+0x150,sizeof(first_output));else C(!memcmp(first_output,model.parts[18].bytes+0x150,sizeof(first_output)));
 }
 C(!fesetround(FE_TONEAREST));
 for(unsigned i=0;i<model.part_count;i++){
  gh_motion_track track;unsigned ctl=0;for(unsigned j=0;j<state.motion.count;j++){C(!gh_motion_track_at(&state.motion,j,&track));if(track.part==i||track.part==127){ctl=track.control&255;break;}}
  if(!ctl||i+2>=model.part_count)continue;positive++;uint8_t*a=model.parts[i].bytes,*middle=model.parts[i+1].bytes,*last=model.parts[i+2].bytes;float aa[3],bb[3],cc[3],length;memcpy(aa,a+0xc0,12);memcpy(bb,middle+0xc0,12);memcpy(cc,last+0xc0,12);
  long double l1=0,l2=0;for(unsigned j=0;j<3;j++){long double x=(long double)bb[j]-aa[j],y=(long double)cc[j]-bb[j];l1+=x*x;l2+=y*y;}memcpy(&length,a+0x158,4);C(fabsl(length-sqrtl(l1))<1e-5L);memcpy(&length,middle+0x158,4);C(fabsl(length-sqrtl(l2))<1e-5L);
  long double dir[3],side[3],up[3],norm=0;for(unsigned j=0;j<3;j++){dir[j]=(long double)aa[j]-cc[j];norm+=dir[j]*dir[j];}norm=sqrtl(norm);for(unsigned j=0;j<3;j++)dir[j]/=norm;
  /* For actualcontrol1: originalaxisZ; independent orthonormal basis.
   * inversebasis*X is (side.x,up.x,direction.x), noquat algorithm copied. */
  C(ctl==1);side[0]=-dir[1];side[1]=dir[0];side[2]=0;norm=sqrtl(side[0]*side[0]+side[1]*side[1]);side[0]/=norm;side[1]/=norm;up[0]=dir[1]*side[2]-dir[2]*side[1];up[1]=dir[2]*side[0]-dir[0]*side[2];up[2]=dir[0]*side[1]-dir[1]*side[0];
  float out[3];memcpy(out,a+0x1a0,12);C(fabsl(out[0]-side[0])<2e-5L&&fabsl(out[1]-up[0])<2e-5L&&fabsl(out[2]-dir[0])<2e-5L);
  C((rd(a+0x154)&0x6000)==0x6000&&(rd(last+0x154)&0x8000));
 }
 C(positive==2&&(rd(raw+0x250)&0x20));float previous[3];memcpy(previous,model.parts[18].bytes+0x1a0,12);globals.xaxis[0]=0;globals.xaxis[1]=1;C(!gh_actor_motion_ik_00152448(&model,&state.motion,&globals,GH_STATIC_BINARY32_REFERENCE,&frontier,&part));C(memcmp(previous,model.parts[18].bytes+0x1a0,12));C(globals.xaxis[0]==0&&globals.xaxis[1]==1); /* borrowed mutableglobal not reset */
 b.ik_globals=&globals;C(!gh_actor_motion_begin_002a8578(&state,&b,motion,seq,0)&&state.complete);C(!gh_actor_motion_begin_002a8578(&state,&b,motion,seq,0));
 gh_actor_model short_model=model;short_model.root.size=0x578;gh_actor_motion_bindings short_b=b;short_b.model=&short_model;gh_actor_motion short_state={0};C(gh_actor_motion_begin_002a8578(&short_state,&short_b,motion,seq,0)==GH_ACTOR_MOTION_ARGUMENT);
 globals.xaxis[0]=1;globals.xaxis[1]=0;globals.xaxis[2]=0; /* restoreoriginalafterexplicitmutableglobaltest */
 gh_actor_motion actual_init={0};C(!gh_actor_motion_begin_002a8578(&actual_init,&b,motion,motion,0)&&actual_init.complete&&!actual_init.sequence_valid); /* raw107F58 a2==a1 MTB3 */
 wr(raw+0x410,0x12345678);wr(raw+0x3ac,0xfeedface);int ended;C(!gh_actor_motion_step_00157c18(&actual_init,1,GH_STATIC_BINARY32_REFERENCE,&ended,&frontier)&&!ended);
 C(rd(raw+0x410)==0x12345678&&rd(raw+0x3ac)==0xfeedface); /* trueNULLSEQ skips clear */
 C(rd(raw+0x430)==0x3f800000&&rd(model.parts[1].bytes+0x100)==0xbd81a100&&rd(model.parts[1].bytes+0x104)==0xbf4c04e0&&rd(model.parts[1].bytes+0x108)==0x3d700080);
 C(rd(model.parts[18].bytes+0x100)==0xbd685900&&rd(model.parts[18].bytes+0x104)==0xbec15c20&&rd(model.parts[18].bytes+0x108)==0xbe186800); /* independent sourceendpoint oracle */
 C((rd(model.parts[18].bytes+0x154)&0x400000)&&(rd(model.parts[20].bytes+0x154)&0x200000));
 uint32_t sampled0=rd(model.parts[1].bytes+0x100);C(!gh_actor_motion_step_00157c18(&actual_init,1,GH_STATIC_BINARY32_REFERENCE,&ended,&frontier)&&!ended&&rd(raw+0x430)==0x40000000&&rd(model.parts[1].bytes+0x100)!=sampled0);
 for(unsigned i=2;i<61;i++)C(!gh_actor_motion_step_00157c18(&actual_init,1,GH_STATIC_BINARY32_REFERENCE,&ended,&frontier));C(ended&&raw[0x436]==1&&rd(raw+0x430)==0x3f800000);
 work.phase569=1;C(!gh_actor_motion_step_00157c18(&actual_init,.5f,GH_STATIC_BINARY32_REFERENCE,&ended,&frontier)&&!ended&&rd(raw+0x430)==0x3fa00000); /* phasehalf .5*.5=.25 */
 work.phase569=2;gh_actor_motion candidate=state;C(candidate.sequence_valid);uint32_t before_time=rd(raw+0x430);data[seq+10]=1;C(gh_actor_motion_step_00157c18(&candidate,1,GH_STATIC_BINARY32_REFERENCE,&ended,&frontier)==GH_ACTOR_MOTION_UNSUPPORTED&&frontier==0x2c7fd0&&rd(raw+0x430)==before_time);data[seq+10]=0;
 C(!gh_actor_motion_step_00157c18(&candidate,0,GH_STATIC_BINARY32_REFERENCE,&ended,&frontier)&&rd(raw+0x410)==0&&rd(raw+0x430)==before_time); gh_game_scene_tables actors={0};C(!gh_scene_actors_0012e670(&actors,&room));C(!gh_actor_model_update_hierarchy_00149eb8(&model,GH_STATIC_BINARY32_REFERENCE));
 C(!fesetround(FE_UPWARD));gh_motion_solve solve={0};gh_motion_solve_bindings solve_b={.model=&model,.work=&work,.actors=&actors};uint64_t queries=actors.ray.native_queries;
 C(gh_actor_motion_solve_00152a20(&solve,&solve_b,GH_STATIC_BINARY32_REFERENCE,&frontier)==GH_ACTOR_MOTION_UNSUPPORTED&&frontier==0x153158&&solve.index==18&&solve.stage==5&&actors.ray.native_queries>queries);
 C(fegetround()==FE_UPWARD);C(!fesetround(FE_TONEAREST));C(solve.request.segmented==1&&solve.request.mask==2&&!solve.request.report_limit&&!(rd(model.parts[18].bytes+0x154)&0x400000));
 C(isfinite(solve.target[1]));
 queries=actors.ray.native_queries;uint32_t smoothing=rd(model.parts[18].bytes+0x1b0);C(gh_actor_motion_solve_00152a20(&solve,&solve_b,GH_STATIC_BINARY32_REFERENCE,&frontier)==GH_ACTOR_MOTION_UNSUPPORTED&&frontier==0x153158&&actors.ray.native_queries==queries&&rd(model.parts[18].bytes+0x1b0)==smoothing);
 gh_camera_envelope envelope={0};C(!gh_game_camera_envelope_from_elf(&envelope,elf,elfsize,&frontier));
 uint8_t beforeA[64],beforeB[64];memcpy(beforeA,model.parts[18].bytes+0x80,64);memcpy(beforeB,model.parts[19].bytes+0x80,64);
 C(!gh_actor_two_bone_00153158(&model,18,&work,&globals,envelope.atan_coeff,GH_STATIC_BINARY32_REFERENCE,&frontier));
 float av[3],bv[3],cv[3],la,lb;memcpy(av,model.parts[18].alias_f0,12);memcpy(bv,model.parts[19].alias_f0,12);memcpy(cv,model.parts[20].alias_f0,12);memcpy(&la,model.parts[18].bytes+0x158,4);memcpy(&lb,model.parts[19].bytes+0x158,4);
 long double d1=0,d2=0;for(unsigned j=0;j<3;j++){d1+=((long double)bv[j]-av[j])*((long double)bv[j]-av[j]);d2+=((long double)cv[j]-bv[j])*((long double)cv[j]-bv[j]);}printf("two bone lengths %.9g %.9g / %.9g %.9g\n",la,lb,(double)sqrtl(d1),(double)sqrtl(d2));C(fabsl(sqrtl(d1)-la)<.00002L&&fabsl(sqrtl(d2)-lb)<.00002L);
 uint8_t solvedA[64],solvedB[64];memcpy(solvedA,model.parts[18].bytes+0x80,64);memcpy(solvedB,model.parts[19].bytes+0x80,64);
 for(unsigned rounding=0;rounding<4;rounding++){memcpy(model.parts[18].bytes+0x80,beforeA,64);memcpy(model.parts[19].bytes+0x80,beforeB,64);C(!fesetround(modes[rounding]));C(!gh_actor_two_bone_00153158(&model,18,&work,&globals,envelope.atan_coeff,GH_STATIC_BINARY32_REFERENCE,&frontier)&&fegetround()==modes[rounding]);C(!memcmp(solvedA,model.parts[18].bytes+0x80,64)&&!memcmp(solvedB,model.parts[19].bytes+0x80,64));}C(!fesetround(FE_TONEAREST)); memcpy(model.parts[18].bytes+0x80,beforeA,64);memcpy(model.parts[19].bytes+0x80,beforeB,64);solve_b.ik_globals=&globals;solve_b.atan_coeff=envelope.atan_coeff;queries=actors.ray.native_queries;
 C(!gh_actor_motion_solve_00152a20(&solve,&solve_b,GH_STATIC_BINARY32_REFERENCE,&frontier)&&!frontier&&!solve.stage&&!solve.model&&actors.ray.native_queries>queries);
 C(!(rd(model.parts[18].bytes+0x154)&0x400000)&&!(rd(model.parts[22].bytes+0x154)&0x400000)&&!(rd(model.parts[20].bytes+0x154)&0x200000)&&!(rd(model.parts[24].bytes+0x154)&0x200000));
 for(unsigned i=0;i<32;i++)for(unsigned k=0;k<16;k++){float x;memcpy(&x,model.parts[i].bytes+0x80+k*4,4);C(isfinite(x));} /* Independent longdouble Rz*Ry*Rx oracle for original147C88 mode0,
  * exercising rationalasinf below/above.5 and nearitsendpoint. */
 uint8_t savedPart[0x220];memcpy(savedPart,model.parts[1].bytes,sizeof(savedPart));
 const long double xs[]={-.7L,0,.4L},ys[]={-1.55L,-.7L,0,.7L,1.55L},zs[]={-.3L,.8L};
 for(unsigned xi=0;xi<3;xi++)for(unsigned yi=0;yi<5;yi++)for(unsigned zi=0;zi<2;zi++){
  long double x=xs[xi],y=ys[yi],z=zs[zi],cx=cosl(x),sx=sinl(x),cy=cosl(y),sy=sinl(y),cz=cosl(z),sz=sinl(z),scales[]={1.1L,.7L,1.3L};
  long double oracle[9]={cy*cz,cy*sz,-sy,sx*sy*cz-cx*sz,sx*sy*sz+cx*cz,sx*cy,cx*sy*cz+sx*sz,cx*sy*sz-sx*cz,cx*cy};uint8_t*q=model.parts[1].bytes;
  for(unsigned col=0;col<3;col++){float scale=(float)scales[col];memcpy(q+0x110+col*4,&scale,4);for(unsigned row=0;row<3;row++){float value=(float)(oracle[col*3+row]*scales[col]);memcpy(q+(col*4+row)*4,&value,4);}wr(q+col*16+12,0);}wr(q+60,0x3f800000);memset(q+0x130,0,12);
  C(!gh_actor_part_extract_rebuild_00147c88(&model,1,envelope.atan_coeff,GH_STATIC_BINARY32_REFERENCE,&frontier));float angles[3];memcpy(angles,q+0x100,12);C(fabsl(angles[0]-x)<.00002L&&fabsl(angles[1]-y)<.00002L&&fabsl(angles[2]-z)<.00002L);
  for(unsigned col=0;col<3;col++)for(unsigned row=0;row<3;row++){float value;memcpy(&value,q+(col*4+row)*4,4);C(fabsl(value-oracle[col*3+row]*scales[col])<.0001L);}
 }
 memcpy(model.parts[1].bytes,savedPart,sizeof(savedPart));
 uint8_t guardA[64],guardB[64];memcpy(guardA,model.parts[18].bytes+0x80,64);memcpy(guardB,model.parts[19].bytes+0x80,64);float originalC[4];memcpy(originalC,model.parts[20].alias_f0,16);float folded[4];memcpy(folded,model.parts[18].alias_f0,16);folded[0]+=.0001f;memcpy(model.parts[20].alias_f0,folded,16);
 C(gh_actor_two_bone_00153158(&model,18,&work,&globals,envelope.atan_coeff,GH_STATIC_BINARY32_REFERENCE,&frontier)==GH_ACTOR_MOTION_UNSUPPORTED&&frontier==0x3a19f8&&!memcmp(guardA,model.parts[18].bytes+0x80,64)&&!memcmp(guardB,model.parts[19].bytes+0x80,64));memcpy(model.parts[20].alias_f0,originalC,16); uint8_t local_before[32][64],world_golden[32][64];for(unsigned i=0;i<32;i++)memcpy(local_before[i],model.parts[i].bytes,64);
 C(rd(model.root.bytes+0x250)&0x20);for(unsigned rounding=0;rounding<4;rounding++){
  C(!fesetround(modes[rounding]));C(!gh_actor_finish_model_0014a038(&model,GH_STATIC_BINARY32_REFERENCE)&&fegetround()==modes[rounding]);for(unsigned i=0;i<32;i++){C(!memcmp(local_before[i],model.parts[i].bytes,64));if(!rounding)memcpy(world_golden[i],model.parts[i].bytes+0x80,64);else C(!memcmp(world_golden[i],model.parts[i].bytes+0x80,64));}}
 C(!fesetround(FE_TONEAREST));for(unsigned i=0;i<32;i++){gh_actor_part*part=&model.parts[i];const uint8_t*parent=part->parent_is_model?raw:model.parts[part->parent].bytes;
  for(unsigned col=0;col<4;col++)for(unsigned row=0;row<4;row++){long double expected=0;for(unsigned k=0;k<4;k++){float x,y;memcpy(&x,parent+0x80+(k*4+row)*4,4);memcpy(&y,part->bytes+(col*4+k)*4,4);expected+=(long double)x*y;}float value;memcpy(&value,part->bytes+0x80+(col*4+row)*4,4);C(fabsl(value-expected)<.00002L);}}
 uint32_t rootflags=rd(raw+0x250);wr(raw+0x250,rootflags&~0x20u);uint8_t root_before[64];memcpy(root_before,raw+0x80,64);C(!gh_actor_finish_model_0014a038(&model,GH_STATIC_BINARY32_REFERENCE)&&!memcmp(root_before,raw+0x80,64));wr(raw+0x250,rootflags);
 uint32_t partflags=rd(model.parts[1].bytes+0x154);wr(model.parts[1].bytes+0x154,partflags|16);memset(model.parts[1].bytes+0x80,0x15,64);C(!gh_actor_finish_model_0014a038(&model,GH_STATIC_BINARY32_REFERENCE));for(unsigned j=0;j<64;j++)C(model.parts[1].bytes[0x80+j]==0x15);wr(model.parts[1].bytes+0x154,partflags);C(!gh_actor_finish_model_0014a038(&model,GH_STATIC_BINARY32_REFERENCE));
 wr(raw+0x27c,0x3f000000);C(gh_actor_finish_model_0014a038(&model,GH_STATIC_BINARY32_REFERENCE)==GH_ACTOR_MODEL_UNSUPPORTED&&model.frontier==0x147568);wr(raw+0x27c,0); C(!gh_scene_actors_reset_0012e800(&actors)); free(elf); C(!gh_actor_model_release(&model)&&!gh_game_room_pools_release(&pools)&&!gh_heap_destroy(heap));free(r);free(data);puts("PASS actualsampled/IKsolvedPL00 14A038 positive32parts worldupdate, LOCAL matrices preserved, independentlongdouble orderedproductoracle,4FEnv, root20 earlyreturn/part16skip/activeblend strictfrontier");return 0;}
