#include "game_player_child_tail.h"
#include "game_player_child_move.h"
#include "game_cloth_forces.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t*load(gh_disc*d,const char*path,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,path,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*b=malloc(*n);C(b&&!gh_disc_read(d,i,0,b,*n));return b;}
static uint32_t w(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}

static float value(const uint8_t*p){float n;memcpy(&n,p,4);return n;}
static void check_small_wave(gh_cloth_step_prefix *prefix, int mode)
{
    gh_cloth_operands *springs = &prefix->operands;
    C(springs->count == 40 && springs->indices[0][0] == 1 &&
        springs->coefficients[1][0] == 0 && springs->coefficients[2][0] == 0x3f800000u);
    uint8_t saved_records[44 * 0xa0];
    memcpy(saved_records, springs->records, sizeof(saved_records));
    uint8_t *record = springs->records + 0xa0;
    uint8_t *parent = springs->parent->root.bytes;
    uint32_t saved_phase = w(springs->config + 0x48);
    uint32_t movement = w(parent + 0x650), wind = w(prefix->bindings.globals->wind);
    static const uint32_t phase[] = {0x374e0000u, 0xb74e0000u};
    /* The independent original VF4.x oracle gives signed 0.00048828125
     * from radical B4800000. The first real PL01 row has a unit multiplier. */
    static const uint32_t expected[] = {0x3f801000u, 0x3f7fe000u};
    uint32_t unit = 0x3f800000u;
    memset(parent + 0x650, 0, 4);
    memcpy(prefix->bindings.globals->wind, &unit, 4);
    for (unsigned i = 0; i < 2; ++i) {
        memcpy(springs->records, saved_records, sizeof(saved_records));
        memset(record + 0x30, 0, 4);
        memset(record + 0x40, 0, 4);
        memcpy(springs->config + 0x48, phase + i, 4);
        gh_cloth_forces force = {0};
        C(!fesetround(mode) && !feclearexcept(FE_ALL_EXCEPT) && !feraiseexcept(FE_INVALID));
        C(!gh_cloth_integrate_forces_0010430c(&force, prefix) && force.complete &&
            force.frontier == 0x10497c && w(record + 0x40) == expected[i] && !w(record + 0x50));
        C(fegetround() == mode && fetestexcept(FE_ALL_EXCEPT) == FE_INVALID);
    }
    memcpy(springs->records, saved_records, sizeof(saved_records));
    memcpy(parent + 0x650, &movement, 4);
    memcpy(prefix->bindings.globals->wind, &wind, 4);
    memcpy(springs->config + 0x48, &saved_phase, 4);
    C(!fesetround(FE_TONEAREST));
}

static void move_checks(gh_game_player_springs*springs,const void*elf,size_t bytes,gh_game_player_factory*factory,gh_actor_model*parent,gh_core_save_view save,gh_boot_work*work){
 gh_game_player_child*child=springs->child;gh_player_child_attachment attachment={0};C(!gh_game_player_child_attach_00100ed8(&attachment,child,factory,parent,17,save)&&attachment.attached);gh_cloth_templates tables={0};gh_cloth_runtime_globals globals={0};C(!gh_cloth_templates_from_elf(&tables,elf,bytes)&&!gh_cloth_templates_initialize_00100fb8(&tables,1,0xffff)&&!gh_cloth_runtime_from_elf(&globals,elf,bytes)&&!gh_cloth_runtime_initialize_00107de0(&globals,1,0xffff));
 uint8_t*raw=child->object.view.bytes,*pr=parent->root.bytes;uint16_t parentFlags=0;memcpy(pr+0x3ac,&parentFlags,2);float xyz[3]={8,2,-4};memcpy(parent->root.alias_f0,xyz,12);C(!gh_actor_model_update_hierarchy_00149eb8(parent,GH_STATIC_BINARY32_REFERENCE));uint32_t flags=0x10010;memcpy(pr+0x250,&flags,4);flags=0x3fc00000;memcpy(pr+0x24c,&flags,4);uint8_t before[GH_PLAYER_CHILD_BYTES];memcpy(before,raw,sizeof(before));
 const int modes[4]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};for(unsigned i=0;i<4;i++){memcpy(raw,before,sizeof(before));gh_game_mt random;gh_game_mt_seed_0030f2b0(&random,5489);gh_child_move_bindings b={.cloth={.springs=springs,.attachment=&attachment,.templates=&tables,.globals=&globals,.random=&random,.matrix_policy=GH_STATIC_BINARY32_REFERENCE,.random_policy=GH_FRAME_CHOP_REFERENCE},.work=work,.work5f4=&parent->root};gh_child_move move={0};C(!fesetround(modes[i])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_INVALID));C(gh_game_player_child_move_00100530(&move,&b)==GH_PLAYER_UNSUPPORTED&&move.frontier==0x10430c&&move.stage==4&&!move.complete&&move.simulation.geometry==&tables.geometry&&tables.geometry.count==12&&fegetround()==modes[i]&&fetestexcept(FE_ALL_EXCEPT)==FE_INVALID);C(!memcmp(child->object.view.alias_f0,xyz,12)&&!memcmp(raw+0x100,attachment.parent_part->bytes+0x100,12)&&w(raw+0x24c)==0x3fc00000&&(w(raw+0x250)&0x10010)==0x10010&&!(w(raw+0x20ac)&0x400));gh_game_mt after=random;C(gh_game_player_child_move_00100530(&move,&b)==GH_PLAYER_UNSUPPORTED&&move.frontier==0x10430c&&!memcmp(&random,&after,sizeof(after)));
  C(!fesetround(FE_TONEAREST));
  float windVector[3]={0.125f,0,0.25f};memcpy(globals.wind,windVector,12);float movement[3]={1,0.25f,-0.5f};memcpy(pr+0x650,movement,12);uint8_t beforeForces[44*0xa0];memcpy(beforeForces,raw+0x4d0,sizeof(beforeForces));gh_cloth_forces force={0};C(!fesetround(modes[i])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_INVALID));C(!gh_cloth_integrate_forces_0010430c(&force,&move.simulation)&&force.complete&&force.frontier==0x10497c&&fegetround()==modes[i]&&fetestexcept(FE_ALL_EXCEPT)==FE_INVALID&&!memcmp(&random,&after,sizeof(after)));C(!fesetround(FE_TONEAREST));
  for(unsigned row=0;row<springs->count;row++){unsigned id=springs->indices[0][row];const uint8_t*old=beforeForces+id*0xa0,*now=raw+0x4d0+id*0xa0;float offset,coef;memcpy(&offset,springs->coefficients[1]+row,4);memcpy(&coef,springs->coefficients[2]+row,4);long double angle=(long double)value(raw+0x2098)+offset;while(angle>3.1415926535897932384626433832795L)angle-=6.283185307179586476925286766559L;long double amplitude=sinl(angle)+1;
   for(unsigned axis=0;axis<3;axis++){long double previous=(long double)value(old+0x40+axis*4)+movement[axis];long double velocity=value(old+0x30+axis*4);if(axis==1)velocity-=value(raw+0x208c);long double expected=previous+velocity+(long double)windVector[axis]*amplitude*coef;C(fabsl((long double)value(now+0x40+axis*4)-expected)<0.0001L&&fabsl((long double)value(now+0x50+axis*4)-previous)<0.00001L);}for(unsigned off=0;off<0xa0;off++)if(!(off>=0x34&&off<0x38)&&!(off>=0x40&&off<0x4c)&&!(off>=0x50&&off<0x5c))C(old[off]==now[off]);
  }
  check_small_wave(&move.simulation, modes[i]);
  uint8_t afterForces[44*0xa0];memcpy(afterForces,raw+0x4d0,sizeof(afterForces));C(!gh_cloth_integrate_forces_0010430c(&force,&move.simulation)&&!memcmp(afterForces,raw+0x4d0,sizeof(afterForces)));memcpy(raw+0x4d0,beforeForces,sizeof(beforeForces));gh_cloth_forces badForce={0};uint32_t one=0x3f800000;memcpy(pr+0x674,&one,4);C(gh_cloth_integrate_forces_0010430c(&badForce,&move.simulation)==GH_CLOTH_UNSUPPORTED&&badForce.frontier==0x104718&&!memcmp(beforeForces,raw+0x4d0,sizeof(beforeForces)));one=0;memcpy(pr+0x674,&one,4);memset(globals.wind,0,12);memset(pr+0x650,0,12);
  memcpy(raw,before,sizeof(before));gh_child_move missing={0};b.work5f4=NULL;C(gh_game_player_child_move_00100530(&missing,&b)==GH_PLAYER_UNSUPPORTED&&missing.frontier==0x2db6b0);b.work5f4=&parent->root;parentFlags=0x800;memcpy(pr+0x3ac,&parentFlags,2);gh_child_move wind={0};C(gh_game_player_child_move_00100530(&wind,&b)==GH_PLAYER_UNSUPPORTED&&wind.frontier==0x147988&&!memcmp(&random,&after,sizeof(after)));parentFlags=0;memcpy(pr+0x3ac,&parentFlags,2);C(gh_game_player_child_move_00100530(&wind,&b)==GH_PLAYER_UNSUPPORTED&&wind.frontier==0x10430c);
 }
 memcpy(raw,before,sizeof(before));gh_game_player_child_attachment_clear(&attachment);C(!attachment.attached);C(!fesetround(FE_TONEAREST));
}
int main(int argc,char**argv){C(argc==3);FILE*f=fopen(argv[2],"rb");C(f&&!fseek(f,0,SEEK_END));long el=ftell(f);C(el>0&&!fseek(f,0,SEEK_SET));uint8_t*elf=malloc((size_t)el);C(elf&&fread(elf,1,(size_t)el,f)==(size_t)el);fclose(f);gh_player_shared_vector shared={0};C(gh_player_shared_vector_from_elf(&shared,elf,51)==GH_PLAYER_BOUNDS&&!shared.initialized);C(!gh_player_shared_vector_from_elf(&shared,elf,(size_t)el)&&!w(shared.bytes)&&!w(shared.bytes+4)&&!w(shared.bytes+8)&&w(shared.bytes+12)==0x3f800000);
 gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t n,fn;uint8_t*room=load(d,"st1/r100.dat",&n),*file=load(d,"pl/pl01.dat",&fn);gh_bundle bundle;C(!gh_bundle_open(room,n,&bundle));gh_heap*heap;gh_room_pools pools={0};C(!gh_heap_create(24*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&bundle,heap));size_t baseline=gh_heap_used(heap);gh_stage_slots gs={0},rs={0};gh_game_objects*global,*objects;C(!gh_game_objects_create(&gs,0,&global)&&!gh_game_objects_create(&rs,1,&objects)&&!gh_stage_slots_reset(&gs,NULL)&&!gh_stage_slots_reset(&rs,NULL));int added;C(!gh_game_objects_add_pinned_001fece0(global,0x101,file,fn,&added)&&!gh_game_objects_set_file_extent(global,0x101,file,fn));uint8_t save_bytes[GH_CORE_SAVE_BYTES];gh_core_save_view save={save_bytes,sizeof(save_bytes)};C(!gh_core_save_system_init(save));uint8_t serial=0;gh_boot_work work={.room=0x100};gh_player_child_bindings bindings={0};bindings.pools=&pools;bindings.global_objects=global;bindings.room_objects=objects;bindings.work=&work;bindings.factory_serial=&serial;const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
 size_t parentBytes;uint8_t*parentFile=load(d,"pl/pl00.dat",&parentBytes);C(!gh_game_objects_add_pinned_001fece0(global,0x100,parentFile,parentBytes,&added)&&!gh_game_objects_set_file_extent(global,0x100,parentFile,parentBytes));gh_game_title_player parentObject={0};gh_game_player_factory parentFactory={0};gh_player_factory_bindings parentBindings={&parentObject,&pools,global,objects,&work,d,save,&serial};int parentOriginal;C(!gh_game_player_factory_create_0030fa98(&parentFactory,&parentBindings,&parentOriginal)&&parentOriginal);gh_bundle parentBundle;gh_bundle_entry md,texture;C(!gh_bundle_open(parentFile,parentBytes,&parentBundle)&&!gh_bundle_find(&parentBundle,"MD",0,&md)&&!gh_bundle_find(&parentBundle,"TM3",0,&texture));gh_actor_model parentModel={0};uint32_t parentFog=0;C(!gh_actor_model_bind_packet_inputs(&parentModel,heap,&parentFog)&&!gh_actor_model_initialize_00149738(&parentModel,&parentObject.view,&work,&pools,&md,&texture,0,1,GH_STATIC_BINARY32_REFERENCE,&parentOriginal)&&parentOriginal);size_t parentBaseline=gh_heap_used(heap);uint32_t parentParts=pools.blocks[0].used_count,parentNodes=pools.blocks[1].used_count,parentArena=pools.arenas[2].live_allocations;
 for(unsigned mode=0;mode<4;mode++){gh_game_player_child child={0};int original;C(!gh_game_player_child_create_0030fa98(&child,&bindings,&original)&&original);uint32_t fog=0;C(!gh_actor_model_bind_packet_inputs(&child.model,heap,&fog));C(gh_game_player_child_initialize_00100378(&child,&original)==GH_PLAYER_UNSUPPORTED&&child.frontier==0x100420&&child.model.ready&&child.init_stage==2&&!original);gh_player_child_tail tail={0};gh_game_player_child marker={0},*publication=&marker;
 if(mode==3){float xyz[3]={.125f,-.25f,.5f};memcpy(shared.bytes,xyz,12);uint8_t snapshot[16];memcpy(snapshot,shared.bytes,16);C(gh_player_shared_vector_from_elf(&shared,elf,(size_t)el)==GH_PLAYER_UNSUPPORTED&&!memcmp(shared.bytes,snapshot,16));}
 C(!fesetround(modes[mode])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));uint32_t angle=0x501502f9;memcpy(child.model.parts[1].bytes+0x100,&angle,4);C(gh_game_player_child_finish_00100420(&tail,&child,&shared,&publication,save,elf,(size_t)el,GH_STATIC_BINARY32_REFERENCE,&original)==GH_PLAYER_UNSUPPORTED&&!original&&tail.frontier==0x1f7da0&&publication==&marker&&tail.stage==1&&!tail.springs.initialized);angle=0;memcpy(child.model.parts[1].bytes+0x100,&angle,4);C(!gh_game_player_child_finish_00100420(&tail,&child,&shared,&publication,save,elf,(size_t)el,GH_STATIC_BINARY32_REFERENCE,&original)&&original&&tail.complete&&tail.springs.initialized&&tail.springs.count==40&&publication==&child&&child.init_stage==3&&!child.frontier);C(fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);C(!memcmp(child.model.parts[0].bytes+0xe0,shared.bytes,12)&&!memcmp(child.model.parts[0].alias_d0,shared.bytes,12)&&w(child.model.parts[0].bytes+0xec)==0x3f800000);C((w(child.object.view.bytes+0x254)&0x8000000)&&(w(child.object.view.bytes+0x2f0)&0x81)==0x81&&w(child.object.view.bytes+0x2050)==40);move_checks(&tail.springs,elf,(size_t)el,&parentFactory,&parentModel,save,&work);uint8_t snapshot[44*0xa0];memcpy(snapshot,child.object.view.bytes+0x4d0,sizeof(snapshot));C(!gh_game_player_child_finish_00100420(&tail,&child,&shared,&publication,save,elf,(size_t)el,GH_STATIC_BINARY32_REFERENCE,&original)&&!memcmp(snapshot,child.object.view.bytes+0x4d0,sizeof(snapshot)));publication=NULL;C(!gh_game_player_child_release_prefix(&child)&&pools.blocks[0].used_count==parentParts&&pools.blocks[1].used_count==parentNodes&&pools.arenas[2].live_allocations==parentArena&&gh_heap_used(heap)==parentBaseline);}
 C(!fesetround(FE_TONEAREST));C(!gh_actor_model_release(&parentModel)&&!gh_game_player_factory_release_prefix(&parentFactory)&&gh_heap_used(heap)==baseline);free(parentFile);gh_game_objects_destroy(objects);gh_game_objects_destroy(global);C(!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));free(file);free(room);free(elf);gh_disc_close(d);puts("PASS actualPL00/PL01 attached40spring force10430C->10497C sameparentmovement/gravity/perrecordphasewave/position+history, longdouble+sinlboundedreferenceoracle/4FEnv/atomicradialstrict/idempotent/sourcepadding; nofullclothframeclaim");return 0;}
