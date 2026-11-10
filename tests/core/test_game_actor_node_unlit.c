#define main original_actor_nodes_test
#include "test_game_actor_model_nodes.c"
#undef main
#include "game_actor_model_draw.h"
static void putf(void*p,float n){memcpy(p,&n,4);}
static float value(const void*p){float n;memcpy(&n,p,4);return n;}
int main(int argc,char**argv){
 C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t n;uint8_t*room=load(d,"st1/r100.dat",&n);gh_bundle rb;C(!gh_bundle_open(room,n,&rb));size_t en=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(en);C(elf&&!gh_disc_read_boot_elf(d,0,elf,en));uint8_t*data=load(d,"pl/pl00.dat",&n);size_t child_n;uint8_t*child_data=load(d,"pl/pl01.dat",&child_n);size_t om_n;uint8_t*om_data=load(d,"om/om00.dat",&om_n);gh_disc_close(d);gh_bundle pb;gh_bundle_entry md,texture;C(!gh_bundle_open(data,n,&pb)&&!gh_bundle_find(&pb,"MD",0,&md)&&!gh_bundle_find(&pb,"TM3",0,&texture));
 gh_heap*heap;gh_room_pools pools={0};C(!gh_heap_create(32*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&rb,heap));uint8_t bytes[GH_TITLE_PLAYER_BYTES]={0};gh_title_player_view v;C(!gh_title_player_construct_00107e40(bytes,sizeof(bytes),&v));gh_boot_work work={0};gh_actor_model m={0};uint32_t fog=0;int original;C(!gh_actor_model_bind_packet_inputs(&m,heap,&fog)&&!gh_actor_model_initialize_00149738(&m,&v,&work,&pools,&md,&texture,0,0,GH_STATIC_BINARY32_REFERENCE,&original)&&original);
 const uint8_t*actual_node;size_t actual_mesh;C(!gh_actor_model_node_at(&m,1,&actual_node,&actual_mesh));printf("PL00 node1 flags%08x extra%08x\n",w(actual_node+0x380),w(actual_node+0x410));
 gh_bundle cb;gh_bundle_entry cmd,ctm;C(!gh_bundle_open(child_data,child_n,&cb)&&!gh_bundle_find(&cb,"MD",0,&cmd)&&!gh_bundle_find(&cb,"TM3",0,&ctm));uint8_t child_bytes[GH_TITLE_PLAYER_BYTES]={0};gh_title_player_view cv;C(!gh_title_player_construct_00107e40(child_bytes,sizeof(child_bytes),&cv));gh_actor_model cm={0};C(!gh_actor_model_bind_packet_inputs(&cm,heap,&fog)&&!gh_actor_model_initialize_00149738(&cm,&cv,&work,&pools,&cmd,&ctm,0,0,GH_STATIC_BINARY32_REFERENCE,&original)&&original);const uint8_t*child_node;C(!gh_actor_model_node_at(&cm,1,&child_node,&actual_mesh));printf("PL01 node1 flags%08x extra%08x\n",w(child_node+0x380),w(child_node+0x410));
 gh_actor_light_globals globals={0},bad={0};gh_player_shared_vector pivot={0};C(!gh_actor_light_globals_from_elf(&globals,elf,en)&&!gh_player_shared_vector_from_elf(&pivot,elf,en));
 for(unsigned k=0;k<4;k++)C(!w(globals.bytes_3bd870+k*4)&&value(globals.bytes_3bd890+k*4)==1);
 C(gh_actor_light_globals_from_elf(&bad,elf,51)==GH_ACTOR_MODEL_BOUNDS&&!bad.initialized);
 uint8_t cold_white[16];memcpy(cold_white,globals.bytes_3bd890,16);putf(globals.bytes_3bd890,2);C(gh_actor_light_globals_from_elf(&globals,elf,en)==GH_ACTOR_MODEL_BUSY&&value(globals.bytes_3bd890)==2);memcpy(globals.bytes_3bd890,cold_white,16);
 /* PL00/PL01 cold node1 shareflags: do not infer actoridentity. This
  * explicit observed-bit fixture exercises the same confirmedunlit branch. */
 C(w(child_node+0x380)==0x08001080u&&!(w(child_node+0x410)&0x20000000u));uint32_t observed=0x08801080;memcpy((uint8_t*)child_node+0x380,&observed,4);
 gh_game_pl_camera child_camera={.constructed=1};gh_game_world_light*child_light=calloc(1,sizeof(*child_light));C(child_light&&!gh_game_world_light_construct_002d6d80(child_light));child_light->ready=1;uint32_t child_pc;
 C(!gh_actor_node_light_00154240_ex(&cm,1,&child_camera,child_light,&globals,&pivot,GH_STATIC_BINARY32_REFERENCE,&child_pc)&&!child_pc);
 for(unsigned k=0;k<4;k++)C(value(child_node+0x160+0x80+k*4)==value(globals.bytes_3bd890+k*4));C(!gh_actor_model_release(&cm));free(child_data);
 gh_bundle ob;gh_bundle_entry omd,otm;C(!gh_bundle_open(om_data,om_n,&ob)&&!gh_bundle_find(&ob,"MD",0,&omd)&&!gh_bundle_find(&ob,"TM3",0,&otm));uint8_t ombytes[GH_TITLE_PLAYER_BYTES]={0};gh_title_player_view ov;C(!gh_title_player_construct_00107e40(ombytes,sizeof(ombytes),&ov));gh_actor_model om={0};C(!gh_actor_model_bind_packet_inputs(&om,heap,&fog)&&!gh_actor_model_initialize_00149738(&om,&ov,&work,&pools,&omd,&otm,0,0,GH_STATIC_BINARY32_REFERENCE,&original)&&original);const uint8_t*omnode;C(!gh_actor_model_node_at(&om,1,&omnode,&actual_mesh));printf("OM00 node1 flags%08x extra%08x\n",w(omnode+0x380),w(omnode+0x410));C(w(omnode+0x380)==0x08801080u&&!w(omnode+0x410));C(!gh_actor_node_light_00154240_ex(&om,1,&child_camera,child_light,&globals,&pivot,GH_STATIC_BINARY32_REFERENCE,&child_pc));free(child_light);C(!gh_actor_model_release(&om));free(om_data);
 const uint8_t*raw;size_t mesh;C(!gh_actor_model_node_at(&m,0,&raw,&mesh));uint8_t*node=(uint8_t*)raw;gh_game_pl_camera camera={.constructed=1};gh_game_world_light*light=calloc(1,sizeof(*light));C(light&&!gh_game_world_light_construct_002d6d80(light));light->ready=1;
 const float pivot_values[4]={.125f,.25f,.375f,.5f},white[4]={2,3,4,5},override[4]={6,7,8,9},ambient[4]={.1f,.2f,.3f,.4f};memcpy(pivot.bytes,pivot_values,16);memcpy(globals.bytes_3bd890,white,16);memcpy(globals.bytes_3bd870,override,16);memcpy(light->scalar+0x30,ambient,16);
 for(unsigned k=0;k<4;k++){putf(bytes+0x240+k*4,.5f);putf(node+0x3a0+k*4,.5f);}bytes[0x2b6]=bytes[0x2b7]=bytes[0x2bc]=255;
 const int modes[4]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};uint32_t pc;
 for(unsigned mode=0;mode<4;mode++)for(unsigned branch=0;branch<2;branch++)for(unsigned black=0;black<2;black++)for(unsigned parity=0;parity<2;parity++){
  C(!fesetround(modes[mode]));uint32_t flags=0x200000|(branch?0:0x800000);memcpy(node+0x380,&flags,4);uint32_t extra=branch?0x20000000:0;memcpy(node+0x410,&extra,4);uint32_t root_flags=black?0x80000:0;memcpy(bytes+0x250,&root_flags,4);memcpy((uint8_t*)&work+0x210,&parity,4);
  memset(node+0x160+parity*0xd0,0x5a,0xd0);uint8_t other[0xd0];memcpy(other,node+0x160+(1-parity)*0xd0,sizeof(other));
  C(!gh_actor_node_light_00154240_ex(&m,0,&camera,light,&globals,&pivot,GH_STATIC_BINARY32_REFERENCE,&pc)&&!pc&&fegetround()==modes[mode]&&!memcmp(other,node+0x160+(1-parity)*0xd0,sizeof(other)));
  uint8_t*out=node+0x160+parity*0xd0;
  for(unsigned i=0;i<3;i++)for(unsigned k=0;k<4;k++){
   C(value(out+0x50+i*16+k*4)==pivot_values[k]);
   C(value(out+0x10+i*16+k*4)==(i==1&&k==3?255:pivot_values[k]));
  }
  for(unsigned k=0;k<4;k++){
   C(value(out+0x40+k*4)==.25f);
   float expected=black?override[k]:(branch?ambient[k]*.25f:white[k]);C(value(out+0x80+k*4)==expected);
  }
 }
 /* Ordinary root800000 fast branch retains prior-frame normal15 words
  * and color12 words. Only later originalfog overridesnormalword7. */
 for(unsigned mode=0;mode<4;mode++)for(unsigned parity=0;parity<2;parity++){
  C(!fesetround(modes[mode]));uint32_t nf=0x200000,extra=0,af=0x800000;memcpy(node+0x380,&nf,4);memcpy(node+0x410,&extra,4);memcpy(bytes+0x250,&af,4);memcpy((uint8_t*)&work+0x210,&parity,4);uint8_t*out=node+0x160+parity*0xd0;memset(out,0xa5,0xd0);uint8_t before[0xd0];memcpy(before,out,sizeof(before));
  C(!gh_actor_node_light_00154240_ex(&m,0,&camera,light,NULL,NULL,GH_STATIC_BINARY32_REFERENCE,&pc)&&fegetround()==modes[mode]);
  for(unsigned k=0;k<16;k++){
   if(k==15)C(value(out+0x10+k*4)==1);else if(k==7)C(value(out+0x10+k*4)==255);else C(!memcmp(out+0x10+k*4,before+0x10+k*4,4));
   if(k<12)C(!memcmp(out+0x50+k*4,before+0x50+k*4,4));else C(value(out+0x50+k*4)==ambient[k-12]*.25f);
  }
 }
 {uint8_t*out=node+0x160+0xd0;uint8_t before[0xd0];memcpy(before,out,sizeof(before));putf(light->scalar+0x3c,NAN);C(gh_actor_node_light_00154240_ex(&m,0,&camera,light,NULL,NULL,GH_STATIC_BINARY32_REFERENCE,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x30a4d0&&!memcmp(before,out,sizeof(before)));memcpy(light->scalar+0x30,ambient,16);}
 C(!fesetround(FE_TONEAREST));uint32_t flags=0xa00000;memcpy(node+0x380,&flags,4);uint8_t snapshot[0xd0];memcpy(snapshot,node+0x160+0xd0,sizeof(snapshot));
 C(gh_actor_node_light_00154240(&m,0,&camera,light,GH_STATIC_BINARY32_REFERENCE,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x154240&&!memcmp(snapshot,node+0x160+0xd0,sizeof(snapshot)));
 C(gh_actor_node_light_00154240_ex(&m,0,&camera,light,&globals,NULL,GH_STATIC_BINARY32_REFERENCE,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x154240);
 C(!gh_actor_model_release(&m)&&!gh_game_room_pools_release(&pools)&&!gh_heap_destroy(heap));free(light);free(data);free(elf);free(room);
 puts("PASS actual PL00/ELF unlit globals, shared mutable pivot, both source branches/ambient override/2parities/4FEnv and legacy strict; notworldframe");return 0;
}
