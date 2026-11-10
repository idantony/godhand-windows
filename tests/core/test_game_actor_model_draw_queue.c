#define main original_actor_nodes_test
#include "test_game_actor_model_nodes.c"
#undef main
#include "game_actor_model_draw.h"
#include "../assets/tm3.h"
#include "ui_order.h"
static void putf(void*p,float x){memcpy(p,&x,4);}
typedef struct {gh_actor_draw_command commands[64];size_t count;unsigned failures;} queue_fixture;
static gh_actor_model_status capture(void *ctx,const gh_actor_draw_command *command,uint32_t *frontier){queue_fixture *q=ctx;if(q->failures){q->failures--;*frontier=0x31a650;return GH_ACTOR_MODEL_UNSUPPORTED;}C(q->count<64&&command->packet_group);q->commands[q->count++]=*command;return GH_ACTOR_MODEL_OK;}
static void put(void*p,uint32_t x){memcpy(p,&x,4);}
static unsigned bit(size_t a,unsigned n){return (unsigned)(a>>n)&1u;}
/* Independent transfer-forward / sample-inverse oracle. Production directly
 * combines the opposite transformations and never constructs this VRAM view. */
static void oracle(const gh_tm3_picture*p,unsigned char*out){
 size_t pixels=(size_t)p->width*p->height;unsigned char*vram=calloc(pixels/2,1),*seen=calloc(pixels,1);C(vram&&seen);
 unsigned pitch=p->width/128;
 if(p->transfer_kind==6){unsigned uw=p->width/2,uh=p->height/4;
  for(unsigned y=0;y<uh;y++)for(unsigned x=0;x<uw;x++){
   unsigned bx=(x%64)/8,by=(y%32)/8,cx=x%8,cy=y%8;
   unsigned block=bx%2+2*(by%2)+4*((bx/2)%2)+8*(by/2)+16*(bx/4);
   unsigned column=cx%2+2*(cy%2)+4*((cx/2)%2)+8*(cx/4)+16*((cy/2)%2)+32*(cy/4);
   size_t a=(((size_t)(y/32)*pitch+x/64)*2048+block*64+column)*4;C(a+4<=pixels/2);memcpy(vram+a,p->image+((size_t)y*uw+x)*4,4);
  }
  for(size_t a=0;a<pixels;a++){
   unsigned x=bit(a,3)|(bit(a,5)<<1)|((bit(a,6)^bit(a,0)^bit(a,7))<<2)|(bit(a,1)<<3)|(bit(a,2)<<4)|(bit(a,10)<<5)|(bit(a,12)<<6);
   unsigned y=bit(a,4)|(bit(a,0)<<1)|(bit(a,7)<<2)|(bit(a,8)<<3)|(bit(a,9)<<4)|(bit(a,11)<<5)|(bit(a,13)<<6);
   x+=(unsigned)((a/16384)%pitch)*128;y+=(unsigned)((a/16384)/pitch)*128;C(x<p->width&&y<p->height&&!seen[(size_t)y*p->width+x]);seen[(size_t)y*p->width+x]=1;
   unsigned index=vram[a/2]>>((a&1)*4)&15;memcpy(out+((size_t)y*p->width+x)*4,p->clut+index*4,4);
  }
 }else for(size_t i=0;i<pixels;i++){unsigned index=p->image[i/2]>>(i%2?4:0)&15;memcpy(out+i*4,p->clut+index*4,4);}
 free(seen);free(vram);
}
int main(int argc,char**argv){
 C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t size;uint8_t*room=load(d,"st1/r100.dat",&size);gh_bundle rb;C(!gh_bundle_open(room,size,&rb));size_t es=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(es);C(elf&&!gh_disc_read_boot_elf(d,0,elf,es));uint8_t*data=load(d,"pl/pl00.dat",&size);gh_disc_close(d);gh_bundle bundle;gh_bundle_entry md,texture;C(!gh_bundle_open(data,size,&bundle)&&!gh_bundle_find(&bundle,"MD",0,&md)&&!gh_bundle_find(&bundle,"TM3",0,&texture));gh_heap*heap;gh_room_pools pools={0};C(!gh_heap_create(32*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&rb,heap));
 _Alignas(16)uint8_t bytes[GH_TITLE_PLAYER_BYTES]={0};gh_title_player_view view;C(!gh_title_player_construct_00107e40(bytes,sizeof(bytes),&view));gh_boot_work work={0};gh_actor_model model={0};uint32_t fog=0;int original;C(!gh_actor_model_bind_packet_inputs(&model,heap,&fog)&&!gh_actor_model_initialize_00149738(&model,&view,&work,&pools,&md,&texture,0,0,GH_STATIC_BINARY32_REFERENCE,&original)&&original&&model.ready);
 gh_game_pl_camera camera={.constructed=1};for(unsigned k=0;k<4;k++){putf(camera.fields+k*20,1);putf(camera.fields+0x80+k*20,k==3?1:.1f);}uint8_t shift=0;gh_game_world_light*light=calloc(1,sizeof(*light));C(light);light->constructed=light->ready=1;putf(light->scalar+0x14,1000);/* Explicitnumericpose/lightfixture, notoriginalbootglobals. */
 gh_player_shadow shadow={0};C(!gh_game_player_shadow_initialize_0014ef50(&shadow,&view));
 (void)shift;(void)shadow;

 gh_stage_textures *textures;C(!gh_stage_textures_create(&textures));gh_tm3_bundle tm3;C(!gh_tm3_open(texture.bytes,texture.size,&tm3));
 size_t total=0;uint32_t pc=0;for(unsigned node_index=0;node_index<bytes[0x2b1];node_index++){
  const gh_actor_node_packets *packet;C(!gh_actor_model_packet_at(&model,node_index,&packet));gh_actor_materials result={0};gh_actor_model_status e=gh_actor_materials_0014b4d0(&model,node_index,textures,&result,&pc);if(e)fprintf(stderr,"node%u error%u pc%08x\n",node_index,e,pc);C(!e&&result.ready&&result.count==packet->stream_count);
  for(size_t i=0;i<result.count;i++){const gh_actor_stream_material *entry=result.streams+i;C(entry->written&&entry->source_member==packet->streams[i].input.material&&entry->selected_member==entry->source_member);gh_tm3_picture picture;C(!gh_tm3_member(&tm3,entry->selected_member,&picture));C(entry->material->source==picture.source&&entry->material->rgba_bytes==(size_t)picture.width*picture.height*4);uint64_t expected=(picture.tex0&~(UINT64_C(3)<<35))|UINT64_C(0x2000000400000000)|((uint64_t)(node_index==24?3:0)<<35);C(entry->tex0==expected);size_t pixels=(size_t)picture.width*picture.height*4;uint8_t *oracle_pixels=malloc(pixels);C(oracle_pixels);oracle(&picture,oracle_pixels);C(!memcmp(oracle_pixels,entry->material->rgba_gs,pixels));free(oracle_pixels);uint64_t stored;memcpy(&stored,(uint8_t*)packet->allocation.pointer+packet->streams[i].frame_offset[0]+0x20,8);C(stored==expected);total++;}
  gh_actor_materials again={0};C(!gh_actor_materials_0014b4d0(&model,node_index,textures,&again,&pc));for(size_t i=0;i<result.count;i++)C(result.streams[i].material==again.streams[i].material);
 }
 C(total>26);const uint8_t *raw;size_t mesh;C(!gh_actor_model_node_at(&model,0,&raw,&mesh));uint8_t *node=(uint8_t*)raw;gh_actor_materials result={0};node[0x387]=1;C(gh_actor_materials_0014b4d0(&model,0,textures,&result,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x148690&&!result.ready);node[0x387]=0;
 put(bytes+0x254,w(bytes+0x254)|0x10000000);memset(bytes+0x2d4,255,256);bytes[0x2d4+0]=1;C(!gh_actor_materials_0014b4d0(&model,0,textures,&result,&pc));for(size_t i=0;i<result.count;i++)if(result.streams[i].source_member==0)C(result.streams[i].selected_member==1);put(bytes+0x254,w(bytes+0x254)&~0x10000000u);
 for(unsigned i=0;i<3;i++)putf(light->scalar+0x161a0+i*16+i*4,-1);putf(light->scalar+0x16154,100);
 gh_actor_draw_bindings b={.camera=&camera,.light=light,.policy=GH_STATIC_BINARY32_REFERENCE,.textures=textures};gh_actor_draw_call call={0};C(gh_actor_model_draw_0014ac90(&call,&model,10,0,&b,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x31a650&&call.phase==7&&call.materials.ready);

 /* Source draw call completes actual enqueue only. Then the independently
  * owned fixture applies real OT order and executes every CPU geometry batch;
  * no collector is described as a GPU/frame result. */
 bytes[0x2b6]=128;queue_fixture queued={.failures=1};b.emit=capture;b.emit_context=&queued;
 call=(gh_actor_draw_call){0};C(gh_actor_model_draw_0014ac90(&call,&model,10,0,&b,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x31a650&&call.phase==7&&!queued.count);
 gh_actor_draw_call copied=call;C(gh_actor_model_draw_0014ac90(&copied,&model,10,0,&b,&pc)==GH_ACTOR_MODEL_BUSY);uint32_t counter=1;memcpy((uint8_t*)&work+0x210,&counter,4);C(gh_actor_model_draw_0014ac90(&call,&model,10,0,&b,&pc)==GH_ACTOR_MODEL_BUSY&&!queued.count);counter=0;memcpy((uint8_t*)&work+0x210,&counter,4);C(!gh_actor_model_draw_0014ac90(&call,&model,10,0,&b,&pc)&&call.complete&&queued.count==27);
 C(queued.commands[26].kind==GH_ACTOR_DRAW_SETUP&&queued.commands[26].bank==2&&queued.commands[26].depth==10);
 gh_ui_order_group groups[64];for(size_t i=0;i<queued.count;i++)groups[i]=(gh_ui_order_group){queued.commands[i].bank,queued.commands[i].depth,i};size_t order[64],written,omitted;C(!gh_ui_order_indices(groups,queued.count,0,0,order,64,&written,&omitted)&&written==27&&!omitted&&order[0]==26);
 size_t batches_executed=0,vertices_executed=0;for(size_t i=1;i<written;i++){const gh_actor_draw_command *command=queued.commands+order[i];C(command->kind==GH_ACTOR_DRAW_GEOMETRY&&command->node_index==26-i&&command->selected_program==2);uint32_t selector;C(!gh_actor_model_node_program_get(&model,(unsigned)command->node_index,0,&selector));C(selector==(command->node_index==25?2:0x80000002u));for(size_t stream=0;stream<command->packet->stream_count;stream++){C(command->materials.streams[stream].written);for(size_t batch=0;batch<command->packet->streams[stream].batches;batch++){gh_static_gs_vertex vertices[26];uint32_t indices[72];gh_static_geometry_result result;gh_static_transform_frontier frontier;C(!gh_skinned_transform_program2_batch(&command->packet->streams[stream].input,batch,&command->inputs,GH_STATIC_BINARY32_REFERENCE,vertices,26,indices,72,&result,&frontier));batches_executed++;vertices_executed+=result.vertices;}}}
 C(batches_executed>300&&vertices_executed==9009);
 uint32_t selector;C(!gh_actor_model_node_program_get(&model,0,0,&selector)&&selector==0x80000002u);C(gh_actor_model_node_program_set(&model,0,0,0x40000002)==GH_ACTOR_MODEL_UNSUPPORTED);uint32_t unchanged;C(!gh_actor_model_node_program_get(&model,0,0,&unchanged)&&selector==unchanged);
 C(!gh_actor_model_node_program_set(&model,0,1,0x8000001eu)&&!gh_actor_model_node_program_get(&model,0,1,&selector)&&selector==0x8000001eu);C(!gh_actor_model_node_program_set(&model,0,1,30)&&!gh_actor_model_node_program_get(&model,0,1,&selector)&&selector==30);
 const gh_actor_object_packets *object;C(!gh_actor_model_object_packet(&model,&object)&&object->object_program[0]==0x80000000u);
 /* The real program30 material now preserves its own shader/TFX3 state. */
 bytes[0x2b6]=255;queued=(queue_fixture){0};call=(gh_actor_draw_call){0};C(!gh_actor_model_draw_0014ac90(&call,&model,10,0,&b,&pc)&&call.complete&&queued.count==27&&queued.commands[24].selected_program==30&&((queued.commands[24].materials.streams[0].tex0>>35)&3)==3);
 gh_stage_textures_destroy(textures);C(!gh_actor_model_release(&model)&&!gh_game_room_pools_release(&pools)&&!gh_heap_destroy(heap));free(light);free(data);free(elf);free(room);printf("PASS actual PL00 %zu material streams/26nodes, direct TM3 source/TEX0 oracle, cache identity/remap/CBP-bias strict, actual27groups OTsetup-first/reverse26nodes, programselectors and full9009vertex CPU consume; no GPU/frame ack\n",total);return 0;
}
