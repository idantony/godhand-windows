#include "id_layout_rotation.h"
#include "game_effect_create.h"
#include "r100_live_tasks.h"
#include "game_effect_release.h"
#include "game_enemy_damage.h"
#include "game_player_attack_hitarea.h"
#include "game_player_init_tail.h"
#include "game_player_child_tail.h"
#include "game_hitarea_frame.h"
#include "game_enemy_ems.h"
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


typedef struct {
    gh_actor_model model;
    gh_game_player_factory *factory;
    gh_heap *heap;
    const void *fog;
    gh_player_shared_vector vector;
    gh_player_child_tail child_tail;
    gh_game_player_child *child_publication;
    gh_core_save_view save;
    const uint8_t *elf;
    size_t elf_size;
} player_fixture;

static gh_player_init_status player_model(void *context, gh_game_title_player *player,
    gh_boot_work *work, const gh_bundle_entry *md, const gh_bundle_entry *tm, uint32_t mode,
    int *original, uint32_t *frontier)
{
    player_fixture *p = context;
    gh_actor_model_status status = gh_actor_model_initialize_00149738(&p->model, &player->view,
        work, p->factory->b.pools, md, tm, 0, mode, GH_STATIC_BINARY32_REFERENCE, original);
    *frontier = p->model.frontier;
    return status ? GH_PLAYER_INIT_UNSUPPORTED : GH_PLAYER_INIT_OK;
}

static gh_title_player_status player_child(void *context, gh_game_player_child *child,
    int *original, uint32_t *frontier)
{
    player_fixture *p = context;
    if (!child->model.bound && gh_actor_model_bind_packet_inputs(&child->model, p->heap, p->fog))
        return GH_PLAYER_ARGUMENT;
    gh_title_player_status status = gh_game_player_child_initialize_00100378(child, original);
    if (status == GH_PLAYER_UNSUPPORTED && child->frontier == 0x100420)
        status = gh_game_player_child_finish_00100420(&p->child_tail, child, &p->vector,
            &p->child_publication, p->save, p->elf, p->elf_size, GH_STATIC_BINARY32_REFERENCE, original);
    *frontier = status ? (p->child_tail.frontier ? p->child_tail.frontier : child->frontier) : 0;
    return status;
}

typedef struct {
    const gh_title_player_view *enemy;
    gh_actor_model *model;
    unsigned count;
    uint16_t reaction_program;
} impact_sound_check;

static gh_actor_motion_status impact_sound(void *context,const gh_motion_sound_request *request,
    uint32_t *handle,uint32_t *frontier)
{
    impact_sound_check *proof=context;
    fprintf(stderr,"fixture SE %u bank %d program %x\n",proof->count,request->bank,(uint16_t)request->program);
    if(request->actor->bytes!=proof->enemy->bytes || request->model!=proof->model ||
        request->bank!=1 || request->part || request->other_actor ||
        request->argument7 || request->argument8 ||
        (proof->count==0 ? request->program!=0x2a :
         proof->count==1 ? (uint16_t)request->program!=proof->reaction_program :
         proof->count!=2 || request->program!=0x11))
        return GH_ACTOR_MOTION_ARGUMENT;
    proof->count++;
    *handle=0;
    *frontier=0;
    return GH_ACTOR_MOTION_OK;
}

static int first_punch(gh_disc *disc, const uint8_t *elf, size_t elf_size,
    const gh_enemy_ems_bindings *bindings, gh_enemy_ems_live *first,
    const gh_enemy_ems_live *selector1, const gh_enemy_ems_live selector1_other[3],
    const gh_room_state *room, const gh_scene_nearcheck *navigation,
    const gh_effect_create_bindings *effect_bind, gh_enemy202_impact_request *output)
{
    const gh_enemy_actor_bindings *b = &bindings->actor;
    gh_game_init_state *init = b->entry.init;
    gh_collision_pairs *pairs = b->shapes.pairs;
    gh_game_title_player *enemy = first->actor;
    gh_heap *heap = b->entry.room_heap;
    gh_game_objects *global = b->entry.global_objects;
    const gh_hitarea_object *body = &first->enemy202->hitareas.units[0];
    CHECK(first->physical_row == 0 && enemy->view.bytes[0x640] == 0 &&
        body->state == init && body->group == 2 && first->enemy202->hitareas.created[0]);
    size_t pl_size, hair_size;
    uint8_t *pl = load(disc, "pl/pl00.dat", &pl_size), *hair = load(disc, "pl/pl01.dat", &hair_size);
    CHECK(pl && hair);
    int original;
    CHECK(!gh_game_objects_add_pinned_001fece0(global,0x100,pl,pl_size,&original) && original &&
        !gh_game_objects_set_file_extent(global,0x100,pl,pl_size));
    CHECK(!gh_game_objects_add_pinned_001fece0(global,0x101,hair,hair_size,&original) && original &&
        !gh_game_objects_set_file_extent(global,0x101,hair,hair_size));
    gh_game_title_player player = {0};
    gh_game_player_factory factory = {0};
    uint8_t serial = 160;
    gh_player_factory_bindings fb = {&player,b->entry.pools,global,b->entry.room_objects,
        b->entry.work,disc,b->entry.save,&serial};
    player_fixture setup = {.factory=&factory,.heap=heap,.fog=b->entry.fog_rgb,.save=b->entry.save,
        .elf=elf,.elf_size=elf_size};
    CHECK(!gh_player_shared_vector_from_elf(&setup.vector,elf,elf_size));
    CHECK(!gh_game_player_factory_create_0030fa98(&factory,&fb,&original) && original);
    CHECK(!gh_actor_model_bind_packet_inputs(&setup.model,heap,b->entry.fog_rgb));
    gh_game_player_init prefix = {0};
    gh_player_init_bindings pb = {.factory=&factory,.work=b->entry.work,.current_player=&player,
        .model_initialize_00149738=player_model,.model_context=&setup};
    CHECK(gh_game_player_initialize_00107f58(&prefix,&pb) == GH_PLAYER_INIT_UNSUPPORTED &&
        prefix.frontier == 0x30fa98);
    gh_game_player_child child = {0};
    gh_player_child_attachment attachment = {0};
    gh_game_player_init_tail tail = {0};
    gh_player_init_tail_bindings tb = {.prefix=&prefix,.parent_model=&setup.model,.child=&child,
        .attachment=&attachment,.solids=b->solids,.default_heap=heap,.save=b->entry.save,
        .initialize_child=player_child,.child_context=&setup,.init=init,.pairs=pairs,
        .math_globals=b->shapes.math,.shape_policy=GH_STATIC_BINARY32_REFERENCE};
    CHECK(gh_game_player_initialize_tail_00107f58(&tail,&tb) == GH_PLAYER_INIT_UNSUPPORTED &&
        tail.frontier == 0x14b730 && tail.give_step == 5);
    uint8_t *p = player.view.bytes, *e = enemy->view.bytes;
    uint32_t frontier;
    const uint8_t *record;
    gh_collision_pair_shape sphere, box;
    CHECK(!gh_hitarea_node_at(body,0,&record,&sphere) && (word(sphere.record) & 2));
    CHECK(!gh_hitarea_node_at(&tail.give_objects[4],0,&record,&box));
    const uint8_t *center;
    CHECK(!gh_collision_pair_shape_position(&sphere,&center));
    float location[3];
    memcpy(location,center,12);
    /* Position only the player so the original box overlaps the original
     * EMS actor's first active body sphere. Pair and LOS publish the hit. */
    float enemy_yaw, yaw, euler[3]={0}, matrix[16];
    memcpy(&enemy_yaw,e+0x104,4);
    CHECK(!gh_id_angle_reference_001f7d48(enemy_yaw+3.14159274f,&yaw));
    euler[1]=yaw;
    CHECK(!gh_actor_matrix_rotation_00147728(euler,0,matrix,GH_STATIC_BINARY32_REFERENCE,&frontier));
    memcpy(p+0x80,matrix,64);
    location[1] -= 1.15f;
    location[0] -= matrix[8]*0.7f;
    location[2] -= matrix[10]*0.7f;
    memcpy(p+0xb0,location,12);
    memcpy(player.view.alias_f0,location,12);
    memcpy(p+0x100,euler,12);
    gh_hitarea_frame ledger = {0};
    const uint32_t pause[2] = {0};
    gh_hitarea_frame_bindings lb = {.tables=b->actors,.boot_elf=elf,.boot_elf_size=elf_size,
        .current_player=&player.view,.player_pause_words=pause};
    CHECK(word(e+0x54c) == 0x40a00000);
    for (unsigned i=0;i<5;i++) {
        CHECK(!gh_collision_pairs_step_002b9d58(pairs,GH_STATIC_BINARY32_REFERENCE,&frontier));
        CHECK(!gh_hitarea_frame_resume_bound_001fdfd8(
            init,b->entry.work,1,GH_STATIC_BINARY32_REFERENCE,&ledger,&lb,&frontier));
    }
    CHECK(!word(e+0x54c));
    const size_t sequence = word(pl+0xe8);
    CHECK(sequence == 0x96520 && sequence+0x48 <= pl_size &&
        !memcmp(pl+sequence,"SEQ",4) && word(pl+sequence+0x40) == 0x00080006);
    p[0x3ac] = (uint8_t)word(pl+sequence+0x44);
    CHECK(p[0x3ac] == 2);
    const size_t damage_record = 0x3bc7d0-0xff000;
    const size_t virtual_descriptor = 0x4479c8+0xa8-0xff000;
    CHECK(virtual_descriptor+8 <= elf_size && damage_record+4 <= elf_size &&
        !word(elf+virtual_descriptor) && word(elf+virtual_descriptor+4) == 0x28fb88 &&
        word(elf+damage_record) == 3);
    memcpy(p+0x1610,&(float){1},4);
    memcpy(p+0x166c,elf+damage_record,4);
    memcpy(p+0x1674,&(uint32_t){7},4);
    p[0x1682] = 100;
    CHECK(!gh_player_attack_hitarea_0010a438(&player.view,tail.give_objects,&frontier));
    CHECK(!gh_collision_pairs_step_002b9d58(pairs,GH_STATIC_BINARY32_REFERENCE,&frontier));
    CHECK(word(box.record+0xd8));
    gh_hitarea_status hit = gh_hitarea_frame_resume_bound_001fdfd8(
        init,b->entry.work,1,GH_STATIC_BINARY32_REFERENCE,&ledger,&lb,&frontier);
    fprintf(stderr,"first punch ledger status %d frontier %08x box count %u flags %u\n",
        hit,frontier,word(box.record+0xd8),word(body->bytes+0x60));
    CHECK(!hit && !frontier);
    const gh_title_player_view *source;
    CHECK(!gh_hitarea_object_last_hit_actor(body,&source) && source == &player.view &&
        body->bytes[0x46] == 7 && word(body->bytes+0x4c) == 3);
    const unsigned hp_before = word(e+0x548)>>16, maximum = word(e+0x548)&0xffff;
    CHECK(hp_before > 3 && !init->heat[0x10] && !word(p+0x5fc));
    gh_enemy202_move move = {0};
    gh_actor_motion_status reaction = gh_enemy202_move_00208be0(&move,first->enemy202,&player.view,elf,elf_size);
    fprintf(stderr,"first reaction status %d frontier %08x phase %u flags %x hp %u\n",reaction,
        move.frontier,move.damage.phase,word(e+0x3ac),word(e+0x548)>>16);
    CHECK(reaction == GH_ACTOR_MOTION_UNSUPPORTED && move.frontier == 0x306838 && move.damage.reacted);
    CHECK((word(e+0x548)>>16) == hp_before-3 && (word(e+0x548)&0xffff) == maximum);
    CHECK(word(init->heat+4) == 0x3f4ccccd && word(init->heat+8) == 0x3f4ccccd &&
        !word(init->heat+0x14));
    CHECK(word(e+0x2f4) == 1 && word(e+0x54c) == 0x40400000 && word(e+0x17ac) == hp_before &&
        word(e+0x17a4) == 3 && word(e+0x17a8) == 3 && word(e+0x17b4) == 3 &&
        e[0x17b0] == 15 && word(e+0x17c4) == 1 && e[0x17c8] == 0x2a &&
        (word(b->entry.save.bytes+0x1c)&0xffff) == 6 && word(e+0x5b4) == 10 && word(e+0x5b8) == 0x42b40000);
    CHECK(move.damage.impact.bank == 0x58 && move.damage.impact.index == 0x10 &&
        move.damage.impact.scale == 1 && move.damage.impact.mode == -1 &&
        move.damage.impact.position[3] == 1 && !memcmp(move.damage.impact.position+1,body->bytes+0x24,4));
    const gh_enemy202_impact_request impact = move.damage.impact;
    *output = impact;
    const uint32_t mt_index = init->random.index;
    fprintf(stderr,"impact point %.9g %.9g %.9g\n",impact.position[0],impact.position[1],impact.position[2]);
    for (unsigned retry=0;retry<3;retry++) {
        CHECK(gh_enemy202_move_00208be0(&move,first->enemy202,&player.view,elf,elf_size) ==
            GH_ACTOR_MOTION_UNSUPPORTED && move.frontier == 0x306838 &&
            (word(e+0x548)>>16) == hp_before-3 && word(init->heat+4) == 0x3f4ccccd &&
            word(e+0x17a4) == 3 && (word(b->entry.save.bytes+0x1c)&0xffff) == 6 && word(e+0x5b4) == 10 && !memcmp(&impact,&move.damage.impact,sizeof(impact)) && init->random.index == mt_index);
    }

    CHECK(!gh_enemy202_move_bind_impact(&move,effect_bind));
    CHECK(gh_enemy202_move_00208be0(&move,first->enemy202,&player.view,elf,elf_size) ==
        GH_ACTOR_MOTION_UNSUPPORTED && move.frontier == 0x2cba48 &&
        move.damage.phase == 11 && move.impact_create.complete && move.impact_create.original_return);
    const uint32_t impact_serial = move.impact_create.serial;
    const uint32_t after_effect_rng = init->random.index;
    CHECK(gh_enemy202_move_00208be0(&move,first->enemy202,&player.view,elf,elf_size) ==
        GH_ACTOR_MOTION_UNSUPPORTED && move.frontier == 0x2cba48 &&
        move.impact_create.serial == impact_serial && init->random.index == after_effect_rng);

    impact_sound_check sound={.enemy=&enemy->view,.model=first->model,
        .reaction_program=(uint16_t)word(e+0x424)};
    gh_actor_motion *motion=&first->enemy202->motion.motion;
    motion->b.sound=impact_sound;
    motion->b.sound_context=&sound;
    const uint32_t old_frame=word(e+0x430);
    const gh_actor_motion_status resumed=gh_enemy202_move_00208be0(
        &move,first->enemy202,&player.view,elf,elf_size);
    fprintf(stderr,"reaction animation status=%d pc=%08x phase=%u sound=%u yaw=%08x frame=%08x old=%08x\n",
        resumed,move.frontier,move.state1_phase,sound.count,word(e+0x75c),word(e+0x430),old_frame);
    CHECK(resumed == GH_ACTOR_MOTION_UNSUPPORTED && move.frontier == 0x26ef60 &&
        move.dispatch_pc == 0x24c0f0 && move.state1_phase == 18 &&
        move.damage.phase == 12 && sound.count == 2 && !e[0x1864] && !e[0x1867] &&
        (word(e+0x16d0)&0x8002)==2);
    CHECK(move.reaction_resource_offset == 0x774 && init->random.index==after_effect_rng+1);
    const uint8_t *reaction_row=first->enemy202->entry.resource.bytes+move.reaction_resource_offset;
    CHECK(word(reaction_row)==0x2c2980 && word(reaction_row+4)==0x2c3640 && motion->motion.frames==11 &&
        move.substate67_motion.motion_offset==word(reaction_row) && motion->sequence_offset==word(reaction_row+4) &&
        move.substate67_motion.blend_frames==2 && word(e+0x430)==0x3f800000 && e[0x2f6]==1);
    CHECK(gh_enemy202_move_00208be0(&move,first->enemy202,&player.view,elf,elf_size) ==
        GH_ACTOR_MOTION_UNSUPPORTED && move.frontier == 0x26ef60 && sound.count == 2 &&
        word(e+0x430)==0x3f800000 && (word(e+0x548)>>16) == hp_before-3 &&
        move.impact_create.serial == impact_serial);
    fprintf(stderr,"reaction row=%x motion=%x seq=%x frames=%u\n",
        move.reaction_resource_offset,word(reaction_row),word(reaction_row+4),motion->motion.frames);
    gh_game_pl_camera view_camera={0};
    CHECK(!gh_game_camera_construct_pl_00139bc8(&view_camera,&init->camera,
        GH_CAMERA_NATIVE_BINARY32_REFERENCE));
    CHECK(init->active_camera==&init->camera);
    memcpy(view_camera.fields+0x200,enemy->view.alias_f0,12);
    memcpy(view_camera.fields+0x210,player.view.alias_f0,12);
    gh_object_opacity_environment opacity={init,&view_camera,&player.view,1};
    CHECK(!gh_enemy202_move_bind_opacity(&move,&opacity));
    const gh_actor_motion_status finished=gh_enemy202_move_00208be0(
        &move,first->enemy202,&player.view,elf,elf_size);
    fprintf(stderr,"reaction common status=%d pc=%08x body=%u frame=%08x returned=%u\n",
        finished,move.frontier,move.substate67_stage,word(e+0x430),move.original_returned);
    CHECK(!finished && !move.frontier && move.original_returned && word(e+0x430)==0x3f800000 && sound.count==2);
    CHECK(!gh_hitarea_object_set_active_001fd668(&tail.give_objects[4],0,&frontier));
    for(unsigned frame=2;frame<=11;frame++){
        CHECK(!gh_collision_pairs_step_002b9d58(pairs,GH_STATIC_BINARY32_REFERENCE,&frontier));
        CHECK(!gh_hitarea_frame_resume_bound_001fdfd8(
            init,b->entry.work,1,GH_STATIC_BINARY32_REFERENCE,&ledger,&lb,&frontier));
        move=(gh_enemy202_move){0};
        CHECK(!gh_enemy202_move_bind_opacity(&move,&opacity));
        gh_actor_motion_status step=gh_enemy202_move_00208be0(&move,first->enemy202,&player.view,elf,elf_size);
        fprintf(stderr,"reaction frame%u status%d pc=%08x phase=%u body=%u state=%08x sample=%08x sounds=%u\n",
            frame,step,move.frontier,move.state1_phase,move.substate67_stage,word(e+0x2f4),word(e+0x430),sound.count);
        const float expected_frame=frame<11?(float)frame:10.0f;
        CHECK(!step && !move.frontier && move.original_returned &&
            !memcmp(e+0x430,&expected_frame,4) && sound.count==2 &&
            word(e+0x2f4)==(frame<11?0x00010001u:0u) && (word(e+0x548)>>16)==145);
    }
    for(unsigned serial=0;serial<3;serial++){
        const float first_frame=(float)(serial==0?1:serial==1?21:61);
        memcpy(e+0x2f4,&(uint32_t){0x01007100u},4);
        memcpy(e+0x17d0,&serial,4);
        move=(gh_enemy202_move){0};
        CHECK(!gh_enemy202_move_bind_opacity(&move,&opacity));
        gh_actor_motion_status run=gh_enemy202_move_00208be0(&move,first->enemy202,&player.view,elf,elf_size);
        fprintf(stderr,"state71 serial%u status%d pc=%08x phase=%u body=%u state=%08x sample=%08x m=%zx seq=%zx\n",
            serial,run,move.frontier,move.state71_phase,move.substate67_stage,word(e+0x2f4),
            word(e+0x430),motion->motion.offset,motion->sequence_offset);
        CHECK(!run && !move.frontier && move.original_returned && !memcmp(e+0x430,&first_frame,4));
        CHECK(init->enemies[0x540]==10 && move.substate67_motion.blend_frames==10 &&
            motion->motion.offset==word(first->enemy202->entry.resource.bytes+0x41c) &&
            motion->sequence_offset==word(first->enemy202->entry.resource.bytes+0x420));
    }
    const unsigned state71_clip_frames=motion->motion.frames;
    for(unsigned frame=61;frame<state71_clip_frames && e[0x2f5]==0x71;frame++){
        move=(gh_enemy202_move){0};
        CHECK(!gh_enemy202_move_bind_opacity(&move,&opacity));
        gh_actor_motion_status run=gh_enemy202_move_00208be0(&move,first->enemy202,&player.view,elf,elf_size);
        if(run || frame+1==state71_clip_frames)fprintf(stderr,"state71 tick%u/%u status%d pc=%08x phase=%u state=%08x sample=%08x\n",
            frame,state71_clip_frames,run,move.frontier,move.state71_phase,word(e+0x2f4),word(e+0x430));
        CHECK(!run && !move.frontier && move.original_returned);
    }
    CHECK(e[0x2f4]==0 && e[0x2f5]==3 && e[0x2f6]==0 && sound.count==3);
    uint8_t *waiting=selector1->actor->view.bytes;
    gh_enemy_actor *waiting_enemy=selector1->enemy202;
    const gh_object_view *waiting_resource=&waiting_enemy->entry.resource;
    CHECK(waiting[0x1650]==5 && word(waiting+0x564)==0x200 &&
        word(waiting+0x2f4)==0x03000100 && word(waiting+0x1644)==0x48800000);
    uint8_t start_position[12];
    memcpy(start_position,selector1->actor->view.alias_f0,sizeof(start_position));
    move=(gh_enemy202_move){0};
    CHECK(!gh_enemy202_move_bind_opacity(&move,&opacity));
    gh_actor_motion_status waiting_run=gh_enemy202_move_00208be0(&move,waiting_enemy,
        &player.view,elf,elf_size);
    if(waiting_run)fprintf(stderr,"selector1 status%d pc=%08x stage=%u state=%08x flags=%08x\n",
        waiting_run,move.frontier,move.substate67_stage,word(waiting+0x2f4),word(waiting+0x16d0));
    CHECK(!waiting_run && !move.frontier && move.original_returned);
    CHECK(word(waiting+0x2f4)==0x03010100 && word(waiting+0x5f0)==29 &&
        (word(waiting+0x16d0)&0x28400)==0x28400 &&
        !memcmp(waiting+0x5c0,start_position,sizeof(start_position)));
    CHECK(move.substate67_motion.motion_offset==word(waiting_resource->bytes+0x424) &&
        waiting_enemy->motion.motion.sequence_offset==word(waiting_resource->bytes+0x428));
    uint8_t *saved_row=NULL;
    for(unsigned i=0;i<room->active[8];i++){
        uint8_t *row=room->active+12+(size_t)i*24;
        if(row[0x13]==8){saved_row=row;break;}
    }
    CHECK(saved_row && word(saved_row+8)==0x48800000);
    /* Original phase-1 activation also accepts the direct actor wake bit. */
    memcpy(waiting+0x16d0,&(uint32_t){word(waiting+0x16d0)|0x20000000u},4);
    for(unsigned phase=2;phase<=3;phase++){
        move=(gh_enemy202_move){0};
        CHECK(!gh_enemy202_move_bind_room_state(&move,room));
        CHECK(!gh_enemy202_move_bind_opacity(&move,&opacity));
        waiting_run=gh_enemy202_move_00208be0(&move,waiting_enemy,&player.view,elf,elf_size);
        if(waiting_run)fprintf(stderr,"selector1 activation %u status%d pc=%08x stage=%u state=%08x flags=%08x\n",
            phase,waiting_run,move.frontier,move.substate67_stage,word(waiting+0x2f4),word(waiting+0x16d0));
        CHECK(!waiting_run && !move.frontier && move.original_returned && waiting[0x2f6]==phase);
    }
    CHECK(word(saved_row+8)==word(waiting+0x1644) && !(word(waiting+0x16d0)&0x8000u));
    CHECK(move.substate67_motion.motion_offset==word(waiting_resource->bytes+0x42c) &&
        waiting_enemy->motion.motion.sequence_offset==word(waiting_resource->bytes+0x430));
    const unsigned activation_frames=waiting_enemy->motion.motion.motion.frames;
    unsigned activation_ticks=1;
    while(waiting[0x2f5]==1 && activation_ticks<activation_frames){
        move=(gh_enemy202_move){0};
        CHECK(!gh_enemy202_move_bind_room_state(&move,room));
        CHECK(!gh_enemy202_move_bind_opacity(&move,&opacity));
        waiting_run=gh_enemy202_move_00208be0(&move,waiting_enemy,&player.view,elf,elf_size);
        if(waiting_run)fprintf(stderr,"selector1 motion tick %u/%u status%d pc=%08x stage=%u state=%08x\n",
            activation_ticks,activation_frames,waiting_run,move.frontier,move.substate67_stage,
            word(waiting+0x2f4));
        CHECK(!waiting_run && !move.frontier && move.original_returned);
        activation_ticks++;
    }
    fprintf(stderr,"selector1 motion returned after %u/%u ticks, state %08x\n",
        activation_ticks,activation_frames,word(waiting+0x2f4));
    CHECK(activation_ticks==activation_frames && waiting[0x2f5]!=1 && waiting[0x2f6]==0);
    static const unsigned app[3]={9,0,0};
    static const unsigned dependency[3]={0x202,0x203,0x200};
    static const unsigned rows[3][2]={{0x490,0x494},{0x648,0x9c},{0x98,0x9c}};
    for(unsigned i=0;i<3;i++){
        gh_enemy_actor *entry=selector1_other[i].enemy202;
        uint8_t *actor=selector1_other[i].actor->view.bytes;
        CHECK(actor[0x1650]==app[i]&&word(actor+0x564)==dependency[i]&&
            word(actor+0x2f4)==0x00000100);
        CHECK((gh_enemy_timers_held_weapon(&entry->timers)!=NULL)==(i==0));
        for(unsigned tick=0;tick<4;tick++){
            move=(gh_enemy202_move){0};
            CHECK(!gh_enemy202_move_bind_room_state(&move,room));
            CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
            CHECK(!gh_enemy202_move_bind_opacity(&move,&opacity));
            gh_actor_motion_status status=gh_enemy202_move_00208be0(&move,entry,
                &player.view,elf,elf_size);
            if(status)fprintf(stderr,"selector1 stamp%u tick%u status%d pc=%08x stage=%u state=%08x\n",
                actor[0x640],tick,status,move.frontier,move.substate67_stage,word(actor+0x2f4));
            CHECK(!status && !move.frontier && move.original_returned);
            const unsigned cooldown=i==0?30:29-tick;
            if(word(actor+0x2f4)!=0x00010100||word(actor+0x5f0)!=cooldown)
                fprintf(stderr,"selector1 stamp%u tick%u state=%08x cooldown=%u spawn=%08x flags=%08x\n",
                    actor[0x640],tick,word(actor+0x2f4),word(actor+0x5f0),word(actor+0x1644),word(actor+0x16d0));
            CHECK(word(actor+0x2f4)==0x00010100 && word(actor+0x5f0)==cooldown);
            if(!tick)CHECK(move.substate67_motion.motion_offset==word(entry->entry.resource.bytes+rows[i][0]) &&
                entry->motion.motion.sequence_offset==word(entry->entry.resource.bytes+rows[i][1]));
        }
    }
    motion->b.sound=NULL;
    motion->b.sound_context=NULL;

    for (unsigned i=0;i<5;i++) {
        CHECK(!gh_hitarea_shapes_abort_object(&tail.give_objects[i],&frontier));
        CHECK(!gh_hitarea_object_release_empty_001fe278(&tail.give_objects[i],&frontier));
    }
    CHECK(!gh_hitarea_shapes_abort_object(&tail.take_object,&frontier));
    CHECK(!gh_hitarea_object_release_empty_001fe278(&tail.take_object,&frontier));
    CHECK(!gh_collision_solid_release_unit_001347e8(b->solids,&player.view,&frontier));
    gh_game_player_child_attachment_clear(&attachment);
    CHECK(!gh_game_player_child_release_prefix(&child));
    CHECK(!gh_actor_model_release(&setup.model));
    CHECK(!gh_game_player_factory_release_prefix(&factory));
    free(hair);
    free(pl);
    printf("PASS actual R100 first 0202 contact through Give4 and production ledger, HP %u to %u\n",
        hp_before, hp_before-3);
    return 0;
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
    CHECK(init&&id&&!gh_heap_create(64*1024*1024,&heap));
    CHECK(!gh_game_room_pools_initialize_002a5d80(&pools,&room_bundle,heap));
    gh_game_init_state_initialize(init);
    uint32_t frontier;
    CHECK(!gh_hitarea_objects_construct_001fde00(init,&frontier));
    gh_boot_work work={.room=0x100,.phase569=2,.canonical_rate574=1};
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
    gh_collision_scroll_mutation scroll={0};
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
    uint8_t stop=1,scenario=0,serial=255;
    gh_em_scene_placements em={.stop_flag=&stop};
    int loaded;
    CHECK(!gh_game_prepare_002a5d40(&prepare,save,0x100,&game_state)&&!gh_room_state_select_002bebc8(&prepare,0x100,0,&room));
    CHECK(!gh_em_scene_load_native_00294b98(&em,&room,&ems,&work,0,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&loaded)&&loaded);

    gh_em_resource_bindings resource_bindings={
        .em=&em,.room=&room,.room_objects=room_objects,.work=&work,.scenario14=&scenario
    };
    size_t weapon_extent;
    uint8_t *weapon_data=load(disc,"om/om63.dat",&weapon_extent);
    CHECK(weapon_data);
    CHECK(!gh_game_objects_add_pinned_001fece0(global,0x363,weapon_data,weapon_extent,&pinned)&&pinned);
    gh_enemy2a2_node_names heavy_names={0};
    gh_enemy_ems_bindings bindings={.actor={
        .entry={.init=init,.pools=&pools,.global_objects=global,
            .room_objects=room_objects,.work=&work,.room_heap=heap,.packet_heap=heap,
            .fog_rgb=light.scalar+4,.save=save,.scenario14=&scenario,.factory_serial=&serial,
            .policy=GH_STATIC_BINARY32_REFERENCE},
        .cockpit=cockpit,.ik=&ik,.actors=&actors,.atan_coeff=camera.atan_coeff,
        .canonical_rate=&work.canonical_rate574,.solids=&solids,.default_heap=heap,.node_names=&names,
        .shapes={&pairs,&math,GH_STATIC_BINARY32_REFERENCE},
        .numeric_policy=GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,.weapon_scroll=&scroll
    },.names_2a2=&heavy_names};
    gh_enemy_ems *owner=NULL;
    gh_em_resource_prefix prefix={0};
    int detail;
    CHECK(!gh_em_resources_00294ca0(&prefix,&resource_bindings)&&prefix.stage==9&&stop==0);
    CHECK(!gh_enemy_ems_spawn_00294ca0(&owner,&prefix,&room,&bindings,&frontier,&detail)&&!owner&&serial==255);

    size_t heavy_size,aux_size;
    uint8_t *heavy=load(disc,"em/ema2.dat",&heavy_size);
    uint8_t *aux=load(disc,"em/em20.dat",&aux_size);
    CHECK(heavy&&aux);
    CHECK(!gh_game_objects_add_pinned_001fece0(room_objects,0x2a2,heavy,heavy_size,&pinned)&&pinned);
    CHECK(!gh_game_objects_set_file_extent(room_objects,0x2a2,heavy,heavy_size));
    CHECK(!gh_game_objects_add_pinned_001fece0(room_objects,0x220,aux,aux_size,&pinned)&&pinned);
    CHECK(!gh_game_objects_set_file_extent(room_objects,0x220,aux,aux_size));

    prefix=(gh_em_resource_prefix){0};
    CHECK(gh_em_resources_00294ca0(&prefix,&resource_bindings)==GH_PLACE_UNSUPPORTED);
    CHECK(prefix.stage==5&&prefix.prefix_complete&&prefix.spawn_cursor==0&&prefix.frontier==0x2905d0);

    CHECK(!fesetround(FE_TOWARDZERO));
    CHECK(gh_enemy_ems_spawn_00294ca0(&owner,&prefix,&room,&bindings,&frontier,&detail)==GH_PLACE_UNSUPPORTED);
    CHECK(frontier==0x14b730&&prefix.spawn_cursor==5&&serial==4);
    CHECK(word(init->enemies+0x508)==5&&solids.unit_count==5);
    CHECK(!gh_enemy2a2_node_names_from_elf(&heavy_names,elf,elf_size));
    /* A saved-health write while the initializer is suspended must be read
     * by the original caller after its virtual call returns. */
    memcpy(room.active+12+5*24+22,&(int16_t){11},2);
    CHECK(gh_enemy_ems_spawn_00294ca0(&owner,&prefix,&room,&bindings,&frontier,&detail)==GH_PLACE_BOUNDS);
    CHECK(frontier==0x31ccf0&&prefix.spawn_cursor==13&&serial==9);
    const gh_room_vector *paused_registry;
    CHECK(!gh_room_registry_view(&pools,0x754c58,&paused_registry)&&paused_registry->count==10);
    uint32_t pending_weapon=word((uint8_t*)paused_registry->allocation.pointer+9*4);
    CHECK(!gh_enemy_ems_held_weapon(owner,0,pending_weapon));
    CHECK(!gh_game_objects_set_file_extent(global,0x363,weapon_data,weapon_extent));
    gh_scene_placement_status completion=gh_enemy_ems_spawn_00294ca0(&owner,&prefix,&room,&bindings,&frontier,&detail);
    if(completion)fprintf(stderr,"actual stop %08x detail %d row %u\n",frontier,detail,prefix.spawn_cursor);
    CHECK(!completion&&prefix.stage==9&&!frontier&&!detail&&prefix.spawn_cursor==room.active[8]&&owner&&serial==11);
    CHECK(word(init->enemies+0x508)==11&&word(init->enemies+0x50c)==1&&solids.unit_count==11);
    const unsigned numbers[]={0,3,4,5,6,8,10,11,18};
    const gh_room_vector *v;
    CHECK(!gh_room_registry_view(&pools,0x754c58,&v)&&v->count==12);
    for(unsigned i=0;i<9;i++){
        uint32_t handle=word((uint8_t*)v->allocation.pointer+i*4);
        gh_game_title_player *object=gh_enemy_ems_lookup(owner,handle);
        if(!object||object!=init->enemy_actors[i]||object->view.bytes[0x640]!=numbers[i]) fprintf(stderr,"slot %u handle %u actor %p expected %p stamp %u expected %u\n",i,handle,(void*)object,init->enemy_actors[i],object?object->view.bytes[0x640]:255,numbers[i]);
        CHECK(object&&object==init->enemy_actors[i]&&object->view.bytes[0x640]==numbers[i]);
    }
    gh_enemy_ems_live first_live,second_live,heavy_live,selector1_live,selector1_other[3];
    uint32_t first_handle=word((uint8_t*)v->allocation.pointer);
    uint32_t next_handle=word((uint8_t*)v->allocation.pointer+4);
    CHECK(!gh_enemy_ems_live_at(owner,first_handle,&first_live)&&first_live.actor==init->enemy_actors[0]&&
        first_live.enemy202&&first_live.model==&first_live.enemy202->entry.initialize.model.model&&
        first_live.move202&&first_live.physical_row==0&&first_live.shadow);
    CHECK(!gh_enemy_ems_live_at(owner,next_handle,&second_live)&&second_live.actor==init->enemy_actors[1]&&
        second_live.move202&&second_live.move202!=first_live.move202&&second_live.physical_row==2);
    CHECK(!gh_enemy_ems_live_at(owner,word((uint8_t*)v->allocation.pointer+4*4),&heavy_live)&&
        heavy_live.actor==init->enemy_actors[4]&&heavy_live.model&&heavy_live.shadow&&!heavy_live.move202);
    CHECK(!gh_enemy_ems_live_at(owner,word((uint8_t*)v->allocation.pointer+5*4),&selector1_live)&&
        selector1_live.actor==init->enemy_actors[5]&&selector1_live.enemy202&&
        selector1_live.physical_row==6);
    static const unsigned selector1_slots[3]={8,10,11};
    for(unsigned i=0;i<3;i++){
        CHECK(!gh_enemy_ems_live_at(owner,word((uint8_t*)v->allocation.pointer+selector1_slots[i]*4),
            &selector1_other[i]) && selector1_other[i].enemy202 &&
            selector1_other[i].actor->view.bytes[0x640]==18+i &&
            selector1_other[i].physical_row==13+i);
    }
    size_t core_size;
    uint8_t *core=load(disc,"etc/core.dat",&core_size);
    gh_game_effects *effects=calloc(1,sizeof(*effects));
    uint8_t *esp=calloc(1,0x100180), *emit=calloc(1,0x26060);
    CHECK(core && effects && esp && emit && !gh_game_effects_initialize_00305a68(
        effects,esp,0x100180,emit,0x26060,core,core_size,&room_bundle));
    gh_effect_view est;
    CHECK(!gh_effect_est_at_00306030(effects,0x58,0x10,&est) && est.size>=16);
    const unsigned effect_rows=word(est.bytes+4);
    CHECK(effect_rows == 2 && effect_rows <= (est.size-16)/0x2f0);
    for(unsigned row=0;row<effect_rows;row++) {
        const uint8_t *item=est.bytes+16+row*0x2f0;
        fprintf(stderr,"bank58/10 EST row %u class %u type %u group %u part %d\n",
            row,item[0x128],item[0x12a],item[0x136],(int)(int8_t)item[0xbe]);
    }
    const gh_effect_create_bindings effect_bind = {
        .storage={.effects=effects,.esp=esp,.esp_bytes=0x100180,.emitters=emit,.emitter_bytes=0x26060},
        .work=&work,.random=&init->random,.atan_coeff=camera.atan_coeff
    };
    gh_enemy202_impact_request impact;
    gh_bundle_entry pfd;
    gh_scene_nearcheck navigation={0};
    CHECK(!gh_bundle_find(&room_bundle,"PFD",0,&pfd) &&
        !gh_scene_nearcheck_bind_0031c1d0(&navigation,&pfd));
    CHECK(!first_punch(disc,elf,elf_size,&bindings,&first_live,&selector1_live,selector1_other,
        &room,&navigation,&effect_bind,&impact));
    gh_scene_nearcheck_release(&navigation);
    uint32_t position[3];
    memcpy(position,impact.position,12);
    for(unsigned row=0;row<effect_rows;row++) {
        gh_effect_emitter_view emitter;
        CHECK(!gh_effect_create_emitter_at(effects,row,&emitter) && emitter.initialized &&
            !emitter.actor && !emitter.selector && emitter.record.bytes==est.bytes+16+row*0x2f0 &&
            !memcmp(emitter.payload+0x40,position,12) && !memcmp(emitter.payload+0x1c0,position,12));
    }
    gh_effect_create_forget_on_owner_destroy(effects);
    free(emit);free(esp);free(effects);free(core);
    CHECK(!gh_enemy_ems_release_native(&owner,&frontier)&&!owner&&!frontier);
    CHECK(!init->enemy_top&&!init->enemy_last&&gh_collision_solid_units_empty(&solids)&&!gh_collision_pair_shapes_have_live(&pairs));
    CHECK(!fesetround(FE_TONEAREST));
    CHECK(!gh_collision_scroll_mutation_release(&scroll));
    gh_scene_ray_triangle_state_release(&actors);
    CHECK(!gh_game_object_pool_release_001fdfa8(init,&frontier)&&!gh_collision_pairs_release(&pairs));
    CHECK(!gh_game_cockpit_destroy(cockpit));
    gh_id_runtime_destroy(runtime);
    gh_game_objects_destroy(room_objects);
    gh_game_objects_destroy(global);
    CHECK(!gh_game_world_light_release_presets(&light)&&!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));
    gh_disc_close(disc);
    free(weapon_data);free(aux);free(heavy);free(init);free(id);free(elf);free(language_data);free(model_data);free(room_data);

    return 0;
}
