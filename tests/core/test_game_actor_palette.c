#include "game_actor_palette.h"
#include "skinned_mesh_blend.h"
#include "disc.h"
#include <fenv.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL line%d: %s\n",__LINE__,#x);exit(1);}}while(0)

static uint8_t *load(gh_disc *disc,const char *path,size_t *size)
{
 size_t index;gh_disc_entry entry;
 C(!gh_disc_find(disc,path,&index)&&!gh_disc_entry_at(disc,index,&entry));
 *size=(size_t)entry.size;uint8_t *bytes=malloc(*size);
 C(bytes&&!gh_disc_read(disc,index,0,bytes,*size));return bytes;
}

/* Independent vertex-first formula: translate the bind vertex by C0,
 * then use the original part's world matrix. No produced palette is read. */
static void oracle(const gh_actor_model *model,const gh_skinned_mesh_vertex *v,
 long double position[3],long double normal[3])
{
 const long double unit=.00999999977648258209228515625L;
 const long double normal_scale=.0078740157186985015869140625L;
 for(unsigned row=0;row<3;row++){
  position[row]=normal[row]=0;
  for(unsigned influence=0;influence<3;influence++){
   unsigned index=v->bone_indices[influence+1]/4;
   C(!(v->bone_indices[influence+1]%4)&&index<model->part_count);
   float world[16],inverse[3];memcpy(world,model->parts[index].bytes+0x80,64);
   memcpy(inverse,model->parts[index].bytes+0xc0,12);
   long double p=world[12+row],n=0;
   for(unsigned axis=0;axis<3;axis++){
    long double signed_normal=v->normal[axis];if(signed_normal>=128)signed_normal-=256;
    p+=(long double)world[axis*4+row]*((long double)v->position[axis]+inverse[axis]);
    n+=(long double)world[axis*4+row]*signed_normal*normal_scale;
   }
   position[row]+=p*v->weights[influence]*unit;
   normal[row]+=n*v->weights[influence]*unit;
  }
 }
}

int main(int argc,char **argv)
{
 C(argc==2);gh_disc *disc;C(!gh_disc_open(argv[1],&disc));size_t bytes;
 uint8_t *room=load(disc,"st1/r100.dat",&bytes);gh_bundle room_bundle;
 C(!gh_bundle_open(room,bytes,&room_bundle));gh_heap *heap;
 C(!gh_heap_create(32*1024*1024,&heap));gh_room_pools pools={0};
 C(!gh_game_room_pools_initialize_002a5d80(&pools,&room_bundle,heap));
 const char *paths[]={"om/om00.dat","pl/pl00.dat","pl/pl01.dat"};
 const unsigned expected_parts[]={7,32,41};
 const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};unsigned vertices=0;
 for(unsigned asset=0;asset<3;asset++){
  uint8_t *raw=load(disc,paths[asset],&bytes);gh_bundle bundle;gh_bundle_entry md,texture;
  C(!gh_bundle_open(raw,bytes,&bundle)&&!gh_bundle_find(&bundle,"MD",0,&md)&&!gh_bundle_find(&bundle,"TM3",0,&texture));
  _Alignas(16)uint8_t object[GH_TITLE_PLAYER_BYTES]={0};gh_title_player_view root;
  C(!gh_title_player_construct_00107e40(object,sizeof(object),&root));gh_boot_work work={0};
  gh_actor_model model={0};uint32_t fog=0;int original;
  C(!gh_actor_model_bind_packet_inputs(&model,heap,&fog));
  C(!gh_actor_model_initialize_00149738(&model,&root,&work,&pools,&md,&texture,0,0,GH_STATIC_BINARY32_REFERENCE,&original));
  C(original&&model.part_count==expected_parts[asset]);
  float translation[3]={2,-4,8},rotation[3]={.25f,-.5f,.125f},scale[3]={2,.5f,-1};
  memcpy(root.alias_f0,translation,12);memcpy(object+0x100,rotation,12);memcpy(object+0x110,scale,12);
  C(!gh_actor_model_update_hierarchy_00149eb8(&model,GH_STATIC_BINARY32_REFERENCE));
  float reference[42][16]={0};size_t count=99;uint32_t frontier;
  for(unsigned mode=0;mode<4;mode++){
   float palette[42][16];memset(palette,0xa5,sizeof(palette));
   C(!fesetround(modes[mode])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));
   C(!gh_actor_palette_from_parts(&model,GH_STATIC_BINARY32_REFERENCE,palette,42,&count,&frontier));
   C(count==model.part_count&&!frontier&&fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);
   if(!mode)memcpy(reference,palette,count*sizeof(palette[0]));
   else C(!memcmp(reference,palette,count*sizeof(palette[0])));
   for(unsigned part=0;part<count;part++){
    float world[16],inverse[3];memcpy(world,model.parts[part].bytes+0x80,64);memcpy(inverse,model.parts[part].bytes+0xc0,12);
    for(unsigned col=0;col<4;col++)for(unsigned row=0;row<4;row++){
     long double expected=world[col*4+row];
     if(col==3)for(unsigned axis=0;axis<3;axis++)expected+=(long double)world[axis*4+row]*inverse[axis];
     C(fabsl((long double)palette[part][col*4+row]-expected)<.00002L*(1+fabsl(expected)));
    }
   }
  }
  C(!fesetround(FE_TONEAREST));
  for(size_t mesh=0;mesh<model.script.mesh_count;mesh++){
   size_t streams;C(!gh_skinned_mesh_stream_count(&model.script,mesh,&streams));
   for(size_t stream=0;stream<streams;stream++){
    gh_skinned_mesh_stream input;C(!gh_skinned_mesh_stream_at(&model.script,mesh,stream,&input));
    for(size_t vertex=0;vertex<input.vertices;vertex++){
     gh_skinned_mesh_vertex v;gh_skinned_blend_result result;
     C(!gh_skinned_mesh_vertex_at(&input,vertex,&v));
     C(!gh_skinned_mesh_blend_program2(&v,reference,count,GH_STATIC_BINARY32_REFERENCE,&result,&frontier));
     long double p[3],n[3];oracle(&model,&v,p,n);
     for(unsigned k=0;k<3;k++)C(fabsl((long double)result.position[k]-p[k])<.00003L*(1+fabsl(p[k]))&&fabsl((long double)result.normal[k]-n[k])<.00003L*(1+fabsl(n[k])));
     C(result.raw_control==v.raw_control);vertices++;
    }
   }
  }
  float output[42][16],unchanged[42][16];memset(output,0xa5,sizeof(output));memcpy(unchanged,output,sizeof(output));size_t sentinel=123;
  C(gh_actor_palette_from_parts(&model,GH_STATIC_BINARY32_REFERENCE,output,count-1,&sentinel,&frontier)==GH_ACTOR_MODEL_BOUNDS&&sentinel==123&&!memcmp(output,unchanged,sizeof(output)));
  float saved;memcpy(&saved,model.parts[count-1].bytes+0xc0,4);float nan=NAN;memcpy(model.parts[count-1].bytes+0xc0,&nan,4);
  C(gh_actor_palette_from_parts(&model,GH_STATIC_BINARY32_REFERENCE,output,42,&sentinel,&frontier)==GH_ACTOR_MODEL_UNSUPPORTED&&frontier==0x30a2e0&&sentinel==123&&!memcmp(output,unchanged,sizeof(output)));
  memcpy(model.parts[count-1].bytes+0xc0,&saved,4);
  memcpy(model.parts[0].bytes+0xcc,&nan,4); /* 1503B8 reads XYZ only. */
  C(!gh_actor_palette_from_parts(&model,GH_STATIC_BINARY32_REFERENCE,output,42,&sentinel,&frontier)&&!memcmp(output,reference,count*sizeof(output[0])));
  gh_actor_model copied=model;
  C(gh_actor_palette_from_parts(&copied,GH_STATIC_BINARY32_REFERENCE,output,42,&sentinel,&frontier)==GH_ACTOR_MODEL_ARGUMENT);
  C(!gh_actor_model_release(&model));free(raw);
 }
 C(vertices==9573&&!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));
 gh_disc_close(disc);free(room);
 printf("PASS original1503B8 same-part world/inverseXYZ palette; %u actual OM00/PL00/PL01 vertices match independent vertex-first oracle; fourFEnv/exceptions, capacity/NaN atomicfailure, ignoredC0.W, copied-owner rejection; no lighting/GPU/frame claim\n",vertices);
 return 0;
}
