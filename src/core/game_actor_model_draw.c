#include "game_actor_model_draw.h"
#include "game_actor_matrix.h"
#include "game_actor_light_normalize.h"
#include "game_actor_light_ambient.h"
#include <string.h>
#include <math.h>
#include <fenv.h>

static uint32_t word(const void *p) {uint32_t n;memcpy(&n,p,4);return n;}
static float number(const void *p) {float n;memcpy(&n,p,4);return n;}
static float product(float a,float b) {volatile float n=a*b;return n;}
static float sum(float a,float b) {volatile float n=a+b;return n;}
static float quotient(float a,float b) {volatile float n=a/b;return n;}
static gh_actor_model_status fail(uint32_t *f,uint32_t pc,gh_actor_model_status e) {*f=pc;return e;}

static gh_actor_model_status palette(gh_actor_model *model,const gh_game_pl_camera *camera,uint32_t *frontier) {
 const uint8_t *root=model->root.bytes;
 const gh_actor_object_packets *packet;
 if(gh_actor_model_object_packet(model,&packet)||!packet->ready)return fail(frontier,0x14c7e8,GH_ACTOR_MODEL_BUSY);
 unsigned parity=word((const uint8_t*)model->work+0x210)&1u;
 uint8_t *constants,*palette_bytes;
 gh_actor_model_status e=gh_actor_model_object_span(model,packet->constants_offset[parity],0x150,&constants);
 if(e)return fail(frontier,0x14c7e8,e);
 e=gh_actor_model_object_span(model,packet->palette_offset[parity]+0x10,(size_t)model->part_count*64,&palette_bytes);
 if(e)return fail(frontier,0x1503b8,e);
 /* Original copies precede special projection branches. The raw pointer
  * fields remain removed; this span IS the existing typed allocation. */
 memcpy(constants+0x10,camera->fields,64);
 memcpy(constants+0x50,camera->fields+0xc0,64);
 float identity[16]={0};for(unsigned i=0;i<4;i++)identity[i*5]=1;
 memcpy(constants+0x110,identity,64);
 uint32_t flags=word(root+0x250);
 if(flags&0x2000)return fail(frontier,0x30a548,GH_ACTOR_MODEL_UNSUPPORTED); /* borrowed4A6900 needed */
 if(flags&(0x80000u|0x1000000u))return fail(frontier,0x30a8c8,GH_ACTOR_MODEL_UNSUPPORTED);
 /* The original do-while underflows for zero parts. Do not turn this into
  * a successful empty palette. Valid initialized skeletal models have1..42. */
 if(!model->part_count||model->part_count!=root[0x2b4]||model->part_count>42)return fail(frontier,0x1503b8,GH_ACTOR_MODEL_UNSUPPORTED);
 for(unsigned i=0;i<model->part_count;i++) {
  const gh_actor_part *part;float inverse_translation[16]={0},world[16],out[16];
  if(gh_actor_model_part_at(model,i,&part))return fail(frontier,0x1503b8,GH_ACTOR_MODEL_BOUNDS);
  for(unsigned k=0;k<4;k++)inverse_translation[k*5]=1;
  memcpy(inverse_translation+12,part->bytes+0xc0,12);memcpy(world,part->bytes+0x80,64);
  uint32_t pc=0;
  if(gh_actor_matrix_product_00147548(world,inverse_translation,out,GH_STATIC_BINARY32_REFERENCE,&pc))return fail(frontier,pc?pc:0x1503b8,GH_ACTOR_MODEL_UNSUPPORTED);
  memcpy(palette_bytes+(size_t)i*64,out,64);
 }
 return GH_ACTOR_MODEL_OK;
}

gh_actor_model_status gh_actor_palette_0014c7e8(gh_actor_model *model,const gh_game_pl_camera *camera,gh_static_transform_policy policy,uint32_t *frontier) {
 uint32_t local=0;if(!frontier)frontier=&local;*frontier=0;
 if(!model||!model->ready||model->native_identity!=model||!model->root.bytes||!model->work||!camera||!camera->constructed||policy!=GH_STATIC_BINARY32_REFERENCE)return GH_ACTOR_MODEL_ARGUMENT;
 fenv_t env;if(feholdexcept(&env))return GH_ACTOR_MODEL_UNSUPPORTED;
 if(fesetround(FE_TONEAREST)){fesetenv(&env);return GH_ACTOR_MODEL_UNSUPPORTED;}
 gh_actor_model_status e=palette(model,camera,frontier);
 if(fesetenv(&env))return GH_ACTOR_MODEL_UNSUPPORTED;return e;
}

/* 154240 canonical lit/unlit branches. Mutable globalvectors are borrowed;
 * positional2D8008 and alternateprojection retain explicit frontiers. */
static gh_actor_model_status node_light(gh_actor_model *model,unsigned index,const gh_game_pl_camera *camera,const gh_game_world_light *light,
 const gh_actor_light_globals *globals,const gh_player_shared_vector *pivot,uint32_t *frontier) {
 const uint8_t *raw;size_t mesh;gh_actor_model_status e=gh_actor_model_node_at(model,index,&raw,&mesh);if(e)return e;
 uint8_t *node=(uint8_t*)raw;uint32_t flags=word(node+0x380);const uint8_t *root=model->root.bytes;
 if(flags&1)return GH_ACTOR_MODEL_OK;
 if(!light||!light->constructed)return fail(frontier,0x2d7e50,GH_ACTOR_MODEL_UNSUPPORTED);
 unsigned parity=word((uint8_t*)model->work+0x210)&1u;uint8_t *out=node+0x160+parity*0xd0;
 float tint[4];for(unsigned k=0;k<4;k++)tint[k]=product(number(root+0x240+k*4),number(node+0x3a0+k*4));
 tint[3]=product(tint[3],quotient((float)root[0x2b6],255));tint[3]=product(tint[3],quotient((float)root[0x2b7],255));tint[3]=product(tint[3],quotient((float)root[0x2bc],255));
 for(unsigned k=0;k<4;k++)if(!isfinite(tint[k]))return fail(frontier,0x154240,GH_ACTOR_MODEL_UNSUPPORTED);
 float normals[16]={0};normals[15]=1;
 float colors[16];
 int ambient_only=0;
 if((flags&0x800000)||(word(node+0x410)&0x20000000)){
  if(!pivot||!pivot->initialized)return fail(frontier,0x154240,GH_ACTOR_MODEL_UNSUPPORTED);
  if((flags&0x800000)&&(!globals||!globals->initialized))return fail(frontier,0x154240,GH_ACTOR_MODEL_UNSUPPORTED);
  /* Both branches copy ALL FOUR words of the canonical3BD880 vector,
   * not a manufactured zero normal/light matrix. Later fog writes2C. */
  for(unsigned i=0;i<3;i++){memcpy(normals+i*4,pivot->bytes,16);memcpy(colors+i*4,pivot->bytes,16);}
  if(flags&0x800000)memcpy(colors+12,globals->bytes_3bd890,16);
  else for(unsigned k=0;k<4;k++){colors[12+k]=product(number(light->scalar+0x30+k*4),tint[k]);if(!isfinite(colors[12+k]))return fail(frontier,0x30a4d0,GH_ACTOR_MODEL_UNSUPPORTED);}
 }else{
 if(word(root+0x250)&0x800000){
  if(!model->root.alias_f0)return fail(frontier,0x2d8008,GH_ACTOR_MODEL_UNSUPPORTED);
  memcpy(normals,out+0x10,64);memcpy(colors,out+0x50,64);
  e=gh_actor_ambient_fast_002d8008(light,normals,colors,model->root.alias_f0,tint,GH_STATIC_BINARY32_REFERENCE,frontier);if(e)return e;
  ambient_only=1;
 }else{
 for(unsigned i=0;i<3;i++) {
  float direction[3],unit[4];
  for(unsigned k=0;k<3;k++)direction[k]=-number(light->scalar+0x161a0+i*16+k*4);
  e=gh_actor_light_normalize_0030a370(direction,unit,GH_STATIC_BINARY32_REFERENCE,frontier);if(e)return e;
  for(unsigned k=0;k<3;k++)normals[k*4+i]=unit[k];
 }
 for(unsigned i=0;i<4;i++)for(unsigned k=0;k<4;k++) {float n=number(light->scalar+(i<3?0x161d0+i*16:0x40)+k*4);if(!isfinite(n))return fail(frontier,0x2d7e50,GH_ACTOR_MODEL_UNSUPPORTED);if(n<0)n=0;if(n>1)n=1;colors[i*4+k]=n;}
 }
 }
 /*30A978 normal matrix, tint fourth column;2D7E50 clipped light colors. */
 if(!ambient_only){memcpy(normals+12,tint,16);}
 memcpy(out+0x10,normals,64);memcpy(out+0x50,colors,64);
 uint32_t actor_flags=word(root+0x250),work608=word((const uint8_t*)model->work+0x608);
 if(actor_flags&0x80000){if(!globals||!globals->initialized)return fail(frontier,0x154240,GH_ACTOR_MODEL_UNSUPPORTED);memcpy(out+0x80,globals->bytes_3bd870,16);}
 if((flags&0x20000000)&&tint[3]==1) {
  if(!model->root.alias_f0)return fail(frontier,0x154774,GH_ACTOR_MODEL_UNSUPPORTED);
  float view_direction[3],halfway[3];
  for(unsigned k=0;k<3;k++){view_direction[k]=sum(number(camera->fields+0x210+k*4),-number(model->root.alias_f0+k*4));if(!isfinite(view_direction[k]))return fail(frontier,0x101e28,GH_ACTOR_MODEL_UNSUPPORTED);}
  float length=sqrtf(sum(sum(product(view_direction[0],view_direction[0]),product(view_direction[1],view_direction[1])),product(view_direction[2],view_direction[2])));
  if(!isfinite(length))return fail(frontier,0x101e28,GH_ACTOR_MODEL_UNSUPPORTED);
  for(unsigned k=0;k<3;k++){view_direction[k]=length>0?product(view_direction[k],quotient(1,length)):0;halfway[k]=sum(view_direction[k],-number(light->scalar+0x16200+k*4));}
  length=sqrtf(sum(sum(product(halfway[0],halfway[0]),product(halfway[1],halfway[1])),product(halfway[2],halfway[2])));
  if(!isfinite(length))return fail(frontier,0x101e28,GH_ACTOR_MODEL_UNSUPPORTED);
  for(unsigned k=0;k<3;k++)halfway[k]=length>0?product(halfway[k],quotient(1,length)):0;
  gh_model_mesh descriptor;if(gh_model_mesh_at(&model->script,mesh,&descriptor)||descriptor.descriptor_offset>model->script.size||0x5a>model->script.size-descriptor.descriptor_offset)return fail(frontier,0x1548b8,GH_ACTOR_MODEL_BOUNDS);
  const uint8_t *material=model->script.bytes+descriptor.descriptor_offset;
  uint16_t exponent=material[0x58];float power=(float)material[0x59];
  float strength=quotient(sum(power,power),15);strength=product(strength,.5f);
  float illumination=quotient(sum(sum(number(light->scalar+0x16210),number(light->scalar+0x16214)),number(light->scalar+0x16218)),3);
  float specular=product(number(out+0x8c),product(strength,illumination));
  if(!isfinite(specular))return fail(frontier,0x154918,GH_ACTOR_MODEL_UNSUPPORTED);
  memcpy(out+0x90,halfway,12);memcpy(out+0xb0,&exponent,2);memcpy(out+0x8c,&specular,4);
  memcpy(out+0xc0,node+0x38c,8);float zw[2]={0,1};memcpy(out+0xc8,zw,8);
 } else if((flags&0x40000000)&&work608)memcpy(out+0x90,camera->fields+0x210,12);
 else if(!(flags&0x10000)){memcpy(out+0xc0,node+0x38c,8);float zw[2]={0,1};memcpy(out+0xc8,zw,8);}
 if(actor_flags&0x2000)return fail(frontier,0x30a548,GH_ACTOR_MODEL_UNSUPPORTED);
 const gh_actor_object_packets *packet;uint8_t *header;if(gh_actor_model_object_packet(model,&packet)||gh_actor_model_object_span(model,packet->header_offset[parity],0x80,&header))return fail(frontier,0x2d8070,GH_ACTOR_MODEL_BOUNDS);
 uint64_t fog=0;float factor=255;
 if(!(flags&0x200000)&&!(word((uint8_t*)model->work+0x58c)&0x1000)) {
  for(unsigned k=0;k<3;k++){float n=number(light->scalar+0x16158+k*4);if(!isfinite(n)||n>=2147483648.0f)return fail(frontier,0x2d8070,GH_ACTOR_MODEL_UNSUPPORTED);uint64_t component=n>=0?(uint64_t)(uint32_t)(int32_t)n:0;fog|=component<<(k*8);}
  int16_t depth;memcpy(&depth,root+0x2ac,2);float distance=product((float)depth,.1f);
  if(distance>0){float near=number(light->scalar+0x16150),far_mode=number(light->scalar+0x16154);volatile float range=far_mode-near;if(!isfinite(range)||range==0)return fail(frontier,0x2d8070,GH_ACTOR_MODEL_UNSUPPORTED);volatile float a=quotient(near,range)+1;volatile float b=a-quotient(distance,range);factor=product(b,255);}
  if(!isfinite(factor))return fail(frontier,0x2d8070,GH_ACTOR_MODEL_UNSUPPORTED);if(factor<0)factor=0;if(factor>255)factor=255;
 }
 memcpy(header+0x30,&fog,8);memcpy(out+0x2c,&factor,4);return GH_ACTOR_MODEL_OK;
}
gh_actor_model_status gh_actor_node_light_00154240(gh_actor_model *model,unsigned index,const gh_game_pl_camera *camera,const gh_game_world_light *light,gh_static_transform_policy policy,uint32_t *frontier) {
 return gh_actor_node_light_00154240_ex(model,index,camera,light,NULL,NULL,policy,frontier);
}
gh_actor_model_status gh_actor_node_light_00154240_ex(gh_actor_model *model,unsigned index,const gh_game_pl_camera *camera,const gh_game_world_light *light,
 const gh_actor_light_globals *globals,const gh_player_shared_vector *pivot,gh_static_transform_policy policy,uint32_t *frontier) {
 uint32_t local=0;if(!frontier)frontier=&local;*frontier=0;if(!model||!model->ready||model->native_identity!=model||!camera||!camera->constructed||policy!=GH_STATIC_BINARY32_REFERENCE)return GH_ACTOR_MODEL_ARGUMENT;
 fenv_t env;if(feholdexcept(&env))return GH_ACTOR_MODEL_UNSUPPORTED;if(fesetround(FE_TONEAREST)){fesetenv(&env);return GH_ACTOR_MODEL_UNSUPPORTED;}
 gh_actor_model_status e=node_light(model,index,camera,light,globals,pivot,frontier);if(fesetenv(&env))return GH_ACTOR_MODEL_UNSUPPORTED;return e;
}

static void transform(const float *m,const float *v,float *out) {for(unsigned r=0;r<4;r++){float n=product(m[r],v[0]);n=sum(n,product(m[4+r],v[1]));n=sum(n,product(m[8+r],v[2]));out[r]=sum(n,product(m[12+r],v[3]));}}
/*1F8DF8 asks whether any corner's camera-space W is below the selected
 * canonical light distance. It is not the ordinary frustum predicate. */
static gh_actor_model_status within_distance(const gh_actor_model *model,const gh_game_pl_camera *camera,float limit,int *yes,uint32_t *frontier) {
 const uint8_t *root=model->root.bytes;float world[16],screen[16],center[4],local[4],world_center[4];
 memcpy(world,root+0x80,64);memcpy(screen,camera->fields,64);memcpy(local,root+0x260,12);local[3]=1;transform(world,local,world_center);
 memcpy(center,world_center,12);center[3]=1;float minimum=INFINITY;
 for(unsigned corner=0;corner<8;corner++) {
  float offset[4],p[4],projected[4];for(unsigned k=0;k<3;k++)offset[k]=number(root+0x26c+k*4)*((corner&(1u<<k))?1:-1);offset[3]=1;
  float rotation[16];memcpy(rotation,world,64);rotation[12]=rotation[13]=rotation[14]=0;transform(rotation,offset,p);
  for(unsigned k=0;k<4;k++)p[k]=sum(p[k],center[k]);transform(screen,p,projected);
  if(!isfinite(projected[3])||!isfinite(limit))return fail(frontier,0x1f8df8,GH_ACTOR_MODEL_UNSUPPORTED);if(projected[3]<minimum)minimum=projected[3];
 }
 *yes=minimum<=limit;return GH_ACTOR_MODEL_OK;
}
static gh_actor_model_status invoke(gh_actor_draw_call *call,uint32_t entry,uint32_t *frontier) {
 if(!call->bindings.callee)return fail(frontier,entry,GH_ACTOR_MODEL_UNSUPPORTED);
 gh_actor_draw_request request={entry,call->parity,call->node_index,call->model,call->bindings.camera,call->bindings.light,call->depth,call->far_mode,&call->materials};
 gh_actor_model_status e=call->bindings.callee(call->bindings.context,&request,frontier);if(e&&!*frontier)*frontier=entry;return e;
}
static void opacity_destination(const gh_actor_draw_call *call,const uint8_t *node,
 gh_actor_draw_command *command)
{
 const uint8_t *root=call->model->root.bytes;
 if(word(root+0x2c0)==10){
  const int translucent=!!(word(node+0x380)&0x4080u);
  command->bank=translucent?2:1;
  command->depth=translucent?call->depth:13;
 }else{
  uint16_t depth;
  memcpy(&depth,root+0x2ac,2);
  command->bank=word(root+0x2c0);
  command->depth=depth;
 }
}

static gh_actor_model_status emit_opacity(gh_actor_draw_call *call,
 const gh_actor_draw_command *command,uint32_t *frontier)
{
 const uint32_t entry=command->kind==GH_ACTOR_DRAW_OPACITY_FRAME?0x31a600:0x31a650;
 if(!call->bindings.emit)return fail(frontier,entry,GH_ACTOR_MODEL_UNSUPPORTED);
 gh_actor_model_status status=call->bindings.emit(call->bindings.emit_context,command,frontier);
 if(status&&!*frontier)*frontier=entry;
 return status;
}

static gh_actor_model_status opacity_pass(gh_actor_draw_call *call,uint32_t *frontier)
{
 gh_actor_model *model=call->model;
 const uint8_t *root=model->root.bytes;
 const uint64_t frame=UINT64_C(0x00080070);
 const uint64_t masked_frame=UINT64_C(0xffffffff00080070);
 const uint64_t test=UINT64_C(0x507fd),alpha=UINT64_C(0x8000000044);
 const uint64_t tex1=UINT64_C(0xffffff3800000360);
 const uint8_t *first;size_t first_mesh;
 if(gh_actor_model_node_at(model,0,&first,&first_mesh))
  return fail(frontier,0x14b6f8,GH_ACTOR_MODEL_BOUNDS);
 if(call->phase==10){
  gh_actor_draw_command command={.kind=GH_ACTOR_DRAW_OPACITY_FRAME,.model=model,
   .node_index=SIZE_MAX,.parity=call->parity,.frame=frame,.packet_group=1};
  opacity_destination(call,first,&command);
  gh_actor_model_status status=emit_opacity(call,&command,frontier);
  if(status)return status;
  call->node_index=0;
  call->phase=11;
 }
 while(call->node_index<root[0x2b1]){
  if(call->phase==11){
   const uint8_t *node;size_t mesh_index;
   if(gh_actor_model_node_at(model,(unsigned)call->node_index,&node,&mesh_index))
    return fail(frontier,0x14b6f8,GH_ACTOR_MODEL_BOUNDS);
   gh_model_mesh mesh;
   if(gh_model_mesh_at(&model->script,mesh_index,&mesh))
    return fail(frontier,0x14b6f8,GH_ACTOR_MODEL_BOUNDS);
   const uint32_t actor_flags=word(root+0x250);
   if((word(node+0x380)&1u)||(!(actor_flags&0x400u)&&call->far_mode!=!!(mesh.flags&0x1000000u))){
    call->node_index++;
    continue;
   }
   if(actor_flags&0x2000u)return fail(frontier,0x155c68,GH_ACTOR_MODEL_UNSUPPORTED);
   const gh_actor_light_globals *globals=call->bindings.light_globals;
   const gh_player_shared_vector *pivot=call->bindings.pivot_owner;
   if(!globals||!globals->initialized||!pivot||!pivot->initialized||!call->bindings.textures)
    return fail(frontier,0x14b9a0,GH_ACTOR_MODEL_UNSUPPORTED);
   const gh_actor_node_packets *packet;
   if(gh_actor_model_packet_at(model,(unsigned)call->node_index,&packet))
    return fail(frontier,0x14b9a0,GH_ACTOR_MODEL_BOUNDS);
   gh_actor_draw_command command={.kind=GH_ACTOR_DRAW_OPACITY_GEOMETRY,.model=model,
    .node_index=call->node_index,.parity=call->parity,.selected_program=2,.packet=packet,
    .test=test,.alpha=alpha,.tex1=tex1,.frame=masked_frame,.packet_group=1};
   opacity_destination(call,node,&command);
   gh_actor_model_status status=gh_actor_materials_existing_0014b9a0(model,
    (unsigned)call->node_index,call->bindings.textures,&command.materials,frontier);
   if(status)return status;
   status=gh_actor_model_program2_inputs(model,(unsigned)call->node_index,&command.inputs,frontier);
   if(status)return status;
   /* 14B9A0 uploads the C8 packet constants, then the same node UV and
    * stream chain. The 255 fog write shares the second vector's W lane. */
   memcpy(command.inputs.normal_light,globals->bytes_3bd860,16);
   memcpy(command.inputs.normal_light+4,globals->bytes_3bd870,16);
   memcpy(command.inputs.normal_light+8,globals->bytes_3bd870,16);
   memcpy(command.inputs.normal_light+12,pivot->bytes,16);
   command.inputs.normal_light[7]=255;
   for(unsigned i=0;i<3;i++)memcpy(command.inputs.light_color+4*i,pivot->bytes,16);
   memcpy(command.inputs.light_color+12,globals->bytes_3bd890,16);
   memcpy(command.inputs.uv,node+0x38c,8);
   command.inputs.fog=255;
   call->pending=command;
   call->phase=12;
  }
  if(call->phase==12){
   gh_actor_model_status status=emit_opacity(call,&call->pending,frontier);
   if(status)return status;
   call->node_index++;
   call->phase=11;
  }
 }
 if(call->phase==11)call->phase=13;
 if(call->phase==13){
  const gh_actor_object_packets *object;
  if(gh_actor_model_object_packet(model,&object))
   return fail(frontier,0x14c5a0,GH_ACTOR_MODEL_BOUNDS);
  gh_actor_draw_command command={.kind=GH_ACTOR_DRAW_OPACITY_SETUP,.model=model,
   .node_index=SIZE_MAX,.parity=call->parity,.selected_program=2,.object=object,
   .test=test,.alpha=alpha,.tex1=tex1,.frame=masked_frame,.packet_group=1};
  opacity_destination(call,first,&command);
  gh_actor_model_status status=emit_opacity(call,&command,frontier);
  if(status)return status;
  call->phase=14;
 }
 return GH_ACTOR_MODEL_OK;
}

static gh_actor_model_status draw(gh_actor_draw_call *call,uint32_t *frontier) {
 gh_actor_model *model=call->model;const uint8_t *root=model->root.bytes;
 if(call->phase==0) {
  float alpha=number(root+0x24c);
  alpha=product(alpha,quotient((float)root[0x2b6],255));
  alpha=product(alpha,quotient((float)root[0x2b7],255));
  alpha=product(alpha,quotient((float)root[0x2bc],255));alpha=product(alpha,128);
  if(!isfinite(alpha)||alpha<0||alpha>=2147483648.0f)return fail(frontier,0x14ad54,GH_ACTOR_MODEL_UNSUPPORTED);
  int32_t threshold=(int32_t)alpha-1;if(threshold<1)threshold=0;
  uint64_t test=((uint64_t)(uint32_t)threshold<<4)|((word(root+0x250)&0x10)?0x5000du:0x5100du);
  for(unsigned i=0;i<root[0x2b1];i++) {
   const uint8_t *node;size_t mesh;gh_actor_model_status e=gh_actor_model_node_at(model,i,&node,&mesh);if(e)return fail(frontier,0x14b6f8,e);
   /*155D60 executes a countdown from385 even though its value is unused. */
   if(!node[0x385])return fail(frontier,0x155d60,GH_ACTOR_MODEL_UNSUPPORTED);
   memcpy((uint8_t*)node+call->parity*0xb0+0x30,&test,8);
  }
  call->phase=1;
 }
 if(call->phase==1) {gh_actor_model_status e=gh_actor_palette_0014c7e8(model,call->bindings.camera,call->bindings.policy,frontier);if(e)return e;call->phase=2;}
 while(call->phase==2&&call->node_index<root[0x2b1]) {
  const uint8_t *node;size_t mesh;gh_actor_model_status e=gh_actor_model_node_at(model,(unsigned)call->node_index,&node,&mesh);if(e)return fail(frontier,0x14b6f8,e);
  if(!(word(node+0x380)&1u)) {e=gh_actor_node_light_00154240_ex(model,(unsigned)call->node_index,call->bindings.camera,call->bindings.light,call->bindings.light_globals,call->bindings.pivot_owner,call->bindings.policy,frontier);if(e)return e;}
  call->node_index++;
 }
 if(call->phase==2) {
  const gh_actor_object_packets *packet;uint8_t *header;
  if(gh_actor_model_object_packet(model,&packet)||gh_actor_model_object_span(model,packet->header_offset[call->parity],0x80,&header))return fail(frontier,0x155be8,GH_ACTOR_MODEL_BOUNDS);
  uint64_t nop=0x10000000;memcpy(header+0x50,&nop,8);memcpy(header+0x60,&nop,8);if(gh_actor_model_object_program_reset(model,call->parity))return fail(frontier,0x155be8,GH_ACTOR_MODEL_BOUNDS);
  call->node_index=0;call->phase=3;
 }
 while(call->phase>=3&&call->phase<=7&&call->node_index<root[0x2b1]) {
  const uint8_t *node;size_t mesh_index;gh_actor_model_status e=gh_actor_model_node_at(model,(unsigned)call->node_index,&node,&mesh_index);if(e)return fail(frontier,0x14b6f8,e);
  gh_model_mesh mesh;if(gh_model_mesh_at(&model->script,mesh_index,&mesh))return fail(frontier,0x14b6f8,GH_ACTOR_MODEL_BOUNDS);
  uint32_t flags=word(node+0x380),actor_flags=word(root+0x250);
  if(call->phase==3) {
   if((flags&1)||(!(actor_flags&0x400)&&call->far_mode!=!!(mesh.flags&0x1000000))) {call->node_index++;continue;}
   if(!(flags&0x200000)) {
    if(!call->bindings.light||!call->bindings.light->constructed)return fail(frontier,0x1f8df8,GH_ACTOR_MODEL_UNSUPPORTED);
    size_t offset=0x14;if(flags&0x20000)offset=0x68;if(flags&2)offset=0x1c;if(flags&0x40000)offset=0x24;
    int visible=0;e=within_distance(model,call->bindings.camera,number(call->bindings.light->scalar+offset),&visible,frontier);if(e)return e;if(!visible){call->node_index++;continue;}
   }
   call->phase=4;
  }
  if(call->phase==4) {if(!(actor_flags&0x80)){e=call->bindings.textures?gh_actor_materials_0014b4d0(model,(unsigned)call->node_index,call->bindings.textures,&call->materials,frontier):invoke(call,0x14b4d0,frontier);if(e)return e;}call->phase=5;}
  if(call->phase==5) {if(actor_flags&0x400000){e=invoke(call,0x14b5d8,frontier);if(e)return e;}call->phase=6;}
  if(call->phase==6) {
   float alpha=number(root+0x24c);alpha=product(alpha,quotient((float)root[0x2b6],255));alpha=product(alpha,quotient((float)root[0x2b7],255));alpha=product(alpha,quotient((float)root[0x2bc],255));
   if(!isfinite(alpha))return fail(frontier,0x14be38,GH_ACTOR_MODEL_UNSUPPORTED);
   uint32_t program=(actor_flags&0x2000)?20:2;if((flags&0x20000000)&&alpha==1)program=30;if((flags&0x40000000)&&word((uint8_t*)model->work+0x608))program=32;
   call->selected_program=program;
   /* Identity is recovered for all four selectors; only actual program2/30's
    * raster algorithms are implemented. Others never route to a static kernel. */
   if(program!=2&&program!=30)return fail(frontier,0x155ae8,GH_ACTOR_MODEL_UNSUPPORTED);
   if(!call->materials.ready)return fail(frontier,0x151368,GH_ACTOR_MODEL_UNSUPPORTED);
   if(gh_actor_model_node_program_set(model,(unsigned)call->node_index,call->parity,program|UINT32_C(0x80000000)))return fail(frontier,0x155ae8,GH_ACTOR_MODEL_UNSUPPORTED);
   if(call->has_previous&&call->previous_program!=program&&gh_actor_model_node_program_set(model,(unsigned)call->previous_node,call->parity,call->previous_program))return fail(frontier,0x155ae8,GH_ACTOR_MODEL_UNSUPPORTED);
   const gh_actor_node_packets *packet;if(gh_actor_model_packet_at(model,(unsigned)call->node_index,&packet))return fail(frontier,0x14b6f8,GH_ACTOR_MODEL_BOUNDS);
   gh_actor_draw_command command={.kind=GH_ACTOR_DRAW_GEOMETRY,.model=model,.node_index=call->node_index,.parity=call->parity,.selected_program=program,.packet=packet,.materials=call->materials,.packet_group=1};
   if(word(root+0x2c0)==10){command.bank=(flags&0x4080)?2:1;command.depth=(flags&0x4080)?call->depth:13;}else{command.bank=word(root+0x2c0);uint16_t depth;memcpy(&depth,root+0x2ac,2);command.depth=depth;}
   memcpy(&command.test,node+call->parity*0xb0+0x30,8);memcpy(&command.tex1,node+call->parity*0xb0+0x40,8);memcpy(&command.alpha,node+call->parity*0xb0+0x50,8);memcpy(&command.clamp,node+call->parity*0xb0+0x60,8);
   if(program==30){gh_skinned_program30_inputs highlighted;e=gh_actor_model_program30_inputs(model,(unsigned)call->node_index,&highlighted,frontier);if(e)return e;command.inputs=highlighted.base;memcpy(command.specular_half,highlighted.half_vector,12);command.specular_squarings=highlighted.squarings;}else{e=gh_actor_model_program2_inputs(model,(unsigned)call->node_index,&command.inputs,frontier);if(e)return e;}
   call->pending=command;call->phase=7;
  }
  if(call->phase==7) {
   if(!call->bindings.emit)return fail(frontier,0x31a650,GH_ACTOR_MODEL_UNSUPPORTED);
   e=call->bindings.emit(call->bindings.emit_context,&call->pending,frontier);if(e){if(!*frontier)*frontier=0x31a650;return e;}
   call->previous_node=call->node_index;call->previous_program=call->selected_program;call->has_previous=1;call->node_index++;call->materials=(gh_actor_materials){0};call->phase=3;
  }
 }
 if(call->phase<8) {
  if(call->has_previous&&gh_actor_model_node_program_set(model,(unsigned)call->previous_node,call->parity,call->previous_program))return fail(frontier,0x155ae8,GH_ACTOR_MODEL_UNSUPPORTED);
  call->phase=8;
 }
 if(call->phase==8) {
  const uint8_t *first;size_t mesh;const gh_actor_object_packets *object;uint8_t *header;
  if(gh_actor_model_node_at(model,0,&first,&mesh)||gh_actor_model_object_packet(model,&object)||gh_actor_model_object_span(model,object->header_offset[call->parity],0x80,&header))return fail(frontier,0x14c5a0,GH_ACTOR_MODEL_BOUNDS);
  gh_actor_draw_command command={.kind=GH_ACTOR_DRAW_SETUP,.model=model,.node_index=SIZE_MAX,.parity=call->parity,.object=object,.packet_group=1};
  if(word(root+0x2c0)==10){command.bank=(word(first+0x380)&0x4080)?2:1;command.depth=(word(first+0x380)&0x4080)?call->depth:13;}else{command.bank=word(root+0x2c0);uint16_t depth;memcpy(&depth,root+0x2ac,2);command.depth=depth;}
  memcpy(&command.fog_rgb,header+0x30,4);command.fog_rgb&=0xffffffu;call->pending=command;call->phase=9;
 }
 if(call->phase==9) {
  if(!call->bindings.emit)return fail(frontier,0x31a650,GH_ACTOR_MODEL_UNSUPPORTED);
  gh_actor_model_status e=call->bindings.emit(call->bindings.emit_context,&call->pending,frontier);if(e){if(!*frontier)*frontier=0x31a650;return e;}call->phase=10;
 }
 if(word(root+0x250)&0x10){
  gh_actor_model_status status=opacity_pass(call,frontier);
  if(status)return status;
 }
 call->complete=1;return GH_ACTOR_MODEL_OK;
}
gh_actor_model_status gh_actor_model_draw_0014ac90(gh_actor_draw_call *call,gh_actor_model *model,uint16_t depth,uint8_t far_mode,const gh_actor_draw_bindings *bindings,uint32_t *frontier) {
 uint32_t local=0;if(!frontier)frontier=&local;*frontier=0;
 if(!call||!model||!model->ready||model->native_identity!=model||!bindings||!bindings->camera||!bindings->camera->constructed||bindings->policy!=GH_STATIC_BINARY32_REFERENCE||far_mode>1)return GH_ACTOR_MODEL_ARGUMENT;
 if(call->active) {
  if(call->identity!=call||call->model!=model||call->root_identity!=model->root.bytes||call->work_identity!=model->work||call->frame_counter!=word((uint8_t*)model->work+0x210)||call->depth!=depth||call->far_mode!=far_mode||call->bindings.camera!=bindings->camera||call->bindings.light!=bindings->light||call->bindings.callee!=bindings->callee||call->bindings.context!=bindings->context||call->bindings.policy!=bindings->policy||call->bindings.textures!=bindings->textures||call->bindings.emit!=bindings->emit||call->bindings.emit_context!=bindings->emit_context||call->bindings.light_globals!=bindings->light_globals||call->bindings.pivot_owner!=bindings->pivot_owner)return GH_ACTOR_MODEL_BUSY;
 } else {*call=(gh_actor_draw_call){.model=model,.bindings=*bindings,.identity=call,.root_identity=model->root.bytes,.parity=word((uint8_t*)model->work+0x210)&1u,.depth=depth,.far_mode=far_mode,.active=1,.work_identity=model->work,.frame_counter=word((uint8_t*)model->work+0x210)};}
 if(call->complete)return GH_ACTOR_MODEL_OK;
 fenv_t env;if(feholdexcept(&env))return GH_ACTOR_MODEL_UNSUPPORTED;if(fesetround(FE_TONEAREST)){fesetenv(&env);return GH_ACTOR_MODEL_UNSUPPORTED;}
 gh_actor_model_status e=draw(call,frontier);if(fesetenv(&env))return GH_ACTOR_MODEL_UNSUPPORTED;return e;
}
