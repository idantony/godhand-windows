#include "game_collision_pair_shapes.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
typedef struct {gh_collision_shape_inputs inputs;const uint8_t *position,*matrix;uint32_t serial,next;uint8_t bound;} shape_row;
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
 if((kind!=4&&!isfinite(value(in->radius_bits)))||(kind==2&&!isfinite(value(in->height_bits))))return fail(f,0x2ba540,GH_PAIR_SHAPE_UNSUPPORTED);
 if(kind==4)for(unsigned k=0;k<3;k++)if(!isfinite(value(in->size_xyz[k])))return fail(f,0x2baaa8,GH_PAIR_SHAPE_UNSUPPORTED);
 uint32_t bounding=in->radius_bits;if(kind==2||kind==4){fenv_t env;if(feholdexcept(&env))return fail(f,0x2ba7b8,GH_PAIR_SHAPE_UNSUPPORTED);if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return fail(f,0x2ba7b8,GH_PAIR_SHAPE_UNSUPPORTED);}if(kind==2){volatile float sum=value(in->radius_bits)+value(in->height_bits);bounding=bits(sum);}else{volatile float x=value(in->size_xyz[0])*0.5f,y=value(in->size_xyz[1])*0.5f,z=value(in->size_xyz[2])*0.5f;volatile float xx=x*x,yy=y*y,zz=z*z,xy=xx+yy,n=xy+zz;volatile float length=sqrtf(n);bounding=bits(length);}if(fesetenv(&env))return fail(f,0x2ba7b8,GH_PAIR_SHAPE_UNSUPPORTED);if(!isfinite(value(bounding)))return fail(f,0x2ba7b8,GH_PAIR_SHAPE_UNSUPPORTED);}
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
 ww(p+0x70,0);ww(p+0xc0,0);memcpy(p+0x10,local_matrix,64);for(unsigned k=0;k<3;k++)ww(p+0x40+k*4,in->offset_xyz[k]);ww(p+0x100,kind==0?0x44bd60:kind==2?0x44bca0:0x44bb20);ww(p,kind);if(kind==4){vf0(p+0x110);vf0(p+0x120);for(unsigned k=0;k<3;k++)ww(p+0x110+k*4,in->size_xyz[k]);}else ww(p+0x110,in->radius_bits);
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
gh_pair_shape_status gh_collision_pair_shapes_release_native(gh_collision_pairs*m){if(!m)return GH_PAIR_SHAPE_ARGUMENT;if(!m->native_shapes)return GH_PAIR_SHAPE_OK;if(m->native_shapes->owner!=m)return GH_PAIR_SHAPE_ARGUMENT;for(unsigned i=0;i<m->count;i++)if(m->native_shapes->rows[i].bound){gh_collision_pair_shape v={m,(uint8_t*)m->records.pointer+(size_t)i*GH_COLLISION_RECORD_BYTES,i,m->native_shapes->rows[i].serial};if(borrowed(&v))return GH_PAIR_SHAPE_BUSY;}free(m->native_shapes);m->native_shapes=NULL;return GH_PAIR_SHAPE_OK;}
