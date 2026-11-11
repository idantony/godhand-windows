#include "game_player_square_frame.h"
#include "game_player_idle_input.h"
#include "game_actor_motion_ik.h"
#include "game_effects_frame.h"
#include "game_enemy_retire.h"
#include "game_object_live_trans.h"
#include "game_enemy_damage.h"
#include "game_enemy_move.h"
#include "game_om303.h"
#include "game_om_factory.h"
#include "game_om_initialize.h"
#include "id_layout_rotation.h"
#include "game_effect_create.h"
#include "r100_live_tasks.h"
#include "game_effect_release.h"
#include "game_player_attack_hitarea.h"
#include "game_player_frame.h"
#include "game_actor_sequence.h"
#include "game_actor_attached_sound.h"
#include "game_player_abort.h"
#include "audio_startup.h"
#include "game_player_init_tail.h"
#include "game_player_child_tail.h"
#include "game_player_targets.h"
#include "game_hitarea_frame.h"
#include "game_enemy_ems.h"
#include "game_enemy_row2.h"
#include "game_enemy_manager.h"
#include "game_camera_envelope.h"
#include "game_camera_projection.h"
#include "display_state.h"
#include "actor_world_queue.h"
#include "frame_state.h"
#include "screen_fade.h"
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

static gh_actor_motion_status release_held_effect(void *context,const gh_title_player_view *actor,
    int32_t kind,uint32_t mode,uint32_t *frontier)
{
    unsigned slot=0;
    gh_effect_status result=gh_game_effect_release_00306200(context,actor,kind,mode,frontier,&slot);
    return !result?GH_ACTOR_MOTION_OK:
        result==GH_EFFECT_UNSUPPORTED?GH_ACTOR_MOTION_UNSUPPORTED:GH_ACTOR_MOTION_CALLEE;
}

typedef struct {
    gh_actor_world_queue *queue;
    unsigned fail_secondary,secondary;
} opacity_draw_fixture;

static gh_actor_model_status capture_opacity_draw(void *context,const gh_actor_draw_command *command,
    uint32_t *frontier)
{
    opacity_draw_fixture *fixture=context;
    if(command->kind==GH_ACTOR_DRAW_OPACITY_GEOMETRY&&fixture->fail_secondary){
        fixture->fail_secondary=0;
        *frontier=0x31a650;
        return GH_ACTOR_MODEL_UNSUPPORTED;
    }
    gh_static_transform_frontier transform={0};
    size_t group;
    gh_actor_world_status status=gh_actor_world_queue_append(fixture->queue,command,&group,&transform);
    if(status){
        fprintf(stderr,"opacity queue kind%u node%zu status%u ee=%08x vu=%08x\n",
            command->kind,command->node_index,status,transform.ee_entry,transform.vu_byte_pc);
        *frontier=transform.ee_entry?transform.ee_entry:0x31a650;
        return GH_ACTOR_MODEL_UNSUPPORTED;
    }
    if(command->kind==GH_ACTOR_DRAW_OPACITY_GEOMETRY)fixture->secondary++;
    return GH_ACTOR_MODEL_OK;
}

static int first_waiting_opacity_draw(const gh_enemy_ems_live *waiting,const gh_game_world_light *light,
    const uint8_t *elf,size_t elf_size)
{
    uint32_t pc=0;
    gh_actor_model *model=waiting->model;
    uint8_t *root=model->root.bytes;
    CHECK(root[0x640]==8&&word(root+0x564)==0x200&&model->part_count==30&&root[0x2b1]==9);
    const uint32_t old_flags=word(root+0x250),old_alpha=word(root+0x24c),old_bank=word(root+0x2c0);
    memcpy(root+0x250,&(uint32_t){0x30},4);
    memcpy(root+0x24c,&(uint32_t){0x3f666666},4);
    memcpy(root+0x2c0,&(uint32_t){10},4);
    gh_game_init_state *init=waiting->enemy202->b.entry.init;
    gh_game_pl_camera camera={0};
    CHECK(!gh_game_camera_construct_pl_00139bc8(&camera,&init->camera,GH_CAMERA_NATIVE_BINARY32_REFERENCE));
    float target[3],eye[3];
    memcpy(target,model->root.alias_f0,12);
    target[1]+=1;
    memcpy(eye,target,12);
    eye[0]+=2;eye[1]+=2;eye[2]+=7;
    memcpy(camera.fields+0x200,target,12);memcpy(camera.fields+0x210,eye,12);
    gh_display_state display;
    CHECK(!gh_display_state_from_elf(&display,elf,elf_size)&&
        !gh_display_static_initialize_002a48a8(&display,1,0xffff));
    const gh_camera_projection_display projection={&display.screen_width,&display.screen_height_bits,
        &display.pixel_aspect_bits,&display.initialized,GH_CAMERA_NATIVE_BINARY32_REFERENCE};
    CHECK(!gh_game_camera_project_00138740(&camera,&projection,&pc)&&!pc);
    gh_actor_light_globals globals={0};
    gh_player_shared_vector pivot={0};
    CHECK(!gh_actor_light_globals_from_elf(&globals,elf,elf_size)&&
        !gh_player_shared_vector_from_elf(&pivot,elf,elf_size));
    gh_stage_textures *textures=NULL;
    opacity_draw_fixture fixture={.fail_secondary=1};
    CHECK(!gh_stage_textures_create(&textures)&&!gh_actor_world_queue_create(&fixture.queue));
    const gh_actor_draw_bindings bindings={.camera=&camera,.light=light,
        .policy=GH_STATIC_BINARY32_REFERENCE,.textures=textures,.emit=capture_opacity_draw,
        .emit_context=&fixture,.light_globals=&globals,.pivot_owner=&pivot};
    gh_actor_draw_call draw={0};
    gh_actor_model_status status=gh_actor_model_draw_0014ac90(&draw,model,20,0,&bindings,&pc);
    if(status!=GH_ACTOR_MODEL_UNSUPPORTED||pc!=0x31a650||draw.phase!=12)
        fprintf(stderr,"opacity draw prefix status%u pc=%08x phase%u node%zu groups%zu\n",
            status,pc,draw.phase,draw.node_index,gh_actor_world_queue_count(fixture.queue));
    CHECK(status==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x31a650&&draw.phase==12&&!fixture.secondary);
    const size_t prefix=gh_actor_world_queue_count(fixture.queue);
    CHECK(prefix>1);
    gh_actor_world_group command;
    CHECK(!gh_actor_world_queue_group_at(fixture.queue,prefix-1,&command)&&
        command.command.kind==GH_ACTOR_DRAW_OPACITY_FRAME&&command.command.frame==0x00080070u&&
        !command.packets);
    status=gh_actor_model_draw_0014ac90(&draw,model,20,0,&bindings,&pc);
    if(status)fprintf(stderr,"opacity draw resume status%u pc=%08x phase%u node%zu\n",
        status,pc,draw.phase,draw.node_index);
    CHECK(!status&&!pc&&draw.complete&&fixture.secondary>0);
    const size_t count=gh_actor_world_queue_count(fixture.queue);
    CHECK(count==prefix+fixture.secondary+1);
    unsigned secondary_packets=0;
    for(size_t i=prefix;i+1<count;i++){
        CHECK(!gh_actor_world_queue_group_at(fixture.queue,i,&command)&&command.packets>0&&
            command.command.kind==GH_ACTOR_DRAW_OPACITY_GEOMETRY&&command.command.selected_program==2&&
            command.command.test==0x507fdu&&command.command.frame==UINT64_C(0xffffffff00080070)&&
            command.command.inputs.fog==255&&command.command.inputs.normal_light[7]==255);
        secondary_packets+=(unsigned)command.packets;
    }
    CHECK(!gh_actor_world_queue_group_at(fixture.queue,count-1,&command)&&!command.packets&&
        command.command.kind==GH_ACTOR_DRAW_OPACITY_SETUP&&command.command.object&&
        command.command.frame==UINT64_C(0xffffffff00080070)&&command.command.test==0x507fdu&&
        command.command.tex1==UINT64_C(0xffffff3800000360)&&command.command.alpha==UINT64_C(0x8000000044));
    printf("PASS actual R100 stamp8 opacity ordering and resume, %u secondary nodes, %u batches\n",
        fixture.secondary,secondary_packets);
    gh_actor_world_queue_destroy(fixture.queue);
    gh_stage_textures_destroy(textures);
    memcpy(root+0x250,&old_flags,4);memcpy(root+0x24c,&old_alpha,4);memcpy(root+0x2c0,&old_bank,4);
    return 0;
}

typedef struct {
    gh_enemy_ems *enemies;
    gh_game_player_factory *player;
    gh_game_player_child *child;
    uint32_t fail_handle;
    unsigned fail_at, resolves;
} camera_target_fixture;

static gh_player_target_status resolve_camera_target(void *context,uint32_t handle,
    gh_title_player_view *out,uint32_t *frontier)
{
    camera_target_fixture *fixture=context;
    if(frontier)*frontier=0;
    if(handle==fixture->fail_handle&&++fixture->resolves==fixture->fail_at){
        if(frontier)*frontier=0x291010;
        return GH_PLAYER_TARGET_UNSUPPORTED;
    }
    const gh_title_player_view *player=NULL;
    if(!gh_game_player_factory_scalar_view(fixture->player,handle,&player)){
        *out=*player;
        return GH_PLAYER_TARGET_OK;
    }
    if(fixture->child->handle==handle&&fixture->child->complete){
        void *registered=NULL;
        if(gh_room_object_lookup(fixture->child->b.pools,handle,GH_ROOM_OBJECT_PLAYER_CHILD,
            &fixture->child->object,&registered)||registered!=&fixture->child->object)
            return GH_PLAYER_TARGET_ARGUMENT;
        *out=fixture->child->object.view;
        return GH_PLAYER_TARGET_OK;
    }
    gh_enemy_ems_live live={0};
    if(gh_enemy_ems_live_at(fixture->enemies,handle,&live)||!live.actor)
        return GH_PLAYER_TARGET_ARGUMENT;
    *out=live.actor->view;
    return GH_PLAYER_TARGET_OK;
}

static int camera_target_00139ff8(gh_enemy_ems *owner,const gh_enemy_actor_bindings *binding,
    gh_game_player_factory *factory,gh_game_player_child *child,
    gh_game_title_player *player,gh_actor_model *model,
    gh_game_title_player *enemy,const uint8_t *elf,size_t elf_size)
{
    uint32_t frontier=0,handle=0;
    CHECK(!gh_enemy_ems_handle_for_actor(owner,enemy,&handle)&&handle);
    CHECK(enemy->view.original_vtable214==0x4479c8);
    const uint8_t *descriptor=elf+(0x4479c8u+0x68u-0xff000u);
    CHECK(!word(descriptor)&&word(descriptor+4)==0x275978u);
    gh_boot_work *work=binding->entry.work;
    const gh_boot_work prior_work=*work;
    work->phase569=1;
    work->canonical_rate574=0.5f;
    CHECK(!gh_frame_rate_policy(work)&&word((const uint8_t*)work+0x564)==1);
    uint8_t *p=player->view.bytes;
    float origin[3],prior_enemy[3],target[3];
    memcpy(origin,player->view.alias_f0,12);
    memcpy(prior_enemy,enemy->view.alias_f0,12);
    memcpy(target,origin,12);
    target[0]+=0.5f;
    target[1]+=2.75f;
    memcpy(enemy->view.alias_f0,target,12);
    uint32_t target_flags=word(enemy->view.bytes+0x250);
    CHECK(!(target_flags&0xe0000000u));
    const gh_actor_part *part0=NULL;
    CHECK(!gh_actor_model_part_at(model,0,&part0)&&part0);
    float prior_part_y,part_y=origin[1]+0.9f;
    memcpy(&prior_part_y,part0->alias_f0+4,4);
    memcpy(part0->alias_f0+4,&part_y,4);
    const uint32_t player_state=word(p+0x2f4),player_flags=word(p+0x15f4);
    const uint32_t old_target=word(p+0x640),old_focus=word(p+0x690);
    memcpy(p+0x2f4,&(uint32_t){0x00001100},4);
    memcpy(p+0x15f4,&(uint32_t){0},4);
    memcpy(p+0x640,&handle,4);
    memcpy(p+0x690,&handle,4);
    gh_game_init_state *init=binding->entry.init;
    gh_game_pl_camera camera={0};
    gh_camera_follow follow={0};
    CHECK(!gh_game_camera_construct_pl_00139bc8(&camera,&init->camera,GH_CAMERA_NATIVE_BINARY32_REFERENCE));
    CHECK(!gh_game_camera_bind_default_00139db8(&follow,&camera,init,&player->view,
        work,elf,elf_size,&frontier));
    memcpy(camera.fields+0x518,&(uint32_t){0},4);
    memcpy(camera.fields+0x51c,&(uint32_t){0},4);
    CHECK(gh_game_camera_follow_00139ff8(&follow,binding->actors,NULL,NULL,&frontier)==GH_CAMERA_UNSUPPORTED&&
        frontier==0x13a3c0&&follow.stage==6);
    gh_camera_envelope envelope={0};
    CHECK(!gh_game_camera_envelope_from_elf(&envelope,elf,elf_size,&frontier) &&
        !gh_game_camera_envelope_bind_model(&envelope,model));
    camera_target_fixture fixture={.enemies=owner,.player=factory,.child=child,
        .fail_handle=handle,.fail_at=2};
    const gh_player_target_bindings targets={.pools=binding->entry.pools,.actors=binding->actors,
        .player=&player->view,.resolve=resolve_camera_target,.context=&fixture,.elf=elf,.elf_bytes=elf_size};
    gh_frame_random random={.policy=GH_FRAME_CHOP_REFERENCE,.auxiliary_word=0x12345678u};
    gh_frame_random expected=random;
    uint8_t draw;
    CHECK(!gh_frame_random_byte_0031cef8(&expected,&draw));
    CHECK(gh_game_camera_finish_follow_target_00139c88(&envelope,&follow,&targets,init,&random,&frontier)==
        GH_CAMERA_UNSUPPORTED&&frontier==0x291010&&envelope.stage==3&&follow.stage==6);
    CHECK(fixture.resolves==2&&random.auxiliary_word==expected.auxiliary_word&&
        envelope.mode==(draw&1u)&&word(camera.fields+0x51c)==15);
    float height;
    memcpy(&height,camera.fields+0x510,4);
    CHECK(fabsf(height-1.375f)<0.00001f&&!memcmp(enemy->view.bytes+0x1680,target,12));
    gh_camera_status status=gh_game_camera_finish_follow_target_00139c88(&envelope,&follow,&targets,
        init,&random,&frontier);
    if(status)fprintf(stderr,"positive camera status%d pc=%08x stage%u\n",status,frontier,envelope.stage);
    CHECK(!status&&!frontier&&!envelope.stage&&!follow.stage&&envelope.mode==1&&
        word(camera.fields+0x51c)==14&&random.auxiliary_word==expected.auxiliary_word);
    printf("PASS actual R100 half-rate active target camera, height %.3f, mode %u, held Rnd across retry\n",
        height,envelope.mode);
    memcpy(enemy->view.alias_f0,prior_enemy,12);
    memcpy(part0->alias_f0+4,&prior_part_y,4);
    memcpy(p+0x2f4,&player_state,4);
    memcpy(p+0x15f4,&player_flags,4);
    memcpy(p+0x640,&old_target,4);
    memcpy(p+0x690,&old_focus,4);
    *work=prior_work;
    return 0;
}

typedef struct {
    const gh_motion_sound_request *pending[2];
    const gh_title_player_view *actor;
    unsigned attempts[2];
    unsigned sequence_count,turn_sound_count;
} scenery_sound_check;

static gh_actor_motion_status scenery_sound(void *context,const gh_motion_sound_request *request,
    uint32_t *handle,uint32_t *frontier)
{
    scenery_sound_check *proof=context;
    if(request->bank==0 && (request->program==0x8d || request->program==0xaa) &&
        request->actor->bytes==proof->actor->bytes && request->model &&
        !request->part && !request->other_actor && !request->argument7 && !request->argument8){
        if(request->program==0x8d)proof->turn_sound_count++;
        else proof->sequence_count++;
        *handle=1;
        *frontier=0;
        return GH_ACTOR_MOTION_OK;
    }
    if(request->actor!=proof->actor || !request->model ||
        (request->bank!=0&&request->bank!=1) ||
        request->program!=(request->bank?0:10) || request->part || request->other_actor ||
        request->argument7 || request->argument8)
        return GH_ACTOR_MOTION_ARGUMENT;
    const unsigned bank=(unsigned)request->bank;
    if(!proof->attempts[bank]++){
        proof->pending[bank]=request;
        *frontier=0x2cba48;
        return GH_ACTOR_MOTION_CALLEE;
    }
    if(request!=proof->pending[bank])return GH_ACTOR_MOTION_ARGUMENT;
    *handle=1;
    *frontier=0;
    return GH_ACTOR_MOTION_OK;
}

static int scenery_contact(const gh_bundle *room_bundle,unsigned kind,
    const gh_room_state *room, const gh_enemy_actor_bindings *b,
    gh_enemy_actor *waiting, const gh_title_player_view *player,
    const gh_hitarea_frame_bindings *ledger_bindings, gh_hitarea_frame *ledger,
    const gh_effect_create_bindings *effects,const gh_scene_nearcheck *navigation,
    const gh_object_opacity_environment *opacity)
{
    int original;
    gh_bundle_entry oms;
    gh_om_scene_placements placements={0};
    CHECK(!gh_bundle_find(room_bundle,"OMS",0,&oms));
    CHECK(gh_om_scene_bind_001c3cb8(&placements,&oms,&original)==GH_PLACE_UNSUPPORTED);
    gh_om_placement_record source_row;
    uint8_t *actor=waiting->entry.object.view.bytes;
    const int active_scenery=kind==0x303 && actor[0x640]==11;
    const int selector8_scenery=kind==0x303 && actor[0x640]==10;
    const gh_hitarea_object *body=&waiting->hitareas.units[0];
    float origin[3];
    memcpy(origin,waiting->entry.object.view.alias_f0,12);
    float nearest=1000000;
    unsigned row_index=0;
    for (unsigned i=kind==0x303?3:8;i<(kind==0x303?8:18);++i) {
        gh_om_placement_record row;
        float xyz[3];
        CHECK(!gh_om_scene_record_at(&placements,i,&row) && row.type==kind);
        memcpy(xyz,row.bytes,12);
        float dx=xyz[0]-origin[0], dz=xyz[2]-origin[2];
        float distance=dx*dx+dz*dz;
        if(distance<nearest){nearest=distance;source_row=row;row_index=i;}
    }
    if (active_scenery||selector8_scenery)
        CHECK(row_index==5 && nearest<4);
    else if (kind==0x303)
        CHECK(row_index==6 && nearest<9);
    else
        CHECK(row_index && nearest<4);
    gh_om_initializer_bindings initializer={.work=b->entry.work,.packet_heap=b->entry.packet_heap,
        .fog_rgb=b->entry.fog_rgb,.init=b->entry.init,.shapes=&b->shapes,.player=player};
    uint8_t serial=0;
    gh_object_resource_bindings resources={0};
    gh_om_factory_bindings bindings={.pools=b->entry.pools,.work=b->entry.work,.room_state=room,
        .objects=b->entry.global_objects,.room_objects=b->entry.room_objects,
        .factory_serial=&serial,.resources=&resources,.initializer=&initializer};
    gh_om_factory factory={0};
    CHECK(!gh_om_factory_bind(&factory,&bindings,1));
    gh_scene_placement_status created=gh_om_factory_create_record(&factory,&source_row,&original);
    if(created)fprintf(stderr,"OM%03x row%u init status=%d pc=%08x\n",kind,row_index,created,factory.frontier);
    CHECK(!created && original && factory.count==1);
    gh_om_object *source=&factory.objects[0];
    gh_om303_state *om=(gh_om303_state *)source->initialization;
    CHECK(om->sphere_created && source->view.original_vtable214==(kind==0x303?0x423230u:0x423338u));
    gh_hitarea_object *give=&om->sphere_unit;
    CHECK(give->group==1 && give->bytes[0x46]==0x2e);
    const uint8_t *give_node;
    gh_collision_pair_shape give_shape;
    CHECK(!gh_hitarea_node_at(give,0,&give_node,&give_shape) &&
        word(give_shape.record+0x1f0)==(kind==0x303?0x40400000u:0x3f800000u));

    uint32_t frontier;
    CHECK(!gh_collision_pairs_step_002b9d58(b->shapes.pairs,GH_STATIC_BINARY32_REFERENCE,&frontier));
    gh_hitarea_status hit=gh_hitarea_frame_resume_bound_001fdfd8(b->entry.init,b->entry.work,1,
        GH_STATIC_BINARY32_REFERENCE,ledger,ledger_bindings,&frontier);
    const gh_title_player_view *resolved;
    if(hit || body->bytes[0x46]!=0x2e)
        fprintf(stderr,"OM%03x row%u distance=%f hit status=%d pc=%08x body=%02x flags=%08x\n",
            kind,row_index,(double)nearest,hit,frontier,body->bytes[0x46],word(body->bytes+0x60));
    CHECK(!hit && !frontier && !gh_hitarea_object_last_hit_actor(body,&resolved) &&
        resolved==&source->view && word(body->bytes+0x44)==0x002e0000 &&
        word(body->bytes+0x40)==UINT32_MAX && !word(body->bytes+0x4c));

    uint8_t actor_before[0x1944];
    memcpy(actor_before,actor,sizeof(actor_before));
    gh_game_mt rng=b->entry.init->random, expected=rng;
    uint32_t ignored;
    if(selector8_scenery){
        /* Actual title-load tick1897 used R100 stamp10 and OM303 row5. */
        CHECK(word(actor+0x564)==0x200 && actor[0x17bc]==1);
        memcpy(actor+0x2f4,&(uint32_t){0x01010800},4);
        memcpy(actor+0x3ac,&(uint16_t){0},2);
        memcpy(actor+0x16d0,&(uint32_t){0x04000000},4);
        memcpy(actor+0x16d4,&(uint32_t){1},4);
        memcpy(actor+0x54a,&(uint16_t){59},2);
        memcpy(actor+0x54c,&(uint32_t){0},4);
        CHECK(!gh_game_mt_next_0030f348(&expected,&ignored));
        gh_enemy202_hit_call call={0};
        CHECK(!gh_enemy202_hit_0020a178(&call,waiting,player,1,&frontier)&&!frontier&&
            call.reacted&&call.phase==5&&word(actor+0x2f4)==0x00001101&&
            (word(actor+0x548)>>16)==59&&word(actor+0x54c)==0x41700000&&
            !memcmp(&expected,&b->entry.init->random,sizeof(expected)));
        CHECK(!gh_enemy202_hit_0020a178(&call,waiting,player,1,&frontier)&&
            !memcmp(&expected,&b->entry.init->random,sizeof(expected)));
        memcpy(actor,actor_before,sizeof(actor_before));
        b->entry.init->random=rng;
        CHECK(!gh_om_factory_release(&factory));
        CHECK(!gh_collision_pairs_step_002b9d58(b->shapes.pairs,GH_STATIC_BINARY32_REFERENCE,&frontier));
        CHECK(!gh_hitarea_frame_resume_bound_001fdfd8(b->entry.init,b->entry.work,1,
            GH_STATIC_BINARY32_REFERENCE,ledger,ledger_bindings,&frontier));
        puts("PASS actual R100 stamp10 OM303 row5 contact, selector08 reaction, HP59 and one RNG draw");
        return 0;
    }
    CHECK(!word(actor+0x54c) && (word(actor+0x2f4)&0xffff)==0x0100);
    if(kind==0x303){
        memcpy(actor+0x2f4,&(uint32_t){active_scenery?0x00011300:0x00011101},4);
        memcpy(actor+0x16d0,&(uint32_t){active_scenery?0x04000400:0x04000202},4);
    }
    CHECK(actor[0x17bc]==1 && !gh_game_mt_next_0030f348(&expected,&ignored));
    gh_enemy202_hit_call call={0};
    gh_actor_motion_status status=gh_enemy202_hit_0020a178(&call,waiting,player,1,&frontier);
    if(status)fprintf(stderr,"OM304 response status=%d pc=%08x flags=%04x %08x %08x\n",
        status,frontier,word(actor+0x3ac)&0xffff,word(actor+0x16d0),word(actor+0x16d4));
    CHECK(!status && !frontier && call.reacted && call.phase==5 && call.effect==0x2e &&
        word(actor+0x2f4)==0x00001101 && word(actor+0x54c)==0x41700000 &&
        word(actor+0x17ac)==74 && (word(actor+0x548)>>16)==74 &&
        (word(actor+0x17ca)&0xffff)==10 && !word(actor+0x17b4) &&
        !memcmp(&expected,&b->entry.init->random,sizeof(expected)));
    CHECK(!gh_enemy202_hit_0020a178(&call,waiting,player,1,&frontier) &&
        !memcmp(&expected,&b->entry.init->random,sizeof(expected)));
    memcpy(actor,actor_before,sizeof(actor_before));
    b->entry.init->random=rng;
    if(kind==0x303&&actor[0x640]==8){
        /* Exact192 title Load stopped on stamp8 in pursuit with HP59. */
        memcpy(actor+0x2f4,&(uint32_t){0x00010600},4);
        memcpy(actor+0x16d0,&(uint32_t){0x04000602},4);
        memcpy(actor+0x54a,&(uint16_t){59},2);
        call=(gh_enemy202_hit_call){0};
        CHECK(!gh_enemy202_hit_0020a178(&call,waiting,player,1,&frontier)&&!frontier&&
            call.reacted&&call.phase==5&&word(actor+0x2f4)==0x00001101&&
            (word(actor+0x548)>>16)==59&&!memcmp(&expected,&b->entry.init->random,sizeof(expected)));
        memcpy(actor,actor_before,sizeof(actor_before));
        b->entry.init->random=rng;
    }
    if(active_scenery){
        memcpy(actor+0x2f4,&(uint32_t){0x00011601},4);
        call=(gh_enemy202_hit_call){0};
        CHECK(gh_enemy202_hit_0020a178(&call,waiting,player,1,&frontier)==GH_ACTOR_MOTION_UNSUPPORTED &&
            frontier==0x20b394 && !call.reacted && !memcmp(&rng,&b->entry.init->random,sizeof(rng)));
        memcpy(actor,actor_before,sizeof(actor_before));
        b->entry.init->random=rng;
        memcpy(actor+0x2f4,&(uint32_t){0x00041601},4);
        call=(gh_enemy202_hit_call){0};
        CHECK(!gh_enemy202_hit_0020a178(&call,waiting,player,1,&frontier)&&!frontier&&
            call.reacted&&word(actor+0x2f4)==0x00001101&&
            !memcmp(&expected,&b->entry.init->random,sizeof(expected)));
        memcpy(actor,actor_before,sizeof(actor_before));
        b->entry.init->random=rng;
    }
    uint8_t *score=b->entry.save.bytes+0x1c;
    const uint16_t original_score=(uint16_t)word(score);
    const uint8_t original_lock=score[2];
    memcpy(score,&(uint16_t){0},2);
    uint32_t behavior=word(actor+0x16d0);
    memcpy(actor+0x16d0,&(uint32_t){behavior|0x1000u},4);
    call=(gh_enemy202_hit_call){0};
    CHECK(!gh_enemy202_hit_0020a178(&call,waiting,player,1,&frontier) &&
        !call.reacted && call.phase==5 && (word(actor+0x17ca)&0xffff)==10 &&
        !word(actor+0x54c) && !memcmp(&rng,&b->entry.init->random,sizeof(rng)));
    memcpy(actor,actor_before,sizeof(actor_before));

    if(kind==0x303){
        memcpy(actor+0x2f4,&(uint32_t){active_scenery?0x00011300:0x00011101},4);
        memcpy(actor+0x16d0,&(uint32_t){active_scenery?0x04000400:0x04000202},4);
    }
    gh_enemy202_move move={0};
    const void *elf=ledger_bindings->boot_elf;
    size_t elf_size=ledger_bindings->boot_elf_size;
    status=gh_enemy202_move_00208be0(&move,waiting,player,elf,elf_size);
    if(move.frontier!=0x306838)fprintf(stderr,"OM304 Move status=%d pc=%08x phase=%u flags=%04x/%08x/%08x\n",
        status,move.frontier,move.damage.phase,word(actor+0x3ac)&0xffff,word(actor+0x16d0),word(actor+0x16d4));
    CHECK(status==GH_ACTOR_MOTION_UNSUPPORTED && move.frontier==0x306838 &&
        move.damage.phase==10 && move.damage.impact.bank==0x58 && move.damage.impact.index==0x10 &&
        (uint16_t)word(score)==24 && actor[0x17b0]==15 &&
        !word(actor+0x5b4) && !word(actor+0x5b8));
    CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
    CHECK(!gh_enemy202_move_bind_opacity(&move,opacity));
    CHECK(!gh_enemy202_move_bind_impact(&move,effects));
    scenery_sound_check proof={.actor=&waiting->entry.object.view};
    gh_actor_motion *motion=&waiting->motion.motion;
    CHECK(!motion->b.sound);
    const gh_actor_motion prior_motion=*motion;
    motion->b.sound=scenery_sound;
    motion->b.sound_context=&proof;
    CHECK(gh_enemy202_move_00208be0(&move,waiting,player,elf,elf_size)==GH_ACTOR_MOTION_CALLEE &&
        move.frontier==0x2cba48 && move.damage.phase==11 && move.impact_create.complete && proof.attempts[0]==1);
    gh_game_mt after_clip=b->entry.init->random;
    uint32_t clip_draw;
    CHECK(!gh_game_mt_next_0030f348(&after_clip,&clip_draw));
    const uint32_t impact_serial=move.impact_create.serial;
    status=gh_enemy202_move_00208be0(&move,waiting,player,elf,elf_size);
    if(status!=GH_ACTOR_MOTION_CALLEE||move.state1_phase!=8)
        fprintf(stderr,"OM304 clip status=%d pc=%08x phase=%u\n",status,move.frontier,move.state1_phase);
    CHECK(status==GH_ACTOR_MOTION_CALLEE && move.frontier==0x2cba48 && move.state1_phase==8 &&
        move.damage.phase==12 && proof.attempts[0]==2 && proof.attempts[1]==1 &&
        move.impact_create.serial==impact_serial && (uint16_t)word(score)==24 &&
        !memcmp(&after_clip,&b->entry.init->random,sizeof(after_clip)));
    const unsigned offset=(clip_draw&1u)?0x5f8:0x600;
    const uint8_t *clip=waiting->entry.resource.bytes+offset;
    CHECK(move.reaction_resource_offset==offset && move.substate67_motion.blend_frames==3 &&
        move.substate67_motion.motion_offset==word(clip) && motion->sequence_offset==word(clip+4) &&
        motion->motion.frames==37 && !actor[0x2f6]);
    status=gh_enemy202_move_00208be0(&move,waiting,player,elf,elf_size);
    if(status)fprintf(stderr,"OM304 clip first step status=%d pc=%08x phase=%u tail=%u\n",
        status,move.frontier,move.state1_phase,move.substate67_stage);
    CHECK((!status||move.frontier==0x26ef60) && move.state1_phase==15 &&
        actor[0x2f6]==1 && word(actor+0x5f0)==9 &&
        !memcmp(actor+0x5d0,resolved->alias_f0,12) && word(actor+0x430)==0x3f800000 &&
        proof.attempts[0]==2 && proof.attempts[1]==2 &&
        !memcmp(&after_clip,&b->entry.init->random,sizeof(after_clip)));
    const uint32_t original_frame=word((const uint8_t *)b->entry.work+0x210);
    CHECK(!status && move.original_returned);
    if(actor[0x640]==8){
        /* Use the genuine OM304 ledger after the cooldown has expired.
         * Restore the first sample afterwards for the uninterrupted clip test. */
        uint8_t after_sample[sizeof(actor_before)];
        memcpy(after_sample,actor,sizeof(after_sample));
        memcpy(actor+0x54c,&(uint32_t){0},4);
        gh_game_mt after_repeat=after_clip;
        CHECK(!gh_game_mt_next_0030f348(&after_repeat,&ignored));
        gh_enemy202_hit_call repeat={0};
        CHECK(!gh_enemy202_hit_0020a178(&repeat,waiting,player,1,&frontier) && !frontier &&
            repeat.reacted && repeat.phase==5 && word(actor+0x2f4)==0x00001101 &&
            word(actor+0x54c)==0x41700000 && (word(actor+0x548)>>16)==74 &&
            !memcmp(&after_repeat,&b->entry.init->random,sizeof(after_repeat)));
        memcpy(actor,after_sample,sizeof(after_sample));
        b->entry.init->random=after_clip;
    }
    if(kind==0x303)goto restore_scenery_fixture;
    for(unsigned frame=1;frame<37;++frame){
        CHECK(!gh_frame_counter_advance(b->entry.work));
        move=(gh_enemy202_move){0};
        CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
        CHECK(!gh_enemy202_move_bind_opacity(&move,opacity));
        status=gh_enemy202_move_00208be0(&move,waiting,player,elf,elf_size);
        if(status)fprintf(stderr,"selector11 frame%u status%d pc%08x stage%u state%08x\n",
            frame,status,move.frontier,move.substate67_stage,word(actor+0x2f4));
        CHECK(!status && move.original_returned);
        CHECK(frame<36 ? actor[0x2f4]==1 && actor[0x2f5]==0x11 :
            !actor[0x2f4] && (actor[0x2f5]==0 || actor[0x2f5]==8) && !actor[0x2f6]);
    }
    if(actor[0x640]==11){
        CHECK(proof.sequence_count==3);
        unsigned updates=0;
        do{
            CHECK(!gh_frame_counter_advance(b->entry.work));
            move=(gh_enemy202_move){0};
            CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
            CHECK(!gh_enemy202_move_bind_opacity(&move,opacity));
            status=gh_enemy202_move_00208be0(&move,waiting,player,elf,elf_size);
            if(status)fprintf(stderr,"stamp11 follow tick%u status%d pc%08x stage%u state%08x\n",
                updates,status,move.frontier,move.substate67_stage,word(actor+0x2f4));
            CHECK(!status && move.original_returned);
            updates++;
        }while(actor[0x2f5]!=0x13 && updates<32);
        CHECK(updates<32 && word(actor+0x2f4)==0x00001300 &&
            word(actor+0x16c0)==0x43e10000 && word(actor+0x16c4)==6 &&
            !memcmp(actor+0x16b0,actor+0x1660,12) && !(word(actor+0x16d0)&2));
        printf("PASS actual R100 stamp11 OM304 row%u to selector13 in %u follow updates, return-home target and timer\n",
            row_index,updates);
        move=(gh_enemy202_move){0};
        CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
        CHECK(!gh_enemy202_move_bind_opacity(&move,opacity));
        status=gh_enemy202_move_00208be0(&move,waiting,player,elf,elf_size);
        if(status)fprintf(stderr,"selector13 first status%d pc%08x state%08x stage%u\n",
            status,move.frontier,word(actor+0x2f4),move.substate67_stage);
        CHECK(!status && move.original_returned && move.substate67_motion.blend_frames==10 &&
            move.substate67_motion.motion_offset==word(waiting->entry.resource.bytes+0x660) &&
            motion->sequence_offset==word(waiting->entry.resource.bytes+0x664));
        CHECK(word(actor+0x2f4)==0x00000300 && !word(actor+0x16c0) &&
            !(word(actor+0x16d0)&0x8002) && (word(actor+0x16d0)&0x400) &&
            word(actor+0x5f0)==90);
        CHECK(!gh_frame_counter_advance(b->entry.work));
        move=(gh_enemy202_move){0};
        CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
        CHECK(!gh_enemy202_move_bind_opacity(&move,opacity));
        status=gh_enemy202_move_00208be0(&move,waiting,player,elf,elf_size);
        if(status)fprintf(stderr,"selector3 mode5 first status%d pc%08x state%08x stage%u\n",
            status,move.frontier,word(actor+0x2f4),move.substate67_stage);
        CHECK(!status && move.original_returned && actor[0x1650]==5 &&
            actor[0x2f5]==3 && move.substate67_motion.blend_frames==10 &&
            move.substate67_motion.motion_offset==word(waiting->entry.resource.bytes+0x648) &&
            motion->sequence_offset==word(waiting->entry.resource.bytes+0x64c) &&
            word(actor+0x5f0)==9);
        for(unsigned frame=1;frame<64;++frame){
            CHECK(!gh_frame_counter_advance(b->entry.work));
            move=(gh_enemy202_move){0};
            CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
            CHECK(!gh_enemy202_move_bind_opacity(&move,opacity));
            status=gh_enemy202_move_00208be0(&move,waiting,player,elf,elf_size);
            if(status)fprintf(stderr,"selector3 mode5 frame%u status%d pc%08x state%08x stage%u\n",
                frame,status,move.frontier,word(actor+0x2f4),move.substate67_stage);
            CHECK(!status && move.original_returned);
        }
        printf("PASS actual stamp11 home arrival and 64 selector3 mode5 updates (state %08x)\n",
            word(actor+0x2f4));
        memcpy(actor+0x2f4,&(uint32_t){0x00020000},4);
        move=(gh_enemy202_move){0};
        CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
        CHECK(!gh_enemy202_move_bind_opacity(&move,opacity));
        status=gh_enemy202_move_00208be0(&move,waiting,player,elf,elf_size);
        CHECK(!status && move.original_returned &&
            move.substate67_motion.motion_offset==word(waiting->entry.resource.bytes+0x650) &&
            motion->sequence_offset==word(waiting->entry.resource.bytes+0x654));
    }else{
    CHECK(proof.sequence_count==3);
    const unsigned successor=actor[0x2f5];
    CHECK(!gh_frame_counter_advance(b->entry.work));
    move=(gh_enemy202_move){0};
    CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
    CHECK(!gh_enemy202_move_bind_opacity(&move,opacity));
    status=gh_enemy202_move_00208be0(&move,waiting,player,elf,elf_size);
    fprintf(stderr,"selector11 exit%u next status%d pc%08x stage%u state%08x distance%08x\n",
        successor,status,move.frontier,move.substate67_stage,word(actor+0x2f4),word(actor+0x618));
    CHECK(!status && move.original_returned);
    const unsigned follow_frames=motion->motion.frames;
    unsigned updates=1;
    while(actor[0x2f5]==8 && updates<follow_frames+2){
        CHECK(!gh_frame_counter_advance(b->entry.work));
        move=(gh_enemy202_move){0};
        CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
        CHECK(!gh_enemy202_move_bind_opacity(&move,opacity));
        status=gh_enemy202_move_00208be0(&move,waiting,player,elf,elf_size);
        if(status)fprintf(stderr,"selector8 tick%u/%u status%d pc%08x stage%u state%08x flags=%08x/%08x\n",
            updates,follow_frames,status,move.frontier,move.substate67_stage,word(actor+0x2f4),
            word(actor+0x16d0),word(actor+0x16d4));
        CHECK(!status && move.original_returned);
        updates++;
    }
    fprintf(stderr,"selector8 terminal ticks%u/%u state%08x\n",updates,follow_frames,word(actor+0x2f4));
    CHECK(updates==follow_frames && follow_frames==11 && !actor[0x2f4] && actor[0x2f5]==0 &&
        proof.turn_sound_count==1 && proof.sequence_count==5);
    CHECK(!gh_frame_counter_advance(b->entry.work));
    move=(gh_enemy202_move){0};
    CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
    CHECK(!gh_enemy202_move_bind_opacity(&move,opacity));
    status=gh_enemy202_move_00208be0(&move,waiting,player,elf,elf_size);
    if(status)fprintf(stderr,"selector0 far status%d pc%08x stage%u state%08x\n",
        status,move.frontier,move.substate67_stage,word(actor+0x2f4));
    CHECK(!status && move.original_returned && word(actor+0x2f4)==0x600 &&
        move.substate67_motion.motion_offset==word(waiting->entry.resource.bytes+0x98));
    gh_game_mt selector6_random=b->entry.init->random;
    uint32_t selector6_draw;
    CHECK(!gh_game_mt_next_0030f348(&selector6_random,&selector6_draw));
    uint8_t selector6_before[0x1944];
    memcpy(selector6_before,actor,sizeof(selector6_before));
    for(unsigned tick=0;tick<12;++tick){
        CHECK(!gh_frame_counter_advance(b->entry.work));
        move=(gh_enemy202_move){0};
        CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
        CHECK(!gh_enemy202_move_bind_opacity(&move,opacity));
        status=gh_enemy202_move_00208be0(&move,waiting,player,elf,elf_size);
        if(status)fprintf(stderr,"selector6 tick%u status%d pc%08x stage%u state%08x\n",
            tick,status,move.frontier,move.substate67_stage,word(actor+0x2f4));
        CHECK(!status && move.original_returned && word(actor+0x2f4)==0x10600 &&
            word(actor+0x5f0)==89-tick && word(actor+0x5f4)==120+selector6_draw%60u &&
            !memcmp(&selector6_random,&b->entry.init->random,sizeof(selector6_random)));
        if(!tick)CHECK(move.substate67_motion.motion_offset==word(waiting->entry.resource.bytes+0xc0) &&
            motion->sequence_offset==word(waiting->entry.resource.bytes+0xc4) &&
            move.substate67_motion.blend_frames==10 && move.substate67_motion.release_previous);
    }
    printf("PASS actual stamp8 selector6 C0/C4 pursuit, 12 moves, one MT word, hold 5F0/5F4\n");
    memcpy(actor,selector6_before,sizeof(selector6_before));
    memcpy(actor+0x2f4,&(uint32_t){0x00020000},4);
    move=(gh_enemy202_move){0};
    CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
    CHECK(!gh_enemy202_move_bind_opacity(&move,opacity));
    status=gh_enemy202_move_00208be0(&move,waiting,player,elf,elf_size);
    fprintf(stderr,"selector0 turn status%d pc%08x stage%u state%08x motion%zx\n",
        status,move.frontier,move.substate67_stage,word(actor+0x2f4),move.substate67_motion.motion_offset);
    CHECK(!status && move.original_returned && move.substate67_motion.motion_offset==
        word(waiting->entry.resource.bytes+0xb0));
    }
restore_scenery_fixture:
    memcpy((uint8_t *)b->entry.work+0x210,&original_frame,4);
    *motion=prior_motion;
    memcpy(score,&original_score,2);
    score[2]=original_lock;
    memcpy(actor,actor_before,sizeof(actor_before));
    b->entry.init->random=rng;
    CHECK(!gh_om_factory_release(&factory));
    CHECK(!gh_collision_pairs_step_002b9d58(b->shapes.pairs,GH_STATIC_BINARY32_REFERENCE,&frontier));
    CHECK(!gh_hitarea_frame_resume_bound_001fdfd8(b->entry.init,b->entry.work,1,
        GH_STATIC_BINARY32_REFERENCE,ledger,ledger_bindings,&frontier));
    if(kind==0x303)
        printf("PASS actual R100 stamp%u OM303 row%u Give, selector11 phase reset and clip rebind %03x\n",
            (unsigned)actor[0x640],row_index,offset);
    else printf("PASS actual R100 stamp%u OM304 row%u, effect2E, 37-frame reaction row %03x and selector8/0 far continuation\n",
        actor[0x640],row_index,offset);
    return 0;
}

typedef struct {
    gh_enemy_ems *owner;
    gh_se_voice_engine *voices;
    gh_enemy_manager_remove remove;
    unsigned removed;
    uint8_t reject_once;
} stamp19_release_fixture;

static gh_actor_motion_status release_stamp19_effect(void *p,const gh_title_player_view *actor,
    int32_t kind,uint32_t mode,uint32_t *frontier)
{
    const gh_effect_create_bindings *binding=p;
    gh_effect_release_bindings storage=binding->storage;
    storage.actor_match=gh_effect_create_actor_match;
    storage.context=storage.effects;
    unsigned slot;
    gh_effect_status status=gh_game_effect_release_00306200(&storage,actor,kind,mode,frontier,&slot);
    return !status?GH_ACTOR_MOTION_OK:status==GH_EFFECT_UNSUPPORTED?
        GH_ACTOR_MOTION_UNSUPPORTED:GH_ACTOR_MOTION_CALLEE;
}

static gh_actor_motion_status release_stamp19_fixture(void *p,const gh_enemy_actor *enemy,uint32_t *frontier)
{
    stamp19_release_fixture *fixture=p;
    if(fixture->reject_once){
        fixture->reject_once=0;*frontier=0x290770;return GH_ACTOR_MOTION_UNSUPPORTED;
    }
    uint32_t handle=0;
    if(gh_enemy_ems_handle_for_actor(fixture->owner,&enemy->entry.object,&handle)||!handle)
        return GH_ACTOR_MOTION_ARGUMENT;
    int removed=0;
    gh_actor_motion_status status=gh_enemy_ems_remove_00290770(fixture->owner,&fixture->remove,
        handle,fixture->voices,&removed,frontier);
    fixture->removed+=(unsigned)removed;
    return status;
}

static gh_actor_motion_status record_stamp19_attack_sound(void *context,
    const gh_motion_sound_request *request,uint32_t *handle,uint32_t *frontier)
{
    const gh_enemy_actor *enemy=context;
    if(!request||!handle||!frontier||!request->actor||request->actor->bytes!=enemy->entry.object.view.bytes||
       request->model!=enemy->motion.motion.b.model)return GH_ACTOR_MOTION_ARGUMENT;
    fprintf(stderr,"stamp19 attack voice bank%d program%04x\n",request->bank,(uint16_t)request->program);
    *handle=0;
    *frontier=0;
    return GH_ACTOR_MOTION_OK;
}

typedef struct {
    gh_actor_motion motion;
    gh_actor_motion_call bind;
    gh_actor_attached_sound attached;
    gh_sound_control control;
    gh_sound_scene scene;
    gh_game_player_abort abort;
    const gh_effect_create_bindings *effects;
    gh_effect_create_call effect;
    const gh_motion_sound_request *sound_request;
    uint32_t first_voice,reaction_voice;
    unsigned sound_attempts;
    uint32_t expected_clip,expected_voice;
} player_hurt_fixture;

static gh_actor_motion_status hurt_sound_request(void *opaque,const gh_motion_sound_request *request,
    uint32_t *handle,uint32_t *frontier)
{
    player_hurt_fixture *fixture=opaque;
    if(!fixture->sound_attempts++){
        fixture->sound_request=request;
        *frontier=0x2cba48;
        return GH_ACTOR_MOTION_CALLEE;
    }
    if(request!=fixture->sound_request)return GH_ACTOR_MOTION_ARGUMENT;
    gh_actor_motion_status status=gh_actor_attached_sound_motion(&fixture->attached,request,handle,frontier);
    if(!status){
        if(request->program==0x6e)fixture->first_voice=*handle;
        else if(request->program==0xcf||request->program==0xd0)fixture->reaction_voice=*handle;
    }
    return status;
}

static gh_player_frame_status hurt_effect_create(void *opaque,const uint32_t position[3],uint32_t *pc)
{
    player_hurt_fixture *fixture=opaque;
    gh_effect_create_call *call=&fixture->effect;
    gh_effect_status status=GH_EFFECT_OK;
    if(!call->active)status=gh_effect_create_player_impact_begin_00306838(call,position);
    if(!status)status=gh_game_effect_create_00306528(call,fixture->effects);
    if(!status){
        gh_game_mt next=*fixture->effects->random;
        if(gh_game_mt_next_0030f348(&next,&fixture->expected_clip)||
           gh_game_mt_next_0030f348(&next,&fixture->expected_voice))return GH_PLAYER_FRAME_ARGUMENT;
    }
    *pc=call->frontier;
    return status?GH_PLAYER_FRAME_CALLEE:GH_PLAYER_FRAME_OK;
}

static gh_player_frame_status hurt_abort(void *opaque,const gh_player_frame_bindings *b,uint32_t *pc)
{
    player_hurt_fixture *fixture=opaque;
    const gh_player_abort_bindings bindings={.player=b->player,.sound=&fixture->control};
    gh_player_abort_status status=gh_game_player_abort_00126770(&fixture->abort,&bindings);
    *pc=fixture->abort.frontier;
    return status?GH_PLAYER_FRAME_CALLEE:GH_PLAYER_FRAME_OK;
}

static gh_player_init_status hurt_motion_begin(void *opaque,const gh_player_motion_request *r,uint32_t *pc)
{
    player_hurt_fixture *fixture=opaque;
    gh_actor_motion_call *call=&fixture->bind;
    gh_actor_motion *motion=&fixture->motion;
    gh_actor_motion_status status=GH_ACTOR_MOTION_OK;
    if(!call->active)status=gh_actor_motion_call_open_parameters_002a8578(call,motion,&motion->b,
        r->motion_offset,r->sequence_candidate_offset,r->start_frame_bits,r->blend_frames,
        r->flags,r->release_previous_sequence);
    if(!status)status=gh_actor_motion_call_step_002a8578(call);
    *pc=call->frontier;
    return status?GH_PLAYER_INIT_UNSUPPORTED:GH_PLAYER_INIT_OK;
}

static gh_player_frame_status hurt_motion_advance(void *opaque,const gh_player_motion_request *r,
    int *ended,uint32_t *pc)
{
    player_hurt_fixture *fixture=opaque;
    gh_actor_motion_status status=gh_actor_motion_step_00157c18(&fixture->motion,*r->rate,
        GH_STATIC_BINARY32_REFERENCE,ended,pc);
    return status?GH_PLAYER_FRAME_CALLEE:GH_PLAYER_FRAME_OK;
}

static int enemy_give_player_take(gh_disc *disc,const gh_bundle *room_bundle,gh_enemy_actor *enemy,gh_game_title_player *player,
    gh_game_player_init_tail *tail,gh_actor_model *player_model,const uint8_t *pl00,size_t pl00_bytes,
    const gh_hitarea_frame_bindings *ledger_bindings,gh_hitarea_frame *ledger,const gh_effect_create_bindings *effects)
{
    const gh_enemy_actor_bindings *b=&enemy->b;
    const gh_hitarea_object *take=&tail->take_object;
    uint8_t *p=enemy->entry.object.view.bytes;
    gh_game_init_state *init=b->entry.init;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_actor_motion prior_motion=*motion;
    uint8_t enemy_before[0x1944];
    memcpy(enemy_before,p,sizeof(enemy_before));
    uint32_t pc;
    CHECK(word(p+0x564)==0x20a && p[0x640]==0);
    const gh_object_view *resource=&enemy->entry.resource;
    const size_t mt=word(resource->bytes+0x918),seq=word(resource->bytes+0x91c);
    CHECK(mt==0x30e3c0 && seq==0x30fb80);
    gh_actor_motion_call bind={0};
    CHECK(!gh_actor_motion_call_open_parameters_002a8578(&bind,motion,&motion->b,mt,seq,0,10,0,1));
    CHECK(!gh_actor_motion_call_step_002a8578(&bind));
    memcpy(p+0x430,&(float){26},4);
    gh_actor_motion_status status=gh_actor_motion_sequence_002c7fd0(motion,1,
        GH_STATIC_BINARY32_REFERENCE,&pc);
    if(status)fprintf(stderr,"enemy original selector15 sample26 status%d pc%08x bank%d program%04x\n",
        status,pc,motion->pending_sound.bank,(uint16_t)motion->pending_sound.program);
    CHECK(!status && (word(p+0x3ac)&3)==2);

    CHECK(!gh_actor_model_update_hierarchy_00149eb8(player_model,GH_STATIC_BINARY32_REFERENCE));
    CHECK(!gh_collision_pairs_step_002b9d58(b->shapes.pairs,GH_STATIC_BINARY32_REFERENCE,&pc));
    gh_hitarea_object *give=&enemy->hitareas.units[4];
    const uint8_t *node,*target_position;
    gh_collision_pair_shape box,target;
    CHECK(give->group==1 && take->group==2 && !gh_hitarea_node_at(give,0,&node,&box));
    CHECK(!gh_hitarea_node_at(take,0,&node,&target) &&
        !gh_collision_pair_shape_position(&target,&target_position));
    float old_offset[3],old_size[3],old_origin[3],take_center[3];
    uint8_t old_metadata[16];
    memcpy(old_offset,box.record+0x130,12);memcpy(old_size,box.record+0x1f0,12);
    memcpy(old_origin,p+0xb0,12);memcpy(take_center,target_position,12);
    memcpy(old_metadata,give->bytes+0x40,sizeof(old_metadata));
    /* Original 260B30 class2 at SEQ bit2. Only the controlled test placement
     * changes actor position. The existing factory box and Take publish the hit. */
    float scale,damage_factor;
    memcpy(&scale,p+0x114,4);memcpy(&damage_factor,p+0x76c,4);
    CHECK(scale>0 && damage_factor==1);
    volatile float inverse=1.0f/scale,offset_y=inverse*.7f;
    const float give_offset[3]={0,offset_y,.75f},give_size[3]={.7f,1,1};
    float matrix[16],position[3];
    memcpy(matrix,p+0x80,sizeof(matrix));
    for(unsigned i=0;i<3;++i)position[i]=take_center[i]-matrix[4+i]*give_offset[1]-matrix[8+i]*give_offset[2];
    memcpy(p+0xb0,position,12);
    CHECK(!gh_collision_pair_shape_offset_002bb258(&box,give_offset,&pc));
    CHECK(!gh_collision_pair_box_size_002baaa8(&box,give_size,&pc));
    memcpy(give->bytes+0x10,matrix+8,12);
    memcpy(give->bytes+0x40,&(uint32_t){UINT32_MAX},4);
    memcpy(give->bytes+0x44,&(uint32_t){0x00370000},4);
    memcpy(give->bytes+0x4c,&(uint32_t){15},4);
    CHECK(!gh_hitarea_object_set_active_001fd668(give,1,&pc));
    CHECK(!gh_collision_pairs_step_002b9d58(b->shapes.pairs,GH_STATIC_BINARY32_REFERENCE,&pc));
    const gh_hitarea_status hit=gh_hitarea_frame_resume_bound_001fdfd8(init,b->entry.work,1,
        GH_STATIC_BINARY32_REFERENCE,ledger,ledger_bindings,&pc);
    const gh_title_player_view *source=NULL;
    CHECK(!gh_hitarea_object_last_hit_actor(take,&source));
    fprintf(stderr,"enemy Give selector15 take: hit%d pc%08x source%u boxhits%u flags%08x kind%08x damage%u\n",
        hit,pc,source==&enemy->entry.object.view,word(box.record+0xd8),word(take->bytes+0x60),
        word(take->bytes+0x44),word(take->bytes+0x4c));
    CHECK(!hit && !pc && source==&enemy->entry.object.view &&
        word(take->bytes+0x44)==0x00370000 && word(take->bytes+0x4c)==15 &&
        word(take->bytes+0x40)==UINT32_MAX && (word(take->bytes+0x60)&1));
    uint8_t *pl=player->view.bytes;
    uint32_t prior_hp=word(pl+0x548),prior_cooldown=word(pl+0x54c);
    uint8_t heat[sizeof(init->heat)];
    memcpy(heat,init->heat,sizeof(heat));
    CHECK((prior_hp>>16)>15 && (prior_hp&0xffff)>15 && !prior_cooldown);
    gh_scene_at_manager scene_at={0};
    CHECK(!gh_scene_at_initialize_units_002c0f90(&scene_at,room_bundle,b->entry.room_heap,0x100,1));
    memcpy(init->heat,&(float){100},4);
    memset(init->heat+4,0,sizeof(init->heat)-4);
    player_hurt_fixture fixture={.effects=effects};
    gh_audio_registry *registry;
    gh_se_voice_engine *voices;
    gh_se_mix mix;
    max_align_t unused_transport;
    CHECK(!gh_audio_registry_create(&registry)&&!gh_audio_startup_load(registry,disc));
    CHECK(!gh_se_mix_boot_defaults_002c9e78(&mix,0,GH_SE_DSP_NATIVE_LINEAR_REFERENCE));
    CHECK(!gh_se_voice_create(registry,(gh_audio_output *)&unused_transport,&mix,&voices));
    CHECK(!gh_sound_control_from_startup(&fixture.control,disc,registry));
    CHECK(!gh_sound_scene_from_startup(&fixture.scene,&fixture.control,registry));
    gh_actor_attached_sound_bindings audio={voices,&fixture.control,&fixture.scene,b->entry.work,&init->random};
    CHECK(!gh_actor_attached_sound_bind(&fixture.attached,&audio));
    CHECK(!gh_actor_attached_sound_bind_scene_tables(&fixture.attached,b->actors));
    gh_actor_motion_bindings motion_bindings={.model=player_model,.resource=pl00,.resource_bytes=pl00_bytes,
        .work=b->entry.work,.ik_globals=b->ik,.save=b->entry.save,.save_binding_known=1,
        .sound=gh_actor_attached_sound_motion,.sound_context=&fixture.attached};
    pl[0x570]=pl[0x571]=0xff;
    CHECK(!gh_actor_motion_begin_002a8578(&fixture.motion,&motion_bindings,word(pl00+0x28),word(pl00+0x2c),0));
    uint8_t vibration[32][16]={0};
    gh_player_frame_bindings bindings={.player=player,.work=b->entry.work,.init=init,
        .save=b->entry.save,.elf=ledger_bindings->boot_elf,.elf_bytes=ledger_bindings->boot_elf_size,
        .numeric_policy=GH_CAMERA_NATIVE_BINARY32_REFERENCE,.scene_at=&scene_at,.take_binding_known=1,.take_object=take,
        .solids=b->solids,.give_binding_known=1,.give_present_mask=31,.give_objects=tail->give_objects,
        .model=player_model,.resource=pl00,.resource_bytes=pl00_bytes,.rate=b->canonical_rate,
        .initialize_motion=hurt_motion_begin,.advance_motion=hurt_motion_advance,.motion_context=&fixture,
        .damage_sound=hurt_sound_request,.damage_sound_context=&fixture,
        .damage_effect=hurt_effect_create,.damage_abort=hurt_abort,.damage_context=&fixture,
        .damage_vibration_rows=vibration,.auxiliary_bindings_known=1};
    /* Earlier player frame owners are already isolated by this focused caller fixture. */
    uint8_t *points=b->entry.save.bytes+0x1c;
    const uint16_t old_points=(uint16_t)word(points);
    memcpy(points,&(uint16_t){200},2);
    gh_game_player_frame frame={.bound=1,.stage=10,.b=bindings};
    const gh_player_frame_status applied=gh_game_player_frame_00108ab0(&frame,&bindings);
    fprintf(stderr,"PL00 original hit prefix status%d pc%08x rawHP%08x original%08x heat%08x\n",
        applied,frame.frontier,word(pl+0x548),prior_hp,word(init->heat+4));
    CHECK(applied==GH_PLAYER_FRAME_CALLEE && frame.frontier==0x2cba48 &&
        frame.damage_stage==6 && frame.pending_damage_kind==0x37 && frame.pending_damage==15);
    const uint32_t next_hp=word(pl+0x548),next_heat=word(init->heat+4);
    CHECK(prior_hp==0x00640064 && next_hp==0x005d0064 && next_heat==0x3f333332 &&
        word(init->heat+8)==next_heat && !word(init->heat+0x14) && (uint16_t)word(points)==40);
    gh_game_mt impact_rng=init->random;
    uint32_t impact_choice;
    CHECK(!gh_game_mt_next_0030f348(&impact_rng,&impact_choice));
    gh_player_frame_status resumed=gh_game_player_frame_00108ab0(&frame,&bindings);
    fprintf(stderr,"PL00 original reaction status%d pc%08x phase%u outerstage%u damage%u effect%u/%u sample%08x sound%u/%u\n",
        resumed,frame.frontier,frame.hurt_phase,frame.stage,frame.damage_stage,fixture.effect.stage,
        fixture.effect.row,word(pl+0x430),fixture.first_voice,fixture.reaction_voice);
    CHECK(resumed==GH_PLAYER_FRAME_UNSUPPORTED&&frame.frontier==0x1236e8&&frame.stage==17&&
        word(pl+0x548)==next_hp&&word(init->heat+4)==next_heat&&(uint16_t)word(points)==40);
    CHECK(frame.state_entry==0x11aaa0&&frame.hurt_phase==10&&word(pl+0x2f4)==0x00010101&&
        word(pl+0x54c)==0x40400000&&word(pl+0x5f4)==40&&word(pl+0x5f8)==150&&
        word(pl+0x430)==0x3f800000&&fixture.effect.complete&&fixture.effect.count==4);
    static const unsigned hurt_rows[]={0x148,0x150,0x4ec,0x4f4};
    const unsigned hurt_row=hurt_rows[fixture.expected_clip&3u];
    CHECK(fixture.bind.motion_offset==word(pl00+hurt_row) &&
        fixture.motion.sequence_offset==word(pl00+hurt_row+4) && fixture.bind.blend_frames==1);
    const unsigned vibration_shift=word((const uint8_t *)b->entry.work+0x564)&31u;
    const uint32_t vibration_duration=0x60000u<<vibration_shift;
    CHECK(word(vibration[0])==0xff03&&word(vibration[0]+4)==(vibration_duration+1u));
    gh_se_voice_info voice;
    CHECK(fixture.first_voice&&fixture.reaction_voice&&fixture.sound_attempts==3&&
        !gh_se_voice_query(voices,fixture.first_voice,&voice)&&voice.pending&&voice.bank==0&&
        voice.program==(int16_t)(0x6e + impact_choice%9u)&&voice.position_mode==0);
    CHECK(!gh_se_voice_query(voices,fixture.reaction_voice,&voice)&&voice.pending&&voice.bank==3&&
        voice.program==(fixture.expected_voice&1u?7:8)&&voice.adsr1==0x80ff&&voice.adsr2==0x5fcb&&
        voice.position_mode==0&&voice.rate==11025);
    CHECK(gh_game_player_frame_00108ab0(&frame,&bindings)==GH_PLAYER_FRAME_UNSUPPORTED&&
        frame.frontier==0x1236e8&&frame.stage==17&&fixture.sound_attempts==3&&word(pl+0x430)==0x3f800000&&
        word(pl+0x548)==next_hp&&(uint16_t)word(points)==40&&!vibration[1][0]);
    CHECK(!gh_actor_attached_sound_abort(&fixture.attached));
    gh_se_voice_destroy(voices);
    gh_audio_registry_destroy(registry);
    memcpy(points,&old_points,2);
    CHECK(!gh_scene_at_units_release(&scene_at));
    memcpy(pl+0x548,&prior_hp,4);
    memcpy(init->heat,heat,sizeof(heat));
    memcpy(p+0xb0,old_origin,12);memcpy(give->bytes+0x40,old_metadata,sizeof(old_metadata));
    CHECK(!gh_collision_pair_shape_offset_002bb258(&box,old_offset,&pc));
    CHECK(!gh_collision_pair_box_size_002baaa8(&box,old_size,&pc));
    CHECK(!gh_hitarea_object_set_active_001fd668(give,0,&pc));
    *motion=prior_motion;
    CHECK(!gh_collision_pairs_step_002b9d58(b->shapes.pairs,GH_STATIC_BINARY32_REFERENCE,&pc));
    CHECK(!gh_hitarea_frame_resume_bound_001fdfd8(init,b->entry.work,1,
        GH_STATIC_BINARY32_REFERENCE,ledger,ledger_bindings,&pc));
    memcpy(p,enemy_before,sizeof(enemy_before));
    return 0;
}

static int selector9_first_steps(const gh_enemy_ems_live *live,const gh_title_player_view *player,
    const void *elf,size_t elf_size,const gh_room_state *room,const gh_scene_nearcheck *navigation,
    const gh_object_opacity_environment *opacity)
{
    gh_enemy_actor *enemy=live->enemy202;
    uint8_t *actor=live->actor->view.bytes;
    const gh_object_view *resource=&enemy->entry.resource;
    const unsigned dependency=word(actor+0x564);
    CHECK(actor[0x640]==10&&dependency==0x200&&!actor[0x17c3]);
    CHECK(!gh_enemy_timers_held_weapon(&enemy->timers));
    for(unsigned variant=0;variant<3;variant++){
        memcpy(actor+0x2f4,&(uint32_t){0x00000900|(variant<<24)},4);
        gh_game_mt expected=enemy->entry.b.init->random;
        unsigned side=variant;
        if(variant==2){
            uint32_t draw;
            CHECK(!gh_game_mt_next_0030f348(&expected,&draw));
            side=draw&1u;
        }
        const unsigned row=0xcc+8*side;
        for(unsigned frame=0;frame<2;frame++){
            gh_enemy202_move move={0};
            CHECK(!gh_enemy202_move_bind_opacity(&move,opacity));
            CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
            CHECK(!gh_enemy202_move_bind_room_state(&move,room));
            CHECK(!gh_enemy202_move_bind_player_aux_698(&move,NULL));
            const gh_actor_motion_status status=gh_enemy202_move_00208be0(&move,enemy,player,elf,elf_size);
            if(status)fprintf(stderr,"stamp10 selector9 dependency%03x row%03x variant%u frame%u status%d pc%08x stage%u state%08x\n",
                dependency,row,variant,frame,status,move.frontier,move.substate67_stage,word(actor+0x2f4));
            CHECK(!status&&!move.frontier&&move.original_returned&&word(actor+0x2f4)==(0x00010900|(variant<<24)));
            CHECK(word(actor+0x600)==0x42b40000&&word(actor+0x604)==0x42b40000&&
                (word(actor+0x16d0)&0x400)&&word(actor+0x430)==(frame?0x40000000u:0x3f800000u));
            if(!frame)CHECK(move.reaction_resource_offset==row&&move.substate67_motion.blend_frames==10&&
                !move.substate67_motion.flags&&
                move.substate67_motion.motion_offset==word(resource->bytes+row));
            CHECK(!memcmp(&expected,&enemy->entry.b.init->random,sizeof(expected)));
        }
    }
    puts("PASS actual R100 stamp10 selector9 two clip rows, MT cadence, and continuing Move");
    return 0;
}
static int stamp10_return_home(const gh_enemy_ems_live *live,const gh_title_player_view *player,
    const void *elf,size_t elf_size,const gh_room_state *room,const gh_scene_nearcheck *navigation,
    const gh_object_opacity_environment *opacity)
{
    gh_enemy_actor *enemy=live->enemy202;
    uint8_t *actor=live->actor->view.bytes;
    const gh_object_view *resource=&enemy->entry.resource;
    CHECK(actor[0x640]==10&&word(actor+0x564)==0x200);
    memcpy(actor+0x2f4,&(uint32_t){0x00001300},4);
    memcpy(actor+0x16c0,&(uint32_t){0x43e10000},4);
    memcpy(actor+0x16c4,&(uint32_t){6},4);
    memcpy(actor+0x16b0,enemy->entry.object.view.alias_f0,12);
    gh_enemy202_move move={0};
    CHECK(!gh_enemy202_move_bind_opacity(&move,opacity));
    CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
    CHECK(!gh_enemy202_move_bind_room_state(&move,room));
    const gh_actor_motion_status status=gh_enemy202_move_00208be0(&move,enemy,player,elf,elf_size);
    if(status)fprintf(stderr,"stamp10 selector13 status%d pc%08x stage%u\n",status,move.frontier,move.substate67_stage);
    CHECK(!status&&move.original_returned&&move.substate67_motion.motion_offset==word(resource->bytes+0xc0)&&
        enemy->motion.motion.sequence_offset==word(resource->bytes+0xc4)&&word(actor+0x2f4)==0x00000300);
    puts("PASS actual R100 stamp10 selector13 EM00 fallback and home arrival");
    return 0;
}
static uint32_t stamp19_effect_clock(void *);
static int retire_stamp19_trans(gh_enemy_ems *,const gh_enemy_ems_live *,uint32_t,
    const gh_effect_create_bindings *,const gh_title_player_view *,gh_game_init_state *,
    gh_game_scene_tables *,const void *,size_t,gh_room_pools *,gh_heap *);

static int verify_stamp19_attack_effect(const gh_enemy202_move *move,
    const gh_effect_create_bindings *effects,const gh_enemy_actor *enemy,
    const gh_title_player_view *player,gh_game_init_state *init,
    gh_game_scene_tables *actors,const void *elf,size_t elf_size)
{
    CHECK(move->death_effect_create.complete&&move->death_effect_create.row==4);
    const gh_title_player_view *actor=&enemy->entry.object.view;
    const gh_actor_model *model=&enemy->entry.initialize.model.model;
    const gh_actor_part *part;
    CHECK(!gh_actor_model_part_at(model,15,&part));
    unsigned matched=0;
    for(unsigned slot=0;slot<256;slot++){
        gh_effect_emitter_view emitter;
        if(gh_effect_create_emitter_at(effects->storage.effects,slot,&emitter)||
           word(emitter.payload+0x18c)!=0x002d0058u)continue;
        CHECK(emitter.initialized&&emitter.actor==actor&&emitter.selector==actor&&
            emitter.model==model&&emitter.part==part&&
            word(emitter.payload+0x1e0)==word(actor->bytes+0x114)&&!word(emitter.payload+0x110));
        matched++;
    }
    CHECK(matched==4);
    gh_effects_frame_globals globals={0};
    gh_game_scene_aux profile={0};
    gh_game_effects_frame frame={0};
    uint32_t clock=0;
    CHECK(!gh_effects_frame_globals_from_elf(&globals,elf,elf_size));
    const gh_effects_frame_bindings update={.storage=effects->storage,.work=effects->work,
        .init=init,.current_player=player,.globals=&globals,.profile=&profile,
        .clock=stamp19_effect_clock,.clock_context=&clock,.atan_coeff=effects->atan_coeff,
        .policy=GH_STATIC_BINARY32_REFERENCE,.actors=actors};
    for(unsigned i=0;i<6;i++)CHECK(!gh_game_effects_frame_update_0030f228(&frame,&update)&&!frame.frontier);
    gh_effect_release_bindings release=effects->storage;
    release.actor_match=gh_effect_create_actor_match;
    release.context=release.effects;
    uint8_t masks[96];
    memcpy(masks,release.emitters+0x26000,sizeof(masks));
    uint32_t frontier;
    unsigned last;
    for(unsigned retry=0;retry<2;retry++){
        CHECK(!gh_game_effect_release_00306200(&release,actor,0,0,&frontier,&last)&&!frontier);
        CHECK(!memcmp(masks,release.emitters+0x26000,sizeof(masks)));
        unsigned stopped=0;
        for(unsigned slot=0;slot<256;slot++){
            gh_effect_emitter_view emitter;
            if(gh_effect_create_emitter_at(release.effects,slot,&emitter)||
               word(emitter.payload+0x18c)!=0x002d0058u)continue;
            CHECK(word(emitter.payload+0x114)&0x40000000u);
            stopped++;
        }
        CHECK(stopped==4);
    }
    puts("PASS original 306200/302918 marks all four stamp19 attack emitters without releasing pool ownership");
    for(unsigned i=0;i<3;i++)
        CHECK(!gh_game_effects_frame_update_0030f228(&frame,&update)&&!frame.frontier);
    for(unsigned slot=0;slot<256;slot++){
        gh_effect_emitter_view emitter;
        CHECK(gh_effect_create_emitter_at(release.effects,slot,&emitter)||
            word(emitter.payload+0x18c)!=0x002d0058u);
    }

    return 0;
}

#define VERIFY(expr) do { if (!(expr)) { fprintf(stderr,"line %d: %s\n",__LINE__,#expr); exit(1); } } while (0)
typedef struct {
 gh_actor_motion motion;
 gh_actor_motion_call call;
 gh_actor_motion_bindings bindings;
 unsigned begins,sounds;
 int block;
 unsigned triangle;
} bridge;
static gh_player_init_status begin_motion(void*ctx,const gh_player_motion_request*r,uint32_t*f)
{
 bridge *b=ctx;
 b->begins++;
 if(b->triangle)VERIFY(r->motion_offset==0x992e0&&r->sequence_candidate_offset==0x9ad20&&
   !r->start_frame_bits&&r->blend_frames==5&&r->flags==0x40&&r->release_previous_sequence);
 else VERIFY(r->motion_offset==0x955a0&&r->sequence_candidate_offset==0x96520&&
   !r->start_frame_bits&&r->blend_frames==7&&r->flags==0x40&&r->release_previous_sequence);
 if(b->block){b->block=0;*f=0x2a8578;return GH_PLAYER_INIT_UNSUPPORTED;}
 gh_actor_motion_status e=gh_actor_motion_call_open_parameters_002a8578(
  &b->call,&b->motion,&b->bindings,r->motion_offset,r->sequence_candidate_offset,
  r->start_frame_bits,r->blend_frames,r->flags,r->release_previous_sequence);
 if(!e)e=gh_actor_motion_call_step_002a8578(&b->call);
 *f=b->call.frontier;
 return e?GH_PLAYER_INIT_UNSUPPORTED:GH_PLAYER_INIT_OK;
}
static gh_actor_motion_status sound(void *context,const gh_motion_sound_request *r,uint32_t *handle,uint32_t *frontier)
{
 bridge *b=context;
 VERIFY(r->actor==&b->bindings.model->root&&r->model==b->bindings.model&&!r->part);
 if(b->triangle){
  VERIFY(r->bank==0&&r->program==(b->sounds?243:203));
  ++b->sounds;*handle=17;return GH_ACTOR_MOTION_OK;
 }
 VERIFY(r->bank==0&&r->program==(b->sounds<2?203:24));
 b->sounds++;
 if(b->sounds==1){*frontier=0x2cba48;return GH_ACTOR_MOTION_BUSY;}
 *handle=17;
 return GH_ACTOR_MOTION_OK;
}
static gh_player_frame_status advance_motion(void *context,const gh_player_motion_request *r,
 int *ended,uint32_t *frontier)
{
 bridge *b=context;
 gh_actor_motion_status e=gh_actor_motion_step_00157c18(
  &b->motion,*r->rate,GH_STATIC_BINARY32_REFERENCE,ended,frontier);
 return e?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_OK;
}
static gh_player_frame_status strike(void *context,const gh_title_player_view *player,
 gh_hitarea_object give[5],uint32_t *frontier)
{
 (void)context;
 return gh_player_attack_hitarea_0010a438(player,give,frontier)?
        GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_OK;
}
static gh_actor_motion_status triangle_impact_sound(void *context,const gh_motion_sound_request *request,
    uint32_t *handle,uint32_t *frontier)
{
    gh_enemy_actor *enemy=context;
    fprintf(stderr,"Triangle sound %u:%u part %p\n",request->bank,request->program,(const void*)request->part);
    VERIFY(request->actor->bytes==enemy->entry.object.view.bytes&&request->model==enemy->motion.motion.b.model&&
        !request->part&&((request->bank==0&&(request->program==10||request->program==180))||
            (request->bank==1&&(request->program==(int16_t)(word(enemy->entry.object.view.bytes+0x424)&0xffff)||
             request->program==(int16_t)((word(enemy->entry.object.view.bytes+0x424)&0xffff)+8u)))));
    *handle=17;
    *frontier=0;
    return GH_ACTOR_MOTION_OK;
}

static int triangle_ledger(const gh_enemy_actor_bindings *b, gh_enemy_actor *enemy,
    gh_game_title_player *player, gh_actor_model *model, const uint8_t *pl, size_t pl_size,
    const uint8_t *elf, size_t elf_size, gh_game_player_init_tail *tail, camera_target_fixture *resolver,
    const gh_hitarea_frame_bindings *lb, gh_hitarea_frame *ledger, const gh_effect_create_bindings *effect, const gh_room_state *room, const gh_enemy_ems_live *live, int mode)
{
    uint8_t *p=player->view.bytes, *e=enemy->entry.object.view.bytes;
    gh_motion_ik_globals ik={0};
    gh_frame_random random={0};
    bridge backend={.triangle=1};
    uint32_t pc;
    CHECK(!gh_motion_ik_globals_from_elf(&ik,elf,elf_size));
    CHECK(!gh_frame_seed_0031ceb8(&random,&b->entry.init->random,GH_FRAME_CHOP_REFERENCE));
    backend.bindings=(gh_actor_motion_bindings){.model=model,.resource=pl,.resource_bytes=pl_size,
        .work=b->entry.work,.ik_globals=&ik,.sequence_random=&random,.sound=sound,.sound_context=&backend};
    const size_t idle=word(pl+0x28);
    p[0x570]=p[0x571]=0xff; /* PlayerInit sequence handle sentinel. */
    const gh_actor_motion_status started=gh_actor_motion_begin_002a8578(&backend.motion,&backend.bindings,idle,word(pl+0x2c),0);
    if(started)fprintf(stderr,"Triangle idle status%d PC%08x detail%08x speed%08x state%08x\n",started,
        backend.motion.frontier,backend.motion.detail,word(p+0x5a8),word(p+0x428));
    CHECK(!started);
    int ended;
    CHECK(!gh_actor_motion_step_00157c18(&backend.motion,1,GH_STATIC_BINARY32_REFERENCE,&ended,&pc));
    const float rate=1;
    const gh_player_frame_bindings frame={.player=player,.work=b->entry.work,.init=b->entry.init,
        .current_player=&player->view,.elf=elf,.elf_bytes=elf_size,.model=model,.resource=pl,
        .resource_bytes=pl_size,.rate=&rate,.initialize_motion=begin_motion,.advance_motion=advance_motion,
        .motion_context=&backend,.give_objects=tail->give_objects,.give_binding_known=1,.give_present_mask=31,
        .auxiliary_bindings_known=1,.effects=&effect->storage,.save=b->entry.save,
        .numeric_policy=GH_CAMERA_NATIVE_BINARY32_REFERENCE};
    const gh_player_target_bindings targets={.pools=b->entry.pools,.actors=b->actors,
        .player=&player->view,.elf=elf,.elf_bytes=elf_size,.resolve=resolve_camera_target,.context=resolver};
    gh_player_target selected={0};
    gh_player_square_frame_bindings attack={.frame=&frame,.target_bindings=&targets,
        .target_640=&selected,.hit_area_0010a438=strike};
    gh_player_square_frame call={0};
    gh_player_idle_input input={0};
    int chosen;
    p[0x622]=3; /* Edge buffer supplied by the normal input owner. */
    memcpy(p+0x15f4,&(uint32_t){0x20},4);
    const gh_player_idle_input_bindings input_bindings={.frame=frame,.targets=targets};
    const gh_player_frame_status picked=gh_game_player_idle_input_00123938(&input,&input_bindings,&chosen);
    if(picked)fprintf(stderr,"Triangle input status %d PC %08x phase %u\n",picked,input.frontier,input.phase);
    CHECK(!picked&&chosen&&p[0x5e2]==1);
    selected.handle=word(p+0x640);
    if(selected.handle)CHECK(!resolve_camera_target(resolver,selected.handle,&selected.view,&pc));
    p[0x622]=0;
    for(unsigned i=0;i<17;i++){
        CHECK(!gh_player_square_frame_reset(&call));
        const gh_player_frame_status status=gh_game_player_square_frame_0010d3a8(&call,&attack);
        if(status)fprintf(stderr,"Triangle player stage %u PC %08x frame %u\n",call.stage,call.frontier,i);
        CHECK(!status);
        if(p[0x5e6])break;
    }
    CHECK(p[0x5e6]&&backend.begins==1&&backend.sounds==2);
    CHECK(!gh_collision_pairs_step_002b9d58(b->shapes.pairs,GH_STATIC_BINARY32_REFERENCE,&pc));
    CHECK(!gh_hitarea_frame_resume_bound_001fdfd8(b->entry.init,b->entry.work,1,
        GH_STATIC_BINARY32_REFERENCE,ledger,lb,&pc));
    const gh_hitarea_object *body=&enemy->hitareas.units[0];
    const gh_title_player_view *source;
    CHECK(!gh_hitarea_object_last_hit_actor(body,&source)&&source==&player->view&&
          body->bytes[0x46]==0x12&&word(body->bytes+0x4c)==12);
    const int lethal=mode==4||mode==5;
    if(mode>=6){
        const unsigned wanted=mode>=10?10u:mode>=8?0x70u:0;
        const unsigned rank=mode>=12?3:2;
        const size_t rank_table=0x3bf160-0xff000;
        CHECK(rank_table+8<=elf_size&&word(elf+rank_table)==1000&&word(elf+rank_table+4)==3000);
        const uint16_t points=(uint16_t)word(elf+rank_table+4*(rank-2));
        memcpy(b->entry.save.bytes+0x1c,&points,2);
        gh_game_mt predicted;
        uint32_t seed;
        for(seed=1;seed<2048;++seed){
            gh_game_mt_seed_0030f2b0(&predicted,seed);
            uint32_t draw[3];
            CHECK(!gh_game_mt_next_0030f348(&predicted,draw)&&
                !gh_game_mt_next_0030f348(&predicted,draw+1)&&!gh_game_mt_next_0030f348(&predicted,draw+2));
            const unsigned counter=!(draw[1]&3u)?0x70u:e[0x640]==20&&(draw[2]&31u)==3?10u:0;
            if(counter==wanted)break;
        }
        CHECK(seed<2048);
        gh_game_mt_seed_0030f2b0(&b->entry.init->random,seed);
        if(wanted){
            gh_enemy202_hit_call hit={0};
            const uint32_t hp=word(e+0x548);
            CHECK(!gh_enemy202_hit_0020a178(&hit,enemy,&player->view,rank,&pc)&&!pc&&
                 !hit.reacted&&hit.phase==5&&word(e+0x548)==hp&&
                 word(e+0x2f4)==wanted*256u&&word(b->entry.init->heat+4)==0x40000000);
            CHECK(b->entry.init->random.index==(wanted==10?3u:2u)&&
                 word(e+0x54c)==(wanted==10?0x40400000u:0x40000000u));
            printf("PASS Triangle rank%u stamp%u original counter %X, HP restored, MT index %u\n",
                rank,e[0x640],wanted,b->entry.init->random.index);
            return 0;
        }
        gh_enemy202_hit_call hit={0};
        const unsigned hp=word(e+0x548)>>16;
        const unsigned excitement=word(e+0x16e8);
        CHECK(!gh_enemy202_hit_0020a178(&hit,enemy,&player->view,rank,&pc)&&!pc&&hit.reacted&&hit.phase==5&&
             word(e+0x2f4)==0x101&&word(b->entry.init->heat+4)==0x40000000&&(word(e+0x548)>>16)==hp-12);
        CHECK(word(e+0x16e8)==excitement+(rank>=3?20u:0));
        printf("PASS Triangle rank%u stamp%u original counter miss, HP %u to %u\n",rank,e[0x640],hp,hp-12);
        return 0;
    }
    if(lethal)memcpy(e+0x54a,&(uint16_t){12},2); /* Controlled pre-contact HP edge. */
    const unsigned hp=word(e+0x548)>>16;
    gh_enemy202_move move={0};
    const gh_actor_motion_status status=gh_enemy202_move_00208be0(&move,enemy,&player->view,elf,elf_size);
    fprintf(stderr,"Triangle Take stamp%u status%d PC%08x phase%u hp%u/%u state%08x heat%08x rank%u stun%u\n",
        e[0x640],status,move.frontier,move.damage.phase,word(e+0x548)>>16,hp,word(e+0x2f4),
        word(b->entry.init->heat+4),word(b->entry.save.bytes+0x1c)&0xffff,word(e+0x5b4));
    CHECK(status==GH_ACTOR_MOTION_UNSUPPORTED&&move.frontier==0x306838&&move.damage.effect==0x12);
    CHECK((word(e+0x548)>>16)==hp-12&&word(e+0x2f4)==(lethal?0x701u:0x101u)&&
        word(b->entry.init->heat+4)==0x40000000&&word(b->entry.init->heat+8)==0x40000000&&
        (word(b->entry.save.bytes+0x1c)&0xffff)==24&&(word(e+0x5b4)==15||word(e+0x5b4)==30));
    CHECK(move.damage.impact.bank==0x58&&move.damage.impact.index==0x10);
    CHECK(!gh_enemy202_move_bind_impact(&move,effect));
    gh_actor_motion *motion=&enemy->motion.motion;
    const gh_motion_sound_consumer old_sound=motion->b.sound;
    void *old_context=motion->b.sound_context;
    motion->b.sound=triangle_impact_sound;
    motion->b.sound_context=enemy;
    const gh_actor_motion_status resumed=gh_enemy202_move_00208be0(&move,enemy,&player->view,elf,elf_size);
    motion->b.sound=old_sound;
    motion->b.sound_context=old_context;
    fprintf(stderr,"Triangle reaction stamp%u status%d PC%08x stage%u phase%u\n",
        e[0x640],resumed,move.frontier,move.stage,move.damage.phase);
    fprintf(stderr,"reaction row%X frame%08x state%08x pos%08x clock%u %u flags%08x\n",
        move.reaction_resource_offset,word(e+0x430),word(e+0x2f4),word(e+0x600),
        word(e+0x5f0),word(e+0x5f4),word(e+0x434));
    gh_game_pl_camera camera={0};
    CHECK(!gh_game_camera_construct_pl_00139bc8(&camera,&b->entry.init->camera,GH_CAMERA_NATIVE_BINARY32_REFERENCE));
    CHECK(b->entry.init->active_camera==&b->entry.init->camera);
    memcpy(camera.fields+0x200,enemy->entry.object.view.alias_f0,12);
    memcpy(camera.fields+0x210,player->view.alias_f0,12);
    gh_object_opacity_environment opacity={b->entry.init,&camera,&player->view,1};
    if(lethal){
        fprintf(stderr,"lethal scope effect stage%u complete%u EST%u marker%u frames%u\n",
            move.death_effect_create.stage,move.death_effect_create.complete,move.death_effect_create.count,
            move.death_effect_create.configuration_death_part_zero,enemy->motion.motion.motion.frames);
        CHECK(!resumed&&move.original_returned&&move.impact_create.complete&&move.death_effect_create.complete);
        CHECK(word(e+0x2f4)==0x00010701&&word(e+0x430)==0x3f800000);
        CHECK(!gh_hitarea_object_set_active_001fd668(&tail->give_objects[4],0,&pc));
        const gh_motion_effect_consumer old_effect=motion->b.effect;
        void *old_effect_context=motion->b.context;
        motion->b.effect=release_stamp19_effect;
        motion->b.context=(void *)effect;
        motion->b.sound=triangle_impact_sound;
        motion->b.sound_context=enemy;
        unsigned frames=0;
        while(frames<160&&(word(e+0x2f4)&0xffff)!=0x0202){
            CHECK(!gh_collision_pairs_step_002b9d58(b->shapes.pairs,GH_STATIC_BINARY32_REFERENCE,&pc));
            CHECK(!gh_hitarea_frame_resume_bound_001fdfd8(b->entry.init,b->entry.work,1,
                GH_STATIC_BINARY32_REFERENCE,ledger,lb,&pc));
            gh_enemy202_move next={0};
            CHECK(!gh_enemy202_move_bind_impact(&next,effect));
            CHECK(!gh_enemy202_move_bind_opacity(&next,&opacity));
            const gh_actor_motion_status tick=gh_enemy202_move_00208be0(&next,enemy,&player->view,elf,elf_size);
            if(tick)fprintf(stderr,"lethal tick %u stamp%u frontier %08x stage%u mode%08x motion%zu frame%08x\n",
                frames+1,e[0x640],next.frontier,next.stage,word(e+0x2f4),motion->motion.offset,word(e+0x430));
            CHECK(!tick);
            ++frames;
        }
        motion->b.effect=old_effect;
        motion->b.context=old_effect_context;
        motion->b.sound=old_sound;
        motion->b.sound_context=old_context;
        fprintf(stderr,"lethal final stamp%u frames%u mode%08x\n",e[0x640],frames,word(e+0x2f4));
        CHECK(frames<160&&(word(e+0x2f4)&0xffff)==0x0202);
        uint8_t *save_row=NULL;
        for(unsigned i=0;i<room->active[8];i++){
            uint8_t *entry=room->active+12+i*24;
            if(entry[0x13]==e[0x640])save_row=entry;
        }
        CHECK(save_row&&!save_row[0x14]);
        motion->b.effect=release_stamp19_effect;
        motion->b.context=(void *)effect;
        const unsigned count=word(b->entry.init->enemies+0x508);
        gh_audio_registry *registry=NULL;
        gh_se_voice_engine *voices=NULL;
        gh_se_mix mix;
        max_align_t transport_identity;
        CHECK(!gh_audio_registry_create(&registry)&&
            !gh_se_mix_boot_defaults_002c9e78(&mix,0,GH_SE_DSP_NATIVE_LINEAR_REFERENCE)&&
            !gh_se_voice_create(registry,(gh_audio_output *)&transport_identity,&mix,&voices));
        stamp19_release_fixture removal={.owner=resolver->enemies,.voices=voices};
        uint32_t handle=0;
        CHECK(!gh_enemy_ems_handle_for_actor(resolver->enemies,&enemy->entry.object,&handle)&&handle);
        /* Select a deterministic no-drop test branch using the actual MT. */
        gh_game_mt_seed_0030f2b0(&b->entry.init->random,1);
        gh_game_mt expected=b->entry.init->random;
        uint32_t draw[2];
        CHECK(!gh_game_mt_next_0030f348(&expected,draw)&&!gh_game_mt_next_0030f348(&expected,draw+1)&&
            draw[0]%50>0&&(draw[1]&3u));
        unsigned fade=0;
        for(;fade<100&&!removal.removed;fade++){
            gh_enemy202_move next={0};
            CHECK(!gh_enemy202_move_bind_impact(&next,effect));
            CHECK(!gh_enemy202_move_bind_opacity(&next,&opacity));
            CHECK(!gh_enemy202_move_bind_room_state(&next,room));
            CHECK(!gh_enemy202_move_bind_death_release(&next,release_stamp19_fixture,&removal));
            const gh_actor_motion_status tick=gh_enemy202_move_00208be0(&next,enemy,&player->view,elf,elf_size);
            if(tick)fprintf(stderr,"Triangle fade frame%u stamp%u status%d PC%08x phase%u remove%u item%u\n",
                fade,e[0x640],tick,next.frontier,next.death_fade_phase,removal.remove.stage,next.death_item);
            CHECK(!tick&&next.original_returned);
            if(!fade)CHECK(!memcmp(&expected,&b->entry.init->random,sizeof(expected)));
        }
        CHECK(removal.removed==1&&word(b->entry.init->enemies+0x508)==count-1&&save_row[0x14]==1);
        printf("PASS lethal Triangle stamp%u actual clips %u + fade %u, RoomSave and 290770\n",e[0x640],frames,fade);
        motion->b.effect=old_effect;
        motion->b.context=old_effect_context;
        gh_se_voice_destroy(voices);
        gh_audio_registry_destroy(registry);
        CHECK(!retire_stamp19_trans(resolver->enemies,live,handle,effect,&player->view,
            b->entry.init,b->actors,elf,elf_size,b->entry.pools,b->default_heap));
        return 0;
    }
    CHECK(resumed==GH_ACTOR_MOTION_UNSUPPORTED&&move.frontier==0x26ef60&&move.impact_create.complete);
    CHECK(move.reaction_resource_offset==(e[0x640]==19?0x784u:0x2e0u)&&
        word(e+0x430)==0x3f800000&&word(e+0x2f4)==0x00010101&&word(e+0x600)==0x41700000&&
        word(e+0x5f0)==9&&word(e+0x5f4)==92&&!(word(e+0x434)&8u));
    CHECK(!gh_enemy202_move_bind_opacity(&move,&opacity));
    const gh_actor_motion_status finished=gh_enemy202_move_00208be0(&move,enemy,&player->view,elf,elf_size);
    if(finished)fprintf(stderr,"Triangle final status%d PC%08x\n",finished,move.frontier);
    CHECK(!finished&&move.original_returned&&word(e+0x430)==0x3f800000);
    CHECK(!gh_hitarea_object_set_active_001fd668(&tail->give_objects[4],0,&pc));
    for(unsigned frame=2;frame<=3;frame++){
        CHECK(!gh_collision_pairs_step_002b9d58(b->shapes.pairs,GH_STATIC_BINARY32_REFERENCE,&pc));
        CHECK(!gh_hitarea_frame_resume_bound_001fdfd8(b->entry.init,b->entry.work,1,
            GH_STATIC_BINARY32_REFERENCE,ledger,lb,&pc));
        gh_enemy202_move next={0};
        CHECK(!gh_enemy202_move_bind_opacity(&next,&opacity));
        const gh_actor_motion_status step=gh_enemy202_move_00208be0(&next,enemy,&player->view,elf,elf_size);
        if(step)fprintf(stderr,"Triangle continuation frame%u PC%08x stage%u\n",frame,next.frontier,next.stage);
        float sample;
        memcpy(&sample,e+0x430,4);
        CHECK(!step&&next.original_returned&&sample==(float)frame);
    }
    CHECK((word(e+0x548)>>16)==hp-12&&word(b->entry.init->heat+4)==0x40000000);

    puts("PASS actual PL00 Triangle clip -> R100 Give4 ledger -> original stamp Take");
    return 0;
}
#undef VERIFY

static int first_punch(gh_disc *disc, const gh_bundle *room_bundle, const uint8_t *elf, size_t elf_size,
    const gh_enemy_ems_bindings *bindings, gh_enemy_ems *owner, gh_enemy_ems_live *first,
    const gh_enemy_ems_live *appearance2, const gh_enemy_ems_live *selector1,
    const gh_enemy_ems_live *stamp10,const gh_enemy_ems_live *stamp11,
    const gh_enemy_ems_live selector1_other[3], const gh_enemy_ems_live *weapon_live,
    const gh_room_state *room, const gh_scene_nearcheck *navigation,
    const gh_effect_create_bindings *effect_bind, const gh_game_world_light *light,
    gh_enemy202_impact_request *output,int attack_effect)
{
    const gh_enemy_actor_bindings *b = &bindings->actor;
    gh_game_init_state *init = b->entry.init;
    gh_collision_pairs *pairs = b->shapes.pairs;
    const gh_enemy_ems_live *target=attack_effect>=2?&selector1_other[1+(attack_effect&1)]:first;
    gh_game_title_player *enemy = target->actor;
    gh_heap *heap = b->entry.room_heap;
    gh_game_objects *global = b->entry.global_objects;
    const gh_hitarea_object *body = &target->enemy202->hitareas.units[0];
    CHECK((attack_effect>=2 || (first->physical_row == 0 && enemy->view.bytes[0x640] == 0)) &&
        body->state == init && body->group == 2 && target->enemy202->hitareas.created[0]);
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
    location[1] -= attack_effect>=2 ? 0.75f : 1.15f;
    location[0] -= matrix[8]*0.7f;
    location[2] -= matrix[10]*0.7f;
    memcpy(p+0xb0,location,12);
    memcpy(player.view.alias_f0,location,12);
    memcpy(p+0x100,euler,12);
    gh_hitarea_frame ledger = {0};
    const uint32_t pause[2] = {0};
    gh_hitarea_frame_bindings lb = {.tables=b->actors,.boot_elf=elf,.boot_elf_size=elf_size,
        .current_player=&player.view,.player_pause_words=pause};
    CHECK(attack_effect>=2 || word(e+0x54c) == 0x40a00000);
    for (unsigned i=0;i<5;i++) {
        CHECK(!gh_collision_pairs_step_002b9d58(pairs,GH_STATIC_BINARY32_REFERENCE,&frontier));
        CHECK(!gh_hitarea_frame_resume_bound_001fdfd8(
            init,b->entry.work,1,GH_STATIC_BINARY32_REFERENCE,&ledger,&lb,&frontier));
    }
    CHECK(!word(e+0x54c));
    if(attack_effect>=2){
        camera_target_fixture resolver={.enemies=owner,.player=&factory,.child=&child};
        CHECK(!triangle_ledger(b,target->enemy202,&player,&setup.model,pl,pl_size,elf,elf_size,
            &tail,&resolver,&lb,&ledger,effect_bind,room,target,attack_effect));
        goto release_player;
    }
    /* Original skill29 class1 Give4, before the class0 Square resets size. */
    const size_t triangle_power=0x3bc7d0-0xff000+29*0x14;
    CHECK(triangle_power+4<=elf_size&&word(elf+triangle_power)==12);
    p[0x3ac]=1;
    memcpy(p+0x1610,&(float){1},4);
    memcpy(p+0x1660,&(uint32_t){1},4);
    memcpy(p+0x1668,elf+triangle_power,4);
    memcpy(p+0x1670,&(uint32_t){0x12},4);
    p[0x1680]=10;
    CHECK(!gh_player_attack_hitarea_0010a438(&player.view,tail.give_objects,&frontier));
    CHECK(word(box.record+0xe0+0x44)==0x3f000000&&word(box.record+0xe0+0x48)==0x3f333333);
    CHECK(word(box.record+0xe0+0x110)==0x3fe66666&&word(box.record+0xe0+0x114)==0x3fc00000&&
        word(box.record+0xe0+0x118)==0x3fe66666);
    CHECK(tail.give_objects[4].bytes[0x46]==0x12&&word(tail.give_objects[4].bytes+0x4c)==12);
    puts("PASS actual R100/PL00 skill29 Give4 class1 size, offset, effect and power");
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
    CHECK(word(box.record+0xe0+0x110)==0x3f333333&&word(box.record+0xe0+0x114)==0x3f800000&&
        word(box.record+0xe0+0x118)==0x3f99999a);
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
    CHECK(!first_waiting_opacity_draw(selector1,light,elf,elf_size));
    size_t scenery_bytes;
    uint8_t *scenery=load(disc,"om/om04.dat",&scenery_bytes);
    CHECK(scenery&&!gh_game_objects_add_pinned_001fece0(global,0x304,scenery,scenery_bytes,&original)&&
        original&&!gh_game_objects_set_file_extent(global,0x304,scenery,scenery_bytes));
    CHECK(!scenery_contact(room_bundle,0x304,room,b,waiting_enemy,&player.view,&lb,&ledger,effect_bind,navigation,&opacity));
    CHECK(stamp11->actor->view.bytes[0x640]==11 && word(stamp11->actor->view.bytes+0x564)==0x203);
    move=(gh_enemy202_move){0};
    CHECK(!gh_enemy202_move_bind_opacity(&move,&opacity));
    CHECK(!gh_enemy202_move_00208be0(&move,stamp11->enemy202,&player.view,elf,elf_size)&&move.original_returned);
    CHECK(!scenery_contact(room_bundle,0x304,room,b,stamp11->enemy202,&player.view,&lb,&ledger,effect_bind,navigation,&opacity));
    size_t barrel_bytes;
    uint8_t *barrel=load(disc,"om/om03.dat",&barrel_bytes);
    CHECK(barrel&&!gh_game_objects_add_pinned_001fece0(global,0x303,barrel,barrel_bytes,&original)&&
        original&&!gh_game_objects_set_file_extent(global,0x303,barrel,barrel_bytes));
    CHECK(!scenery_contact(room_bundle,0x303,room,b,waiting_enemy,&player.view,&lb,&ledger,effect_bind,navigation,&opacity));
    CHECK(!scenery_contact(room_bundle,0x303,room,b,stamp11->enemy202,&player.view,&lb,&ledger,effect_bind,navigation,&opacity));
    const uint32_t stamp10_contact[3]={0xc0266666,0x3f266666,0xc1cb3333};
    uint8_t *stamp10_position=stamp10->actor->view.alias_f0;
    uint8_t prior_stamp10_position[12];
    memcpy(prior_stamp10_position,stamp10_position,sizeof(prior_stamp10_position));
    memcpy(stamp10_position,stamp10_contact,sizeof(stamp10_contact));
    CHECK(!scenery_contact(room_bundle,0x303,room,b,stamp10->enemy202,&player.view,&lb,&ledger,effect_bind,navigation,&opacity));
    memcpy(stamp10_position,prior_stamp10_position,sizeof(prior_stamp10_position));
    free(barrel);
    free(scenery);
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
    }
    gh_boot_work *work=b->entry.work;
    const uint32_t counter=word((const uint8_t*)work+0x210);
    CHECK(!word((const uint8_t*)work+0x1a0));
    CHECK(!gh_frame_rate_policy(work));
    gh_frame_random frame_random={.policy=GH_FRAME_CHOP_REFERENCE};
    gh_screen_fade fade={0};
    gh_screen_fade_context fade_context={512,2048-224,word((const uint8_t*)work+0x564),
        word((const uint8_t*)work+0x5e0)};
    CHECK(!gh_screen_fade_start_00307208(&fade,&fade_context,0,8,0,0xff000000u,0,15));
    gh_effect_release_bindings release=effect_bind->storage;
    release.actor_match=gh_effect_create_actor_match;
    release.context=release.effects;
    CHECK(weapon_live->weapon==gh_enemy_timers_held_weapon(&selector1_other[0].enemy202->timers));
    for(unsigned tick=0;tick<35;tick++){
        CHECK(!gh_frame_rate_policy(work)&&!gh_frame_counter_advance(work) &&
            !gh_frame_random_before_tasks_0030b008(&frame_random,&init->random,work));
        CHECK(word((const uint8_t*)work+0x210)==counter+tick+1);
        CHECK(!gh_collision_pairs_step_002b9d58(pairs,GH_STATIC_BINARY32_REFERENCE,&frontier));
        for(unsigned i=0;i<3;i++){
            gh_enemy_actor *entry=selector1_other[i].enemy202;
            uint8_t *actor=selector1_other[i].actor->view.bytes;
            gh_enemy202_move *frame=selector1_other[i].move202;
            if(frame->original_returned)*frame=(gh_enemy202_move){0};
            CHECK(!gh_enemy202_move_bind_room_state(frame,room));
            CHECK(!gh_enemy202_move_bind_navigation(frame,navigation));
            CHECK(!gh_enemy202_move_bind_opacity(frame,&opacity));
            gh_actor_motion_status status=gh_enemy202_move_00208be0(frame,entry,
                &player.view,elf,elf_size);
            if(status)fprintf(stderr,"selector1 stamp%u tick%u status%d pc=%08x stage=%u state=%08x\n",
                actor[0x640],tick,status,frame->frontier,frame->substate67_stage,word(actor+0x2f4));
            CHECK(!status && !frame->frontier && frame->original_returned);
            if(!tick)CHECK(word(actor+0x2f4)==0x00010100 && word(actor+0x5f0)==(i==0?30:29) &&
                frame->substate67_motion.motion_offset==word(entry->entry.resource.bytes+rows[i][0]) &&
                entry->motion.motion.sequence_offset==word(entry->entry.resource.bytes+rows[i][1]));
            if(i==0){
                gh_enemy_other_move *held=weapon_live->other_move;
                if(held->original_returned)*held=(gh_enemy_other_move){0};
                status=gh_enemy363_move_001c9860(held,weapon_live->weapon,release_held_effect,&release);
                if(status)fprintf(stderr,"selector1 held tick%u status%d pc=%08x\n",tick,status,held->frontier);
                CHECK(!status && !held->frontier && held->original_returned);
            }
        }
        int special;
        CHECK(!gh_enemy_manager_step_rates_00290250(init,work->canonical_rate574,&special));
        CHECK(!gh_hitarea_frame_resume_bound_001fdfd8(init,work,work->canonical_rate574,
            GH_STATIC_BINARY32_REFERENCE,&ledger,&lb,&frontier));
        gh_fade_draw draw;
        CHECK(!gh_screen_fade_step_00307388(&fade,&fade_context,&draw));
        CHECK(draw.visible==(tick<=15) && (!tick?draw.rgba==0xff000000u:1) &&
            (tick<15?fade.state==9:fade.state==0));
        if(tick==15)CHECK(!draw.rgba && draw.test==0x70000);
        if(tick==34)for(unsigned i=0;i<3;i++){
            const uint8_t *actor=selector1_other[i].actor->view.bytes;
            fprintf(stderr,"selector1 neutral stamp%u state=%08x cooldown=%u flags=%08x sample=%08x\n",
                actor[0x640],word(actor+0x2f4),word(actor+0x5f0),word(actor+0x16d0),word(actor+0x430));
            CHECK(word(actor+0x2f4)==0x00010100 && word(actor+0x5f0)==(i==0?30:0) &&
                word(actor+0x16d0)==(i==0?0x04009400u:0x04008400u) && word(actor+0x430)==0x41100000u);
        }
    }
    gh_enemy_actor *turner=appearance2->enemy202;
    uint8_t *turning=appearance2->actor->view.bytes;
    uint8_t *raw_work=(uint8_t *)b->entry.work;
    const uint32_t old_tick=word(raw_work+0x210),old_rate=word(raw_work+0x55c);
    CHECK(turning[0x1650]==2 && word(turning+0x564)==0x200 &&
        turning[0x2f5]==0x71 && turning[0x2f6]==0);
    /* Exercise the selector reached by the separately verified 0x71 exit.
     * Use one nonperiodic Work tick and an initial yaw requiring a turn. */
    memcpy(raw_work+0x210,&(uint32_t){word(turning+0x17d0)^1u},4);
    memcpy(raw_work+0x55c,&(uint32_t){0x3f800000},4);
    memcpy(turning+0x2f4,&(uint32_t){0x00000300},4);
    float turn_origin[3],turn_target[3],bearing;
    memcpy(turn_origin,appearance2->actor->view.alias_f0,12);
    memcpy(turn_target,player.view.alias_f0,12);
    CHECK(!gh_player_target_angle_002da038(elf,elf_size,turn_origin,turn_target,0,3.1415927f,
        &bearing,&frontier));
    float initial_yaw=bearing-0.7f;
    CHECK(!gh_id_angle_reference_001f7d48(initial_yaw,&initial_yaw));
    memcpy(turning+0x104,&initial_yaw,4);
    unsigned turn_ticks=0;
    do{
        move=(gh_enemy202_move){0};
        CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
        CHECK(!gh_enemy202_move_bind_opacity(&move,&opacity));
        gh_actor_motion_status status=gh_enemy202_move_00208be0(&move,turner,&player.view,elf,elf_size);
        if(status)fprintf(stderr,"selector3 tick%u status%d pc=%08x stage=%u state=%08x\n",
            turn_ticks,status,move.frontier,move.substate67_stage,word(turning+0x2f4));
        CHECK(!status&&!move.frontier&&move.original_returned&&turning[0x2f5]==3);
        if(turn_ticks<2){
            const unsigned offset=turn_ticks?0xb0:0x98;
            CHECK(turning[0x2f6]==2+turn_ticks &&
                move.substate67_motion.motion_offset==word(turner->entry.resource.bytes+offset) &&
                turner->motion.motion.sequence_offset==word(turner->entry.resource.bytes+offset+4));
        }
        turn_ticks++;
    }while(turning[0x2f6]!=0 && turn_ticks<16);
    CHECK(turn_ticks>2 && turn_ticks<16 && turning[0x2f6]==0);
    /* Exercise the positive periodic route using the same native R100
     * collision table, with the player placed half a unit from the actor. */
    float old_player[3],old_previous[3];
    memcpy(old_player,player.view.alias_f0,12);
    memcpy(old_previous,player.view.bytes+0x630,12);
    memcpy(turn_origin,appearance2->actor->view.alias_f0,12);
    memcpy(turn_target,turn_origin,12);
    turn_target[0]+=0.5f;
    memcpy(player.view.alias_f0,turn_target,12);
    memcpy(player.view.bytes+0x630,turn_target,12);
    memcpy(raw_work+0x210,turning+0x17d0,4);
    const gh_game_mt saved_rng=init->random;
    gh_game_mt_seed_0030f2b0(&init->random,1);
    move=(gh_enemy202_move){0};
    CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
    CHECK(!gh_enemy202_move_bind_opacity(&move,&opacity));
    const gh_actor_motion_status route=gh_enemy202_move_00208be0(&move,turner,
        &player.view,elf,elf_size);
    fprintf(stderr,"selector3 periodic status%d pc=%08x route=%u state=%08x flags=%08x\n",
        route,move.frontier,move.route_stage,word(turning+0x2f4),word(turning+0x16d0));
    CHECK(!route && !move.frontier && move.original_returned && move.route_stage==4 &&
        word(turning+0x2f4)==0x00006b00 && (word(turning+0x16d0)&0x8002u)==2u);
    CHECK(turning[0x530] && (word(turning+0x16d4)&0x08000000u) &&
        !memcmp(turning+0x16a0,turning+0x520,12));
    init->random=saved_rng;
    memcpy(player.view.alias_f0,old_player,12);
    memcpy(player.view.bytes+0x630,old_previous,12);
    memcpy(raw_work+0x210,&old_tick,4);
    memcpy(raw_work+0x55c,&old_rate,4);
    {
        gh_enemy_actor *entry=selector1_other[2].enemy202;
        uint8_t *actor=entry->entry.object.view.bytes;
        const uint32_t old_player_yaw=word(player.view.bytes+0x104);
        const uint32_t radius=word(actor+0x16c8);
        const uint8_t retreat=init->enemies[0x53b];
        const uint32_t shared_timer=word(init->enemies+0x510);
        uint8_t *score=b->entry.save.bytes+0x1c;
        uint16_t score_before;
        memcpy(&score_before,score,2);
        camera_target_fixture target_fixture={.enemies=owner,.player=&factory,.child=&child};
        const gh_player_target_bindings targets={.pools=b->entry.pools,.actors=b->actors,
            .player=&player.view,.resolve=resolve_camera_target,.context=&target_fixture,.elf=elf,.elf_bytes=elf_size};
        gh_title_player_view *peer=&selector1_other[1].actor->view;
        uint32_t peer_handle;
        CHECK(!gh_enemy_ems_handle_for_actor(owner,selector1_other[1].actor,&peer_handle)&&peer_handle);
        const uint32_t peer_state=word(peer->bytes+0x2f4);
        const uint8_t peer_hidden=peer->bytes[0x617];
        float peer_position[3];
        memcpy(peer_position,peer->alias_f0,12);
        const gh_game_mt rng=init->random;
        uint8_t actor_before[0x1944];
        memcpy(actor_before,actor,sizeof(actor_before));
        const gh_actor_motion motion_before=entry->motion.motion;
        CHECK(actor[0x640]==20&&word(actor+0x564)==0x200&&!word(actor+0x6ec));
        float origin[3],target[3],enemy_yaw,player_yaw;
        memcpy(origin,entry->entry.object.view.alias_f0,12);
        memcpy(target,origin,12);
        target[2]+=4.645f;
        memcpy(player.view.alias_f0,target,12);
        memcpy(player.view.bytes+0x630,target,12);
        CHECK(!gh_player_target_angle_002da038(elf,elf_size,origin,target,0,3.1415927f,&enemy_yaw,&frontier));
        CHECK(!gh_player_target_angle_002da038(elf,elf_size,target,origin,0,3.1415927f,&player_yaw,&frontier));
        memcpy(player.view.bytes+0x104,&player_yaw,4);
        for(unsigned test=0;test<8;test++){
            memcpy(actor,actor_before,sizeof(actor_before));
            entry->motion.motion=motion_before;
            memcpy(actor+0x104,&enemy_yaw,4);
            memcpy(actor+0x2f4,&(uint32_t){0x00010000},4);
            memcpy(actor+0x16d0,&(uint32_t){test>=3&&test<=5?0x04200402u:0x04000402u},4);
            memcpy(actor+0x16d4,&(uint32_t){0x08000000},4);
            memcpy(actor+0x16c0,&(uint32_t){0},4);
            memcpy(actor+0x16ec,&(uint32_t){0},4);
            if(test==1)memcpy(actor+0x16c8,&(float){1.0f},4);
            init->enemies[0x53b]=(uint8_t)(test==2);
            memcpy(raw_work+0x210,&(uint32_t){word(actor+0x17d0)^(test>=6?0u:1u)},4);
            if(test>=6){
                const float matrix[16]={1,0,0,0,0,1,0,0,0,0,1,0,origin[0],origin[1],origin[2],1};
                memcpy(actor+0x80,matrix,sizeof(matrix));
                memcpy(peer->alias_f0,peer_position,12);
                memcpy(peer->bytes+0x2f4,&peer_state,4);
                peer->bytes[0x617]=peer_hidden;
            }
            memcpy(score,&(uint16_t){test>=4&&test<=5?1000:score_before},2);
            if(test==4||test==5)memcpy(actor+0x17a0,&(uint32_t){peer_handle},4);
            if(test==5||test==7){
                const float close[3]={origin[0]-(test==7?.2f:0),origin[1],origin[2]+1};
                memcpy(peer->alias_f0,close,12);
                memset(peer->bytes+0x2f4,0,4);
                peer->bytes[0x617]=0;
            }
            move=(gh_enemy202_move){0};
            CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
            CHECK(!gh_enemy202_move_bind_opacity(&move,&opacity));
            CHECK(!gh_enemy202_move_bind_player_aux_698(&move,NULL));
            CHECK(!gh_enemy202_move_bind_targets(&move,&targets));
            const gh_actor_motion_status status=gh_enemy202_move_00208be0(&move,entry,&player.view,elf,elf_size);
            fprintf(stderr,"actual stamp20 ordinary phase1 mode%u status%d pc%08x stage%u state%08x distance%08x rank%u heading%08x homeRadius%08x goto%08x/%u\n",
                test,status,move.frontier,move.substate67_stage,word(actor+0x2f4),word(actor+0x618),
                word(actor+0x16d8),word(actor+0x768),radius,word(actor+0x16c0),word(actor+0x16c4));
            CHECK(!status&&!move.frontier&&move.original_returned&&word(actor+0x618)>0x41800000&&
                word(actor+0x618)<0x41c80000);
            if(!test||test==3)CHECK(word(actor+0x2f4)==0x00010000&&word(actor+0x5f4)>=60&&word(actor+0x5f4)<=240);
            if(test==3)CHECK(word(actor+0x171c)==0x44160000&&word(actor+0x16d0)==0x04200402u);
            if(test>=4)fprintf(stderr,"stamp20 peer mode%u score%u peer%u state%08x lock%u shared%u delay%u\n",
                test,(uint16_t)word(score),word(actor+0x17a0),word(actor+0x2f4),
                word(actor+0x16dc),word(init->enemies+0x510),word(actor+0x16e0));
            if(test==4)CHECK(!word(actor+0x17a0)&&word(actor+0x2f4)==0x00000800&&(word(actor+0x16d4)&1u));
            if(test==5)CHECK(word(actor+0x17a0)==peer_handle&&word(actor+0x2f4)==0x00004200&&
                word(actor+0x16dc)==50&&word(init->enemies+0x510)>=15&&word(actor+0x16e0)==300);
            if(test>=6){
                fprintf(stderr,"periodic route mode%u flags%08x frame%u serial%u chosen%08x\n",
                    test,word(actor+0x16d4),word(raw_work+0x210),word(actor+0x17d0),word(actor+0x2f4));
                CHECK(((word(raw_work+0x210)^word(actor+0x17d0))&7u)==0);
                CHECK(word(actor+0x2f4)==(test==6?0x00010000u:0x01000900u));
            }
            if(test==2)CHECK(word(actor+0x2f4)==0x00006c00);
            if(test==1)CHECK(word(actor+0x2f4)==0x00001300&&word(actor+0x16c0)==0x43e10000&&
                word(actor+0x16c4)==6&&!(word(actor+0x16d0)&2u)&&!memcmp(actor+0x16b0,actor+0x1660,12));
        }
        memcpy(peer->alias_f0,peer_position,12);
        memcpy(peer->bytes+0x2f4,&peer_state,4);
        peer->bytes[0x617]=peer_hidden;
        gh_title_player_view *rank_peer[3];
        float rank_positions[3][3];
        uint8_t rank_hidden[3];
        unsigned rank_count=0;
        CHECK(!init->enemies[0x518]);
        for(unsigned i=0;i<64&&rank_count<3;i++){
            gh_game_title_player *candidate=init->enemy_actors[i];
            if(!candidate||candidate==&entry->entry.object||!candidate->view.alias_f0)continue;
            const unsigned kind=word(candidate->view.bytes+0x2fe)&0xffffu;
            if(kind<0x200||kind>=0x280)continue;
            rank_peer[rank_count]=&candidate->view;
            memcpy(rank_positions[rank_count],candidate->view.alias_f0,12);
            rank_hidden[rank_count++]=candidate->view.bytes[0x617];
        }
        CHECK(rank_count==3);
        memcpy(target,origin,12);
        target[2]+=6.787f;
        memcpy(player.view.alias_f0,target,12);
        memcpy(player.view.bytes+0x630,target,12);
        memcpy(score,&(uint16_t){0},2);
        for(unsigned selector=5;selector<=6;selector++)
        for(unsigned promoted=0;promoted<2;promoted++){
            memcpy(actor,actor_before,sizeof(actor_before));
            entry->motion.motion=motion_before;
            init->random=rng;
            for(unsigned i=0;i<rank_count;i++){
                const float closer[3]={target[0]+2.0f+i,target[1],target[2]};
                memcpy(rank_peer[i]->alias_f0,promoted?closer:rank_positions[i],12);
                rank_peer[i]->bytes[0x617]=promoted?0:rank_hidden[i];
            }
            init->enemies[0x53b]=0;
            memcpy(init->enemies+0x510,&shared_timer,4);
            memcpy(raw_work+0x210,&(uint32_t){word(actor+0x17d0)^1u},4);
            memcpy(actor+0x2f4,&(uint32_t){selector<<8},4);
            memcpy(actor+0x16d0,&(uint32_t){0x04000002},4);
            memcpy(actor+0x16d4,&(uint32_t){0x08000000},4);
            memcpy(actor+0x1760,&(uint32_t){0},4);
            memcpy(actor+0x104,&enemy_yaw,4);
            memcpy(actor+0x16a0,target,12);
            gh_game_mt expected=rng;
            uint32_t draw;
            CHECK(!gh_game_mt_next_0030f348(&expected,&draw));
            move=(gh_enemy202_move){0};
            CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
            CHECK(!gh_enemy202_move_bind_opacity(&move,&opacity));
            CHECK(!gh_enemy202_move_bind_player_aux_698(&move,NULL));
            CHECK(!gh_enemy202_move_bind_targets(&move,&targets));
            const gh_actor_motion_status status=gh_enemy202_move_00208be0(&move,entry,&player.view,elf,elf_size);
            fprintf(stderr,"stamp20 selector%u promoted%u status%d pc%08x state%08x distance%08x rank%u\n",
                selector,promoted,status,move.frontier,word(actor+0x2f4),word(actor+0x618),word(actor+0x16d8));
            CHECK(!status&&!move.frontier&&move.original_returned);
            CHECK(word(actor+0x618)>0x42340000&&word(actor+0x618)<0x42440000&&
                (selector==5||word(actor+0x5f0)==89)&&word(actor+0x5f4)==120+draw%60u&&
                move.substate67_motion.motion_offset==word(entry->entry.resource.bytes+(selector==5?0xb0:0xc0))&&
                !memcmp(&expected,&init->random,sizeof(expected)));
            CHECK(promoted?word(actor+0x16d8)>=3&&!actor[0x2f6]&&actor[0x2f5]!=selector:
                word(actor+0x16d8)<3&&word(actor+0x2f4)==(0x00010000u|selector<<8));
        }
        for(unsigned i=0;i<rank_count;i++){
            memcpy(rank_peer[i]->alias_f0,rank_positions[i],12);
            rank_peer[i]->bytes[0x617]=rank_hidden[i];
        }
        puts("PASS actual stamp20 selectors 5/6 in the original distance band, both original rank outcomes");
        memcpy(actor,actor_before,sizeof(actor_before));
        entry->motion.motion=motion_before;
        init->enemies[0x53b]=retreat;
        memcpy(init->enemies+0x510,&shared_timer,4);
        memcpy(score,&score_before,2);
        memcpy(peer->alias_f0,peer_position,12);
        memcpy(peer->bytes+0x2f4,&peer_state,4);
        peer->bytes[0x617]=peer_hidden;
        init->random=rng;
        memcpy(player.view.alias_f0,old_player,12);
        memcpy(player.view.bytes+0x630,old_previous,12);
        memcpy(player.view.bytes+0x104,&old_player_yaw,4);
        memcpy(raw_work+0x210,&old_tick,4);
    }
    {
        gh_enemy_actor *near=selector1_other[1].enemy202;
        uint8_t *actor=near->entry.object.view.bytes;
        const uint32_t old_player_yaw=word(player.view.bytes+0x104);
        float actor_xyz[3],target_xyz[3],enemy_yaw,player_yaw;
        memcpy(actor_xyz,near->entry.object.view.alias_f0,12);
        memcpy(target_xyz,actor_xyz,12);
        target_xyz[2]+=1.005f;
        memcpy(player.view.alias_f0,target_xyz,12);
        memcpy(player.view.bytes+0x630,target_xyz,12);
        CHECK(!gh_player_target_angle_002da038(elf,elf_size,actor_xyz,target_xyz,0,3.1415927f,&enemy_yaw,&frontier));
        CHECK(!gh_player_target_angle_002da038(elf,elf_size,target_xyz,actor_xyz,0,3.1415927f,&player_yaw,&frontier));
        memcpy(actor+0x104,&enemy_yaw,4);
        memcpy(player.view.bytes+0x104,&player_yaw,4);
        memcpy(actor+0x2f4,&(uint32_t){0},4);
        memcpy(raw_work+0x210,&(uint32_t){word(actor+0x17d0)^1u},4);
        CHECK(actor[0x640]==19 && word(actor+0x564)==0x203);
        move=(gh_enemy202_move){0};
        CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
        CHECK(!gh_enemy202_move_bind_opacity(&move,&opacity));
        CHECK(!gh_enemy202_move_bind_player_aux_698(&move,NULL));
        gh_actor_motion_status status=gh_enemy202_move_00208be0(&move,near,&player.view,elf,elf_size);
        fprintf(stderr,"stamp19 near status%d pc%08x stage%u state%08x dist=%08x route=%08x behavior=%08x rank=%u heading=%08x\n",
            status,move.frontier,move.substate67_stage,word(actor+0x2f4),word(actor+0x618),
            word(actor+0x16d4),word(actor+0x16d0),word(actor+0x16d8),word(actor+0x768));
        CHECK(!status&&!move.frontier&&move.original_returned &&
            word(actor+0x2f4)==0x00010000 && word(actor+0x16d0)==0x04000402 &&
            word(actor+0x5f4)>=60 && word(actor+0x5f4)<=240 && (word(actor+0x5f4)-60)%20==0);
        CHECK(!gh_id_angle_reference_001f7d48(player_yaw+3.1415927f,&player_yaw));
        memcpy(player.view.bytes+0x104,&player_yaw,4);
        memcpy(actor+0x16d4,&(uint32_t){0x08000000},4);
        move=(gh_enemy202_move){0};
        CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
        CHECK(!gh_enemy202_move_bind_opacity(&move,&opacity));
        CHECK(!gh_enemy202_move_bind_player_aux_698(&move,NULL));
        status=gh_enemy202_move_00208be0(&move,near,&player.view,elf,elf_size);
        fprintf(stderr,"stamp19 near route status%d pc%08x stage%u state%08x route%08x heading%08x\n",
            status,move.frontier,move.substate67_stage,word(actor+0x2f4),word(actor+0x16d4),word(actor+0x768));
        CHECK(!status&&!move.frontier&&move.original_returned &&
            word(actor+0x2f4)==0x00000900 && (word(actor+0x16d4)&0x08000000u));
        camera_target_fixture target_fixture={.enemies=owner,.player=&factory,.child=&child};
        const gh_player_target_bindings targets={.pools=b->entry.pools,.actors=b->actors,
            .player=&player.view,.resolve=resolve_camera_target,.context=&target_fixture,.elf=elf,.elf_bytes=elf_size};
        CHECK(weapon_live->actor&&weapon_live->weapon);
        const gh_title_player_view *weapon=&weapon_live->actor->view;
        CHECK((word(weapon->bytes+0x2fc)>>16)==0x363 && !weapon->bytes[0x664]);
        uint8_t old_weapon_position[12];
        memcpy(old_weapon_position,weapon->alias_f0,12);
        const uint32_t old_weapon_flags=word(weapon->bytes+0x250);
        const uint32_t old_weapon_timer=word(weapon->bytes+0x66c);
        float weapon_xyz[3]={actor_xyz[0],actor_xyz[1],actor_xyz[2]-3};
        memcpy(weapon->alias_f0,weapon_xyz,12);
        memcpy(weapon->bytes+0x250,&(uint32_t){old_weapon_flags&~0x80008000u},4);
        memcpy(weapon->bytes+0x66c,&(uint32_t){0},4);
        weapon->bytes[0x664]=1; /* Original 1D07E0 available floor-weapon input. */

        memcpy(actor+0x2f4,&(uint32_t){0x00010000},4);
        memcpy(raw_work+0x210,actor+0x17d0,4);
        move=(gh_enemy202_move){0};
        CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
        CHECK(!gh_enemy202_move_bind_opacity(&move,&opacity));
        CHECK(!gh_enemy202_move_bind_player_aux_698(&move,NULL));
        CHECK(!gh_enemy202_move_bind_targets(&move,&targets));
        status=gh_enemy202_move_00208be0(&move,near,&player.view,elf,elf_size);
        fprintf(stderr,"stamp19 periodic weapon status%d pc%08x stage%u state%08x available=%u timer=%08x goto=%08x/%u\n",
            status,move.frontier,move.substate67_stage,word(actor+0x2f4),weapon->bytes[0x664],
            word(weapon->bytes+0x66c),word(actor+0x16c0),word(actor+0x16c4));
        CHECK(!status&&!move.frontier&&move.original_returned && word(actor+0x2f4)==0x00001300);
        CHECK(word(weapon->bytes+0x66c)==0x43e10000 && word(actor+0x16c0)==0x43960000 &&
            word(actor+0x16c4)==8 && !memcmp(actor+0x16b0,weapon_xyz,12));
        memcpy(weapon->alias_f0,old_weapon_position,12);
        memcpy(weapon->bytes+0x250,&old_weapon_flags,4);
        memcpy(weapon->bytes+0x66c,&old_weapon_timer,4);
        weapon->bytes[0x664]=0;
        memcpy(actor+0x16c0,&(uint32_t){0},4);
        memcpy(actor+0x16c4,&(uint32_t){0},4);

        float current_actor[3];
        memcpy(current_actor,near->entry.object.view.alias_f0,12);
        memcpy(target_xyz,current_actor,12);
        target_xyz[2]+=1.005f;
        memcpy(player.view.alias_f0,target_xyz,12);
        memcpy(player.view.bytes+0x630,target_xyz,12);
        CHECK(!gh_player_target_angle_002da038(elf,elf_size,current_actor,target_xyz,0,3.1415927f,&enemy_yaw,&frontier));
        CHECK(!gh_player_target_angle_002da038(elf,elf_size,target_xyz,current_actor,0,3.1415927f,&player_yaw,&frontier));
        memcpy(actor+0x104,&enemy_yaw,4);
        memcpy(player.view.bytes+0x104,&player_yaw,4);
        memcpy(actor+0x2f4,&(uint32_t){0x00010000},4);
        memcpy(actor+0x16d4,&(uint32_t){0x08000000},4);
        memcpy(init->enemies+0x510,&(uint32_t){0},4);
        memcpy(init->enemies+0x514,&(uint32_t){0},4);
        gh_game_mt_seed_0030f2b0(&init->random,attack_effect?1:4);
        move=(gh_enemy202_move){0};
        CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
        CHECK(!gh_enemy202_move_bind_opacity(&move,&opacity));
        CHECK(!gh_enemy202_move_bind_player_aux_698(&move,NULL));
        CHECK(!gh_enemy202_move_bind_targets(&move,&targets));
        status=gh_enemy202_move_00208be0(&move,near,&player.view,elf,elf_size);
        fprintf(stderr,"stamp19 periodic attack status%d pc%08x stage%u state%08x distance=%08x flags=%08x phase=%u class=%u rng=%u timer=%u/%u\n",
            status,move.frontier,move.substate67_stage,word(actor+0x2f4),word(actor+0x618),
            word(actor+0x16d0),actor[0x2f6],word(actor+0x474),init->random.index,
            word(actor+0x16dc),word(init->enemies+0x510));
        CHECK(!status&&!move.frontier&&move.original_returned);
        CHECK(word(actor+0x2f4)==0x00001500 && (word(actor+0x16d0)&0x200000u) &&
            word(actor+0x16dc)==50 && word(init->enemies+0x510)==15 &&
            word(actor+0x171c)==0x44160000u);

        near->motion.motion.b.sound=record_stamp19_attack_sound;
        near->motion.motion.b.sound_context=near;
        uint32_t first_attack_row=0;
        unsigned attack_frames=0, active_frames=0;
        for(;attack_frames<(attack_effect?240u:100u)&&actor[0x2f5]==0x15;attack_frames++){
            memcpy(raw_work+0x210,&(uint32_t){word(actor+0x17d0)^1u},4);
            move=(gh_enemy202_move){0};
            CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
            CHECK(!gh_enemy202_move_bind_opacity(&move,&opacity));
            CHECK(!gh_enemy202_move_bind_player_aux_698(&move,NULL));
            CHECK(!gh_enemy202_move_bind_targets(&move,&targets));
            CHECK(!gh_enemy202_move_bind_impact(&move,effect_bind));
            status=gh_enemy202_move_00208be0(&move,near,&player.view,elf,elf_size);
            if(!attack_frames)first_attack_row=move.reaction_resource_offset;
            if(attack_effect&&attack_frames==1)
                CHECK(!verify_stamp19_attack_effect(&move,effect_bind,near,&player.view,init,
                    b->actors,elf,elf_size));
            if(near->hitareas.units[4].bytes[0x46]) {
                CHECK(near->hitareas.units[4].bytes[0x46]==(attack_effect?0x3e:0x37) && word(near->hitareas.units[4].bytes+0x4c)==(attack_effect?60u:20u));
                active_frames++;
            }
            fprintf(stderr,"stamp19 attack15 frame%u status%d pc%08x stage%u row%04x motion%08zx seq%08zx state%08x flags%04x sample%08x Give4 %02x/%u\n",
                attack_frames,status,move.frontier,move.substate67_stage,move.reaction_resource_offset,
                move.substate67_motion.motion_offset,near->motion.motion.sequence_offset,
                word(actor+0x2f4),word(actor+0x3ac)&0xffff,word(actor+0x430),
                near->hitareas.units[4].bytes[0x46],word(near->hitareas.units[4].bytes+0x4c));
            CHECK(!status&&!move.frontier&&move.original_returned);
        }
        fprintf(stderr,"stamp19 attack15 terminal row %04x frames %u active %u state %08x\n",
            first_attack_row,attack_frames,active_frames,word(actor+0x2f4));
        if(attack_effect){
            CHECK(attack_frames>1&&attack_frames<240&&active_frames&&first_attack_row==0x6b0&&!word(actor+0x2f4));
            puts("PASS actual stamp19 selector15 58:2D part15 four emitter publication");
            memcpy(actor+0x2f4,&(uint32_t){0x00004b00},4);
            for(unsigned frame=0;frame<3;++frame){
                move=(gh_enemy202_move){0};
                CHECK(!gh_enemy202_move_bind_opacity(&move,&opacity));
                CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
                CHECK(!gh_enemy202_move_bind_player_aux_698(&move,NULL));
                CHECK(!gh_enemy202_move_bind_targets(&move,&targets));
                CHECK(!gh_enemy202_move_bind_impact(&move,effect_bind));
                status=gh_enemy202_move_00208be0(&move,near,&player.view,elf,elf_size);
                if(status)fprintf(stderr,"stamp19 selector4B frame%u status%d pc%08x stage%u\n",
                    frame,status,move.frontier,move.substate67_stage);
                CHECK(!status&&move.original_returned&&word(actor+0x2f4)==0x00014b00&&
                    (word(actor+0x16d4)&0x400)&&word(actor+0x5f0)==14-frame);
                if(!frame)CHECK(move.substate67_motion.motion_offset==word(near->entry.resource.bytes+0x7cc)&&
                    near->motion.motion.sequence_offset==word(near->entry.resource.bytes+0x7d0));
            }
            puts("PASS actual stamp19 selector4B motion, tracking, and three Move returns");
        }else CHECK(attack_frames==64 && active_frames==2 && first_attack_row==0x6a4 && word(actor+0x2f4)==0);
        if(!attack_effect){
            /* Only the HP and position are controlled. The original player box,
             * native collision ledger and original Take select the death mode. */
            const gh_hitarea_object *death_body=&near->hitareas.units[0];
            gh_collision_pair_shape death_sphere;
            CHECK(!gh_hitarea_node_at(death_body,0,&record,&death_sphere));
            CHECK(!gh_collision_pair_shape_position(&death_sphere,&center));
            memcpy(location,center,12);
            location[1]-=1.15f;location[0]-=matrix[8]*.7f;location[2]-=matrix[10]*.7f;
            memcpy(p+0xb0,location,12);
            memcpy(player.view.alias_f0,location,12);memcpy(player.view.bytes+0x630,location,12);
            memcpy(p+0x100,euler,12);
            p[0x3ac]=2;
            memcpy(actor+0x54a,&(uint16_t){1},2);
            memcpy(actor+0x54c,&(uint32_t){0},4);
            CHECK(!gh_player_attack_hitarea_0010a438(&player.view,tail.give_objects,&frontier));
            CHECK(!gh_collision_pairs_step_002b9d58(pairs,GH_STATIC_BINARY32_REFERENCE,&frontier));
            CHECK(!gh_hitarea_frame_resume_bound_001fdfd8(init,work,1,GH_STATIC_BINARY32_REFERENCE,&ledger,&lb,&frontier));
            CHECK(!gh_hitarea_object_last_hit_actor(death_body,&source)&&source==&player.view&&
                death_body->bytes[0x46]==7&&word(death_body->bytes+0x4c)==3);
            gh_enemy202_hit_call lethal={0};
            status=gh_enemy202_hit_0020a178(&lethal,near,&player.view,1,&frontier);
            fprintf(stderr,"stamp19 lethal Take status%d pc%08x flags%04x behavior%08x statusflags%08x state%08x HP%u\n",
                status,frontier,word(actor+0x3ac)&0xffff,word(actor+0x16d0),word(actor+0x16d4),
                word(actor+0x2f4),word(actor+0x548)>>16);
            CHECK(!status&&lethal.reacted&&!frontier&&word(actor+0x2f4)==2&&!(word(actor+0x548)>>16));
            CHECK(!gh_hitarea_object_set_active_001fd668(&tail.give_objects[4],0,&frontier));
            CHECK(!gh_hitarea_frame_resume_bound_001fdfd8(init,work,1,GH_STATIC_BINARY32_REFERENCE,&ledger,&lb,&frontier));
            unsigned death_frames=0;
            uint32_t death_row=0;
            for(;death_frames<200&&actor[0x2f5]==0;death_frames++){
                move=(gh_enemy202_move){0};
                CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
                CHECK(!gh_enemy202_move_bind_opacity(&move,&opacity));
                CHECK(!gh_enemy202_move_bind_player_aux_698(&move,NULL));
                CHECK(!gh_enemy202_move_bind_targets(&move,&targets));
                CHECK(!gh_enemy202_move_bind_impact(&move,effect_bind));
                status=gh_enemy202_move_00208be0(&move,near,&player.view,elf,elf_size);
                if(!death_frames)death_row=move.reaction_resource_offset;
                if(status)fprintf(stderr,"stamp19 death frame%u status%d pc%08x phase%u row%04x flags%08x\n",
                    death_frames,status,move.frontier,move.death_phase,death_row,word(actor+0x16d0));
                CHECK(!status&&!move.frontier&&move.original_returned);
            }
            fprintf(stderr,"stamp19 death terminal after %u complete frames state%08x\n",death_frames,word(actor+0x2f4));
            CHECK(death_frames==39&&word(actor+0x2f4)==0x0202);
            unsigned emitters=0;
            for(unsigned slot=0;slot<256;++slot){
                gh_effect_emitter_view emitter;
                if(gh_effect_create_emitter_at(effect_bind->storage.effects,slot,&emitter)||
                   emitter.actor!=&near->entry.object.view)continue;
                CHECK(emitter.initialized&&emitter.selector==emitter.actor&&
                    emitter.model==selector1_other[1].model&&emitter.part==emitter.model->parts&&
                    emitter.payload[0x18c]==0x58&&emitter.payload[0x18e]==2&&emitter.payload[0x1ec]==0&&
                    word(emitter.payload+0x1e0)==0x3f800000);
                ++emitters;
            }
            CHECK(emitters==1);
            move=(gh_enemy202_move){0};
            CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
            CHECK(!gh_enemy202_move_bind_opacity(&move,&opacity));
            CHECK(!gh_enemy202_move_bind_player_aux_698(&move,NULL));
            CHECK(!gh_enemy202_move_bind_targets(&move,&targets));
            gh_audio_registry *death_registry;
            gh_se_voice_engine *death_voices;
            gh_se_mix death_mix;
            unsigned transport_identity=0;
            CHECK(!gh_audio_registry_create(&death_registry)&&
                !gh_se_mix_boot_defaults_002c9e78(&death_mix,0,GH_SE_DSP_NATIVE_LINEAR_REFERENCE)&&
                !gh_se_voice_create(death_registry,(gh_audio_output *)&transport_identity,&death_mix,&death_voices));
            stamp19_release_fixture removal={.owner=owner,.voices=death_voices,.reject_once=1};
            uint8_t *save_row=room->active+12+14*24;
            CHECK(save_row[0x13]==19&&!save_row[0x14]);
            uint32_t retiring_handle=0;
            CHECK(!gh_enemy_ems_handle_for_actor(owner,&near->entry.object,&retiring_handle)&&retiring_handle);
            const uint32_t prior_count=word(init->enemies+0x508);
            const unsigned prior_pity=word(init->enemies+0x534);
            gh_game_mt predicted=init->random;
            uint32_t first_draw,second_draw;
            CHECK(!gh_game_mt_next_0030f348(&predicted,&first_draw)&&
                !gh_game_mt_next_0030f348(&predicted,&second_draw)&&first_draw%50>0&&(second_draw&3u));
            near->motion.motion.b.effect=release_stamp19_effect;
            near->motion.motion.b.context=(void*)effect_bind;
            unsigned fade_frames=0;
            for(;fade_frames<120&&!removal.removed;++fade_frames){
                int special=0;
                CHECK(!gh_enemy_manager_step_rates_00290250(init,work->canonical_rate574,&special));
                move=(gh_enemy202_move){0};
                CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
                CHECK(!gh_enemy202_move_bind_opacity(&move,&opacity));
                CHECK(!gh_enemy202_move_bind_player_aux_698(&move,NULL));
                CHECK(!gh_enemy202_move_bind_targets(&move,&targets));
                CHECK(!gh_enemy202_move_bind_room_state(&move,room));
                CHECK(!gh_enemy202_move_bind_death_release(&move,release_stamp19_fixture,&removal));
                status=gh_enemy202_move_00208be0(&move,near,&player.view,elf,elf_size);
                if(status){
                    fprintf(stderr,"stamp19 fade frame%u status%d pc%08x phase%u original%u effects%08x\n",
                        fade_frames,status,move.frontier,move.death_fade_phase,actor[0x2f6],word(actor+0x16d4));
                    if(status==GH_ACTOR_MOTION_UNSUPPORTED&&move.frontier==0x290770)
                        status=gh_enemy202_move_00208be0(&move,near,&player.view,elf,elf_size);
                }
                if(status)fprintf(stderr,"fade retry status%d pc%08x remove stage%u mark%u removed%u\n",
                    status,move.frontier,removal.remove.stage,removal.remove.mark.stage,removal.removed);
                CHECK(!status&&!move.frontier&&move.original_returned);
                if(!fade_frames)CHECK(!memcmp(&init->random,&predicted,sizeof(predicted))&&
                    word(init->enemies+0x534)==prior_pity+1&&save_row[0x14]==1);
            }
            fprintf(stderr,"fade result frames%u removed%u state%08x alpha%08x timer%08x rate%08x flags%08x\n",
                fade_frames,removal.removed,word(actor+0x2f4),word(actor+0x24c),word(actor+0x600),word(actor+0x5a8),word(actor+0x250));
            CHECK(fade_frames&&fade_frames<120&&removal.removed==1&&word(init->enemies+0x508)==prior_count-1&&
                !word(actor+0x24c)&&actor[0x2f6]==4&&(word(actor+0x250)&0x80000000u));
            printf("PASS stamp19 original no-drop, %u fade frames, real 290770\n",fade_frames);
            gh_se_voice_destroy(death_voices);
            gh_audio_registry_destroy(death_registry);
            near->motion.motion.b.sound=NULL;
            near->motion.motion.b.sound_context=NULL;
            CHECK(!retire_stamp19_trans(owner,&selector1_other[1],retiring_handle,effect_bind,
                &player.view,init,b->actors,elf,elf_size,b->entry.pools,b->default_heap));

        }
        memcpy(player.view.alias_f0,old_player,12);
        memcpy(player.view.bytes+0x630,old_previous,12);
        memcpy(player.view.bytes+0x104,&old_player_yaw,4);
        memcpy(raw_work+0x210,&old_tick,4);
    }
    CHECK(!selector9_first_steps(stamp10,&player.view,elf,elf_size,room,navigation,&opacity));
    CHECK(!stamp10_return_home(stamp10,&player.view,elf,elf_size,room,navigation,&opacity));
    CHECK(!camera_target_00139ff8(owner,b,&factory,&child,&player,&setup.model,enemy,elf,elf_size));
    motion->b.sound=NULL;
    motion->b.sound_context=NULL;

    CHECK(!enemy_give_player_take(disc,room_bundle,first->enemy202,&player,&tail,&setup.model,pl,pl_size,&lb,&ledger,effect_bind));
release_player:
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
    if(attack_effect<2)printf("PASS actual R100 first 0202 contact through Give4 and production ledger, HP %u to %u\n",
        hp_before, hp_before-3);
    return 0;
}

typedef struct {
    const gh_title_player_view *actor;
    int32_t kinds[64];
    uint32_t modes[64];
    unsigned total, visited;
    int reject_once;
} death_effect_trace;

static gh_actor_motion_status record_death_effect(void *context,const gh_title_player_view *actor,
    int32_t kind,uint32_t mode,uint32_t *frontier)
{
    death_effect_trace *trace=context;
    *frontier=0;
    if(actor!=trace->actor||trace->visited>=trace->total||
       kind!=trace->kinds[trace->visited]||mode!=trace->modes[trace->visited])
        return GH_ACTOR_MOTION_ARGUMENT;
    if(trace->reject_once){
        trace->reject_once=0;
        *frontier=0x306200;
        return GH_ACTOR_MOTION_UNSUPPORTED;
    }
    ++trace->visited;
    return GH_ACTOR_MOTION_OK;
}

static unsigned death_half(const uint8_t *p)
{
    return p[0]|(unsigned)p[1]<<8;
}

static int expected_death_effects(death_effect_trace *trace,const gh_actor_motion *motion)
{
    trace->actor=&motion->b.model->root;
    if(motion->sequence_valid){
        const uint8_t *sequence=motion->b.resource+motion->sequence_offset;
        unsigned offset=death_half(sequence+0x14),count=death_half(sequence+0x16);
        CHECK(count<64&&motion->sequence_offset+offset+count*12<=motion->b.resource_bytes);
        for(unsigned i=0;i<count;i++){
            const uint8_t *row=sequence+offset+12*i;
            uint32_t bits=word(row);
            if(bits==UINT32_MAX)continue;
            memcpy(trace->kinds+trace->total,&bits,4);
            trace->modes[trace->total++]=(row[10]&2)?2:0;
        }
    }
    const uint8_t *actor=trace->actor->bytes;
    int16_t kind;
    memcpy(&kind,actor+0x570,2);
    if(kind!=-1){
        trace->kinds[trace->total]=kind;
        trace->modes[trace->total++]=word(actor+0x574);
    }
    return 0;
}

typedef struct {
    gh_enemy_ems *enemies;
    gh_enemy_full_retirement retirement;
    gh_enemy_retire_bindings bindings;
    unsigned invokes,finalizes;
    int reject_finalize;
} enemy_trans_fixture;
static gh_object_step_status enemy_trans_resolve(void *p,uint32_t handle,
    gh_title_player_view *actor,uint32_t *frontier)
{
    enemy_trans_fixture *fixture=p;
    gh_game_title_player *enemy=gh_enemy_ems_lookup(fixture->enemies,handle);
    if(!enemy){*frontier=0x30f69c;return GH_OBJSTEP_POOLS;}
    *actor=enemy->view;
    return GH_OBJSTEP_OK;
}
static gh_object_step_status unused_enemy_move(void *context,const gh_object_move_request *r,
    const gh_boot_work *work,uint32_t *frontier)
{
    (void)context;(void)r;(void)work;(void)frontier;
    return GH_OBJSTEP_ARGUMENT;
}
static gh_object_step_status enemy_trans_invoke(void *p,const gh_object_trans_request *r,
    const gh_boot_work *work,uint32_t *frontier)
{
    enemy_trans_fixture *fixture=p;
    (void)work;
    ++fixture->invokes;
    if(r->kind!=GH_OBJECT_TRANS_DESTROY||r->argument!=3||r->this_delta||
       !((r->original_vtable==0x4479c8&&r->original_entry==0x202058)||
         (r->original_vtable==0x448ca0&&r->original_entry==0x284a90)))
        return GH_OBJSTEP_ARGUMENT;
    gh_actor_motion_status status=gh_enemy_ems_destroy_detached_mode3(fixture->enemies,
        &fixture->retirement,r->handle,&r->actor,&fixture->bindings,frontier);
    return !status?GH_OBJSTEP_OK:status==GH_ACTOR_MOTION_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE;
}
static gh_object_step_status enemy_trans_erase(void *p,const gh_object_trans_request *r,
    const void *owner,int *handled,uint32_t *frontier)
{
    enemy_trans_fixture *fixture=p;
    ++fixture->finalizes;
    gh_actor_motion_status status=gh_enemy_ems_finish_detached_mode3(fixture->enemies,
        &fixture->retirement,r->handle,fixture->reject_finalize?fixture:owner,frontier);
    fixture->reject_finalize=0;
    if(status)return GH_OBJSTEP_POOLS;
    *handled=1;
    return GH_OBJSTEP_OK;
}

static uint32_t stamp19_effect_clock(void *p)
{
    uint32_t *clock=p;
    return (*clock)++;
}

static int retire_stamp19_trans(gh_enemy_ems *owner,const gh_enemy_ems_live *live,uint32_t handle,
    const gh_effect_create_bindings *effects,const gh_title_player_view *player,gh_game_init_state *init,
    gh_game_scene_tables *actors,const void *elf,size_t elf_size,gh_room_pools *pools,gh_heap *heap)
{
    gh_effects_frame_globals globals={0};
    gh_game_scene_aux profile={0};
    gh_game_effects_frame frame={0};
    uint32_t clock=0,frontier=0;
    CHECK(!gh_effects_frame_globals_from_elf(&globals,elf,elf_size));
    const gh_effects_frame_bindings update={.storage=effects->storage,.work=effects->work,.init=init,
        .current_player=player,.globals=&globals,.profile=&profile,.clock=stamp19_effect_clock,
        .clock_context=&clock,.atan_coeff=effects->atan_coeff,.policy=GH_STATIC_BINARY32_REFERENCE,.actors=actors};
    int borrowed=0;
    CHECK(!gh_effect_create_actor_has_live_refs(effects->storage.effects,&live->actor->view,&borrowed)&&borrowed);
    gh_scene_dynamic_owner dynamic={0};
    CHECK(!gh_scene_dynamic_bind(&dynamic,actors,heap));
    death_effect_trace trace={0};
    CHECK(!expected_death_effects(&trace,&live->enemy202->motion.motion));
    enemy_trans_fixture fixture={.enemies=owner,.bindings={.effects=effects->storage.effects,.dynamic=&dynamic,
        .release_effect=record_death_effect,.effect_context=&trace}};
    gh_game_object_step trans={0};
    const gh_object_move_bindings resolve={.resolve=enemy_trans_resolve,.invoke=unused_enemy_move,
        .context=&fixture,.elf=elf,.elf_bytes=elf_size};
    const gh_object_trans_bindings callbacks={.invoke=enemy_trans_invoke,.context=&fixture,.erase=enemy_trans_erase};
    CHECK(!gh_game_object_step_construct_002c53a8(&trans,init)&&
          !gh_game_object_step_bind_live(&trans,&resolve)&&!gh_game_object_step_bind_trans(&trans,&callbacks));
    const uint32_t primary=pools->vectors[0].count,secondary=pools->vectors[3].count;
    const uint32_t parts=pools->blocks[0].used_count-live->model->parts_allocated;
    const uint32_t nodes=pools->blocks[1].used_count-live->model->script.mesh_count;
    int erased=0;
    for(unsigned i=0;i<3;++i){
        gh_effect_status status=gh_game_effects_frame_update_0030f228(&frame,&update);
        if(status){
            fprintf(stderr,"stamp19 fx retire frame%u status%u pc%08x slot%u stage%u\n",i,status,frame.frontier,frame.emitter_index,frame.emitter_stage);
            if(frame.emitter.payload)fprintf(stderr,"blocked emitter bank%u index%u actor%d flags%08x\n",
                frame.emitter.payload[0x18c],frame.emitter.payload[0x18e],
                frame.emitter.actor!=NULL,word(frame.emitter.payload+0x114));
            const gh_effect_emitter_view *v=&frame.emitter;
            fprintf(stderr,"detail selector%u model%u part%u kind%04x dep%04x stamp%04x fields%u,%u,%u config%u sizes%zu,%zu\n",
                v->selector==v->actor,v->model&&v->model->root.bytes==v->actor->bytes,
                v->model&&v->part==v->model->parts,death_half(v->actor->bytes+0x2fe),
                word(v->actor->bytes+0x564),death_half(v->actor->bytes+0x640),v->record.bytes[0xbe],
                v->record.bytes[0x128],v->record.bytes[0x136],v->payload[0x1ec],v->record.size,v->payload_size);
        }
        CHECK(!status);
        gh_object_step_status destroy=gh_object_live_trans_one_0030f650(&trans,pools,effects->work,handle,&erased,&frontier);
        if(destroy)fprintf(stderr,"stamp19 Trans retire frame%u status%u pc%08x\n",i,destroy,frontier);
        CHECK(!destroy&&erased==(i==2));
    }
    CHECK(!gh_enemy_ems_lookup(owner,handle)&&trace.visited==trace.total&&
        pools->vectors[0].count==primary-1&&pools->vectors[3].count==secondary-1&&
        pools->blocks[0].used_count==parts&&pools->blocks[1].used_count==nodes);
    CHECK(!gh_scene_dynamic_release(&dynamic));
    puts("PASS stamp19 original 58:02 expiry, ordinary Trans mode3 and physical owner retirement");
    return 0;
}

static int check_enemy_destructors(gh_enemy_ems *owner,gh_game_init_state *init,
    gh_game_collision_solids *solids,gh_room_pools *pools,gh_game_effects *effects,
    gh_scene_dynamic_owner *dynamic,gh_room_state *room,gh_game_objects *room_objects,
    const uint8_t *elf,size_t elf_size,gh_boot_work *work,uint32_t standard,uint32_t heavy)
{
    gh_audio_registry *registry;
    gh_se_voice_engine *voices;
    gh_se_mix mix;
    unsigned output_identity=0;
    CHECK(!gh_audio_registry_create(&registry) &&
          !gh_se_mix_boot_defaults_002c9e78(&mix,0,GH_SE_DSP_NATIVE_LINEAR_REFERENCE) &&
          !gh_se_voice_create(registry,(gh_audio_output *)&output_identity,&mix,&voices));
    const uint32_t handles[]={standard,heavy};
    const uint32_t before=solids->unit_count;
    unsigned release_calls=0;
    for(unsigned i=0;i<2;i++){
        gh_enemy_ems_live live;
        CHECK(!gh_enemy_ems_live_at(owner,handles[i],&live)&&live.actor&&live.model);
        gh_actor_motion *motion=i?&live.enemy2a2->prefix.motion:&live.enemy202->motion.motion;
        if(!i){
            const uint8_t *row=motion->b.resource+0x144;
            CHECK(word(row)==0x198c00&&word(row+4)==0x19a380);
            gh_actor_motion_call change={0};
            CHECK(!gh_actor_motion_call_open_parameters_002a8578(&change,motion,&motion->b,
                word(row),word(row+4),0,0,0,0));
            CHECK(!gh_actor_motion_call_step_002a8578(&change)&&motion->sequence_valid&&
                  motion->sequence_offset==0x19a380);
        }
        gh_enemy_manager_remove remove={0};
        uint32_t frontier=0;
        const uint32_t count=word(init->enemies+0x508);
        const uint32_t kind_count=word(init->enemies+0x50c);
        const uint32_t resource_kind=i?0x2a2:0x202;
        gh_object_view resource_before,resource_after;
        CHECK(!gh_game_objects_find(room_objects,resource_kind,&resource_before)&&
              resource_before.use_count<0);
        const unsigned stamp=death_half(live.actor->view.bytes+0x640);
        uint8_t *record=NULL;
        for(unsigned j=0;j<room->active[8];j++)
            if(room->active[12+j*24+0x13]==stamp){record=room->active+12+j*24;break;}
        CHECK(record);
        if(i)memset(live.actor->view.bytes+0x54a,0,2); /* Controlled dead-HP branch. */
        int16_t original_hp;
        memcpy(&original_hp,live.actor->view.bytes+0x54a,2);
        const int16_t retained_hp=original_hp>0?original_hp:1;
        memset(record+0x16,0xa5,2);
        int found=0;
        CHECK(!gh_enemy_ems_remove_00290770(owner,&remove,handles[i],voices,&found,&frontier)&&
              found&&!frontier&&word(init->enemies+0x508)==count-1&&
              word(init->enemies+0x50c)==kind_count&&
              death_half(record+0x16)==(uint16_t)retained_hp&&
              solids->unit_count==before-i-1&&gh_enemy_ems_lookup(owner,handles[i])==live.actor);
        gh_game_title_player *manager_actor=NULL;
        CHECK(!gh_enemy_manager_get_00290868(init,(uint8_t)stamp,&manager_actor)&&!manager_actor);
        gh_enemy_manager_remove missing={0};
        CHECK(!gh_enemy_ems_remove_00290770(owner,&missing,handles[i],voices,&found,&frontier)&&
              !found&&word(init->enemies+0x508)==count-1);
        uint8_t *bytes=live.actor->view.bytes;
        death_effect_trace trace={0};
        CHECK(!expected_death_effects(&trace,motion));
        enemy_trans_fixture fixture={.enemies=owner,.bindings={.effects=effects,.dynamic=dynamic,
            .release_effect=record_death_effect,.effect_context=&trace}};
        gh_game_object_step trans={0};
        const gh_object_move_bindings resolve={.resolve=enemy_trans_resolve,.invoke=unused_enemy_move,.context=&fixture,
            .elf=elf,.elf_bytes=elf_size};
        const gh_object_trans_bindings callbacks={.invoke=enemy_trans_invoke,.context=&fixture,
            .erase=enemy_trans_erase};
        CHECK(!gh_game_object_step_construct_002c53a8(&trans,init)&&
              !gh_game_object_step_bind_live(&trans,&resolve)&&!gh_game_object_step_bind_trans(&trans,&callbacks));
        const uint32_t primary=pools->vectors[0].count,secondary=pools->vectors[3].count;
        const uint32_t parts=live.model->parts_allocated,nodes=live.model->script.mesh_count;
        const uint32_t before_parts=pools->blocks[0].used_count,before_nodes=pools->blocks[1].used_count;
        int erased=-1;
        CHECK((word(bytes+0x250)&0xe0000000u)==0x80000000u);
        CHECK(!gh_object_live_trans_one_0030f650(&trans,pools,work,handles[i],&erased,&frontier)&&!erased&&
              (word(bytes+0x250)&0xe0000000u)==0xc0000000u&&!fixture.invokes&&!fixture.finalizes);
        CHECK(!gh_object_live_trans_one_0030f650(&trans,pools,work,handles[i],&erased,&frontier)&&!erased&&
              (word(bytes+0x250)&0xe0000000u)==0xe0000000u&&!fixture.invokes&&!fixture.finalizes);
        if(trace.total){
            trace.reject_once=1;
            CHECK(gh_object_live_trans_one_0030f650(&trans,pools,work,handles[i],&erased,&frontier)==
                  GH_OBJSTEP_UNSUPPORTED&&frontier==0x306200&&fixture.retirement.stage==1&&!trace.visited&&
                  !fixture.finalizes&&!erased&&pools->vectors[0].count==primary&&pools->vectors[3].count==secondary-1);
        }
        fixture.reject_finalize=1;
        CHECK(gh_object_live_trans_one_0030f650(&trans,pools,work,handles[i],&erased,&frontier)==
              GH_OBJSTEP_POOLS&&frontier==0x30f7d0&&trans.trans_live_phase==5&&!erased&&
              fixture.retirement.stage==6&&fixture.finalizes==1&&pools->vectors[0].count==primary);
        const unsigned invoke_count=fixture.invokes;
        CHECK(!gh_object_live_trans_one_0030f650(&trans,pools,work,handles[i],&erased,&frontier)&&
              !frontier&&erased&&fixture.finalizes==2&&fixture.invokes==invoke_count&&
              fixture.retirement.stage==7&&trace.visited==trace.total&&
              pools->vectors[0].count==primary-1&&pools->vectors[3].count==secondary-1&&
              pools->blocks[0].used_count==before_parts-parts&&pools->blocks[1].used_count==before_nodes-nodes&&
              !gh_enemy_ems_lookup(owner,handles[i])&&word(init->enemies+0x508)==count-1);
        CHECK(!gh_game_objects_find(room_objects,resource_kind,&resource_after)&&
              resource_after.bytes==resource_before.bytes&&resource_after.state==3&&
              resource_after.use_count==resource_before.use_count);
        printf("PASS actual EMS descriptor %08x via ordinary Trans mode3, with resume\n",
            fixture.retirement.destructor.entry);
        release_calls+=trace.total;
    }
    CHECK(release_calls>0);
    gh_se_voice_destroy(voices);
    gh_audio_registry_destroy(registry);
    puts("PASS actual R100 0202/02A2 manager unlink and complete mode3 physical retirement");
    return 0;
}

static int run_fixture(int argc,char **argv,int attack_effect)
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
    gh_enemy_ems_live first_live,second_live,heavy_live,selector1_live,stamp10,stamp11,selector1_other[3],weapon_live;
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
    CHECK(!gh_enemy_ems_live_at(owner,word((uint8_t*)v->allocation.pointer+6*4),&stamp10)&&stamp10.enemy202);
    CHECK(!gh_enemy_ems_live_at(owner,word((uint8_t*)v->allocation.pointer+7*4),&stamp11)&&stamp11.enemy202);
    static const unsigned selector1_slots[3]={8,10,11};
    CHECK(!gh_enemy_ems_live_at(owner,pending_weapon,&weapon_live)&&weapon_live.weapon&&weapon_live.other_move);
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
    CHECK(!first_punch(disc,&room_bundle,elf,elf_size,&bindings,owner,&first_live,&second_live,&selector1_live,&stamp10,&stamp11,selector1_other,&weapon_live,
        &room,&navigation,&effect_bind,&light,&impact,attack_effect));
    gh_scene_nearcheck_release(&navigation);
    uint32_t position[3];
    if(attack_effect<2)memcpy(position,impact.position,12);
    for(unsigned row=0;!attack_effect&&row<effect_rows;row++) {
        gh_effect_emitter_view emitter;
        CHECK(!gh_effect_create_emitter_at(effects,row,&emitter) && emitter.initialized &&
            !emitter.actor && !emitter.selector && emitter.record.bytes==est.bytes+16+row*0x2f0 &&
            !memcmp(emitter.payload+0x40,position,12) && !memcmp(emitter.payload+0x1c0,position,12));
    }
    gh_scene_dynamic_owner retirement_dynamic={0};
    CHECK(!gh_scene_dynamic_bind(&retirement_dynamic,&actors,heap));
    if(attack_effect<2)CHECK(!check_enemy_destructors(owner,init,&solids,&pools,effects,&retirement_dynamic,&room,room_objects,
        elf,elf_size,&work,first_handle,word((uint8_t*)v->allocation.pointer+4*4)));
    CHECK(!gh_scene_dynamic_release(&retirement_dynamic));
    gh_effect_create_forget_on_owner_destroy(effects);
    free(emit);free(esp);free(effects);free(core);
    CHECK(!gh_enemy_ems_release_native(&owner,&frontier)&&!owner&&!frontier);
    CHECK(!init->enemy_top&&!init->enemy_last&&gh_collision_solid_units_empty(&solids)&&(attack_effect>=2||gh_collision_pair_shapes_have_live(&pairs)));
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

int main(int argc,char **argv)
{
    CHECK(!run_fixture(argc,argv,0));
    CHECK(!run_fixture(argc,argv,1));
    CHECK(!run_fixture(argc,argv,2));
    CHECK(!run_fixture(argc,argv,3));
    CHECK(!run_fixture(argc,argv,4));
    CHECK(!run_fixture(argc,argv,5));
    for(int mode=6;mode<=9;++mode)CHECK(!run_fixture(argc,argv,mode));
    CHECK(!run_fixture(argc,argv,11));
    return run_fixture(argc,argv,13);
}
