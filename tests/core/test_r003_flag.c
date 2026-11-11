#include "game_om041c.h"
#include "game_om_frame.h"
#include "r003_flags.h"
#include "disc.h"

#include <fenv.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if(!(condition)){fprintf(stderr,"line %d: %s\n",__LINE__,#condition);exit(1);} } while(0)
static uint32_t word(const uint8_t *p){uint32_t v;memcpy(&v,p,4);return v;}
static float number(const uint8_t *p){float v;memcpy(&v,p,4);return v;}
static uint8_t *load(gh_disc *disc,const char *path,size_t *size)
{
    size_t index;
    gh_disc_entry entry;
    CHECK(!gh_disc_find(disc,path,&index)&&!gh_disc_entry_at(disc,index,&entry));
    *size=(size_t)entry.size;
    uint8_t *p=malloc(*size);
    CHECK(p&&!gh_disc_read(disc,index,0,p,*size));
    return p;
}

typedef struct {gh_room_module module;int done,status;} module_loader;

static void load_module(void *context,int32_t argument)
{
    (void)argument;
    module_loader *loader=context;
    loader->status=gh_room_module_load_002be708(&loader->module,3);
    loader->done=1;
}

static void check_wind(gh_r003_event *event,gh_om_factory *factory)
{
    gh_game_mt mt,original;
    gh_frame_random random;
    CHECK(!gh_frame_seed_0031ceb8(&random,&mt,GH_FRAME_CHOP_REFERENCE));
    original=mt;
    const uint32_t auxiliary=random.auxiliary_word;
    uint32_t pc=0;
    CHECK(!gh_r003_move_flags_000003d8(event,factory,&mt,&random,&pc)&&!pc);
    CHECK(!memcmp(&mt,&original,sizeof(mt))&&random.auxiliary_word==auxiliary);
}

static void check_wind_forces(gh_r003_event *event,gh_om_factory *factory)
{
    gh_game_mt mt;
    gh_frame_random random;
    CHECK(!gh_frame_seed_0031ceb8(&random,&mt,GH_FRAME_CHOP_REFERENCE));
    uint32_t expected_aux=random.auxiliary_word;
    unsigned consumed=12;
    for(unsigned i=0;i<6;i++){
        uint32_t p=(expected_aux&0xffffu)*5u;
        expected_aux=((p&255u)<<8)|((p>>8)&255u);
        if((expected_aux&255u)<8)consumed++;
    }
    float start[3]={0.125f,0.5f,0.0f};
    memcpy(event->work.pointer,start,sizeof(start));
    uint32_t pc=0;
    CHECK(!gh_r003_move_flags_000003d8(event,factory,&mt,&random,&pc)&&!pc);
    CHECK(random.auxiliary_word==expected_aux&&mt.index==consumed);
    CHECK(!memcmp(start,event->work.pointer,sizeof(start))&&fegetround()==FE_UPWARD);
    for(unsigned actor=0;actor<6;actor++){
        const uint8_t *root=factory->objects[actor].view.bytes;
        for(unsigned i=0;i<15;i++){
            const uint8_t *p=root+0x600+i*0x50;
            CHECK(!number(p+0x10)&&!number(p+0x14)&&word(p+0x1c)==0x3f800000);
            if(p[0x48])CHECK(!number(p+0x18));
            else CHECK(number(p+0x18)<=0);
        }
    }
    gh_om_object *object=&factory->objects[0];
    gh_om041c_state *flag=(gh_om041c_state *)object->initialization;
    uint8_t *p=object->view.bytes+0x600+3*0x50;
    float center[3],direction[3]={0,0,-1};
    memcpy(center,p,12);
    float old=number(p+0x18);
    CHECK(!p[0x48]&&old<0);
    CHECK(!gh_om041c_add_wind_002ae218(flag,center,direction,0.1f,0.25f));
    CHECK(fabsf(number(p+0x18)-(old-0.25f))<0.000001f&&word(p+0x1c)==0x3f800000);
    CHECK(gh_om041c_add_wind_002ae218(flag,center,direction,-1.0f,1.0f)==GH_PLACE_UNSUPPORTED&&
        flag->frontier==0x2ae218);

    /* An auxiliary zero word stays zero, so every original conditional gust
     * consumes its third MT draw. Starting beyond the phase threshold
     * causes exactly one additional amplitude/speed pair on the first flag. */
    CHECK(!gh_frame_seed_0031ceb8(&random,&mt,GH_FRAME_CHOP_REFERENCE));
    random.auxiliary_word=0;
    const float before_reset[3]={4.0f,0.5f,0.01f};
    memcpy(event->work.pointer,before_reset,sizeof(before_reset));
    CHECK(!gh_r003_move_flags_000003d8(event,factory,&mt,&random,&pc)&&!pc);
    float after_reset[3];
    memcpy(after_reset,event->work.pointer,sizeof(after_reset));
    CHECK(mt.index==20&&!random.auxiliary_word);
    CHECK(after_reset[1]>=0.2f&&after_reset[1]<0.5f&&after_reset[2]>=0.0001f&&after_reset[2]<0.05f);
    CHECK(fabsf(after_reset[0]-5*after_reset[2])<0.000001f);
    CHECK(fegetround()==FE_UPWARD&&(fetestexcept(FE_ALL_EXCEPT)&FE_DIVBYZERO));
}

int main(int argc,char **argv)
{
    CHECK(argc==3);
    FILE *file=fopen(argv[2],"rb");
    CHECK(file&&!fseek(file,0,SEEK_END));
    long elf_size=ftell(file);
    CHECK(elf_size>0&&!fseek(file,0,SEEK_SET));
    uint8_t *elf=malloc((size_t)elf_size);
    CHECK(elf&&fread(elf,1,(size_t)elf_size,file)==(size_t)elf_size);
    CHECK(!fclose(file));

    gh_disc *disc;
    CHECK(!gh_disc_open(argv[1],&disc));
    size_t room_size,resource_size;
    uint8_t *room_data=load(disc,"st0/r003.dat",&room_size);
    uint8_t *resource=load(disc,"ol/ol1c.dat",&resource_size);
    gh_bundle room;
    gh_bundle_entry oms;
    CHECK(!gh_bundle_open(room_data,room_size,&room)&&!gh_bundle_find(&room,"OMS",0,&oms));
    gh_om_scene_placements records={0};
    int result;
    CHECK(gh_om_scene_bind_001c3cb8(&records,&oms,&result)==GH_PLACE_UNSUPPORTED);
    gh_game_prepare prepare={0};
    gh_room_state room_state;
    uint8_t start=0;
    CHECK(!gh_game_prepare_002a5d40(&prepare,(gh_core_save_view){0},3,&start));
    CHECK(!gh_room_state_select_002bebc8(&prepare,3,0,&room_state));
    gh_heap *heap;
    gh_room_pools pools={0};
    CHECK(!gh_heap_create(16*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&room,heap));
    const size_t baseline=gh_heap_used(heap);
    gh_stage_slots gs={0},rs={0};
    gh_game_objects *global,*local;
    CHECK(!gh_game_objects_create(&gs,0,&global)&&!gh_game_objects_create(&rs,1,&local));
    CHECK(!gh_stage_slots_reset(&gs,NULL)&&!gh_stage_slots_reset(&rs,NULL));
    CHECK(!gh_game_objects_add_pinned_001fece0(local,0x41c,resource,resource_size,&result));
    CHECK(!gh_game_objects_set_file_extent(local,0x41c,resource,resource_size));
    gh_boot_work work={.room=3};
    uint8_t serial=0;
    uint32_t fog=0;
    gh_om_initializer_bindings init={.work=&work,.packet_heap=heap,.fog_rgb=&fog,
        .elf=elf,.elf_bytes=(size_t)elf_size};
    gh_om_factory_bindings bindings={.pools=&pools,.work=&work,.room_state=&room_state,
        .objects=global,.room_objects=local,.factory_serial=&serial,.initializer=&init};
    gh_om_factory factory={0};
    CHECK(!gh_om_factory_bind(&factory,&bindings,25));

    gh_dvd_jobs dvd;
    gh_dvd_jobs_init(&dvd,disc);
    gh_task_record32 task_records[16]={0};
    gh_task_manager manager={.records=task_records,.capacity=16,.count=16,.current_index=-1};
    gh_task_threads *threads;
    CHECK(!gh_task_threads_create(&manager,&threads));
    module_loader loader={.module={.input={.dvd=&dvd,.heap=heap,.threads=threads}}};
    int32_t slot;
    CHECK(!gh_task_threads_start(threads,-1,load_module,&loader,0,&slot));
    for(unsigned i=0;i<20&&!loader.done;i++)
        CHECK(!gh_tasks_run_matching_002d52a0(&manager).status&&!gh_dvd_pump(&dvd));
    CHECK(loader.done&&!loader.status&&loader.module.move_offset==0x228);
    gh_game_event event;
    CHECK(!gh_game_event_from_elf(&event,elf,(size_t)elf_size));
    gh_event_reserve_work reserve={0};
    gh_r003_event r003={0};
    gh_r003_bindings event_bindings={.module=&loader.module,.heap=heap,.disc=disc,.threads=threads,
        .reserve=&reserve,.soft={.work=&work},.play={.event=&event,.work=&work}};
    CHECK(!gh_r003_event_init_000001a0(&r003,&event_bindings));
    check_wind(&r003,&factory);

    _Alignas(16) uint8_t storage[GH_OM041C_BYTES];
    memset(storage,0xa5,sizeof(storage));
    gh_title_player_view view;
    CHECK(gh_om041c_construct_0017b870(storage,sizeof(storage)-1,&view)==GH_PLACE_BOUNDS&&storage[0]==0xa5);
    CHECK(!gh_om041c_construct_0017b870(storage,sizeof(storage),&view));
    CHECK(view.original_vtable214==0x421bb8&&word(storage+0x214)==0x421bb8);
    CHECK(word(storage+0xd80)==2&&word(storage+0xd84)==4&&word(storage+0xd88)==30);
    CHECK(!word(storage+0xd90)&&!word(storage+0xd94)&&word(storage+0x1d9c)==0x3f800000);
    CHECK(word(storage+0x640)==0xa5a5a5a5&&word(storage+0x60c)==0x3f800000);

    CHECK(!fesetround(FE_UPWARD)&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));
    for(unsigned i=0;i<6;i++){
        gh_om_placement_record record;
        CHECK(!gh_om_scene_record_at(&records,i,&record)&&record.type==0x41c&&record.bytes[0x2d]==0);
        gh_scene_placement_status status=gh_om_factory_create_record(&factory,&record,&result);
        if(status)fprintf(stderr,"flag %u status %u pc %08x detail %u\n",i,status,factory.frontier,factory.detail);
        CHECK(!status&&result&&factory.count==i+1);
        gh_om_object *object=&factory.objects[i];
        gh_om041c_state *flag=(gh_om041c_state *)object->initialization;
        const uint8_t *root=object->view.bytes;
        gh_actor_model *borrowed=NULL;
        CHECK(!gh_om_factory_model_at(&factory,object->handle,&borrowed)&&borrowed==&flag->common.model);
        CHECK(flag->initialized&&flag->common.model.ready&&flag->common.model.part_count==10);
        CHECK(object->allocation.bytes==GH_OM041C_BYTES&&object->registered&&object->stage==7);
        CHECK(!memcmp(root+0x490,record.bytes,12)&&!memcmp(object->view.alias_f0,record.bytes,12));
        CHECK(!memcmp(root+0x100,record.bytes+0x10,12)&&!memcmp(root+0x538,record.bytes+0x30,8));
        for(unsigned row=0;row<5;row++)for(unsigned column=0;column<3;column++){
            const uint8_t *p=root+0x600+(3*row+column)*0x50;
            const int pinned=row==0||(row==1&&column!=0);
            CHECK(p[0x48]==pinned&&word(p+0xc)==0x3f800000);
            CHECK(!word(p+0x10)&&!word(p+0x14)&&!word(p+0x18)&&word(p+0x1c)==0x3f800000);
            CHECK(fabsf(number(p+0x40)*number(p+0x44)-1)<0.000001f);
            if(column==1){
                for(unsigned axis=0;axis<3;axis++)
                    CHECK(fabsf(number(p+4*axis)-0.5f*(number(p-0x50+4*axis)+number(p+0x50+4*axis)))<0.00001f);
            }else{
                const gh_actor_part *part=&flag->common.model.parts[5*(column/2)+row];
                CHECK(!memcmp(p,part->bytes+0xb0,12));
                CHECK(!!(word(part->bytes+0x154)&0x18)==(row<4));
            }
        }
        for(unsigned j=0;j<30;j++){
            const uint8_t *spring=root+0xab0+j*0x18;
            CHECK(!word(spring)&&!word(spring+4)&&word(spring+8)==0x4362b333&&word(spring+0xc)==0x40000000);
            CHECK(number(spring+0x10)>0&&number(spring+0x10)<4.0f);
        }
        CHECK(fegetround()==FE_UPWARD&&(fetestexcept(FE_ALL_EXCEPT)&FE_DIVBYZERO));
    }
    CHECK(serial==6);
    check_wind_forces(&r003,&factory);
    CHECK(!gh_r003_event_release(&r003)&&!gh_room_module_release(&loader.module)&&!gh_task_threads_destroy(threads));
    CHECK(!gh_om_factory_release(&factory)&&!pools.blocks[0].used_count&&!pools.blocks[1].used_count);
    CHECK(!pools.vectors[0].count&&!pools.arenas[2].live_allocations&&gh_heap_used(heap)==baseline);
    gh_game_objects_destroy(local);
    gh_game_objects_destroy(global);
    CHECK(!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));
    free(elf);
    free(resource);
    free(room_data);
    gh_disc_close(disc);
    puts("PASS actual R003 six 041C flag constructors, OL1C model/pose, 15 points, 30 constraints, original MoveFlag RNG/force/phase, and scoped release");
    return 0;
}
