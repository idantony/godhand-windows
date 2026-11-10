#include "game_cloth_wind.h"
#include "sdk_rotation_trig.h"
#include "id_layout_rotation.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t u32(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static unsigned u16(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static float rf(const uint8_t*p){float v;memcpy(&v,p,4);return v;}
static void put(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
static float add(float a,float b){volatile float f=a+b;return f;}
static float mul(float a,float b){volatile float f=a*b;return f;}
static float divide(float a,float b){volatile float f=a/b;return f;}
static gh_cloth_status body(gh_game_player_child*c,const gh_title_player_view*player,gh_cloth_runtime_globals*g,gh_static_transform_policy policy,uint32_t*f){
 float sine,cosine;uint32_t pc;if(gh_sdk_rotation_sincos_reference_0030a5d0(rf(player->bytes+0x104),&sine,&cosine,policy,&pc)){*f=pc;return GH_CLOTH_UNSUPPORTED;}
 /* Actual147988 identity+singleSDKY30A7B8, not multi-axiscompose. */
 float matrix[16]={cosine,0,-sine,0,0,1,0,0,sine,0,cosine,0,0,0,0,1},direction[3],source[4];memcpy(source,g->data,12);source[3]=1;
 for(unsigned row=0;row<3;row++){float n=mul(matrix[row],source[0]);n=add(n,mul(matrix[4+row],source[1]));n=add(n,mul(matrix[8+row],source[2]));direction[row]=add(n,mul(matrix[12+row],source[3]));}
 float norm=sqrtf(add(add(mul(direction[0],direction[0]),mul(direction[1],direction[1])),mul(direction[2],direction[2])));if(!isfinite(norm)){*f=0x30a2b0;return GH_CLOTH_NUMERIC;}if(norm>0){float inverse=divide(1,norm);for(unsigned k=0;k<3;k++)direction[k]=mul(direction[k],inverse);}else direction[0]=direction[1]=direction[2]=0;
 float phase=rf(g->data+0x10),unused;if(gh_sdk_rotation_sincos_reference_0030a5d0(phase,&sine,&unused,policy,&pc)){*f=0x1f7ef0;return GH_CLOTH_UNSUPPORTED;}
 float advanced=add(phase,rf(g->data+0x14)),strength=mul(mul(add(sine,1),0.5f),rf(g->data+0xc)),wrapped;
 if(gh_id_angle_reference_001f7d48(advanced,&wrapped)){*f=0x1f7d48;return GH_CLOTH_UNSUPPORTED;}float wind[3];for(unsigned k=0;k<3;k++){wind[k]=mul(direction[k],strength);if(!isfinite(wind[k])){*f=0x100df0;return GH_CLOTH_NUMERIC;}}
 memcpy(g->data+0x10,&wrapped,4);memcpy(c->object.view.bytes+0x20b0,wind,12);put(c->object.view.bytes+0x20ac,u32(c->object.view.bytes+0x20ac)|0x400);return GH_CLOTH_OK;
}
gh_cloth_status gh_cloth_wind_prefix_00100ce0(gh_game_player_child*c,const gh_player_child_attachment*a,const gh_title_player_view*player,gh_cloth_runtime_globals*g,gh_static_transform_policy policy,uint32_t*f){
 if(!c||!c->complete||!c->object.view.bytes||c->object.view.size<GH_PLAYER_CHILD_BYTES||!f)return GH_CLOTH_ARGUMENT;*f=0;uint8_t*p=c->object.view.bytes;if(u16(p+0x2fe)!=0x101)return GH_CLOTH_OK;put(p+0x20ac,u32(p+0x20ac)&~0x400u);
 if(u32(p+0x20d0)){*f=0x100d00;return GH_CLOTH_UNSUPPORTED;}if(!a||!a->attached)return GH_CLOTH_OK;if(a->child!=c||!a->parent_model||c->model.external_parent!=a->parent_model||a->parent_model->native_identity!=a->parent_model)return GH_CLOTH_ARGUMENT;if(u16(a->parent_model->root.bytes+0x2fe)!=0x100)return GH_CLOTH_OK;
 if(!player||player->bytes!=a->parent_model->root.bytes||player->size<0x3ae){*f=0x2db6b0;return GH_CLOTH_UNSUPPORTED;}if(!(u16(player->bytes+0x3ac)&0x800))return GH_CLOTH_OK;if(!g||!g->loaded||!g->initialized)return GH_CLOTH_ARGUMENT;if(policy!=GH_STATIC_BINARY32_REFERENCE){*f=0x147988;return GH_CLOTH_UNSUPPORTED;}
 fenv_t env;if(feholdexcept(&env))return GH_CLOTH_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_CLOTH_NUMERIC;}gh_cloth_status e=body(c,player,g,policy,f);if(fesetenv(&env))return GH_CLOTH_NUMERIC;return e;
}

gh_title_player_status gh_game_player_child_update_wind_00100530(gh_child_wind_update*s,const gh_child_move_bindings*b,gh_player_shared_vector*pivot){
 if(!s||!b)return GH_PLAYER_ARGUMENT;gh_title_player_status e=gh_game_player_child_update_00100530(&s->original,b,pivot);
 if(e!=GH_PLAYER_UNSUPPORTED||s->original.frontier!=0x147988||s->original.stage||s->original.move.stage!=3)return e;
 if(s->wind_prefix_complete)return GH_PLAYER_ARGUMENT;uint32_t pc;gh_cloth_status status=gh_cloth_wind_prefix_00100ce0(b->cloth.springs->child,b->cloth.attachment,b->work5f4,b->cloth.globals,b->cloth.matrix_policy,&pc);if(status){s->original.frontier=pc?pc:0x100ce0;return status==GH_CLOTH_ARGUMENT?GH_PLAYER_ARGUMENT:GH_PLAYER_UNSUPPORTED;}
 s->wind_prefix_complete=1;s->original.move.stage=4;s->original.move.frontier=0;
 return gh_game_player_child_update_00100530(&s->original,b,pivot);
}
