#include "game_children.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static void put(uint8_t*p,size_t o,uint32_t n){memcpy(p+o,&n,4);}
static void half(uint8_t*p,size_t o,uint16_t n){memcpy(p+o,&n,2);}
/* Typedcontractfixture only: tests realcoreordering/arguments, doesNOTclaim
 * GPUdraw/presentation. ProductionrequiresRoot'srealOT/GPUcoverconsumer. */
typedef struct{gh_render_draw_env*env;unsigned calls;int emitted;uint32_t color;int fail;} fixture;
static gh_child_status submit(void*ctx,const gh_child_cover_quad*q,int*emitted,uint32_t*frontier){fixture*c=ctx;c->calls++;
 C(q->rgba==c->color&&q->mode==1&&q->bank==6&&q->depth==5&&q->descriptor_flags==1&&q->alpha==UINT64_C(0x8000000044)&&q->test_override==0x3001d);
 for(unsigned i=0;i<4;i++){C(q->vertices[i][0]==((i&1)?513:-1)&&q->vertices[i][1]==((i&2)?449:-1)&&q->vertices[i][2]==0&&q->vertices[i][3]==1);}
 C(c->env->scalar.bank==55&&c->env->scalar.depth==66); /* before2C54E8 */
 if(c->fail){*frontier=0x15b0f0;return GH_CHILD_MEMORY;}*emitted=c->emitted;return GH_CHILD_OK;
}
static void prepare_env(gh_render_draw_env*env){memset(env,0,sizeof(*env));env->ready=1;env->scalar.bank=55;env->scalar.depth=66;env->scalar.reserved=0x1234;env->sprites.bytes=(const uint8_t*)env;env->sprites.size=7;}
int main(void){gh_game_children*s;C(!gh_game_children_create(NULL,&s)&&!gh_game_children_initialize_0015fd98(s));const uint8_t*view;size_t n;C(!gh_game_children_manager_view(s,&view,&n)&&n==0xf18);uint8_t*p=(uint8_t*)(void*)view,before[0xf18];memcpy(before,p,n);gh_boot_work work={0};work.flags=0x80092c00;put((uint8_t*)&work,0x5d8,0xf00c6000);gh_render_draw_env env;prepare_env(&env);gh_render_draw_env old=env;uint32_t frontier;
 C(gh_game_children_draw_00160b90(s,&work,&env,NULL,NULL,&frontier)==GH_CHILD_UNSUPPORTED&&frontier==0x15b880&&!memcmp(&old,&env,sizeof(env))&&!memcmp(before,p,n));
 fixture f={&env,0,1,0,0};C(!gh_game_children_draw_00160b90(s,&work,&env,submit,&f,&frontier)&&f.calls==1&&!frontier&&!memcmp(before,p,n));
 C(env.scalar.bank==7&&env.scalar.depth==1&&env.scalar.clip==0xffff&&env.scalar.reserved==0x1234&&env.scalar.alpha==1&&env.scalar.red==1&&env.scalar.blue==1&&env.scalar.green==1&&env.sprites.bytes==old.sprites.bytes&&env.sprites.size==7);
 /* Poisonbothneighbors: actual747A2C is58C, not588 or5D8 sign. */
 prepare_env(&env);put((uint8_t*)&work,0x588,0x80000000);C(!gh_game_children_draw_00160b90(s,&work,&env,submit,&f,&frontier)&&f.calls==2);
 prepare_env(&env);old=env;work.system_flags=0x80000000;put((uint8_t*)&work,0x588,0);C(!gh_game_children_draw_00160b90(s,&work,&env,NULL,NULL,&frontier)&&!memcmp(&old,&env,sizeof(env)));work.system_flags=0;
 put((uint8_t*)&work,0x5dc,0x100000);C(!gh_game_children_draw_00160b90(s,&work,NULL,NULL,NULL,&frontier));put((uint8_t*)&work,0x5dc,0);
 /* Actualpacketallocationfailure stillrunsdrawinit andbank7/depth1, asoriginal. */
 prepare_env(&env);f.emitted=0;C(!gh_game_children_draw_00160b90(s,&work,&env,submit,&f,&frontier)&&env.scalar.bank==7&&env.scalar.depth==1);f.emitted=1;
 prepare_env(&env);old=env;f.fail=1;C(gh_game_children_draw_00160b90(s,&work,&env,submit,&f,&frontier)==GH_CHILD_MEMORY&&frontier==0x15b0f0&&!memcmp(&old,&env,sizeof(env)));f.fail=0;
 /* Selectedchilddrawgate runsAFTERcover, neverassumesall9childrenaredrawn. */
 put(p,0x48,0);half(p,0x50,4);p[0x60+0x59]=1;prepare_env(&env);C(!gh_game_children_draw_00160b90(s,&work,&env,submit,&f,&frontier));p[0x60+0x59]=0;
 prepare_env(&env);C(gh_game_children_draw_00160b90(s,&work,&env,submit,&f,&frontier)==GH_CHILD_UNSUPPORTED&&frontier==0x165e88&&env.scalar.bank==7&&env.scalar.depth==1);
 half(p,0x50,0);prepare_env(&env);C(!gh_game_children_draw_00160b90(s,&work,&env,submit,&f,&frontier));
 put(p,0xed0,0x40112233);f.color=0x40112233;prepare_env(&env);C(!gh_game_children_draw_00160b90(s,&work,&env,submit,&f,&frontier));
 gh_game_children_destroy(s);puts("PASS actualchildrenDraw gates/requiredzeroalphaquad/TEST/order/sharedenv/nullselected/activevtablefrontier; callbackfixtureisnotGPUcompletion");return 0;}
