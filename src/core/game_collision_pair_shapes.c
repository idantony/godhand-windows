#include "game_collision_pair_shapes.h"
#include "game_collision_pair_queries.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
#include <float.h>
typedef struct {gh_collision_shape_inputs inputs;const uint8_t *position,*matrix;uint32_t serial,next,hit_indices[20],hit_serials[20];uint8_t bound;} shape_row;
struct gh_collision_pair_shapes {gh_collision_pairs *owner;shape_row rows[GH_COLLISION_CAPACITY];uint32_t first;};
static uint32_t w(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static void ww(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
static float value(uint32_t bits){float v;memcpy(&v,&bits,4);return v;}
static uint32_t bits(float f){uint32_t v;memcpy(&v,&f,4);return v;}
static void vf0(uint8_t*p){memset(p,0,12);ww(p+12,0x3f800000);}
static gh_pair_shape_status fail(uint32_t*f,uint32_t pc,gh_pair_shape_status e){if(f)*f=pc;return e;}
static int valid(const gh_collision_pair_shape*v){return v&&v->manager&&v->manager->native_shapes&&v->manager->native_shapes->owner==v->manager&&v->index<v->manager->count&&v->index<GH_COLLISION_CAPACITY&&v->record==(uint8_t*)v->manager->records.pointer+(size_t)v->index*GH_COLLISION_RECORD_BYTES&&v->manager->native_shapes->rows[v->index].bound&&v->manager->native_shapes->rows[v->index].serial==v->serial;}
static int borrowed(const gh_collision_pair_shape*v){shape_row*r=&v->manager->native_shapes->rows[v->index];gh_hitarea_object*o=&r->inputs.owner;gh_title_player_view*actor=NULL;if(gh_hitarea_object_actor(o,&actor))return 0;if(!o->state||o->fallback||o->index>=416||o->generation!=o->state->hitarea_generation)return 0;size_t guard=0;for(gh_game_damage_link*n=o->state->object_links[o->index];n;n=n->next){if(++guard>4096)return 1;if(n->target_mask==(uint32_t*)(void*)(v->record+0x10))return 1;}return 0;}
static gh_pair_shape_status create(gh_collision_pairs*m,const gh_collision_shape_inputs*in,unsigned kind,gh_collision_pair_shape*out,int*original,uint32_t*f){
 if(f)*f=0;if(!m||!in||!out||!original||!m->constructed||!m->records.pointer||m->count>GH_COLLISION_CAPACITY)return GH_PAIR_SHAPE_ARGUMENT;*out=(gh_collision_pair_shape){0};*original=0;
 unsigned index=0;for(;index<m->count;index++)if(!(w((uint8_t*)m->records.pointer+(size_t)index*GH_COLLISION_RECORD_BYTES)&0x603))break;if(index==m->count)return GH_PAIR_SHAPE_OK;
 if(!in->matrix||!in->math||!in->math->initialized)return fail(f,0x3079c8,GH_PAIR_SHAPE_UNSUPPORTED);if(in->global_identity_matrix&&in->matrix!=(const uint8_t*)in->math->identity)return GH_PAIR_SHAPE_ARGUMENT;
 uint32_t rotation[3];if(kind==0)memcpy(rotation,in->math->zero_vector,12);else memcpy(rotation,in->rotation_xyz,12);
 for(unsigned k=0;k<3;k++)if(!isfinite(value(in->offset_xyz[k])))return fail(f,0x147728,GH_PAIR_SHAPE_UNSUPPORTED);
 float angles[3],local_matrix[16];for(unsigned k=0;k<3;k++)angles[k]=value(rotation[k]);uint32_t math_frontier=0;if(gh_actor_matrix_rotation_00147728(angles,0,local_matrix,in->policy,&math_frontier))return fail(f,math_frontier?math_frontier:0x147728,GH_PAIR_SHAPE_UNSUPPORTED);
 if(kind!=4&&!isfinite(value(in->radius_bits)))return fail(f,kind==2?0x2ba788:0x2ba540,GH_PAIR_SHAPE_UNSUPPORTED);if(kind==2&&!isfinite(value(in->height_bits)))return fail(f,0x2ba750,GH_PAIR_SHAPE_UNSUPPORTED);
 if(kind==4)for(unsigned k=0;k<3;k++)if(!isfinite(value(in->size_xyz[k])))return fail(f,0x2baaa8,GH_PAIR_SHAPE_UNSUPPORTED);
 uint32_t bounding=in->radius_bits,bounding_pc=kind==4?0x2bab30:0x2ba7b8;if(kind==2||kind==4){fenv_t env;if(feholdexcept(&env))return fail(f,bounding_pc,GH_PAIR_SHAPE_UNSUPPORTED);if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return fail(f,bounding_pc,GH_PAIR_SHAPE_UNSUPPORTED);}if(kind==2){volatile float sum=value(in->radius_bits)+value(in->height_bits);bounding=bits(sum);}else{volatile float x=value(in->size_xyz[0])*0.5f,y=value(in->size_xyz[1])*0.5f,z=value(in->size_xyz[2])*0.5f;volatile float xx=x*x,yy=y*y,zz=z*z,xy=xx+yy,n=xy+zz;volatile float length=sqrtf(n);bounding=bits(length);}if(fesetenv(&env))return fail(f,bounding_pc,GH_PAIR_SHAPE_UNSUPPORTED);if(!isfinite(value(bounding)))return fail(f,bounding_pc,GH_PAIR_SHAPE_UNSUPPORTED);}
 int offset=0;{fenv_t env;if(feholdexcept(&env))return fail(f,0x2bb2d8,GH_PAIR_SHAPE_UNSUPPORTED);if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return fail(f,0x2bb2d8,GH_PAIR_SHAPE_UNSUPPORTED);}volatile float sums[2]={0,0};for(unsigned j=0;j<2;j++){const uint32_t*input=j?rotation:in->offset_xyz;volatile float x=value(input[0])*value(input[0]),y=value(input[1])*value(input[1]),z=value(input[2])*value(input[2]);volatile float xy=x+y;sums[j]=xy+z;}offset=sums[0]!=0||sums[1]!=0;if(fesetenv(&env))return fail(f,0x2bb2d8,GH_PAIR_SHAPE_UNSUPPORTED);}
 if(m->native_shapes&&m->native_shapes->owner!=m)return GH_PAIR_SHAPE_ARGUMENT;
 if(!m->native_shapes){m->native_shapes=calloc(1,sizeof(*m->native_shapes));if(!m->native_shapes)return fail(f,0x2ba300,GH_PAIR_SHAPE_MEMORY);m->native_shapes->owner=m;}
 if(m->shape_serial==UINT32_MAX)return fail(f,0x2ba300,GH_PAIR_SHAPE_UNSUPPORTED);
 struct gh_collision_pair_shapes*registry=m->native_shapes;shape_row*row=&registry->rows[index];uint32_t next=row->next;int listed=0;size_t guard=0;for(uint32_t link=registry->first;link;link=registry->rows[link-1].next){if(link>m->count||++guard>m->count)return fail(f,0x2ba300,GH_PAIR_SHAPE_UNSUPPORTED);if(link==index+1){listed=1;break;}}
 if(!listed){next=registry->first;registry->first=index+1;}
 *row=(shape_row){0};row->inputs=*in;row->serial=++m->shape_serial;row->next=next;row->bound=1;
 uint8_t*r=(uint8_t*)m->records.pointer+(size_t)index*GH_COLLISION_RECORD_BYTES;ww(r,3);ww(r+4,0);ww(r+8,0x44baf8);r[0xd]=0;memset(r+0x14,0,32);ww(r+0x10,in->type);r[0xc]=(uint8_t)(in->type<9&&(in->type==0||(in->type&1)));
 uint8_t*p=r+0xe0;ww(p+0x100,0x44bed0);p[4]=(uint8_t)(in->global_identity_matrix?0:2);vf0(p+0x50);vf0(p+0x60);vf0(p+0xd0);vf0(p+0xf0);ww(p+8,0);for(unsigned k=0;k<3;k++){ww(p+0x50+k*4,in->offset_xyz[k]);ww(p+0x60+k*4,rotation[k]);}
 
 if(offset){p[4]|=1;row->matrix=p+(in->global_identity_matrix?0x10:0x80);row->position=p+(in->global_identity_matrix?0x40:0xb0);}else{p[4]&=0xfe;row->matrix=in->matrix;row->position=in->matrix+0x30;}
 ww(p+0x70,0);ww(p+0xc0,0);memcpy(p+0x10,local_matrix,64);for(unsigned k=0;k<3;k++)ww(p+0x40+k*4,in->offset_xyz[k]);ww(p+0x100,kind==0?0x44bd60:kind==2?0x44bca0:0x44bb20);ww(p,kind);if(kind==4){vf0(p+0x110);vf0(p+0x120);for(unsigned k=0;k<3;k++)ww(p+0x110+k*4,in->size_xyz[k]);}else ww(p+0x110,in->radius_bits);/* kind2 compatibilityfield carries originalf12 HEIGHT; 114 carriesf13 RADIUS. */
 ww(p+0xe4,bounding);if(kind==2)ww(p+0x114,in->height_bits);
 ww(r+0x34,0);/* Nativeattr1 holds copiedDamageUnit view, not truncatedpointer. */ww(r+0x14,1);ww(r+0x18,0);*out=(gh_collision_pair_shape){m,r,index,row->serial};*original=1;return GH_PAIR_SHAPE_OK;
}
gh_pair_shape_status gh_collision_pair_sphere_002b9df0(gh_collision_pairs*m,const gh_collision_shape_inputs*i,gh_collision_pair_shape*out,int*original,uint32_t*f){return create(m,i,0,out,original,f);}
gh_pair_shape_status gh_collision_pair_cylinder_002b9ef0(gh_collision_pairs*m,const gh_collision_shape_inputs*i,gh_collision_pair_shape*out,int*original,uint32_t*f){return create(m,i,2,out,original,f);}
gh_pair_shape_status gh_collision_pair_box_002ba020(gh_collision_pairs*m,const gh_collision_shape_inputs*i,gh_collision_pair_shape*out,int*original,uint32_t*f){return create(m,i,4,out,original,f);}
int gh_collision_pair_shapes_have_live(const gh_collision_pairs*m){if(!m||!m->constructed)return 0;if(m->native_shapes&&m->native_shapes->first)return 1;if(m->records.pointer){if(m->count>GH_COLLISION_CAPACITY)return 1;for(unsigned i=0;i<m->count;i++)if(w((const uint8_t*)m->records.pointer+(size_t)i*GH_COLLISION_RECORD_BYTES)&0x603)return 1;}return 0;}
gh_pair_shape_status gh_collision_pair_shape_source_matrix(const gh_collision_pair_shape*v,const uint8_t**out){if(!out||!valid(v))return GH_PAIR_SHAPE_ARGUMENT;*out=v->manager->native_shapes->rows[v->index].inputs.matrix;return GH_PAIR_SHAPE_OK;}
gh_pair_shape_status gh_collision_pair_shape_position(const gh_collision_pair_shape*v,const uint8_t**out){if(!out||!valid(v))return GH_PAIR_SHAPE_ARGUMENT;*out=v->manager->native_shapes->rows[v->index].position;return GH_PAIR_SHAPE_OK;}
gh_pair_shape_status gh_collision_pair_shape_mode_pointer(const gh_collision_pair_shape*v,uint32_t**out){if(!out||!valid(v))return GH_PAIR_SHAPE_ARGUMENT;*out=(uint32_t*)(void*)(v->record+0x10);return GH_PAIR_SHAPE_OK;}
gh_pair_shape_status gh_collision_pair_shape_abort(gh_collision_pair_shape*v,uint32_t*f){if(f)*f=0;if(!valid(v))return GH_PAIR_SHAPE_ARGUMENT;if(borrowed(v))return fail(f,0x1fd580,GH_PAIR_SHAPE_BUSY);struct gh_collision_pair_shapes*r=v->manager->native_shapes;uint32_t*link=&r->first;size_t guard=0;while(*link&&*link!=v->index+1){if(*link>v->manager->count||++guard>v->manager->count)return fail(f,0x2bae98,GH_PAIR_SHAPE_UNSUPPORTED);link=&r->rows[*link-1].next;}if(*link)*link=r->rows[v->index].next;r->rows[v->index].bound=0;ww(v->record,0);*v=(gh_collision_pair_shape){0};return GH_PAIR_SHAPE_OK;}
gh_pair_shape_status gh_collision_pair_shapes_release_native(gh_collision_pairs*m){if(!m)return GH_PAIR_SHAPE_ARGUMENT;if(!m->native_shapes)return GH_PAIR_SHAPE_OK;if(m->native_shapes->owner!=m||m->count>GH_COLLISION_CAPACITY)return GH_PAIR_SHAPE_ARGUMENT;for(unsigned i=0;i<m->count;i++)if(m->native_shapes->rows[i].bound){gh_collision_pair_shape v={m,(uint8_t*)m->records.pointer+(size_t)i*GH_COLLISION_RECORD_BYTES,i,m->native_shapes->rows[i].serial};if(borrowed(&v))return GH_PAIR_SHAPE_BUSY;}free(m->native_shapes);m->native_shapes=NULL;return GH_PAIR_SHAPE_OK;}


/* Original 1FCBA0 performs this before the DamageNode is unlinked. Unlike
 * native abort it does not require the original borrow to have disappeared. */
gh_pair_shape_status gh_collision_pair_shape_remove_001fcba0(gh_collision_pair_shape*v,const gh_hitarea_object*o,uint32_t*f){
 if(f)*f=0;if(!valid(v)||!o)return GH_PAIR_SHAPE_ARGUMENT;
 struct gh_collision_pair_shapes*r=v->manager->native_shapes;shape_row*row=&r->rows[v->index];const gh_hitarea_object*owner=&row->inputs.owner;
 if(owner->state!=o->state||owner->index!=o->index||owner->generation!=o->generation||owner->serial!=o->serial)return GH_PAIR_SHAPE_ARGUMENT;
 if(v->manager->deletion_mode!=2)return fail(f,0x1fcba0,GH_PAIR_SHAPE_UNSUPPORTED);
 if((w(v->record)&0x201)==1){uint32_t*link=&r->first;size_t guard=0;while(*link&&*link!=v->index+1){if(*link>v->manager->count||++guard>v->manager->count)return fail(f,0x1fcba0,GH_PAIR_SHAPE_UNSUPPORTED);link=&r->rows[*link-1].next;}if(*link){*link=row->next;row->next=0;ww(v->record+4,0);}}
 ww(v->record,w(v->record)|0x200);row->bound=0;*v=(gh_collision_pair_shape){0};return GH_PAIR_SHAPE_OK;
}

static gh_pair_shape_status sweep_row(gh_collision_pairs*m,unsigned i,uint32_t*f){
 uint8_t*p=(uint8_t*)m->records.pointer+(size_t)i*GH_COLLISION_RECORD_BYTES;uint32_t flags=w(p);if(!(flags&0x603))return GH_PAIR_SHAPE_OK;
 if(flags&0x400){if(!m->native_shapes||m->native_shapes->owner!=m||!m->native_shapes->rows[i].serial||m->native_shapes->rows[i].bound||w(p+8)!=0x44baf8)return fail(f,0x2bac98,GH_PAIR_SHAPE_UNSUPPORTED);
 /* 44BAF8+0C=2BAC98, then original2BA250's flags0. */
 ww(p+8,0x42c320);ww(p,flags&0xfffff9fcu);ww(p,0);
 }else if(flags&0x200)ww(p,flags|0x400);return GH_PAIR_SHAPE_OK;
}
gh_pair_shape_status gh_collision_pairs_sweep_002ba250(gh_collision_pairs*m,uint32_t*f){
 if(f)*f=0;if(!m||!m->constructed||m->count>GH_COLLISION_CAPACITY||(m->count&&!m->records.pointer))return GH_PAIR_SHAPE_ARGUMENT;
 for(unsigned i=0;i<m->count;i++){gh_pair_shape_status e=sweep_row(m,i,f);if(e)return e;}return GH_PAIR_SHAPE_OK;
}
static float reference_mul(float a,float b){volatile float n=a*b;return n;}
static float reference_add(float a,float b){volatile float n=a+b;return n;}
static int reference_normal(float a){return isfinite(a)&&(a==0||fabsf(a)>=FLT_MIN);}
static gh_pair_shape_status geometry_update(gh_collision_pairs*m,unsigned i,unsigned*phase,gh_static_transform_policy policy,uint32_t*f){
 struct gh_collision_pair_shapes*registry=m->native_shapes;shape_row*r=&registry->rows[i];uint8_t*p=(uint8_t*)m->records.pointer+(size_t)i*GH_COLLISION_RECORD_BYTES;if(!*phase){ww(p+0xd8,0);*phase=1;}if((w(p)&0x203)!=3)return GH_PAIR_SHAPE_OK;
 gh_title_player_view*actor=NULL;if(!r->bound||gh_hitarea_object_actor(&r->inputs.owner,&actor))return fail(f,0x2bb1f0,GH_PAIR_SHAPE_UNSUPPORTED);
 uint8_t*g=p+0xe0;if(*phase==1){if((g[4]&3)==3){float a[16],b[16],out[16];memcpy(a,r->inputs.matrix,64);memcpy(b,g+0x10,64);uint32_t pc=0;if(gh_actor_matrix_product_00147548(a,b,out,policy,&pc))return fail(f,pc?pc:0x147548,GH_PAIR_SHAPE_UNSUPPORTED);memcpy(g+0x80,out,64);}*phase=2;}
 unsigned kind=w(g);uint32_t expected=kind==0?0x44bd60:kind==2?0x44bca0:kind==4?0x44bb20:0;if(w(p+8)!=0x44baf8||!expected||w(g+0x100)!=expected)return fail(f,0x2bb1f0,GH_PAIR_SHAPE_UNSUPPORTED);if(kind==0||kind==2)return GH_PAIR_SHAPE_OK;if(kind!=4)return fail(f,0x2bb1f0,GH_PAIR_SHAPE_UNSUPPORTED);
 /* 2BABD0: sizeY/2 transforms (0,h,0,1); output XYZ only, W preserved. */
 float matrix[16],half=reference_mul(value(w(g+0x114)),0.5f),out[3];memcpy(matrix,r->matrix,64);for(unsigned k=0;k<16;k++)if(!reference_normal(matrix[k]))return fail(f,0x30a2b0,GH_PAIR_SHAPE_UNSUPPORTED);
 for(unsigned k=0;k<3;k++){float n=reference_mul(matrix[k],0);n=reference_add(n,reference_mul(matrix[4+k],half));n=reference_add(n,reference_mul(matrix[8+k],0));out[k]=reference_add(n,reference_mul(matrix[12+k],1));if(!reference_normal(out[k]))return fail(f,0x30a2b0,GH_PAIR_SHAPE_UNSUPPORTED);}memcpy(g+0x120,out,12);return GH_PAIR_SHAPE_OK;
}
static void register_hit(gh_collision_pairs*m,unsigned a,unsigned b,float distance){
 uint8_t*p=(uint8_t*)m->records.pointer+(size_t)a*GH_COLLISION_RECORD_BYTES;const uint8_t*other=(uint8_t*)m->records.pointer+(size_t)b*GH_COLLISION_RECORD_BYTES;if(other[0xd]&1)return;unsigned n=w(p+0xd8),slot=20;if(n<20){slot=n;ww(p+0xd8,n+1);}else for(unsigned k=0;k<20;k++)if(distance<value(w(p+0x88+k*4))){slot=k;break;}
 if(slot<20){shape_row*r=&m->native_shapes->rows[a];r->hit_indices[slot]=b;r->hit_serials[slot]=m->native_shapes->rows[b].serial;ww(p+0x38+slot*4,0);ww(p+0x88+slot*4,bits(distance));}
}
static gh_pair_shape_status pair_process_one(gh_collision_pairs*m,uint32_t a,uint32_t b,uint32_t*f){
 struct gh_collision_pair_shapes*r=m->native_shapes;uint8_t*p=(uint8_t*)m->records.pointer+(size_t)(a-1)*GH_COLLISION_RECORD_BYTES;if((w(p)&0x203)!=3||p[0xc]!=1)return GH_PAIR_SHAPE_OK;
 uint8_t*q=(uint8_t*)m->records.pointer+(size_t)(b-1)*GH_COLLISION_RECORD_BYTES;if((w(q)&0x203)!=3||q[0xc]!=0)return GH_PAIR_SHAPE_OK;uint32_t source=w(p+0x10),target=w(q+0x10);if(source>=32)return fail(f,0x2b8f48,GH_PAIR_SHAPE_UNSUPPORTED);if(!(m->pair_masks[source]&(UINT32_C(1)<<(target&31))))return GH_PAIR_SHAPE_OK;
 const uint8_t*ap=w(p+0xe0)==4?p+0xe0+0x120:r->rows[a-1].position;const uint8_t*bp=w(q+0xe0)==4?q+0xe0+0x120:r->rows[b-1].position;float squared=0;for(unsigned k=0;k<3;k++){float x=value(w(bp+k*4)),y=value(w(ap+k*4));if(!reference_normal(x)||!reference_normal(y))return fail(f,0x2bb3f0,GH_PAIR_SHAPE_UNSUPPORTED);volatile float d=x-y;float term=reference_mul(d,d);squared=k?reference_add(squared,term):term;}
 float sum=reference_add(value(w(p+0x1c4)),value(w(q+0x1c4))),bound=reference_mul(sum,sum);if(!reference_normal(squared)||!reference_normal(bound))return fail(f,0x2bb3f0,GH_PAIR_SHAPE_UNSUPPORTED);if(!(squared<bound))return GH_PAIR_SHAPE_OK;
 unsigned x=w(p+0xe0),y=w(q+0xe0);gh_collision_pair_dispatch dispatch;if(gh_collision_pair_dispatch_002b8f48(x,y,&dispatch))return fail(f,0x2b8f48,GH_PAIR_SHAPE_UNSUPPORTED);if(!dispatch.case_id)return GH_PAIR_SHAPE_OK;
 if(dispatch.case_id==3){uint8_t*sphere=x==0?p:q;uint8_t*cylinder=x==2?p:q;shape_row*sr=&r->rows[(x==0?a:b)-1];shape_row*cr=&r->rows[(x==2?a:b)-1];int hit;uint32_t distance,pc=0;
 /* Kind2 VT+14=114(radius) ->f13; VT+24=110(height) ->f14. */
 if(gh_collision_query_sphere_cylinder_002bb670(sr->position,w(sphere+0x1f0),cr->matrix,w(cylinder+0x1f4),w(cylinder+0x1f0),sr->inputs.policy,&hit,&distance,&pc))return fail(f,pc?pc:0x2bb670,GH_PAIR_SHAPE_UNSUPPORTED);if(!hit)return GH_PAIR_SHAPE_OK;
 }else if(dispatch.case_id==5){uint8_t*sphere=x==0?p:q;uint8_t*box=x==4?p:q;shape_row*sr=&r->rows[(x==0?a:b)-1];shape_row*br=&r->rows[(x==4?a:b)-1];uint32_t size[3]={w(box+0x1f0),w(box+0x1f4),w(box+0x1f8)},pc=0;int hit;if(gh_collision_query_sphere_box_002bbba0(sr->position,w(sphere+0x1f0),br->matrix,size,sr->inputs.policy,&hit,&pc))return fail(f,pc?pc:0x2bbba0,GH_PAIR_SHAPE_UNSUPPORTED);if(!hit)return GH_PAIR_SHAPE_OK;
 }else if(dispatch.case_id==14){int hit;uint32_t pc=0;if(gh_collision_query_kind2_pair_002bcf28(r->rows[a-1].matrix,w(p+0x1f4),w(p+0x1f0),r->rows[b-1].matrix,w(q+0x1f4),w(q+0x1f0),r->rows[a-1].inputs.policy,&hit,&pc))return fail(f,pc?pc:0x2bcf28,GH_PAIR_SHAPE_UNSUPPORTED);if(!hit)return GH_PAIR_SHAPE_OK;
 }else if(dispatch.case_id==16){uint8_t*kind2=x==2?p:q;uint8_t*box=x==4?p:q;shape_row*kr=&r->rows[(x==2?a:b)-1];shape_row*br=&r->rows[(x==4?a:b)-1];uint32_t size[3]={w(box+0x1f0),w(box+0x1f4),w(box+0x1f8)},pc=0;int hit;if(gh_collision_query_kind2_box_002bd200(kr->matrix,w(kind2+0x1f4),w(kind2+0x1f0),br->matrix,size,kr->inputs.policy,&hit,&pc))return fail(f,pc?pc:0x2bd200,GH_PAIR_SHAPE_UNSUPPORTED);if(!hit)return GH_PAIR_SHAPE_OK;
 }else if(dispatch.case_id==23){uint32_t sa[3]={w(p+0x1f0),w(p+0x1f4),w(p+0x1f8)},sb[3]={w(q+0x1f0),w(q+0x1f4),w(q+0x1f8)},pc=0;int hit;if(gh_collision_query_box_box_002bd828(r->rows[a-1].matrix,sa,r->rows[b-1].matrix,sb,r->rows[a-1].inputs.policy,&hit,&pc))return fail(f,pc?pc:0x2bd828,GH_PAIR_SHAPE_UNSUPPORTED);if(!hit)return GH_PAIR_SHAPE_OK;
 }else if(dispatch.case_id!=1&&dispatch.case_id!=7)return fail(f,dispatch.entry?dispatch.entry:0x2b8f48,GH_PAIR_SHAPE_UNSUPPORTED);
 /* Original hit ranking reads the two C0 origins, not box broadphase120. */
 float origin_distance=0;for(unsigned k=0;k<3;k++){float av=value(w(r->rows[a-1].position+k*4)),bv=value(w(r->rows[b-1].position+k*4));if(!reference_normal(av)||!reference_normal(bv))return fail(f,0x1f7e28,GH_PAIR_SHAPE_UNSUPPORTED);volatile float d=av-bv;float term=reference_mul(d,d);origin_distance=k?reference_add(origin_distance,term):term;}if(!reference_normal(origin_distance))return fail(f,0x1f7e28,GH_PAIR_SHAPE_UNSUPPORTED);
 register_hit(m,a-1,b-1,origin_distance);register_hit(m,b-1,a-1,origin_distance);return GH_PAIR_SHAPE_OK;
}
static gh_pair_shape_status validate_list(gh_collision_pairs*m,uint32_t*f){
 if(m->used)return fail(f,0x2b8aa8,GH_PAIR_SHAPE_UNSUPPORTED);
 struct gh_collision_pair_shapes*r=m->native_shapes;if(!r){for(unsigned i=0;i<m->count;i++)if(w((uint8_t*)m->records.pointer+(size_t)i*GH_COLLISION_RECORD_BYTES)&0x603)return fail(f,0x2b8aa8,GH_PAIR_SHAPE_UNSUPPORTED);return GH_PAIR_SHAPE_OK;}if(r->owner!=m)return GH_PAIR_SHAPE_ARGUMENT;
 for(unsigned i=0;i<m->count;i++){uint8_t*p=(uint8_t*)m->records.pointer+(size_t)i*GH_COLLISION_RECORD_BYTES;if((w(p)&0x603)&&!r->rows[i].bound&&!(w(p)&0x600))return fail(f,0x2b8aa8,GH_PAIR_SHAPE_UNSUPPORTED);}size_t count=0;for(uint32_t link=r->first;link;link=r->rows[link-1].next){if(link>m->count||++count>m->count||!r->rows[link-1].bound)return fail(f,0x2b8aa8,GH_PAIR_SHAPE_UNSUPPORTED);}return GH_PAIR_SHAPE_OK;
}
gh_pair_shape_status gh_collision_pairs_step_resume_002b9d58(gh_collision_pairs*m,gh_collision_pair_frame*c,gh_static_transform_policy policy,uint32_t*f){
 if(f)*f=0;if(!m||!c||!m->constructed||m->count>GH_COLLISION_CAPACITY||(m->count&&!m->records.pointer))return GH_PAIR_SHAPE_ARGUMENT;if(policy!=GH_STATIC_BINARY32_REFERENCE)return fail(f,0x2b9d58,GH_PAIR_SHAPE_UNSUPPORTED);
 if(!c->active){*c=(gh_collision_pair_frame){.identity=c,.manager=m,.records=m->records.pointer,.serial=m->shape_serial,.count=m->count,.active=1};}else if(c->identity!=c||c->manager!=m||c->records!=m->records.pointer||c->serial!=m->shape_serial||c->count!=m->count||c->stage>2)return GH_PAIR_SHAPE_ARGUMENT;
 gh_pair_shape_status e=GH_PAIR_SHAPE_OK;if(!c->stage){while(c->sweep_row<m->count){if((e=sweep_row(m,c->sweep_row,f)))return e;c->sweep_row++;}c->stage=1;c->geometry_link=m->native_shapes?m->native_shapes->first:0;}
 if((e=validate_list(m,f)))return e;
 fenv_t env;if(feholdexcept(&env))return fail(f,0x2b9d58,GH_PAIR_SHAPE_UNSUPPORTED);if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return fail(f,0x2b9d58,GH_PAIR_SHAPE_UNSUPPORTED);}
 if(c->stage==1){while(c->geometry_link){if(c->geometry_link>m->count||c->geometry_visited>=m->count){e=fail(f,0x2b8aa8,GH_PAIR_SHAPE_UNSUPPORTED);break;}if((e=geometry_update(m,c->geometry_link-1,&c->geometry_phase,policy,f)))break;c->geometry_link=m->native_shapes->rows[c->geometry_link-1].next;c->geometry_phase=0;c->geometry_visited++;}if(!e){c->stage=2;c->source_link=m->native_shapes?m->native_shapes->first:0;c->target_link=c->source_link;}}
 if(!e&&c->stage==2)while(c->source_link){if(c->source_link>m->count){e=fail(f,0x2b8f48,GH_PAIR_SHAPE_UNSUPPORTED);break;}while(c->target_link){if(c->target_link>m->count){e=fail(f,0x2b8f48,GH_PAIR_SHAPE_UNSUPPORTED);break;}if((e=pair_process_one(m,c->source_link,c->target_link,f)))break;c->target_link=m->native_shapes->rows[c->target_link-1].next;}if(e)break;c->source_link=m->native_shapes->rows[c->source_link-1].next;c->target_link=m->native_shapes?m->native_shapes->first:0;}
 if(fesetenv(&env))return fail(f,0x2b9d58,GH_PAIR_SHAPE_UNSUPPORTED);if(!e)*c=(gh_collision_pair_frame){0};return e;
}
gh_pair_shape_status gh_collision_pairs_step_002b9d58(gh_collision_pairs*m,gh_static_transform_policy policy,uint32_t*f){gh_collision_pair_frame frame={0};return gh_collision_pairs_step_resume_002b9d58(m,&frame,policy,f);}

gh_pair_shape_status gh_collision_pair_shape_hit_at(const gh_collision_pair_shape*v,unsigned ordinal,gh_collision_pair_shape*other,uint32_t*distance){
 if(!valid(v)||!other||!distance)return GH_PAIR_SHAPE_ARGUMENT;unsigned count=w(v->record+0xd8);if(count>20||ordinal>=count)return GH_PAIR_SHAPE_ARGUMENT;shape_row*r=&v->manager->native_shapes->rows[v->index];unsigned index=r->hit_indices[ordinal];if(index>=v->manager->count)return GH_PAIR_SHAPE_ARGUMENT;shape_row*t=&v->manager->native_shapes->rows[index];if(!t->bound||t->serial!=r->hit_serials[ordinal])return GH_PAIR_SHAPE_ARGUMENT;*other=(gh_collision_pair_shape){v->manager,(uint8_t*)v->manager->records.pointer+(size_t)index*GH_COLLISION_RECORD_BYTES,index,t->serial};*distance=w(v->record+0x88+ordinal*4);return GH_PAIR_SHAPE_OK;
}

gh_pair_shape_status gh_collision_pair_dispatch_002b8f48(unsigned a,unsigned b,gh_collision_pair_dispatch*out){
 static const uint8_t cases[7][7]={{1,2,3,4,5,6,7},{2,8,9,10,11,12,13},{3,9,14,15,16,17,18},{4,10,15,19,20,21,22},{5,11,16,20,23,24,25},{6,12,17,21,24,0,0},{7,13,18,22,25,0,0}};
 static const uint32_t entries[26]={0,0,0x2bb488,0x2bb670,0x2bb760,0x2bbba0,0x2bbce0,0,0x2bbdb8,0x2bc470,0x2bc6c8,0x2bcb80,0x2bccc0,0x2bcd58,0x2bcf28,0x2bd078,0x2bd200,0x2bd340,0x2bd3d8,0x2bd4b0,0x2bd6a8,0x2bd7e8,0x2bd7f8,0x2bd828,0x2bda10,0x2bda20};
 if(!out||a>=7||b>=7)return GH_PAIR_SHAPE_ARGUMENT;unsigned c=cases[a][b];*out=(gh_collision_pair_dispatch){entries[c],(uint8_t)c,(uint8_t)(c&&b<a)};return GH_PAIR_SHAPE_OK;
}

gh_pair_shape_status gh_collision_pair_shape_hitarea_owner(const gh_collision_pair_shape*v,gh_hitarea_object*out){
 if(!out||!valid(v))return GH_PAIR_SHAPE_ARGUMENT;shape_row*r=&v->manager->native_shapes->rows[v->index];
 if(w(v->record+0x14)!=1)return GH_PAIR_SHAPE_UNSUPPORTED;
 gh_title_player_view*a;if(gh_hitarea_object_actor(&r->inputs.owner,&a))return GH_PAIR_SHAPE_UNSUPPORTED;
 *out=r->inputs.owner;return GH_PAIR_SHAPE_OK;
}
gh_pair_shape_status gh_collision_pair_shape_sort_hits_001fc810(const gh_collision_pair_shape*v,uint32_t*f){
 if(f)*f=0;if(!valid(v))return GH_PAIR_SHAPE_ARGUMENT;unsigned n=w(v->record+0xd8);if((int32_t)n<=0)return GH_PAIR_SHAPE_OK;
 if(n>20)return fail(f,0x1fc810,GH_PAIR_SHAPE_UNSUPPORTED);
 for(unsigned i=0;i<n;i++){gh_collision_pair_shape other;uint32_t d;if(gh_collision_pair_shape_hit_at(v,i,&other,&d))return fail(f,0x1fc810,GH_PAIR_SHAPE_ARGUMENT);if(!isfinite(value(d)))return fail(f,0x1fc810,GH_PAIR_SHAPE_UNSUPPORTED);}
 shape_row*r=&v->manager->native_shapes->rows[v->index];
 for(unsigned i=0;i+1<n;i++){for(unsigned j=i+1;j<n;j++){if(value(w(v->record+0x88+j*4))<value(w(v->record+0x88+i*4))){
  uint32_t t=r->hit_indices[i];r->hit_indices[i]=r->hit_indices[j];r->hit_indices[j]=t;t=r->hit_serials[i];r->hit_serials[i]=r->hit_serials[j];r->hit_serials[j]=t;
  t=w(v->record+0x88+i*4);ww(v->record+0x88+i*4,w(v->record+0x88+j*4));ww(v->record+0x88+j*4,t);
 }}}
 return GH_PAIR_SHAPE_OK;
}
