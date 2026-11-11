#include "effect_postprocess_reference.h"
#include <stdio.h>
#include <stdlib.h>
#include <fenv.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static gh_effect_trans_packet packet(void){
 gh_effect_trans_packet p={0};
 p.kind=GH_EFFECT_PASS_SCENE_WARP;p.original_callee=0x2f02b8;p.quad_count=1;
 p.quads[0].vertex_count=6;p.prim=0x5d;p.test=0x5000c;
 p.tex0=UINT64_C(0x2000000620113c00);p.writes_texa=1;p.texa=0x80;
 p.alpha=UINT64_C(0x8000000044);p.frame_mask=0xff000000;p.zbuf=UINT64_C(0x13a0000f8);
 const int xy[6][2]={{8,0},{0,-16},{16,-16},{16,16},{0,16},{0,-16}};
 for(unsigned i=0;i<6;i++){
  gh_effect_trans_vertex*v=p.quads[0].vertices+i;
  v->x12_4=28672+xy[i][0]*16;v->y12_4=29184+xy[i][1]*16;
  v->t=(float)(64+xy[i][1]*4)/256;v->q=1;v->rgba=i?0:0x40000000;
 }
 return p;
}
static gh_effect_post_status run(const gh_effect_trans_packet*p,const gh_effect_post_sampler*s){
 return gh_effect_postprocess_warp_source_known(p,s,512,448,28672,29184,0);
}
static void t_all(gh_effect_trans_packet*p,float t){for(unsigned i=0;i<6;i++)p->quads[0].vertices[i].t=t;}
int main(void){
 gh_effect_post_sampler linear={0x60,0,1,GH_EFFECT_SAMPLE_NATIVE_SEPARABLE_4BIT};
 gh_effect_post_sampler nearest={0,0,1,GH_EFFECT_SAMPLE_NATIVE_SEPARABLE_4BIT};
 gh_effect_trans_packet p=packet();
 C(p.quads[0].vertices[1].t==0);
 C(run(&p,&linear)==GH_EFFECT_POST_OK);
 C(run(&p,&nearest)==GH_EFFECT_POST_OK);
 t_all(&p,0);
 C(run(&p,&linear)==GH_EFFECT_POST_BOUNDS);
 C(run(&p,&nearest)==GH_EFFECT_POST_BOUNDS);
 for(unsigned i=0;i<6;i++)p.quads[0].vertices[i].rgba=0;
 C(run(&p,&linear)==GH_EFFECT_POST_OK);
 p.zbuf=0x3a0000f8;
 C(run(&p,&linear)==GH_EFFECT_POST_BOUNDS);
 p=packet();t_all(&p,255.5f/256);
 C(run(&p,&linear)==GH_EFFECT_POST_OK);
 t_all(&p,255.5625f/256);
 C(run(&p,&linear)==GH_EFFECT_POST_BOUNDS);
 C(run(&p,&nearest)==GH_EFFECT_POST_OK);
 t_all(&p,1);
 C(run(&p,&nearest)==GH_EFFECT_POST_BOUNDS);
 t_all(&p,1.5f/256);
 C(run(&p,&linear)==GH_EFFECT_POST_OK);
 t_all(&p,1.4375f/256);
 C(run(&p,&linear)==GH_EFFECT_POST_BOUNDS);
 C(gh_effect_postprocess_warp_source_known(&p,&linear,512,448,28672,29184,1)==GH_EFFECT_POST_OK);
 p=packet();t_all(&p,0);
 for(unsigned i=0;i<6;i++)p.quads[0].vertices[i].y12_4-=32*16;
 C(run(&p,&linear)==GH_EFFECT_POST_OK);
 /* Actual scene409, slot50, serial65 failed the previous vertex-only guard. */
 p=packet();
 const int xy[6][2]={{30906,36689},{29975,36602},{30735,35769},{31838,36775},{31078,37609},{29975,36602}};
 const uint32_t tb[6]={0x3F6B0000u,0x3F698937u,0x3F4F8937u,0x3F6BF6C9u,0x3F82FB64u,0x3F698937u};
 for(unsigned i=0;i<6;i++){
  p.quads[0].vertices[i].x12_4=xy[i][0];p.quads[0].vertices[i].y12_4=xy[i][1];
  memcpy(&p.quads[0].vertices[i].t,tb+i,4);
  p.quads[0].vertices[i].rgba=i?0x00808080u:0x05808080u;
 }
 linear.tex1=UINT64_C(0xffffff3800000360);
 C(p.quads[0].vertices[4].t>1);
 C(run(&p,&linear)==GH_EFFECT_POST_OK);
 const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
 for(unsigned i=0;i<4;i++){
  C(!fesetround(modes[i])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));
  p=packet();C(run(&p,&linear)==GH_EFFECT_POST_OK);
  C(fegetround()==modes[i]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);
 }
 puts("PASS integer fan viewport coverage, inert alpha, repeat linear row weights, nearest, known rows and FEnv");
}
