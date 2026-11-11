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
    CHECK(kind==0x303 ? row_index==6 && nearest<9 : row_index && nearest<4);
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
    CHECK(!word(actor+0x54c) && (word(actor+0x2f4)&0xffff)==0x0100);
    if(kind==0x303){
        memcpy(actor+0x2f4,&(uint32_t){0x00011101},4);
        memcpy(actor+0x16d0,&(uint32_t){0x04000202},4);
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
        memcpy(actor+0x2f4,&(uint32_t){0x00011101},4);
        memcpy(actor+0x16d0,&(uint32_t){0x04000202},4);
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
        printf("PASS actual R100 stamp8 OM303 row%u Give, repeated selector11 phase reset and clip rebind %03x\n",
            row_index,offset);
    else printf("PASS actual R100 stamp%u OM304 row%u, effect2E, 37-frame reaction row %03x and selector8/0 far continuation\n",
        actor[0x640],row_index,offset);
    return 0;
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

static int first_punch(gh_disc *disc, const gh_bundle *room_bundle, const uint8_t *elf, size_t elf_size,
    const gh_enemy_ems_bindings *bindings, gh_enemy_ems *owner, gh_enemy_ems_live *first,
    const gh_enemy_ems_live *appearance2, const gh_enemy_ems_live *selector1,const gh_enemy_ems_live *stamp11,
    const gh_enemy_ems_live selector1_other[3], const gh_enemy_ems_live *weapon_live,
    const gh_room_state *room, const gh_scene_nearcheck *navigation,
    const gh_effect_create_bindings *effect_bind, const gh_game_world_light *light,
    gh_enemy202_impact_request *output)
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
        gh_game_mt_seed_0030f2b0(&init->random,4);
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
        for(;attack_frames<100&&actor[0x2f5]==0x15;attack_frames++){
            memcpy(raw_work+0x210,&(uint32_t){word(actor+0x17d0)^1u},4);
            move=(gh_enemy202_move){0};
            CHECK(!gh_enemy202_move_bind_navigation(&move,navigation));
            CHECK(!gh_enemy202_move_bind_opacity(&move,&opacity));
            CHECK(!gh_enemy202_move_bind_player_aux_698(&move,NULL));
            CHECK(!gh_enemy202_move_bind_targets(&move,&targets));
            status=gh_enemy202_move_00208be0(&move,near,&player.view,elf,elf_size);
            if(!attack_frames)first_attack_row=move.reaction_resource_offset;
            if(near->hitareas.units[4].bytes[0x46]) {
                CHECK(near->hitareas.units[4].bytes[0x46]==0x37 && word(near->hitareas.units[4].bytes+0x4c)==20);
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
        CHECK(attack_frames==64 && active_frames==2 && first_attack_row==0x6a4 && word(actor+0x2f4)==0);
        near->motion.motion.b.sound=NULL;
        near->motion.motion.b.sound_context=NULL;

        memcpy(player.view.alias_f0,old_player,12);
        memcpy(player.view.bytes+0x630,old_previous,12);
        memcpy(player.view.bytes+0x104,&old_player_yaw,4);
        memcpy(raw_work+0x210,&old_tick,4);
    }
    CHECK(!camera_target_00139ff8(owner,b,&factory,&child,&player,&setup.model,enemy,elf,elf_size));
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
    gh_enemy_ems_live first_live,second_live,heavy_live,selector1_live,stamp11,selector1_other[3],weapon_live;
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
    CHECK(!first_punch(disc,&room_bundle,elf,elf_size,&bindings,owner,&first_live,&second_live,&selector1_live,&stamp11,selector1_other,&weapon_live,
        &room,&navigation,&effect_bind,&light,&impact));
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
