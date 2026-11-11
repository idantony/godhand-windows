#include "../../src/core/game_actor_render.h"
#include "game_enemy_2a2_final.h"
#include "game_camera_envelope.h"
#include "game_ray_static.h"
#include "game_enemy_request.h"
#include "game_actor_motion_ik.h"
#include "disc.h"
#include <fenv.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr,"line %d: %s\n",__LINE__,#expr); return 1; } } while (0)

static uint8_t *load(gh_disc *disc,const char *path,size_t *extent)
{
    size_t index;
    gh_disc_entry file;
    if (gh_disc_find(disc,path,&index) || gh_disc_entry_at(disc,index,&file)) return NULL;
    uint8_t *data=malloc((size_t)file.size);
    if (!data || gh_disc_read(disc,index,0,data,(size_t)file.size)) {
        free(data);
        return NULL;
    }
    *extent=(size_t)file.size;
    return data;
}

static uint32_t word(const uint8_t *bytes)
{
    uint32_t value;
    memcpy(&value,bytes,sizeof(value));
    return value;
}

static unsigned half(const uint8_t *bytes)
{
    return bytes[0] | ((unsigned)bytes[1]<<8);
}

/* Each double product is exact for the binary32 significands. Quantize
 * each positive operation independently to a 24-bit significand. */
static double chop32(double value)
{
    if (!value) return 0;
    int exponent;
    double fraction=frexp(value,&exponent);
    return ldexp(floor(ldexp(fraction,24)),exponent-24);
}

static uint32_t timer_oracle(uint32_t word)
{
    fenv_t env;
    if (feholdexcept(&env)) abort();
    double unit=ldexp(chop32((double)(word>>1)),-31);
    float value=(float)chop32(chop32(unit*90.0)+90.0);
    uint32_t bits;
    memcpy(&bits,&value,4);
    if (fesetenv(&env)) abort();
    return bits;
}

typedef struct {unsigned attempts,count;int fail_next;uint32_t depth;} marker_probe;
static gh_effect_status emit_enemy_marker(void *context,const gh_effect_trans_packet *packet)
{
 marker_probe *p=context;
 p->attempts++;
 if(packet->original_callee!=0x15bcf0||packet->order.bank!=6||packet->order.depth!=p->depth||
    packet->kind!=GH_EFFECT_PASS_SPRITE||packet->test!=0x5000d||!packet->material||
    packet->material->psm!=19||packet->prim!=0x54||packet->tex1!=0x60||
    !packet->inherits_scene_target||packet->zbuf||packet->frame_mask||
    packet->quad_count!=1||packet->quads[0].vertex_count!=4||packet->quads[0].vertices[0].q!=1)
  return GH_EFFECT_ARGUMENT;
 if(p->fail_next){p->fail_next=0;return GH_EFFECT_UNSUPPORTED;}
 p->count++;
 return GH_EFFECT_OK;
}
static int check_enemy_marker(gh_enemy2a2_virtual_init *full,gh_boot_work *work,
    gh_core_save_view save,gh_game_cockpit *cockpit,gh_game_mt *random,
    gh_id_runtime *runtime,const gh_id_state *manager)
{
    gh_actor_model *model=&full->prefix.model.model;
    uint8_t *actor=model->root.bytes;
    gh_game_pl_camera camera={.constructed=1};
    memcpy(camera.fields+0x200,&(float){1},4);
    gh_title_player_view player={0};
    uint8_t player_bytes[0xbc]={0};
    player.bytes=player_bytes;
    player.size=sizeof(player_bytes);
    gh_actor_render_call call={.model=model,.bindings={
        .work=work,.camera=&camera,.marker_player=&player,.marker_save=save,
        .marker_cockpit=cockpit,.marker_random=random,.marker_id=&full->prefix.base.cockpit_base
    }};
    uint32_t frontier=0;
    work->flags|=8;
    CHECK(!gh_enemy_marker_after_shadow_0028edd0(&call,&frontier)&&!frontier&&
          !(half(actor+0x62c)&1));
    call.marker_stage=0;
    work->flags&=~8u;
    save.bytes[0xbba]|=1;
    CHECK(!gh_enemy_marker_after_shadow_0028edd0(&call,&frontier)&&!frontier&&
          !(half(actor+0x62c)&1));
    call.marker_stage=0;
    save.bytes[0xbba]&=~1u;
    float enemy[3];
    memcpy(enemy,model->root.alias_f0,sizeof(enemy));
    const float far[3]={enemy[0]-20,enemy[1],enemy[2]};
    memcpy(player_bytes+0xb0,far,12);
    CHECK(!gh_enemy_marker_after_shadow_0028edd0(&call,&frontier)&&!frontier);
    CHECK((half(actor+0x62c)&1)&&full->prefix.base.cockpit_base->playing&&
          word(actor+0x650)==word(actor+0xb0)&&word(actor+0x658)==word(actor+0xb8));
    const gh_actor_part *part;
    CHECK(!gh_actor_model_part_at(model,4,&part)&&word(actor+0x654)==word(part->bytes+0xb4));
    call.marker_stage=0;
    const float near[3]={enemy[0]-2,enemy[1],enemy[2]};
    memcpy(player_bytes+0xb0,near,12);
    memcpy(actor+0x62c,&(uint16_t){7},2);
    memcpy(actor+0x650,&(float){enemy[0]-1},4);
    CHECK(gh_enemy_marker_after_shadow_0028edd0(&call,&frontier)==GH_ACTOR_MODEL_UNSUPPORTED&&
          frontier==0x28f108&&call.marker_stage==2&&word(actor+0x644)==0x3f4ccccd);
    const uint32_t anchor=word(actor+0x650);
    CHECK(gh_enemy_marker_after_shadow_0028edd0(&call,&frontier)==GH_ACTOR_MODEL_UNSUPPORTED&&
          frontier==0x28f108&&word(actor+0x644)==0x3f4ccccd&&word(actor+0x650)==anchor);
    call.marker_stage=0;
    actor[0x614]=1;
    CHECK(gh_enemy_marker_after_shadow_0028edd0(&call,&frontier)==GH_ACTOR_MODEL_UNSUPPORTED&&
          frontier==0x28f5b0);
    const float screen[16]={16,0,0,0,0,16,0,0,0,0,0,0,2048,2048,1024,1};
    const float view[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    memcpy(camera.fields,screen,64);
    memcpy(camera.fields+0x140,view,64);
    gh_player_shared_vector pivot={.initialized=1};
    gh_stage_textures *textures;
    CHECK(!gh_stage_textures_create(&textures));
    marker_probe probe={.fail_next=1,.depth=27};
    call.bindings.marker_runtime=runtime;
    call.bindings.marker_manager=manager;
    call.bindings.marker_emit=emit_enemy_marker;
    call.bindings.marker_context=&probe;
    call.bindings.pivot_owner=&pivot;
    call.bindings.textures=textures;
    memcpy(actor+0x2ac,&(uint16_t){27},2);
    actor[0x614]=0;
    call.marker_stage=0;
    gh_actor_model_status first=gh_enemy_marker_after_shadow_0028edd0(&call,&frontier);
    if(first!=GH_ACTOR_MODEL_UNSUPPORTED||frontier!=0x31a650)
        fprintf(stderr,"marker status=%u pc=%x stage=%u draw=%u attempts=%u accepted=%u\n",
            first,frontier,call.marker_stage,call.marker_cursor,probe.attempts,probe.count);
    CHECK(first==GH_ACTOR_MODEL_UNSUPPORTED&&frontier==0x31a650&&
          probe.attempts==1&&!probe.count&&call.marker_stage==4);
    const uint16_t id_frame=full->prefix.base.cockpit_base->frame;
    CHECK(!gh_enemy_marker_after_shadow_0028edd0(&call,&frontier)&&!frontier&&
          probe.count==4&&probe.attempts==5&&call.marker_stage==3);
    CHECK(full->prefix.base.cockpit_base->frame==id_frame);
    probe=(marker_probe){.depth=27};
    gh_id_base *boss_id;
    CHECK(!gh_game_cockpit_base(cockpit,0xcc0,&boss_id)&&boss_id->id==6);
    call.marker_stage=call.marker_cursor=0;
    actor[0x614]=1;
    memcpy(actor+0x564,&(uint32_t){0x220},4);
    memcpy(actor+0x62c,&(uint16_t){0},2);
    uint16_t maximum=half(actor+0x548);
    uint16_t hit_points=maximum/2;
    memcpy(actor+0x54a,&hit_points,2);
    CHECK(!gh_enemy_marker_after_shadow_0028edd0(&call,&frontier)&&!frontier&&
          probe.count==5&&probe.attempts==5&&call.marker_stage==3&&
          full->prefix.base.cockpit_base==boss_id);
    CHECK(word(actor+0x63c)==0x3f800000&&word(actor+0x634)==word((const uint8_t *)&(float){(float)hit_points}));
    puts("PASS actual cockpit ID6 alternative 28F5B0 marker to five native world groups");
    gh_stage_textures_destroy(textures);
    puts("PASS actual EMA2 positive health marker to four native world groups and emit retry");
    return 0;
}

int main(int argc,char **argv)
{
    CHECK(argc==2);
    gh_disc *disc;
    CHECK(!gh_disc_open(argv[1],&disc));
    size_t room_bytes,asset_bytes,language_bytes;
    uint8_t *room_data=load(disc,"st1/r100.dat",&room_bytes);
    uint8_t *asset=load(disc,"em/ema2.dat",&asset_bytes);
    uint8_t *language=load(disc,"etc/core_usa.bin",&language_bytes);
    CHECK(room_data&&asset&&language);
    gh_bundle room_bundle,language_bundle;
    gh_bundle_entry ems,idd;
    CHECK(!gh_bundle_open(room_data,room_bytes,&room_bundle)&&!gh_bundle_find(&room_bundle,"EMS",0,&ems));
    CHECK(!gh_bundle_open(language,language_bytes,&language_bundle)&&!gh_id_find_language_data(&language_bundle,&idd));
    gh_heap *heap;
    gh_room_pools pools={0};
    CHECK(!gh_heap_create(32*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&room_bundle,heap));
    gh_boot_work work={.room=0x100,.phase569=2};
    gh_game_init_state *init=calloc(1,sizeof(*init));
    gh_id_state *id=calloc(1,sizeof(*id));
    CHECK(init&&id);
    gh_game_init_state_initialize(init);
    uint32_t frontier;
    CHECK(!gh_hitarea_objects_construct_001fde00(init,&frontier));
    uint8_t save_bytes[GH_CORE_SAVE_BYTES],game_state=0;
    gh_core_save_view save={save_bytes,sizeof(save_bytes)};
    float rate=1.0f;
    CHECK(!gh_core_save_system_init(save)&&!gh_game_init_002a5b18(init,&work,&game_state,&rate,save));
    gh_id_runtime *runtime;
    gh_game_cockpit *cockpit;
    CHECK(!gh_id_system_init_002acc70(id,&idd)&&!gh_id_runtime_create(id,1,&runtime));
    CHECK(!gh_id_runtime_set_path_policy(runtime,GH_ID_PATH_REFERENCE_NEAREST));
    CHECK(!gh_game_cockpit_create(runtime,id,&init->random,&cockpit)&&!gh_game_cockpit_initialize_00140c88(cockpit,save));
    gh_id_base *canonical_base;
    CHECK(!gh_game_cockpit_base(cockpit,0xe50,&canonical_base));

    gh_game_prepare prepare={0};
    gh_room_state room;
    CHECK(!gh_game_prepare_002a5d40(&prepare,save,0x100,&game_state)&&!gh_room_state_select_002bebc8(&prepare,0x100,0,&room));
    gh_em_scene_placements em={0};
    int loaded;
    CHECK(!gh_em_scene_load_native_00294b98(&em,&room,&ems,&work,0,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&loaded)&&loaded);
    gh_enemy_request requests[2];
    int spawn;
    int16_t saved;
    CHECK(!gh_enemy_request_from_room_00294ca0(&room,5,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&requests[0],&spawn,&saved,&frontier)&&spawn);
    CHECK(!gh_enemy_request_from_room_00294ca0(&room,10,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&requests[1],&spawn,&saved,&frontier)&&!spawn);
    /* Unit fixture only. Decode the sign-blocked entry14 data from a separate
     * page while keeping the actual RoomSave gate and source unchanged. */
    uint8_t *test_page=malloc(room.size);
    CHECK(test_page);
    memcpy(test_page,room.active,room.size);
    const size_t row14_flags=12+10*24+8;
    uint32_t flags=word(test_page+row14_flags),unmasked=flags&0x7fffffffu;
    CHECK(flags==0x80000000u);
    memcpy(test_page+row14_flags,&unmasked,4);
    gh_room_state isolated=room;
    isolated.active=test_page;
    CHECK(!gh_enemy_request_from_room_00294ca0(&isolated,10,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&requests[1],&spawn,&saved,&frontier)&&spawn);
    requests[1].flags=flags;
    free(test_page);
    CHECK(word(room.active+row14_flags)==flags);
    CHECK(requests[0].entry_number==6&&requests[0].dependency_kind==0x2a2);
    CHECK(requests[1].entry_number==14&&requests[1].dependency_kind==0x2a3);

    uint32_t fog=0;
    size_t elf_bytes=(size_t)gh_disc_boot_elf_size(disc);
    uint8_t *elf=malloc(elf_bytes);
    gh_motion_ik_globals ik={0};
    gh_enemy2a2_node_names names={0};
    CHECK(elf&&!gh_disc_read_boot_elf(disc,0,elf,elf_bytes)&&!gh_motion_ik_globals_from_elf(&ik,elf,elf_bytes));
    gh_game_math_globals math={0};
    gh_camera_envelope camera={0};
    CHECK(!gh_game_math_globals_from_elf(&math,elf,elf_bytes)&&!gh_game_math_static_initialize_003079c8(&math,&work,1,0xffff));
    CHECK(!gh_game_camera_envelope_from_elf(&camera,elf,elf_bytes,&frontier));
    CHECK(!gh_enemy2a2_node_names_from_elf(&names,elf,elf_bytes));
    free(elf);
    gh_game_scene_tables actors={0};
    gh_collision_pairs pairs={0};
    gh_game_collision_solids solids={0};
    CHECK(!gh_scene_actors_0012e670(&actors,&room_bundle));
    CHECK(!gh_collision_pairs_construct_002b8ae0(&pairs,heap)&&!gh_collision_pairs_initialize_002b8b50(&pairs));
    gh_game_collision_solids_reset_00134690(&solids);
    gh_game_title_player foreign={0};
    gh_collision_solid_unit *foreign_unit;
    int foreign_nonnull;
    CHECK(!gh_enemy2a2_allocate_00313820(&foreign,heap,&work,&foreign_nonnull)&&foreign_nonnull);
    CHECK(!gh_collision_solid_create_unit_001346f8(&solids,heap,&foreign.view,5,0x3e4ccccd,&foreign_unit,&frontier));
    gh_enemy2a2_pose_bindings pose_bindings={&rate,&actors,camera.atan_coeff,init,
        {&pairs,&math,GH_STATIC_BINARY32_REFERENCE},&solids,heap};
    size_t baseline=gh_heap_used(heap);
    gh_enemy2a2_init_bindings bindings={&work,&pools,heap,&fog,cockpit,&init->random,GH_STATIC_BINARY32_REFERENCE,&ik};
    gh_object_view resource={.kind=0x2a2,.bytes=asset,.size=asset_bytes,.file_extent=asset_bytes,.file_extent_known=1};
    const int modes[]={FE_TONEAREST,FE_TOWARDZERO,FE_DOWNWARD,FE_UPWARD};
    for (unsigned mode=0;mode<6;mode++) {
        CHECK(!fesetround(modes[mode%4]));
        for (unsigned variant=0;variant<(mode>=4?1u:2u);variant++) {
            gh_enemy2a2_virtual_init full={0};
            gh_enemy2a2_init *call=&full.prefix;
            gh_game_title_player actor={0};
            int original=9;
            CHECK(!gh_enemy2a2_allocate_00313820(&actor,heap,&work,&original)&&original);
            uint8_t *bytes=actor.view.bytes;
            memcpy(bytes+0x2fe,&(uint16_t){0x2a2},2); /* Original factory kind stamp. */
            ((uint8_t *)&work)[0x5b1]=(uint8_t)(2+6*variant);
            memcpy(bytes+0x5a8,&(uint32_t){0x3f000000},4); /* .5 is not the direct f12. */
            if (mode==5) {
                CHECK(!gh_enemy2a2_initialize_00284af8(&full,&actor.view,&requests[variant],0,
                    &resource,&bindings,&pose_bindings,&names,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&original));
                CHECK(original==1&&full.final.complete&&!full.frontier&&word(bytes+0x15bc)&&word(bytes+0x560));
                CHECK(!fesetround(FE_TONEAREST));
                CHECK(!check_enemy_marker(&full,&work,save,cockpit,&init->random,runtime,id));
                CHECK(!gh_enemy2a2_release_native(&full)&&!gh_game_title_player_release(&actor));
                CHECK(gh_heap_used(heap)==baseline&&!gh_collision_pair_shapes_have_live(&pairs)&&solids.unit_count==1);
                continue;
            }
            CHECK(gh_enemy2a2_initialize_prefix_00284af8(call,&actor.view,&requests[variant],0,&resource,&bindings,&original)==GH_ACTOR_MODEL_UNSUPPORTED);
            CHECK(!original&&call->frontier==0x157c18&&call->model.model.ready&&call->model.occurrence==variant);
            CHECK(call->base.cockpit_base==canonical_base&&canonical_base->playing);
            CHECK(call->motion.complete&&call->motion.sequence_valid&&call->motion.b.model==&call->model.model);
            CHECK(call->motion.motion.offset==call->motion_offset&&call->motion.sequence_offset==call->sequence_offset);
            CHECK(!word(bytes+0x430)&&!word(bytes+0x438)&&!word(bytes+0x43c));
            CHECK(word(bytes+0x440)==0x3f800000&&word(bytes+0x444)==0x3f800000&&word(bytes+0x5a8)==0x3f000000);
            CHECK(!memcmp(bytes+0x1540,requests[variant].position_xyz,12)&&word(bytes+0x154c)==0x3f800000);
            CHECK(!memcmp(actor.view.alias_f0,requests[variant].position_xyz,12));
            CHECK(word(bytes+0x1550)==requests[variant].yaw_bits&&word(bytes+0x1554)==requests[variant].flags&&bytes[0x1561]==requests[variant].entry_number);
            CHECK(half(bytes+0x548)==600+600*variant&&half(bytes+0x54a)==600+600*variant);
            CHECK(!bytes[0x531]&&!word(bytes+0x15ec)&&word(bytes+0x2f4)==0x200);
            CHECK(call->motion_offset==word(asset+0x28)&&call->sequence_offset==word(asset+0x2c));
            CHECK(!memcmp(asset+call->motion_offset,"mtb3",4)&&!memcmp(asset+call->sequence_offset,"SEQ\0",4));
            gh_game_mt random=init->random;
            size_t used=gh_heap_used(heap);
            ((uint8_t *)&work)[0x5b1]=1;
            CHECK(gh_enemy2a2_initialize_prefix_00284af8(call,&actor.view,&requests[variant],0,&resource,&bindings,&original)==GH_ACTOR_MODEL_UNSUPPORTED);
            CHECK(!original&&call->frontier==0x157c18&&half(bytes+0x54a)==600+600*variant);
            CHECK(gh_heap_used(heap)==used&&!memcmp(&random,&init->random,sizeof(random))&&fegetround()==modes[mode%4]);
            gh_enemy2a2_pose *pose=&full.pose;
            gh_actor_model_status posed=gh_enemy2a2_pose_continue_002851b8(pose,call,&pose_bindings);
            if(posed!=GH_ACTOR_MODEL_UNSUPPORTED||pose->frontier!=0x31d000)
                fprintf(stderr,"pose %u variant %u status %d frontier %x stage %u\n",mode,variant,posed,pose->frontier,pose->stage);
            CHECK(posed==GH_ACTOR_MODEL_UNSUPPORTED&&pose->frontier==0x31d000);
            CHECK(word(bytes+0x430)>0&&pose->shadow.packets[0]==bytes+0x690&&pose->shadow.packets[1]==bytes+0xde0);
            const uint8_t *seq=asset+call->sequence_offset;
            const unsigned count=half(seq+0x12),table=half(seq+0x10);
            CHECK(count>0&&seq[0x1b]==1);
            unsigned expected_interval_flags=0;
            for(unsigned i=0;i<count;i++) {
                const uint8_t *record=seq+table+(size_t)i*8;
                /* At frame 0, only intervals starting at 0 can match. */
                if(!half(record))expected_interval_flags|=half(record+4);
            }
            CHECK(half(bytes+0x3ac)==expected_interval_flags);
            CHECK(!memcmp(bytes+0x1590,actor.view.alias_f0,12)&&bytes[0x140]==4);
            CHECK(pose->hitarea_created&&!pose->hitarea.fallback&&pose->hitarea.group==2&&pose->hit_cursor==4);
            CHECK(pose->unit&&pose->unit->object==&actor.view&&pose->unit->type==5&&pose->unit->bound_radius_bits==0x3e4ccccd);
            CHECK(pose->sphere&&pose->sphere->matrix==bytes+0x80&&pose->sphere->radius_bits==0x3f000000&&solids.unit_count==2);
            CHECK(!word(bytes+0x670)&&!word(bytes+0x15b8)&&word(bytes+0x15b4)==0x3ca3d70a);
            const unsigned bones[]={1,3,19,23};
            unsigned seen=0;
            for (unsigned ordinal=0;ordinal<4;ordinal++) {
                const uint8_t *record,*matrix;
                gh_collision_pair_shape shape;
                CHECK(!gh_hitarea_node_at(&pose->hitarea,ordinal,&record,&shape));
                CHECK(!gh_collision_pair_shape_source_matrix(&shape,&matrix)&&word(shape.record+0xe0+0x110)==0x3e800000);
                for(unsigned k=0;k<4;k++) {
                    const gh_actor_part *part;
                    CHECK(!gh_actor_model_part_at(&call->model.model,bones[k],&part));
                    if(matrix==part->bytes+0x80)seen|=1u<<k;
                }
            }
            CHECK(seen==15);
            uint32_t frame=word(bytes+0x430);
            used=gh_heap_used(heap);
            CHECK(gh_enemy2a2_pose_continue_002851b8(pose,call,&pose_bindings)==GH_ACTOR_MODEL_UNSUPPORTED&&pose->frontier==0x31d000);
            CHECK(word(bytes+0x430)==frame&&gh_heap_used(heap)==used&&!memcmp(&random,&init->random,sizeof(random))&&fegetround()==modes[mode%4]);
            CHECK(gh_enemy2a2_initialize_00284af8(&full,&actor.view,&requests[variant],0,&resource,
                &bindings,&pose_bindings,&names,GH_EM_NUMERIC_EXACT,&original)==GH_ACTOR_MODEL_UNSUPPORTED);
            CHECK(full.frontier==0x31d000&&!original&&!memcmp(&random,&init->random,sizeof(random)));
            if (mode==4) {
                CHECK(!gh_enemy2a2_release_native(&full)&&!gh_game_title_player_release(&actor));
                CHECK(gh_heap_used(heap)==baseline&&!gh_collision_pair_shapes_have_live(&pairs)&&solids.unit_count==1);
                continue;
            }
            uint32_t draws[3];
            gh_game_mt expected_random=random;
            for(unsigned draw=0;draw<3;draw++)CHECK(!gh_game_mt_next_0030f348(&expected_random,&draws[draw]));
            CHECK(!gh_enemy2a2_initialize_00284af8(&full,&actor.view,&requests[variant],0,&resource,
                &bindings,&pose_bindings,&names,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&original));
            CHECK(original==1&&full.final.complete&&!full.frontier&&!memcmp(&expected_random,&init->random,sizeof(expected_random)));
            CHECK(full.final.timer_word==draws[0]&&full.final.loot_words[0]==draws[1]&&full.final.loot_words[1]==draws[2]);
            CHECK(word(bytes+0x15bc)==timer_oracle(draws[0])&&bytes[0x1604]==1&&word(bytes+0x15c0)==0x43160000);
            unsigned low=draws[1]&15;
            uint32_t loot=(draws[2]&7)==0?0x3d8:low<7?0x3c1:low<13?0x3c8:low<15?0x3d1:0x3d0;
            CHECK(word(bytes+0x560)==loot);
            CHECK(full.final.nodes[0].bytes&&full.final.nodes[4].bytes);
            for(unsigned node=0;node<8;node++) {
                const gh_player_mesh_reference *ref=&full.final.nodes[node];
                CHECK(!word(bytes+0x15cc+node*4));
                if(ref->bytes)CHECK(ref->owner==&call->model.model&&(word(ref->bytes+0x380)&1)==(node!=0&&node!=4));
            }
            CHECK(!gh_enemy2a2_initialize_00284af8(&full,&actor.view,&requests[variant],0,&resource,
                &bindings,&pose_bindings,&names,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&original));
            CHECK(original==1&&!memcmp(&expected_random,&init->random,sizeof(expected_random))&&gh_heap_used(heap)==used);
            CHECK(!gh_enemy2a2_pose_release_prefix(pose)&&!gh_collision_pair_shapes_have_live(&pairs));
            CHECK(solids.unit_count==1&&solids.native_units==foreign_unit);
            /* Reuse the same native state after discarding the finished unit
             * fixture, to verify direct Begin's old-effect bypass separately. */
            gh_actor_motion_bindings motion=call->motion.b;
            call->motion=(gh_actor_motion){0};
            memcpy(bytes+0x570,&(uint16_t){7},2);
            memcpy(bytes+0x574,&(uint32_t){0x11223344},4);
            memcpy(bytes+0x34c,&(uint32_t){0x3f400000},4);
            CHECK(!gh_actor_motion_begin_initial_00157568(&call->motion,&motion,call->motion_offset,call->sequence_offset));
            CHECK(half(bytes+0x570)==7&&word(bytes+0x574)==0x11223344&&word(bytes+0x35c)==0x3f400000);
            CHECK(!gh_enemy2a2_release_native(&full)&&!gh_game_title_player_release(&actor)&&gh_heap_used(heap)==baseline);
        }
    }
    CHECK(!fesetround(FE_TONEAREST)&&!gh_game_cockpit_destroy(cockpit));
    gh_id_runtime_destroy(runtime);
    gh_scene_ray_triangle_state_release(&actors);
    CHECK(!gh_collision_solid_release_unit_001347e8(&solids,&foreign.view,&frontier)&&!gh_game_title_player_release(&foreign));
    CHECK(!gh_game_object_pool_release_001fdfa8(init,&frontier)&&!gh_collision_pairs_release(&pairs));
    CHECK(!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));
    free(init);
    free(id);
    free(language);
    free(room_data);
    free(asset);
    gh_disc_close(disc);
    puts("PASS actual EMA2 appearance0 Init original return1, MT/timer, nodes, loot and scoped cleanup for entry6 and isolated entry14 data");
    return 0;
}
