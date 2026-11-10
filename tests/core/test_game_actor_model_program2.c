#define main original_actor_nodes_test
#include "test_game_actor_model_nodes.c"
#undef main
#include "game_actor_model_draw.h"
#include "game_actor_model_program2.h"
static void putf(void*p,float n){memcpy(p,&n,4);}
static float value(const void*p){float n;memcpy(&n,p,4);return n;}
int main(int argc,char**argv){
 C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t size;uint8_t*room=load(d,"st1/r100.dat",&size);gh_bundle rb;C(!gh_bundle_open(room,size,&rb));size_t es=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(es);C(elf&&!gh_disc_read_boot_elf(d,0,elf,es));uint8_t*data=load(d,"pl/pl00.dat",&size);gh_disc_close(d);gh_bundle bundle;gh_bundle_entry md,texture;C(!gh_bundle_open(data,size,&bundle)&&!gh_bundle_find(&bundle,"MD",0,&md)&&!gh_bundle_find(&bundle,"TM3",0,&texture));gh_heap*heap;gh_room_pools pools={0};C(!gh_heap_create(32*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&rb,heap));
 _Alignas(16)uint8_t bytes[GH_TITLE_PLAYER_BYTES]={0};gh_title_player_view view;C(!gh_title_player_construct_00107e40(bytes,sizeof(bytes),&view));gh_boot_work work={0};gh_actor_model model={0};uint32_t fog=0;int original;C(!gh_actor_model_bind_packet_inputs(&model,heap,&fog)&&!gh_actor_model_initialize_00149738(&model,&view,&work,&pools,&md,&texture,0,0,GH_STATIC_BINARY32_REFERENCE,&original)&&original&&model.ready);
 gh_game_pl_camera camera={.constructed=1};for(unsigned k=0;k<4;k++){putf(camera.fields+k*20,1);putf(camera.fields+0x80+k*20,k==3?1:.1f);}uint8_t shift=0;gh_game_world_light*light=calloc(1,sizeof(*light));C(light);light->constructed=light->ready=1;putf(light->scalar+0x14,1000);/* Explicitnumericpose/lightfixture, notoriginalbootglobals. */
 gh_player_shadow shadow={0};C(!gh_game_player_shadow_initialize_0014ef50(&shadow,&view));
 (void)shift;(void)shadow;
 const gh_actor_object_packets *packet;C(!gh_actor_model_object_packet(&model,&packet));uint8_t *span;C(!gh_actor_model_object_span(&model,0,packet->bytes,&span));
 C(gh_actor_model_object_span(&model,packet->bytes,1,&span)==GH_ACTOR_MODEL_BOUNDS);
 for(unsigned mode=0;mode<4;mode++) {const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};C(!fesetround(modes[mode]));uint32_t pc;C(!gh_actor_palette_0014c7e8(&model,&camera,GH_STATIC_BINARY32_REFERENCE,&pc)&&!pc&&fegetround()==modes[mode]);
  uint8_t *palette;C(!gh_actor_model_object_span(&model,packet->palette_offset[0]+16,model.part_count*64,&palette));
  for(unsigned i=0;i<model.part_count;i++) {const gh_actor_part *part;C(!gh_actor_model_part_at(&model,i,&part));for(unsigned row=0;row<4;row++) {long double expected=value(part->bytes+0x80+48+row*4);for(unsigned k=0;k<3;k++)expected+=(long double)value(part->bytes+0x80+(k*4+row)*4)*value(part->bytes+0xc0+k*4);C(fabsl((long double)value(palette+i*64+48+row*4)-expected)<.0001L);}}
 }

 /* Explicit numeric deformation fixture on SAME real part storage proves
  * palette multiplication rather than a copy of the bind pose. */
 const gh_actor_part *part0;C(!gh_actor_model_part_at(&model,0,&part0));
 float fixture[16]={0,1,0,0,-1,0,0,0,0,0,2,0,3,4,5,1};memcpy(part0->bytes+0x80,fixture,64);float inverse[3]={2,3,4};memcpy(part0->bytes+0xc0,inverse,12);
 uint8_t *other;C(!gh_actor_model_object_span(&model,packet->palette_offset[1]+16,64,&other));memset(other,0x7d,64);
 uint32_t pcf=0;C(!gh_actor_palette_0014c7e8(&model,&camera,GH_STATIC_BINARY32_REFERENCE,&pcf));uint8_t *first;C(!gh_actor_model_object_span(&model,packet->palette_offset[0]+16,64,&first));C(value(first+48)==0&&value(first+52)==6&&value(first+56)==13);for(unsigned k=0;k<64;k++)C(other[k]==0x7d);
 uint32_t parity=1;memcpy((uint8_t*)&work+0x210,&parity,4);C(!gh_actor_palette_0014c7e8(&model,&camera,GH_STATIC_BINARY32_REFERENCE,&pcf)&&!memcmp(first,other,64));parity=0;memcpy((uint8_t*)&work+0x210,&parity,4);
 gh_actor_model copy=model;C(gh_actor_model_object_span(&copy,0,1,&span)==GH_ACTOR_MODEL_ARGUMENT);
 /* A nonfinite direction exercises retry; exact zero is now supported. */
 putf(light->scalar+0x161a0,NAN);
 C(!fesetround(FE_TONEAREST));uint32_t pc;gh_actor_draw_call call={0};gh_actor_draw_bindings b={.camera=&camera,.light=light,.policy=GH_STATIC_BINARY32_REFERENCE};C(gh_actor_model_draw_0014ac90(&call,&model,10,0,&b,&pc)==GH_ACTOR_MODEL_UNSUPPORTED);C(pc==0x30a370&&call.phase==2&&call.node_index==0);
 for(unsigned i=0;i<3;i++)putf(light->scalar+0x161a0+i*16+i*4,-1);putf(light->scalar+0x16154,100);for(unsigned i=0;i<4;i++)for(unsigned k=0;k<4;k++)putf(light->scalar+(i<3?0x161d0+i*16:0x40)+k*4,(float)(i+1)*.2f);
 gh_actor_model_status er=gh_actor_model_draw_0014ac90(&call,&model,10,0,&b,&pc);C(er==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x14b4d0&&call.phase==4);
 const uint8_t *node;size_t mesh;C(!gh_actor_model_node_at(&model,0,&node,&mesh));C(value(node+0x160+0x10)==1&&value(node+0x160+0x24)==1&&value(node+0x160+0x38)==1&&value(node+0x160+0x50)==.2f);
 uint32_t marker=0xdeadbeef;memcpy((uint8_t*)node+0x30,&marker,4);C(gh_actor_model_draw_0014ac90(&call,&model,10,0,&b,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x14b4d0&&w(node+0x30)==marker);


 /* Node24's original reflection flag exercises the positive material branch
  * with a nonzero canonical camera/light half-vector and source exponent. */
 putf(camera.fields+0x210,0);putf(camera.fields+0x214,1);putf(camera.fields+0x218,0);
 putf(light->scalar+0x16200,0);putf(light->scalar+0x16204,0);putf(light->scalar+0x16208,-1);
 C(!gh_actor_node_light_00154240(&model,24,&camera,light,GH_STATIC_BINARY32_REFERENCE,&pc));const uint8_t *reflect;size_t mi;C(!gh_actor_model_node_at(&model,24,&reflect,&mi));C(fabsf(value(reflect+0x160+0x94)-sqrtf(.5f))<.00001f&&fabsf(value(reflect+0x160+0x98)-sqrtf(.5f))<.00001f);
 gh_model_mesh source_mesh;C(!gh_model_mesh_at(&model.script,mi,&source_mesh));uint16_t exponent;memcpy(&exponent,reflect+0x160+0xb0,2);C(exponent==model.script.bytes[source_mesh.descriptor_offset+0x58]);
 gh_skinned_program2_inputs actual;C(!gh_actor_model_program2_inputs(&model,0,&actual,&pc));const gh_actor_object_packets *object;C(!gh_actor_model_object_packet(&model,&object));uint8_t *palette_view;C(!gh_actor_model_object_span(&model,object->palette_offset[0]+16,model.part_count*64,&palette_view)&&actual.palette==palette_view&&actual.part_count==32);C(!memcmp(actual.normal_light,node+0x170,64));
 C(!gh_actor_model_release(&model)&&!gh_game_room_pools_release(&pools)&&!gh_heap_destroy(heap));free(light);free(data);free(elf);free(room);puts("PASS actual PL00 palette, independent longdouble translations, 4FEnv, bounded same allocation, strict light");return 0;}
