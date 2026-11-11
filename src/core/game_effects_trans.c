#include "game_effects_trans.h"
#include "game_esp_trail_draw.h"
#include "game_esp14.h"
#include "game_effect_node.h"
#include "game_scene_at_geometry.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
static float number(const void*p){float n;memcpy(&n,p,4);return n;}
static float multiply(float a,float b){volatile float n=a*b;return n;}
static gh_effect_status fail(gh_game_effects_trans*s,uint32_t f,gh_effect_status e){s->frontier=f;return e;}
static int same(const gh_effects_trans_bindings*a,const gh_effects_trans_bindings*b){return a->storage.effects==b->storage.effects&&a->storage.esp==b->storage.esp&&a->storage.emitters==b->storage.emitters&&a->storage.esp_bytes==b->storage.esp_bytes&&a->storage.emitter_bytes==b->storage.emitter_bytes&&a->work==b->work&&a->globals==b->globals&&a->display==b->display&&a->policy==b->policy&&a->cvt_policy==b->cvt_policy&&a->model_effect==b->model_effect&&(a->camera==b->camera||(!a->camera&&b->camera))&&(a->init==b->init||(!a->init&&b->init))&&(a->textures==b->textures||(!a->textures&&b->textures))&&((a->emit==b->emit&&a->context==b->context)||(!a->emit&&b->emit));}
static int cvt_byte(float x,uint8_t*out){if(!isfinite(x)||x < -2147483648.0f||x>=2147483648.0f)return 0;*out=(uint8_t)(int32_t)x;return 1;}
static void vertex(gh_effect_trans_vertex*v,int x,int y,unsigned u,unsigned t,uint32_t rgba,uint32_t z){*v=(gh_effect_trans_vertex){.x12_4=x,.y12_4=y,.z=z,.u12_4=(uint16_t)u,.v12_4=(uint16_t)t,.rgba=rgba,.q=1};}
static void quad(gh_effect_trans_quad*q,int x0,int y0,int x1,int y1,unsigned u0,unsigned v0,unsigned u1,unsigned v1,uint32_t rgba){q->vertex_count=4;vertex(q->vertices,x0,y0,u0,v0,rgba,0);vertex(q->vertices+1,x1,y0,u1,v0,rgba,0);vertex(q->vertices+2,x0,y1,u0,v1,rgba,0);vertex(q->vertices+3,x1,y1,u1,v1,rgba,0);}
static gh_effect_status packet_type6(gh_game_effects_trans*s,const gh_effect_esp_view*v){
 const gh_display_state*d=s->b.display;if(!d||!d->initialized)return fail(s,0x2e9808,GH_EFFECT_ARGUMENT);int width=d->screen_width,height=d->screen_lines;if(width<=0||height<=0||width>2048||height>2048)return fail(s,0x2e9808,GH_EFFECT_UNSUPPORTED);if(s->b.policy!=GH_STATIC_BINARY32_REFERENCE)return fail(s,0x2e8318,GH_EFFECT_UNSUPPORTED);
 unsigned stage=s->call_stage;gh_effect_trans_packet p={0};p.order=(gh_ui_order_group){.bank=4,.depth=12,.identity=((uint64_t)v->serial<<32)|((uint64_t)s->esp_index<<8)|stage};p.slot=s->esp_index;p.serial=v->serial;p.inherited_sampler=1;p.restores_scene_target=1;p.texture_flush_after=1;p.restore_zbuf=UINT64_C(0x3a0000f8);p.zbuf=UINT64_C(0x13a0000f8);p.test=0x30000;p.alpha=UINT64_C(0x8000000044);p.prim=0x15c;p.quad_count=1;int x=(2048-width/2)*16,y=(2048-height/2)*16;
 if(stage<3){p.original_callee=0x2e9808;p.kind=GH_EFFECT_PASS_CLEAR;p.destination=stage==0?GH_EFFECT_SURFACE_3B00:stage==1?GH_EFFECT_SURFACE_3A00:GH_EFFECT_SURFACE_3900;p.prim=6;p.quads[0].vertex_count=2;vertex(p.quads[0].vertices,x,y,0,0,0,0xffff);vertex(p.quads[0].vertices+1,x+128*16,y+128*16,0,0,0,0xffff);}
 else if(stage==3){p.original_callee=0x2e8b38;p.order.depth=11;p.kind=GH_EFFECT_PASS_DOWNSAMPLE;p.source=GH_EFFECT_SURFACE_SCENE_COLOR;p.destination=GH_EFFECT_SURFACE_3900;p.writes_tex0=1;p.tex0=UINT64_C(0x2000000664020e00);p.alpha=UINT64_C(0x8000000048);p.test=0x3080d;quad(p.quads,x,y,x+127*16,y+112*16,8,8,(unsigned)width*16+8,(unsigned)height*16+8,0x80808080u);}
 else if(stage==4||stage==5){p.original_callee=0x2e83d8;p.order.depth=10;p.kind=GH_EFFECT_PASS_GAUSSIAN;p.writes_tex0=1;p.alpha=UINT64_C(0x8000000068);p.quad_count=25;int horizontal=stage==5;p.source=horizontal?GH_EFFECT_SURFACE_3900:GH_EFFECT_SURFACE_3A00;p.destination=horizontal?GH_EFFECT_SURFACE_3A00:GH_EFFECT_SURFACE_3B00;p.tex0=UINT64_C(0x20000005dc008000)|(horizontal?0x3900u:0x3a00u);if(s->b.cvt_policy!=GH_FRAME_CHOP_REFERENCE)return fail(s,0x2e8600,GH_EFFECT_UNSUPPORTED);for(unsigned i=0;i<25;i++){int32_t offset;memcpy(&offset,s->b.globals->gaussian_offsets_61b540+i,4);int limit=horizontal?127:112;if(offset<=-limit||offset>=limit)return fail(s,0x2e83d8,GH_EFFECT_UNSUPPORTED);uint8_t weight;if(!cvt_byte(multiply(number(s->b.globals->gaussian_weights_61b5c0+i),256),&weight))return fail(s,0x2e8600,GH_EFFECT_UNSUPPORTED);uint32_t rgba=(uint32_t)weight*0x10101u|0x80000000u;int dest_begin=offset<0?0:offset,dest_end=offset<0?limit+offset:limit;unsigned source_begin=(unsigned)(offset<0?-offset:0)*16+8,source_end=(unsigned)(offset<0?limit:limit-offset)*16+8;if(horizontal)quad(p.quads+i,x+dest_begin*16,y,x+dest_end*16,y+112*16,source_begin,8,source_end,112*16+8,rgba);else quad(p.quads+i,x,y+dest_begin*16,x+127*16,y+dest_end*16,8,source_begin,127*16+8,source_end,rgba);}}
 else if(stage==6){p.original_callee=0x2e8ee8;p.order.depth=9;p.kind=GH_EFFECT_PASS_COMPOSITE;p.source=GH_EFFECT_SURFACE_3B00;p.destination=GH_EFFECT_SURFACE_SCENE_COLOR;p.frame_mask=0xff000000u;p.writes_tex0=1;p.tex0=UINT64_C(0x20000005dc008000)|0x3b00u;p.alpha=UINT64_C(0x8000000068);p.test=0x3000a;if(word(v->payload+0x2b0))return fail(s,0x2e8ee8,GH_EFFECT_UNSUPPORTED);if(s->b.cvt_policy!=GH_FRAME_CHOP_REFERENCE)return fail(s,0x2e8ee8,GH_EFFECT_UNSUPPORTED);uint8_t colour;if(!cvt_byte(multiply(number(v->payload+0x194),128),&colour))return fail(s,0x2e8ee8,GH_EFFECT_UNSUPPORTED);uint32_t rgba=(uint32_t)colour*0x10101u|0x80000000u;quad(p.quads,x,y,x+width*16,y+height*16,8,8,127*16+8,112*16+8,rgba);}
 else return fail(s,0x2e8318,GH_EFFECT_ARGUMENT);s->packet=p;s->pending=1;return GH_EFFECT_OK;
}
static float add(float a,float b){volatile float n=a+b;return n;}
static float divide(float a,float b){volatile float n=a/b;return n;}
static unsigned half(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
/* 2FE8A0 projects the original screen-space sprite with Q=1. */
static gh_effect_status project_screen_sprite(gh_game_effects_trans*s,const gh_effect_esp_view*v){
 const gh_display_state*d=s->b.display;
 if(!d||!d->initialized)return fail(s,0x2fe8a0,GH_EFFECT_ARGUMENT);
 if(d->screen_width<=0||d->screen_width>2048||d->screen_lines<=0||d->screen_lines>2048||
    d->gs_y_origin<0||d->gs_y_origin>2048||s->b.cvt_policy!=GH_FRAME_CHOP_REFERENCE||
    v->parent_emitter!=-1)return fail(s,0x2fe8a0,GH_EFFECT_UNSUPPORTED);
 float matrix[16];
 uint32_t pc=0;
 gh_effect_status status=gh_effect_node_world_matrix_native(s->b.storage.effects,0,s->esp_index,
    v->serial,matrix,s->b.policy,&pc);
 if(status)return fail(s,pc?pc:0x2b6328,status);
 const uint8_t*p=v->payload;
 float aspect=number(p+0x210),ax=number(p+0x214),ay=number(p+0x218);
 float left=multiply(-ax,aspect),right=multiply(add(1,-ax),aspect),top=-ay,bottom=add(1,-ay);
 const float points[4][3]={{left,top,0},{right,top,0},{left,bottom,0},{right,bottom,0}};
 int32_t x0=(2048-d->screen_width/2)*16,y0=d->gs_y_origin*16;
 int32_t x1=x0+d->screen_width*16,y1=y0+d->screen_lines*16;
 unsigned outside[4]={0};
 gh_effect_sprite_projection result={.original_depth=1};
 for(unsigned i=0;i<4;i++){
    float out[4];
    if(gh_scene_at_apply_world_001508d8(matrix,points[i],out))return fail(s,0x30a2b0,GH_EFFECT_UNSUPPORTED);
    float x=multiply(out[0],16),y=multiply(out[1],16);
    if(!isfinite(x)||!isfinite(y)||x<-65536.0f||x>=65536.0f||y<-65536.0f||y>=65536.0f)
       return fail(s,0x2fe9c0,GH_EFFECT_UNSUPPORTED);
    int32_t gx=(int32_t)x+x0,gy=(int32_t)y+y0;
    if(gx<0xa0||gx>0xff60||gy<0xa0||gy>0xff60){s->sprite=result;return GH_EFFECT_OK;}
    result.xyz_fixed4[i][0]=gx;
    result.xyz_fixed4[i][1]=gy;
    result.xyz_fixed4[i][2]=0xfff00;
    result.q[i]=1;
    outside[0]+=gx<x0;
    outside[1]+=gx>x1;
    outside[2]+=gy<y0;
    outside[3]+=gy>y1;
 }
 result.visible=outside[0]!=4&&outside[1]!=4&&outside[2]!=4&&outside[3]!=4;
 s->sprite=result;
 return GH_EFFECT_OK;
}
static gh_effect_status packet_sprite(gh_game_effects_trans*s,const gh_effect_esp_view*v){
 const uint8_t*p=v->payload;uint32_t flags=word(p+0x11c);if(word(p+0xf0)!=(v->type?0x44f270u:0x44fad0u))return fail(s,0x2feb18,GH_EFFECT_UNSUPPORTED);
 if(!s->call_stage){
  if(v->actor&&(word(v->actor->bytes+0x250)&2u)&&!(flags&0x80u)){
   s->call_stage=3;return GH_EFFECT_OK;
  }
  if(number(p+0x194)<0.001f){s->call_stage=3;return GH_EFFECT_OK;}if(!v->target120_known_null||word(p+0x120))return fail(s,0x2fec38,GH_EFFECT_UNSUPPORTED);gh_effect_status status;
 if(!(flags&0x1000000u)){if(v->type)return fail(s,0x2fe8a0,GH_EFFECT_UNSUPPORTED);status=project_screen_sprite(s,v);if(status)return status;}
 else {gh_effect_sprite_camera camera={s->b.camera,s->b.init,s->b.display,s->b.policy,s->b.cvt_policy};uint32_t f=0;status=gh_effect_sprite_project_002fe0d8(s->b.storage.effects,s->esp_index,v->serial,&camera,&s->sprite,&f);if(status)return fail(s,f,status);}
 if(!s->sprite.visible){s->call_stage=3;return GH_EFFECT_OK;}int depth=s->sprite.original_depth;unsigned near=p[0x24a],fade=p[0x24b],far=half(p+0x276);float factor=1;if(near){if(depth<(int)(near*2)){s->call_stage=3;return GH_EFFECT_OK;}if(depth<(int)((near+fade)*2)){if(!fade)return fail(s,0x2fee20,GH_EFFECT_UNSUPPORTED);factor=divide((float)(depth-(int)(near*2)),(float)(fade*2));}}if(far){if((int)(far*2)<depth){s->call_stage=3;return GH_EFFECT_OK;}if((int)((int)far-(int)fade)*2<depth){if(!fade)return fail(s,0x2fee70,GH_EFFECT_UNSUPPORTED);factor=divide(fabsf((float)(depth-(int)(far*2))),(float)(fade*2));}}if(p[0x24c]){if(p[0x24d]>p[0x24c])return fail(s,0x2feea0,GH_EFFECT_UNSUPPORTED);factor=multiply(factor,divide((float)(p[0x24c]-p[0x24d]),(float)p[0x24c]));}if(!isfinite(factor))return fail(s,0x2fec38,GH_EFFECT_UNSUPPORTED);s->sprite_fade=factor;s->call_stage=1;}
 if(s->call_stage==1){unsigned mode=(int8_t)p[0x101]<3?1:2;uint32_t f=0;gh_effect_status status=gh_effect_sprite_material_00151398(s->b.textures,v->texture,mode,&s->sprite_material,&f);if(status)return fail(s,f,status);s->call_stage=2;}
 if(s->call_stage==2&&!s->pending){const gh_stage_material*m=s->sprite_material;unsigned columns=p[0x245],rows=p[0x246];if(!columns||!rows||!m||!m->width||!m->height)return fail(s,0x2fef78,GH_EFFECT_UNSUPPORTED);float du=divide((float)(m->width/columns),(float)m->width),dv=divide((float)(m->height/rows),(float)m->height);int frame=(int8_t)p[0x244];float u=multiply((float)(frame%(int)columns),du),t=multiply((float)(frame/(int)columns),dv);if(flags&0x10000000u){u=add(u,du);du=-du;}if(flags&0x8000000u){t=add(t,dv);dv=-dv;}if(s->b.cvt_policy!=GH_FRAME_CHOP_REFERENCE)return fail(s,0x2ff1f0,GH_EFFECT_UNSUPPORTED);uint32_t rgba=0;for(unsigned j=0;j<4;j++){float colour=multiply(number(p+0x188+j*4),128);if(!isfinite(colour))return fail(s,0x2ff1f0,GH_EFFECT_UNSUPPORTED);if(colour<0)colour=0;if(colour>255)colour=255;if(j==3){
  if(v->actor){uint32_t frontier=0;gh_effect_status status=gh_effect_actor_sprite_alpha_002fec38(s->b.storage.effects,v,
     colour,s->sprite_fade,&colour,&frontier);if(status)return fail(s,frontier?frontier:0x2fec38,status);}
  else colour=multiply(colour,s->sprite_fade);
 }uint8_t value;if(!cvt_byte(colour,&value))return fail(s,0x2ff1f0,GH_EFFECT_UNSUPPORTED);rgba|=(uint32_t)value<<(j*8);}
 gh_effect_trans_packet packet={0};packet.original_callee=0x2fec38;packet.slot=s->esp_index;packet.serial=v->serial;packet.kind=GH_EFFECT_PASS_SPRITE;packet.source=GH_EFFECT_SURFACE_MATERIAL;packet.destination=GH_EFFECT_SURFACE_SCENE_COLOR;packet.prim=0x54;packet.writes_tex0=1;packet.tex0=m->source_tex0;packet.inherited_sampler=1;packet.writes_clamp=1;packet.clamp=0;packet.material=m;packet.frame_mask=(word(v->record.bytes+4)&0x1000u)?0:0xff000000u;int cls=(int8_t)p[0x101];packet.test=((flags&0x1000u)||cls==4?0x30000u:0x50000u)|0xdu;packet.zbuf=UINT64_C(0x3a0000f8)|((flags&0x80000u)?0:UINT64_C(1)<<32);static const uint64_t alpha[]={0x8000000044,0x8000000048,0x80000000a1,0x8000000041,0x8000000049,0x8000000068,0x8000000009,0x8000000046,0x80000000a4,0x8000000042,0x8000000006};if(p[0x247]>10)return fail(s,0x2d9f68,GH_EFFECT_UNSUPPORTED);packet.alpha=alpha[p[0x247]];packet.restores_scene_target=1;packet.restore_zbuf=0x3a0000f8;packet.quad_count=1;packet.quads[0].vertex_count=4;const float uv[4][2]={{u,t},{add(u,du),t},{u,add(t,dv)},{add(u,du),add(t,dv)}};for(unsigned k=0;k<4;k++){gh_effect_trans_vertex*out=packet.quads[0].vertices+k;out->x12_4=s->sprite.xyz_fixed4[k][0];out->y12_4=s->sprite.xyz_fixed4[k][1];out->z=(uint32_t)s->sprite.xyz_fixed4[k][2]>>4;out->rgba=rgba;out->q=s->sprite.q[k];out->s=multiply(uv[k][0],out->q);out->t=multiply(uv[k][1],out->q);}
 uint32_t bank=2;int depth=s->sprite.original_depth;if(!(flags&0x1000000u)){bank=4;switch(cls){case -2:depth=12;break;case -1:depth=11;break;case 0:depth=10;break;case 1:depth=9;break;case 2:depth=8;break;case 3:bank=6;depth=7;break;case 4:bank=6;depth=3;break;default:return fail(s,0x2ff748,GH_EFFECT_UNSUPPORTED);}}else {if(depth>1020)depth=1020;if(depth<0)depth=0;}if(flags&0x2000u){bank=3;depth=flags&0x4000u?1:2;}else if(flags&0x4000u){bank=3;depth=5;}if(cls>2){if(cls==3){bank=6;depth=7;}else if(cls==4){bank=6;depth=3;}}if(depth==0)return fail(s,0x14ff00,GH_EFFECT_UNSUPPORTED);packet.order=(gh_ui_order_group){.bank=bank,.depth=(uint32_t)depth,.identity=((uint64_t)v->serial<<32)|((uint64_t)s->esp_index<<8)};s->packet=packet;s->pending=1;}
 return GH_EFFECT_OK;
}
static gh_effect_status warp_coordinates(gh_game_effects_trans*s,const gh_effect_esp_view*v,
    gh_effect_trans_quad*fan,uint32_t center_rgba,uint32_t edge_rgba)
{
 const uint8_t*p=v->payload;
 const gh_display_state*d=s->b.display;
 float matrix[16];
 uint32_t pc=0;
 gh_effect_status status=gh_effect_node_world_matrix_native(s->b.storage.effects,0,s->esp_index,
    v->serial,matrix,s->b.policy,&pc);
 if(status)return fail(s,pc?pc:0x2b6328,status);
 float aspect=number(p+0x210),anchor_x=number(p+0x214),anchor_y=number(p+0x218);
 float left=multiply(-anchor_x,aspect),right=multiply(add(1,-anchor_x),aspect);
 float top=-anchor_y,bottom=add(1,-anchor_y);
 const float points[5][3]={{left,top,0},{right,top,0},{left,bottom,0},{right,bottom,0},{0,0,0}};
 const unsigned sequence[]={4,0,1,3,2,0};
 int origin_x=(2048-d->screen_width/2)*16,origin_y=d->gs_y_origin*16;
 float offset=number(p+0x2d4);
 if(!isfinite(offset))return fail(s,0x2f07ac,GH_EFFECT_UNSUPPORTED);

 for(unsigned i=0;i<6;i++){
    unsigned k=sequence[i];
    float transformed[4];
    if(gh_scene_at_apply_world_001508d8(matrix,points[k],transformed))
       return fail(s,0x30a2b0,GH_EFFECT_UNSUPPORTED);
    float x=multiply(transformed[0],16),y=multiply(transformed[1],16);
    if(!isfinite(x)||!isfinite(y)||x<-65536.0f||x>=65536.0f||y<-65536.0f||y>=65536.0f)
       return fail(s,0x2fe9c0,GH_EFFECT_UNSUPPORTED);
    int32_t gs_x=(int32_t)x+origin_x,gs_y=(int32_t)y+origin_y;
    if(gs_x<0xa0||gs_x>0xff60||gs_y<0xa0||gs_y>0xff60)
       return fail(s,0x2feab8,GH_EFFECT_UNSUPPORTED);

    float sx=(float)(gs_x/16-2048+d->screen_width/2);
    float sy=(float)(gs_y/16-2048+d->screen_lines/2);
    float ds=k==4?number(p+0x2b8):(k==0||k==2?offset:-offset);
    float dt=k==4?number(p+0x2bc):(k==0||k==1?offset:-offset);
    if(!isfinite(ds)||!isfinite(dt))return fail(s,0x2f07ac,GH_EFFECT_UNSUPPORTED);
    gh_effect_trans_vertex*out=fan->vertices+i;
    vertex(out,gs_x,gs_y,0,0,i?edge_rgba:center_rgba,0xfff0);
    out->s=add(add(ds,divide(sx,512)),0.001953125f);
    out->t=add(add(dt,divide(sy,512)),0.001953125f);
 }
 fan->vertex_count=6;
 return GH_EFFECT_OK;
}

static gh_effect_status warp_projected_coordinates(gh_game_effects_trans *s,
    const gh_effect_esp_view *v,gh_effect_trans_quad *fan,uint32_t center_rgba,uint32_t edge_rgba)
{
 const gh_display_state *d=s->b.display;
 const unsigned sequence[]={4,0,1,3,2,0};
 const float offset=number(v->payload+0x2d4);
 const int x_origin=2048-d->screen_width/2,y_origin=2048-d->screen_lines/2;
 if(!isfinite(offset))return fail(s,0x2f07ac,GH_EFFECT_UNSUPPORTED);
 for(unsigned i=0;i<6;++i){
    const unsigned corner=sequence[i];
    const int32_t *xyz=i?s->sprite.xyz_fixed4[corner]:s->sprite.center_fixed4;
    const float q=i?s->sprite.q[corner]:s->sprite.center_q;
    const float ds=i?(corner==0||corner==2?offset:-offset):number(v->payload+0x2b8);
    const float dt=i?(corner==0||corner==1?offset:-offset):number(v->payload+0x2bc);
    if(!isfinite(ds)||!isfinite(dt)||!isfinite(q)||q<=0)
       return fail(s,0x2f07ac,GH_EFFECT_UNSUPPORTED);
    gh_effect_trans_vertex *out=fan->vertices+i;
    vertex(out,xyz[0],xyz[1],0,0,i?edge_rgba:center_rgba,(uint32_t)xyz[2]>>4);
    out->q=q;
    out->s=multiply(add(add(ds,divide((float)(xyz[0]/16-x_origin),512)),0.001953125f),q);
    out->t=multiply(add(add(dt,divide((float)(xyz[1]/16-y_origin),512)),0.001953125f),q);
 }
 fan->vertex_count=6;
 return GH_EFFECT_OK;
}

static gh_effect_status packet_type12(gh_game_effects_trans*s,const gh_effect_esp_view*v)
{
 const uint8_t*p=v->payload;
 const gh_display_state*d=s->b.display;
 const uint32_t flags=word(p+0x11c);
 const int projected=(flags&0x1000000u)!=0;
 if(!d||!d->initialized)return fail(s,0x2f02b8,GH_EFFECT_ARGUMENT);
 if(d->screen_width!=512||d->screen_lines!=448||d->gs_y_origin!=1824||
    s->b.policy!=GH_STATIC_BINARY32_REFERENCE||s->b.cvt_policy!=GH_FRAME_CHOP_REFERENCE||
    word(p+0xf0)!=0x44eb70u||!v->target120_known_null||word(p+0x120)||
    p[0x24a]||half(p+0x276)||p[0x24c]||p[0x2e0])
    return fail(s,0x2f02b8,GH_EFFECT_UNSUPPORTED);
 if(projected){
    if(p[0x2e2]||!v->actor||!gh_effect_em00_attack_esp_is_bound(s->b.storage.effects,v))
       return fail(s,0x2f02b8,GH_EFFECT_UNSUPPORTED);
    if(!(flags&0x80u)&&(word(v->actor->bytes+0x250)&2u))return GH_EFFECT_OK;
    const gh_effect_sprite_camera camera={s->b.camera,s->b.init,d,s->b.policy,s->b.cvt_policy};
    uint32_t frontier=0;
    gh_effect_status status=gh_effect_sprite_project_002fe0d8(s->b.storage.effects,s->esp_index,
       v->serial,&camera,&s->sprite,&frontier);
    if(status)return fail(s,frontier,status);
    if(!s->sprite.visible)return GH_EFFECT_OK;
 }else if(v->parent_emitter!=-1||p[0x2e2]!=1){
    return fail(s,0x2f02b8,GH_EFFECT_UNSUPPORTED);
 }

 int cls=(int8_t)p[0x101];
 if(cls<-2||cls>4||p[0x247]>10)return fail(s,0x2ff8a0,GH_EFFECT_UNSUPPORTED);
 unsigned bank=4,depth=(unsigned)(10-cls);
 if(projected){
    bank=2;
    if(s->sprite.original_depth<=0)return fail(s,0x14ff00,GH_EFFECT_UNSUPPORTED);
    depth=s->sprite.original_depth>1020?1020u:(unsigned)s->sprite.original_depth;
 }
 if(flags&0x2000u){bank=3;depth=flags&0x4000u?1:2;}
 else if(flags&0x4000u){bank=3;depth=5;}
 if(cls>2){bank=6;depth=cls==3?7:3;}

 gh_effect_trans_packet packet={0};
 packet.order=(gh_ui_order_group){.bank=bank,.depth=depth,
    .identity=((uint64_t)v->serial<<32)|((uint64_t)s->esp_index<<8)|s->call_stage};
 packet.slot=s->esp_index;
 packet.serial=v->serial;
 packet.writes_tex0=packet.inherited_sampler=packet.restores_scene_target=1;
 packet.frame_mask=0xff000000;
 packet.restore_zbuf=0x3a0000f8;
 packet.zbuf=UINT64_C(0x13a0000f8);
 if(s->call_stage==0){
    static const uint64_t alpha[]={0x8000000044,0x8000000048,0x80000000a1,0x8000000041,0x8000000049,
       0x8000000068,0x8000000009,0x8000000046,0x80000000a4,0x8000000042,0x8000000006};
    packet.original_callee=0x2f02b8;
    packet.kind=GH_EFFECT_PASS_SCENE_WARP;
    packet.source=projected?GH_EFFECT_SURFACE_DISPLAY_BASE0:GH_EFFECT_SURFACE_3C00;
    packet.destination=GH_EFFECT_SURFACE_SCENE_COLOR;
    packet.prim=0x5d;
    packet.test=0x5000c;
    packet.tex0=projected?UINT64_C(0x2000000664120000):UINT64_C(0x2000000620113c00);
    packet.writes_texa=1;
    packet.texa=0x80;
    packet.alpha=alpha[p[0x247]];
    packet.quad_count=1;
    packet.zbuf=UINT64_C(0x3a0000f8)|((flags&0x80000u)?0:UINT64_C(1)<<32);
    uint8_t colour[3];
    for(unsigned j=0;j<3;j++){
       float value=multiply(number(p+0x188+j*4),128);
       if(!isfinite(value))return fail(s,0x2f02b8,GH_EFFECT_UNSUPPORTED);
       if(value<0)value=0;
       if(value>255)value=255;
       if(!cvt_byte(value,colour+j))return fail(s,0x2f02b8,GH_EFFECT_UNSUPPORTED);
    }
    uint8_t center_alpha_byte,edge_alpha;
    float center_alpha=multiply(number(p+0x194),128);
    if(center_alpha<0)center_alpha=0;
    if(center_alpha>255)center_alpha=255;
    if(projected){
       uint32_t frontier=0;
       gh_effect_status status=gh_effect_actor_sprite_alpha_002fec38(s->b.storage.effects,v,
          center_alpha,1,&center_alpha,&frontier);
       if(status)return fail(s,frontier,status);
    }
    if(!cvt_byte(center_alpha,&center_alpha_byte))return fail(s,0x2f02b8,GH_EFFECT_UNSUPPORTED);
    if(!cvt_byte(multiply(multiply(center_alpha,(float)p[0x2e1]),0.003921568859368563f),
        &edge_alpha))return fail(s,0x2f02b8,GH_EFFECT_UNSUPPORTED);
    uint32_t rgb=colour[0]|(uint32_t)colour[1]<<8|(uint32_t)colour[2]<<16;
    const uint32_t center=rgb|(uint32_t)center_alpha_byte<<24,edge=rgb|(uint32_t)edge_alpha<<24;
    gh_effect_status status=projected?warp_projected_coordinates(s,v,packet.quads,center,edge):
       warp_coordinates(s,v,packet.quads,center,edge);
    if(status)return status;
 }else{
    packet.original_callee=0x2fa620;
    packet.kind=GH_EFFECT_PASS_SCENE_CAPTURE;
    packet.source=GH_EFFECT_SURFACE_SCENE_COLOR;
    packet.destination=GH_EFFECT_SURFACE_3C00;
    packet.prim=0x116;
    packet.test=0x30000;
    packet.tex0=UINT64_C(0x2000000664020e00);
    packet.alpha=UINT64_C(0x8000000044);
    packet.texture_flush_after=1;
    packet.quad_count=8;
    int x=(2048-d->screen_width/2)*16,y=(2048-d->screen_lines/2)*16;
    for(unsigned i=0;i<8;i++){
       gh_effect_trans_quad*q=packet.quads+i;
       q->vertex_count=2;
       vertex(q->vertices,x+(int)i*32*16,y,i*64*16+8,4,0x80808080,0xffff);
       vertex(q->vertices+1,x+(int)(i+1)*32*16,y+d->screen_lines*16,
          (i+1)*64*16+8,(unsigned)d->screen_lines*16+4,0x80808080,0xffff);
    }
 }
 s->packet=packet;
 s->pending=1;
 return GH_EFFECT_OK;
}

gh_effect_status gh_effects_trans_begin_002fb310(gh_game_effects_trans*s){if(!s)return GH_EFFECT_ARGUMENT;if(s->identity&&(s->identity!=s||!s->complete))return GH_EFFECT_ARGUMENT;*s=(gh_game_effects_trans){.identity=s};return GH_EFFECT_OK;}
static gh_effect_status body(gh_game_effects_trans*s,const gh_effects_trans_bindings*b){
 if(!s||!b||!b->work)return GH_EFFECT_ARGUMENT;if(s->identity&&s->identity!=s)return GH_EFFECT_ARGUMENT;if(s->bound&&!same(&s->b,b))return fail(s,0x2fb310,GH_EFFECT_ARGUMENT);if(s->bound&&!s->b.emit&&b->emit){s->b.emit=b->emit;s->b.context=b->context;}if(s->bound){if(!s->b.camera)s->b.camera=b->camera;if(!s->b.init)s->b.init=b->init;if(!s->b.textures)s->b.textures=b->textures;}s->frontier=0;if(s->complete)return GH_EFFECT_OK;if(!s->bound){s->identity=s;s->b=*b;s->bound=1;}if(!s->entered){s->gate_10000=word((const uint8_t*)b->work+0x5e4)&0x10000u;if((word((const uint8_t*)b->work+0x5e0)&0x10000000u)||(word((const uint8_t*)b->work+0x58c)&0x400u)){s->complete=1;return GH_EFFECT_OK;}s->entered=1;}
 if(!b->storage.effects||!b->storage.effects->ready||!b->storage.esp||b->storage.esp_bytes<0x100180||!b->globals||!b->globals->initialized)return fail(s,0x2fb310,GH_EFFECT_ARGUMENT);uint8_t*base=b->storage.esp;
 for(;s->esp_index<1024;s->esp_index++){unsigned index=s->esp_index,offset=(index>>5)*4;uint32_t mask=0x80000000u>>(index&31);if(!(word(base+0x100000+offset)&mask)||(word(base+0x100080+offset)&mask)||(word(base+0x100100+offset)&mask)){if(s->serial)return fail(s,0x2fb310,GH_EFFECT_ARGUMENT);continue;}gh_effect_esp_view v;if(gh_effect_create_esp_at(b->storage.effects,index,&v)||!v.initialized||v.payload!=base+(size_t)index*0x400||v.globals!=b->globals)return fail(s,0x2fb310,GH_EFFECT_UNSUPPORTED);if(s->serial&&s->serial!=v.serial)return fail(s,0x2fb310,GH_EFFECT_ARGUMENT);if(((v.actor||v.selector)&&!gh_esp14_nested_sprite_is_bound(v.record,v.actor,v.selector)&&
    !gh_effect_event_part_is_bound(v.record,v.actor,v.selector)&&
    !gh_effect_em00_attack_esp_is_bound(b->storage.effects,&v)&&
    !gh_effect_knockdown_esp_is_bound(b->storage.effects,&v))||
    word(v.payload+0x114))return fail(s,0x2fb310,GH_EFFECT_UNSUPPORTED);
 if(!s->serial&&v.actor&&(word(v.actor->bytes+0x250)&0x8000u))continue;uint32_t flags=word((const uint8_t*)b->work+0x5e4),config=word(v.payload+0x104);if(!s->serial&&s->gate_10000&&!(config&2u)&&!((config&0x40u)?(flags&0x4000000u)!=0:v.actor!=NULL))continue;if(!s->serial)s->serial=v.serial;
 if(!s->call_stage&&!s->pending&&(word(v.payload+0x11c)&0x800000u)){s->serial=s->call_stage=0;continue;}if(v.type==0x14){
   const gh_esp14_provider *provider=b->model_effect;
   if(!provider||!provider->trans)return fail(s,0x2f2758,GH_EFFECT_UNSUPPORTED);
   uint32_t frontier=0;
   gh_effect_status status=provider->trans(provider->context,&v,index,&frontier);
   if(status)return fail(s,frontier?frontier:0x2f2758,status);
   s->serial=s->call_stage=0;
   continue;
 }if(v.type==9){
   if(!s->pending){
      uint32_t frontier=0;
      gh_effect_status status=gh_esp09_draw_002ec398(&s->b,index,&v,&s->packet,&frontier);
      if(status)return fail(s,frontier?frontier:0x2ec398,status);
      s->pending=1;
   }
   if(!b->emit)return fail(s,s->packet.original_callee,GH_EFFECT_UNSUPPORTED);
   gh_effect_status status=b->emit(b->context,&s->packet);
   if(status)return fail(s,s->packet.original_callee,status);
   s->pending=0;s->enqueued++;s->serial=s->call_stage=0;continue;
 }if(v.type==0x1a||v.type==0){gh_effect_status status=packet_sprite(s,&v);if(status)return status;if(s->pending){if(!b->emit)return fail(s,s->packet.original_callee,GH_EFFECT_UNSUPPORTED);status=b->emit(b->context,&s->packet);if(status)return fail(s,s->packet.original_callee,status);s->pending=0;s->enqueued++;}s->serial=s->call_stage=0;s->sprite_material=NULL;continue;}if((v.type==0x13&&word(v.payload+0xf0)==0x44ec68u)||
    (v.type==0x18&&word(v.payload+0xf0)==0x44f078u)){s->serial=s->call_stage=0;continue;}
 if(v.type==0x12){
   const unsigned calls=(word(v.payload+0x11c)&0x1000000u)?1:2;
   while(s->call_stage<calls){
     if(!s->pending){gh_effect_status status=packet_type12(s,&v);if(status)return status;}
     if(!s->pending)break;
     if(!b->emit)return fail(s,s->packet.original_callee,GH_EFFECT_UNSUPPORTED);
     gh_effect_status status=b->emit(b->context,&s->packet);
     if(status)return fail(s,s->packet.original_callee,status);
     s->pending=0;s->enqueued++;s->call_stage++;
   }
   s->serial=s->call_stage=0;
   continue;
 }if(v.type!=6||word(v.payload+0xf0)!=0x44e6d8u)return fail(s,0x2fb310,GH_EFFECT_UNSUPPORTED);
 while(s->call_stage<7){if(!s->pending){gh_effect_status status=packet_type6(s,&v);if(status)return status;}if(!b->emit)return fail(s,s->packet.original_callee,GH_EFFECT_UNSUPPORTED);gh_effect_status status=b->emit(b->context,&s->packet);if(status)return fail(s,s->packet.original_callee,status);s->pending=0;s->enqueued++;s->call_stage++;}s->serial=s->call_stage=0;
 }s->complete=1;return GH_EFFECT_OK;
}
gh_effect_status gh_game_effects_trans_002fb310(gh_game_effects_trans*s,const gh_effects_trans_bindings*b){fenv_t env;if(feholdexcept(&env))return GH_EFFECT_UNSUPPORTED;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_EFFECT_UNSUPPORTED;}gh_effect_status result=body(s,b);if(fesetenv(&env))return GH_EFFECT_UNSUPPORTED;return result;}
