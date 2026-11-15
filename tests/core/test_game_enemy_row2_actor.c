#include "game_enemy_request.h"
#include "game_enemy_row2.h"
#include "game_camera_envelope.h"
#include "game_world_light.h"
#include "game_ray_static.h"
#include "disc.h"
#include <fenv.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr,"line %d: %s\n",__LINE__,#expr); return 1; } } while (0)

static uint8_t *load(gh_disc *disc,const char *path,size_t *size)
{
    size_t index;
    gh_disc_entry file;
    if (gh_disc_find(disc,path,&index) || gh_disc_entry_at(disc,index,&file)) return NULL;
    uint8_t *data=malloc((size_t)file.size);
    if (!data || gh_disc_read(disc,index,0,data,(size_t)file.size)) {free(data);return NULL;}
    *size=(size_t)file.size;
    return data;
}

static uint32_t word(const uint8_t *bytes)
{
    uint32_t value;
    memcpy(&value,bytes,4);
    return value;
}

static uint32_t body_radius_oracle(uint32_t scale_bits,uint32_t radius_bits)
{
    float scale,radius;
    memcpy(&scale,&scale_bits,4);
    memcpy(&radius,&radius_bits,4);
    double exact=(double)scale*(double)radius;
    float narrowed=(float)exact;
    if ((double)narrowed>exact) narrowed=nextafterf(narrowed,0);
    uint32_t bits;
    memcpy(&bits,&narrowed,4);
    return bits;
}

int main(int argc,char **argv)
{
    CHECK(argc==2);
    gh_disc *disc;
    CHECK(!gh_disc_open(argv[1],&disc));
    size_t room_size,model_size,language_size;
    uint8_t *room_data=load(disc,"st1/r100.dat",&room_size);
    uint8_t *model_data=load(disc,"em/em02.dat",&model_size);
    uint8_t *language_data=load(disc,"etc/core_usa.bin",&language_size);
    CHECK(room_data&&model_data&&language_data);
    /* Actual R101 row 0 uses the original dependency-203 appearance-1 pair. */
    size_t r101_bytes;
    uint8_t *r101=load(disc,"st1/r101.dat",&r101_bytes);
    gh_bundle r101_bundle;
    gh_bundle_entry r101_ems;
    CHECK(r101&&!gh_bundle_open(r101,r101_bytes,&r101_bundle)&&!gh_bundle_find(&r101_bundle,"EMS",0,&r101_ems));
    CHECK(word(r101_ems.bytes+4)==35);
    gh_bundle_entry r101_records=r101_ems;
    r101_records.bytes+=12;r101_records.size-=12;
    uint8_t r101_save[0x60c]={0};
    gh_room_state r101_page={.active=r101_save,.size=sizeof(r101_save)};
    uint32_t r101_frontier;
    CHECK(!gh_em_room_table_write_002bede8(&r101_page,35,&r101_records,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&r101_frontier));
    gh_enemy_request r101_req;
    int r101_spawn;int16_t r101_hp;
    CHECK(!gh_enemy_request_from_room_00294ca0(&r101_page,0,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,
        &r101_req,&r101_spawn,&r101_hp,&r101_frontier)&&r101_spawn);
    CHECK(r101_req.entry_number==0&&r101_req.appearance==1&&r101_req.dependency_kind==0x203);
    CHECK(gh_enemy202_is_room_supported_profile(&r101_req,0x101)&&!gh_enemy202_is_room_supported_profile(&r101_req,0x100));
    const gh_object_view r101_model={.bytes=model_data,.size=model_size,.file_extent=model_size,.file_extent_known=1,.kind=0x202};
    gh_enemy_initial_motion_selection r101_motion;
    CHECK(!gh_enemy202_appearance1_select_00203a9c(&r101_req,&r101_model,&r101_motion));
    CHECK(r101_motion.motion_offset==word(model_data+0x3fc)&&r101_motion.sequence_offset==word(model_data+0x400)&&
        r101_motion.substate==1&&r101_motion.phase_detail==2);
    static const unsigned expected_rows[]={0,2,4,7,8,9};
    static const unsigned expected_stamps[]={0,5,9,12,13,15};
    unsigned eligible=0;
    for(unsigned row=0;row<35;++row){
        CHECK(!gh_enemy_request_from_room_00294ca0(&r101_page,row,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,
            &r101_req,&r101_spawn,&r101_hp,&r101_frontier));
        if(!r101_spawn)continue;
        CHECK(eligible<sizeof(expected_rows)/sizeof(*expected_rows)&&row==expected_rows[eligible]&&
            r101_req.entry_number==expected_stamps[eligible++]&&gh_enemy202_is_room_supported_profile(&r101_req,0x101));
        if(r101_req.appearance==4){
            CHECK(!gh_enemy202_appearance4_select_00203a18(&r101_req,&r101_model,&r101_motion));
            CHECK(r101_motion.motion_offset==word(model_data+0x40c)&&r101_motion.sequence_offset==word(model_data+0x410)&&
                r101_motion.substate==1&&r101_motion.phase_detail==1);
        }
    }
    CHECK(eligible==sizeof(expected_rows)/sizeof(*expected_rows));
    free(r101);

    size_t r102_bytes;
    uint8_t *r102=load(disc,"st1/r102.dat",&r102_bytes);
    gh_bundle r102_bundle;
    gh_bundle_entry r102_ems;
    CHECK(r102&&!gh_bundle_open(r102,r102_bytes,&r102_bundle)&&
        !gh_bundle_find(&r102_bundle,"EMS",0,&r102_ems)&&word(r102_ems.bytes+4)==34);
    r102_ems.bytes+=12;
    r102_ems.size-=12;
    uint8_t r102_save[0x60c]={0};
    gh_room_state r102_page={.active=r102_save,.size=sizeof(r102_save)};
    const gh_em_numeric_policy numeric=GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE;
    CHECK(!gh_em_room_table_write_002bede8(&r102_page,34,&r102_ems,numeric,&r101_frontier));
    struct r102_motion_case {
        unsigned row, stamp, appearance, dependency, flags, dependency_flags;
        size_t motion, sequence;
        uint8_t substate, detail;
    };
    static const struct r102_motion_case r102_cases[]={
        {2,2,5,0x20a,0x10000000u,0,0x393d40,0x395140,1,3},
        {3,3,1,0x200,0,0,0x20ce40,0x20e000,1,2},
        {4,4,0,0x200,0,1,0x1792c0,0x17bd00,1,0},
        {13,15,2,0x200,0,1,0x212e00,0x2154c0,0x71,0},
        {16,18,0,0x20f,0x10000000u,0,0x44b080,0x44b080,1,0},
    };
    for(size_t i=0;i<sizeof(r102_cases)/sizeof(r102_cases[0]);++i){
        const struct r102_motion_case *expected=&r102_cases[i];
        gh_enemy_request req;
        int spawn;
        int16_t hp;
        CHECK(!gh_enemy_request_from_room_00294ca0(&r102_page,expected->row,numeric,
            &req,&spawn,&hp,&r101_frontier)&&spawn);
        CHECK(req.entry_number==expected->stamp&&req.appearance==expected->appearance&&
            req.dependency_kind==expected->dependency&&req.flags==expected->flags&&
            req.dependency_flags==expected->dependency_flags);
        CHECK(gh_enemy202_is_room_supported_profile(&req,0x102)&&
            !gh_enemy202_is_room_supported_profile(&req,0x105));
        gh_enemy_initial_motion_selection motion;
        gh_actor_model_status status;
        switch(req.appearance){
        case 0: status=gh_enemy202_appearance0_select_0020393c(&req,&r101_model,&motion); break;
        case 1: status=gh_enemy202_appearance1_select_00203a9c(&req,&r101_model,&motion); break;
        case 2: status=gh_enemy202_appearance2_select_00203c38(&req,&r101_model,&motion); break;
        default: status=gh_enemy202_appearance5_select_00203b3c(&req,&r101_model,&motion); break;
        }
        CHECK(!status&&motion.motion_offset==expected->motion&&motion.sequence_offset==expected->sequence&&
            !motion.main_state&&motion.substate==expected->substate&&!motion.phase&&motion.phase_detail==expected->detail);
    }
    free(r102);
    gh_bundle room_bundle,language;
    gh_bundle_entry ems,idd;
    CHECK(!gh_bundle_open(room_data,room_size,&room_bundle)&&!gh_bundle_find(&room_bundle,"EMS",0,&ems));
    CHECK(!gh_bundle_open(language_data,language_size,&language)&&!gh_id_find_language_data(&language,&idd));
    size_t elf_size=(size_t)gh_disc_boot_elf_size(disc);
    uint8_t *elf=malloc(elf_size);
    CHECK(elf&&!gh_disc_read_boot_elf(disc,0,elf,elf_size));

    gh_heap *heap;
    gh_room_pools pools={0};
    gh_game_init_state *init=calloc(1,sizeof(*init));
    gh_id_state *id=calloc(1,sizeof(*id));
    gh_enemy_actor *first=calloc(1,sizeof(*first)),*second=calloc(1,sizeof(*second));
    CHECK(init&&id&&first&&second&&!gh_heap_create(64*1024*1024,&heap));
    CHECK(!gh_game_room_pools_initialize_002a5d80(&pools,&room_bundle,heap));
    gh_game_init_state_initialize(init);
    uint32_t frontier;
    CHECK(!gh_hitarea_objects_construct_001fde00(init,&frontier));
    gh_boot_work work={.room=0x100,.phase569=2};
    /* Original Work bit disables enemy SE in this CPU-only fixture. */
    memcpy((uint8_t *)&work+0x5e4,&(uint32_t){0x20000000},4);
    uint8_t save_bytes[GH_CORE_SAVE_BYTES],game_state=0;
    gh_core_save_view save={save_bytes,sizeof(save_bytes)};
    float rate=1;
    CHECK(!gh_core_save_system_init(save)&&!gh_game_init_002a5b18(init,&work,&game_state,&rate,save));
    gh_id_runtime *runtime;
    gh_game_cockpit *cockpit;
    CHECK(!gh_id_system_init_002acc70(id,&idd)&&!gh_id_runtime_create(id,1,&runtime));
    CHECK(!gh_id_runtime_set_path_policy(runtime,GH_ID_PATH_REFERENCE_NEAREST));
    CHECK(!gh_game_cockpit_create(runtime,id,&init->random,&cockpit)&&!gh_game_cockpit_initialize_00140c88(cockpit,save));

    gh_game_math_globals math={0};
    gh_motion_ik_globals ik={0};
    gh_camera_envelope camera={0};
    gh_enemy_node_names names={0};
    CHECK(!gh_game_math_globals_from_elf(&math,elf,elf_size)&&!gh_game_math_static_initialize_003079c8(&math,&work,1,0xffff));
    CHECK(!gh_motion_ik_globals_from_elf(&ik,elf,elf_size)&&!gh_game_camera_envelope_from_elf(&camera,elf,elf_size,&frontier));
    CHECK(!gh_enemy_node_names_from_elf(&names,elf,elf_size));
    gh_game_world_light light={0};
    gh_game_scene_tables actors={0};
    gh_collision_pairs pairs={0};
    gh_game_collision_solids solids={0};
    CHECK(!gh_game_world_light_construct_002d6d80(&light)&&!gh_game_world_light_initialize_with_heap_002d6f40(&light,&room_bundle,heap));
    CHECK(!gh_scene_actors_0012e670(&actors,&room_bundle));
    CHECK(!gh_collision_pairs_construct_002b8ae0(&pairs,heap)&&!gh_collision_pairs_initialize_002b8b50(&pairs));
    gh_game_collision_solids_reset_00134690(&solids);

    gh_stage_slots global_slots={0},room_slots={0};
    gh_game_objects *global,*room_objects;
    int pinned;
    CHECK(!gh_game_objects_create(&global_slots,0,&global)&&!gh_game_objects_create(&room_slots,1,&room_objects));
    CHECK(!gh_stage_slots_reset(&global_slots,heap)&&!gh_stage_slots_reset(&room_slots,heap));
    CHECK(!gh_game_objects_add_pinned_001fece0(room_objects,0x202,model_data,model_size,&pinned)&&pinned);
    CHECK(!gh_game_objects_set_file_extent(room_objects,0x202,model_data,model_size));
    gh_game_prepare prepare={0};
    gh_room_state room;
    gh_em_scene_placements em={0};
    int loaded;
    CHECK(!gh_game_prepare_002a5d40(&prepare,save,0x100,&game_state)&&!gh_room_state_select_002bebc8(&prepare,0x100,0,&room));
    CHECK(!gh_em_scene_load_native_00294b98(&em,&room,&ems,&work,0,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&loaded)&&loaded);
    gh_enemy_request row0,row2;
    int spawn;
    int16_t saved;
    CHECK(!gh_enemy_request_from_room_00294ca0(&room,0,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&row0,&spawn,&saved,&frontier)&&spawn);
    CHECK(!gh_enemy_request_from_room_00294ca0(&room,1,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&row2,&spawn,&saved,&frontier)&&!spawn);
    CHECK(!gh_enemy_request_from_room_00294ca0(&room,2,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&row2,&spawn,&saved,&frontier)&&spawn);
    CHECK(gh_enemy202_is_row2_profile(&row2)&&row2.entry_number==3);

    uint8_t scenario=0,serial=255;
    gh_enemy_actor_bindings bindings={.entry={.init=init,.pools=&pools,.global_objects=global,
        .room_objects=room_objects,.work=&work,.room_heap=heap,.packet_heap=heap,.fog_rgb=light.scalar+4,
        .save=save,.scenario14=&scenario,.factory_serial=&serial,.policy=GH_STATIC_BINARY32_REFERENCE},
        .cockpit=cockpit,.ik=&ik,.actors=&actors,.atan_coeff=camera.atan_coeff,.canonical_rate=&rate,
        .solids=&solids,.default_heap=heap,.node_names=&names,.shapes={&pairs,&math,GH_STATIC_BINARY32_REFERENCE},
        .numeric_policy=GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE};
    size_t baseline=gh_heap_used(heap);
    gh_game_title_player *first_object,*second_object;
    CHECK(!fesetround(FE_TOWARDZERO));
    CHECK(!gh_enemy_actor_enter_002905d0(first,&bindings,&row0,0,0,&first_object)&&first_object==&first->entry.object);
    gh_actor_motion_status status=gh_enemy_actor_enter_002905d0(second,&bindings,&row2,0,0,&second_object);
    if(status) fprintf(stderr,"row2 status %d frontier %08x stage %u\n",status,second->frontier,second->stage);
    CHECK(!status&&second_object==&second->entry.object&&second->final.complete);
    const uint8_t *bytes=second_object->view.bytes;
    CHECK(second->entry.resource.bytes==first->entry.resource.bytes&&first->entry.resource.bytes==model_data);
    CHECK(second->entry.initialize.model.model.part_count==30&&second->entry.initialize.model.model.script.mesh_count==9);
    CHECK(second->entry.initialize.model.occurrence==0&&second->hp.hp==74&&bytes[0x2f5]==0x71);
    CHECK(word(bytes+0x17d0)==1&&word(bytes+0x118)==0x3f88f5c3&&second->solids.sphere->radius_bits==0x3f000000);
    CHECK(bytes[0x17b8]==1&&bytes[0x17b9]==1&&bytes[0x17ba]==1&&bytes[0x17bb]==1&&bytes[0x17bc]==1&&bytes[0x17bd]==1);
    const uint32_t expected_body[]={body_radius_oracle(0x3f88f5c3,0x3ecccccd),body_radius_oracle(0x3f88f5c3,0x3e99999a)};
    unsigned first_radius=0;
    for(unsigned i=0;i<4;i++) {
        const uint8_t *node;
        gh_collision_pair_shape shape;
        CHECK(!gh_hitarea_node_at(&second->hitareas.units[0],i,&node,&shape));
        uint32_t actual_radius=word(shape.record+0x1f0);
        CHECK(actual_radius==expected_body[0]||actual_radius==expected_body[1]);
        first_radius+=actual_radius==expected_body[0];
    }
    CHECK(first_radius==1&&fegetround()==FE_TOWARDZERO);
    CHECK(init->enemy_actors[0]==first_object&&init->enemy_actors[1]==second_object&&init->enemy_next[0]==1&&init->enemy_prev[1]==0);
    CHECK(word(init->enemies+0x508)==2&&word(init->enemies+0x50c)==1&&serial==1&&solids.unit_count==2);
    void *registered=NULL;
    CHECK(!gh_room_object_lookup(&pools,second->entry.handle,GH_ROOM_OBJECT_ENEMY,&second->entry,&registered)&&registered==second_object);
    gh_game_mt random=init->random;
    CHECK(!gh_enemy_actor_enter_002905d0(second,&bindings,&row2,0,0,&second_object)&&second_object==&second->entry.object);
    CHECK(!memcmp(&random,&init->random,sizeof(random))&&serial==1);
    gh_enemy_actor *extra=calloc(2,sizeof(*extra));
    CHECK(extra);
    const unsigned occurrences[]={4,1};
    const unsigned scales[]={0x3f87ae14,0x3f8ccccd};
    for(unsigned i=0;i<2;i++) {
        gh_enemy_request request;
        gh_game_title_player *object;
        CHECK(!gh_enemy_request_from_room_00294ca0(&room,i+3,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&request,&spawn,&saved,&frontier)&&spawn);
        CHECK(request.entry_number==i+4&&request.dependency_kind==i+0x202&&gh_enemy202_is_r100_appearance2_profile(&request));
        status=gh_enemy_actor_enter_002905d0(&extra[i],&bindings,&request,0,0,&object);
        if(status)fprintf(stderr,"row%u status %d frontier %08x stage %u\n",i+3,status,extra[i].frontier,extra[i].stage);
        CHECK(!status&&object==&extra[i].entry.object&&extra[i].final.complete);
        const uint8_t *p=object->view.bytes;
        CHECK(extra[i].entry.initialize.model.occurrence==occurrences[i]&&extra[i].hp.hp==74&&p[0x2f5]==0x71);
        CHECK(word(p+0x17d0)==i+2&&word(p+0x118)==scales[i]&&p[0x17b9]==0&&p[0x17ba]==1&&p[0x17bc]==1);
        CHECK(init->enemy_actors[i+2]==object&&init->enemy_prev[i+2]==i+1);
    }
    CHECK(word(init->enemies+0x508)==4&&solids.unit_count==4&&serial==3);
    CHECK(!gh_enemy_actor_shutdown_native(&extra[0])&&!gh_enemy_actor_shutdown_native(&extra[1]));
    CHECK(init->enemy_actors[1]==second_object&&init->enemy_next[1]==255&&solids.unit_count==2);
    free(extra);
    CHECK(!gh_enemy_actor_shutdown_native(first));
    CHECK(!init->enemy_actors[0]&&init->enemy_actors[1]==second_object&&init->enemy_top==init->enemy_last&&init->enemy_prev[1]==255);
    CHECK(word(init->enemies+0x508)==1&&word(init->enemies+0x50c)==1&&solids.unit_count==1);
    CHECK(!gh_enemy_actor_shutdown_native(second));
    CHECK(!init->enemy_top&&!init->enemy_last&&gh_collision_solid_units_empty(&solids)&&!gh_collision_pair_shapes_have_live(&pairs));
    CHECK(gh_heap_used(heap)==baseline&&!fesetround(FE_TONEAREST));
    gh_scene_ray_triangle_state_release(&actors);
    CHECK(!gh_game_object_pool_release_001fdfa8(init,&frontier)&&!gh_collision_pairs_release(&pairs));
    CHECK(!gh_game_cockpit_destroy(cockpit));
    gh_id_runtime_destroy(runtime);
    gh_game_objects_destroy(room_objects);
    gh_game_objects_destroy(global);
    CHECK(!gh_game_world_light_release_presets(&light)&&!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));
    gh_disc_close(disc);
    free(second);free(first);free(init);free(id);free(elf);free(language_data);free(model_data);free(room_data);
    puts("PASS ordered real EMS rows 0, 2, 3 and 4 concurrently, original returns, scaled body and non-LIFO scoped release");
    return 0;
}
