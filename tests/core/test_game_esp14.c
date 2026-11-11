#include "disc.h"
#include "game_esp14.h"
#include "game_esp_create.h"
#include "game_esp_update.h"
#include "game_effects_frame.h"
#include "game_camera_envelope.h"
#include "game_camera_projection.h"
#include "game_effect_sprite.h"
#include "game_effects_trans.h"
#include "game_actor_model_trans.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d %s\n",__LINE__,#x); exit(1); } } while (0)
static uint32_t word(const uint8_t *p) {uint32_t value;memcpy(&value,p,4);return value;}
static uint8_t *load(gh_disc *disc,const char *path,size_t *size) {
    size_t i;gh_disc_entry e;
    CHECK(!gh_disc_find(disc,path,&i)&&!gh_disc_entry_at(disc,i,&e));
    *size=(size_t)e.size;
    uint8_t *data=malloc(*size);
    CHECK(data&&!gh_disc_read(disc,i,0,data,*size));
    return data;
}

typedef struct {
    gh_effect_create_call call;
    gh_effect_create_bindings bindings;
    unsigned attempts;
} nested_context;

static gh_effect_status nested(void *opaque, const gh_esp14_nested_request *request,
    const gh_actor_model *model, int *original, uint32_t *frontier) {
    nested_context *context = opaque;
    ++context->attempts;
    CHECK(request->request.bank == 6 && request->request.index == 1 &&
          request->request.mode == -1 && request->request.selector == -1 &&
          request->group_74 == 1 && request->config_40 == 0x3f800000 &&
          request->request.actor->bytes == model->root.bytes);
    if (!context->call.active) {
        gh_effect_status status = gh_effect_create_nested_begin_002f1d3c(&context->call,
            &request->request, request->group_74, request->config_40, request->config_48);
        if (status) return status;
    }
    context->bindings.model = model;
    gh_effect_status status = gh_game_effect_create_00306528(&context->call, &context->bindings);
    *frontier = context->call.frontier;
    if (!status) *original = context->call.original_return;
    return status;
}

typedef struct {
    gh_esp14_instance *instance;
    unsigned model_slot,model_calls,sprite_slot,sprite_calls;
    uint32_t depth,rgba;
} draw_probe;

static gh_effect_status model_trans(void *opaque,const gh_effect_esp_view *view,
    unsigned slot,uint32_t *frontier)
{
    draw_probe *probe=opaque;
    CHECK(slot==probe->model_slot && view->type==0x14);
    ++probe->model_calls;
    return gh_esp14_trans_002f2758(probe->instance,view,frontier);
}

static gh_effect_status sprite_trans(void *opaque,const gh_effect_trans_packet *p)
{
    draw_probe *probe=opaque;
    CHECK(p->slot==probe->sprite_slot && p->kind==GH_EFFECT_PASS_SPRITE);
    CHECK(p->order.bank==2 && p->order.depth==probe->depth && p->original_callee==0x2fec38);
    CHECK(p->quad_count==1 && p->quads[0].vertex_count==4 && p->material);
    probe->rgba=p->quads[0].vertices[0].rgba;
    ++probe->sprite_calls;
    return GH_EFFECT_OK;
}

static gh_effect_status draw_all(const gh_effects_trans_bindings *bindings,uint32_t *frontier)
{
    gh_game_effects_trans draw={0};
    CHECK(!gh_effects_trans_begin_002fb310(&draw));
    gh_effect_status status=gh_game_effects_trans_002fb310(&draw,bindings);
    *frontier=draw.frontier;
    if(!status)CHECK(draw.complete && !draw.pending);
    return status;
}

typedef struct {
    gh_actor_model *expect;
    unsigned calls, fail_first;
} retire_sound;

static gh_effect_status detach_sound(void *opaque, gh_actor_model *model, uint32_t *frontier) {
    retire_sound *retire=opaque;
    CHECK(retire->expect==model);
    ++retire->calls;
    if(retire->fail_first){
        retire->fail_first=0;
        *frontier=0x2ce488;
        return GH_EFFECT_UNSUPPORTED;
    }
    CHECK(!model->sound_borrowers);
    *frontier=0;
    return GH_EFFECT_OK;
}

typedef struct {
    gh_esp14_instance *instance;
    retire_sound sound;
    unsigned destroys;
} retire_probe;

static gh_effect_status destroy_model(void *opaque,const gh_effect_esp_view *v,unsigned slot,
    uint32_t *frontier) {
    retire_probe *probe=opaque;
    CHECK(slot==probe->instance->slot&&v->serial==probe->instance->serial);
    ++probe->destroys;
    return gh_esp14_destroy_hidden_002f27e8(probe->instance,detach_sound,&probe->sound,frontier);
}

static uint32_t clock_tick(void *opaque) {
    uint32_t *clock=opaque;
    return ++*clock;
}

static void set_mask(uint8_t *base,unsigned offset,unsigned slot,int on) {
    uint8_t *p=base+offset+(slot>>5)*4;
    uint32_t bit=UINT32_C(0x80000000)>>(slot&31),value=word(p);
    value=on?(value|bit):(value&~bit);
    memcpy(p,&value,4);
}

typedef struct {
    const gh_game_pl_camera *camera;
    gh_actor_model *model;
    unsigned calls;
} model_light_probe;

static gh_actor_model_status observe_light(void *opaque,const gh_actor_trans_request *request,
    uint32_t *frontier) {
    model_light_probe *probe=opaque;
    CHECK(request->entry==0x14d828&&request->camera==probe->camera&&request->model==probe->model);
    ++probe->calls;
    *frontier=request->entry;
    return GH_ACTOR_MODEL_UNSUPPORTED;
}

int main(int argc,char **argv) {
    CHECK(argc==2);
    gh_disc *disc;
    CHECK(!gh_disc_open(argv[1],&disc));
    size_t cs,rs,es;
    uint8_t *core=load(disc,"etc/core.dat",&cs),*room=load(disc,"st1/r100.dat",&rs),
        *event=load(disc,"event/ev01.dat",&es);
    gh_bundle room_bundle,event_effect;
    const size_t offset=word(event+0x1c);
    CHECK(!gh_bundle_open(room,rs,&room_bundle));
    CHECK(offset<es&&!gh_bundle_open(event+offset,es-offset,&event_effect));
    gh_bundle_entry table;
    CHECK(!gh_bundle_find(&event_effect,"TBL",1,&table)&&table.size>=8);
    const size_t est_offset=word(table.bytes+4);
    CHECK(est_offset<table.size && 16+3*0x2f0 <= table.size-est_offset);
    const uint8_t *est=table.bytes+est_offset;
    const uint8_t *row=est+16+2*0x2f0;
    CHECK(word(est)==0x00545345 && word(est+4)==10 && row-event==0xcd440);
    CHECK(row[0x133]==0x14 && word(row+4)==0x00a00000 && row[0x18f]==1);
    CHECK(row[0x12e]==0 && row[0x18c]==0 && row[0x18e]==0);

    uint8_t payload[0x400];
    memset(payload,0xa5,sizeof(payload));
    gh_esp14_construct_tail_002ffd88(payload);
    payload[0x102]=0x14; /* 2FAE80 delay slot belongs to the selector */
    CHECK(word(payload+0xf0)==GH_ESP14_VTABLE && !word(payload+0x2b0));
    CHECK(payload[0xef]==0xa5 && payload[0xf4]==0xa5 && payload[0x2af]==0xa5);
    uint32_t frontier=0;
    CHECK(!gh_esp14_init_prefix_002f1d58(payload,(gh_effect_view){row,0x2f0},0x3f000000,&frontier));
    CHECK(!frontier&&word(payload+0x300)==0x3f000000&&payload[0x2e8]==1&&payload[0x2e9]==0xa5);
    CHECK(word(payload+0x2ec)==0x3f800000&&word(payload+0x2f0)==0x3f800000);
    CHECK(!word(payload+0x2b4)&&!word(payload+0x2b8)&&!word(payload+0x2bc)&&!word(payload+0x2d0));

    gh_game_effects *effects=calloc(1,sizeof(*effects));
    uint8_t *esp=calloc(1,0x100180),*emitters=calloc(1,0x26060);
    CHECK(effects&&esp&&emitters);
    CHECK(!gh_game_effects_initialize_00305a68(effects,esp,0x100180,emitters,0x26060,core,cs,&room_bundle));
    CHECK(!gh_game_effects_register_event_002fa0b0(effects,event+offset,es-offset));
    gh_esp14_resources resources;
    int found;
    CHECK(!gh_esp14_resource_00306c78(effects,64,&resources,&found)&&!found);
    CHECK(!gh_esp14_resource_00306c78(effects,row[0x12e],&resources,&found)&&found);
    CHECK(resources.geometry.bytes==effects->emd_tables[0].bytes+0x70);
    CHECK(resources.geometry.size==0x1e0&&resources.texture.size==0x1c0);
    CHECK(resources.motions.bytes==effects->emitter_entries[0].bytes+0x3b0);

    gh_heap *heap;
    gh_room_pools pools={0};
    CHECK(!gh_heap_create(32*1024*1024,&heap));
    CHECK(!gh_game_room_pools_initialize_002a5d80(&pools,&room_bundle,heap));
    gh_boot_work work={0};
    uint8_t serial=0x34;
    uint32_t fog=0;
    const gh_esp14_model_bindings bindings={&pools,&work,&serial,heap,&fog,GH_STATIC_BINARY32_REFERENCE};
    gh_esp14_model child={0};
    CHECK(!gh_esp14_model_create_0030fa98(&child,&bindings,&found,&frontier)&&found&&!frontier);
    CHECK(child.object.view.size==0x4e0&&child.object.view.original_vtable214==0x44b840);
    CHECK(child.object.view.bytes[0x2fc]==0x34&&serial==0x35&&word(child.object.view.bytes+0x2d0)==0x10000000);
    CHECK(pools.vectors[0].count==1&&pools.vectors[3].count==1&&pools.vectors[1].count==0);
    CHECK(*(uint32_t*)pools.vectors[0].allocation.pointer==child.handle);
    CHECK(*(uint32_t*)pools.vectors[3].allocation.pointer==child.handle);
    void *identity=NULL;
    CHECK(!gh_room_object_lookup(&pools,child.handle,0x500,&child,&identity)&&identity==&child.object);
    CHECK(!gh_esp14_model_create_0030fa98(&child,&bindings,&found,&frontier)&&found&&serial==0x35);
    gh_effect_status status=gh_esp14_model_initialize_002f1b60(&child,&bindings,&resources,&found,&frontier);
    printf("model status=%d original=%d frontier=%08X parts=%u detail=%u\n",status,found,frontier,child.model.part_count,child.model.detail);
    CHECK(!status&&found&&child.model.ready);
    CHECK(!gh_esp14_model_dispose(&child)&&!pools.vectors[0].count&&!pools.vectors[3].count);
    const size_t elf_size = (size_t)gh_disc_boot_elf_size(disc);
    uint8_t *elf = malloc(elf_size);
    CHECK(elf && !gh_disc_read_boot_elf(disc, 0, elf, elf_size));
    gh_effects_frame_globals globals = {0};
    CHECK(!gh_effects_frame_globals_from_elf(&globals, elf, elf_size));
    gh_game_mt random;
    gh_game_mt_seed_0030f2b0(&random, 123);
    ((uint8_t *)&work)[0x5e4] = 0x40;
    gh_effect_create_bindings producer = {
        .storage = {.effects = effects, .esp = esp, .esp_bytes = 0x100180, .emitters = emitters, .emitter_bytes = 0x26060},
        .work = &work, .random = &random
    };
    gh_effect_create_call event_cut = {0};
    CHECK(!gh_effect_create_event_cut_begin_002fa158(&event_cut, 0));
    status = gh_game_effect_create_00306528(&event_cut, &producer);
    CHECK((!status || event_cut.frontier == 0x297c50) && event_cut.row > 2);
    gh_effect_emitter_view source;
    CHECK(!gh_effect_create_emitter_at(effects, 2, &source) && source.record.bytes == row);
    gh_esp_create_call create = {0};
    gh_esp_create_bindings create_bind = {
        .storage = producer.storage, .random = &random, .work = &work,
        .policy = GH_STATIC_BINARY32_REFERENCE, .globals = &globals
    };
    CHECK(!gh_esp_create_begin_00300e30(&create, 2, source.serial));
    status = gh_game_esp_create_00300e30(&create, &create_bind);
    printf("shared status=%d frontier=%08X stage=%u\n", status, create.frontier, create.stage);
    CHECK(status == GH_EFFECT_UNSUPPORTED && create.frontier == 0x2f1d58 && create.stage == 10);
    gh_effect_esp_view view;
    CHECK(!gh_effect_create_esp_at(effects, create.slot, &view) && !view.initialized);
    gh_camera_envelope envelope={0};
    CHECK(!gh_game_camera_envelope_from_elf(&envelope,elf,elf_size,&frontier));
    const float *atan_coeff=envelope.atan_coeff;
    gh_esp14_instance instance = {0};
    nested_context context = {.bindings = producer};
    gh_esp14_bindings ib = {
        .model = bindings, .effects = effects, .atan_coeff = atan_coeff,
        .context = &context
    };
    status = gh_esp14_initialize_002f1d58(&instance, &ib, &view, create.slot,
        word(source.payload + 0x1e0), &found, &frontier);
    CHECK(status == GH_EFFECT_UNSUPPORTED && frontier == 0x306528 && instance.stage == 3 && !found);
    ib.nested_create = nested;
    status = gh_esp14_initialize_002f1d58(&instance, &ib, &view, create.slot,
        word(source.payload + 0x1e0), &found, &frontier);
    printf("specific status=%d frontier=%08X stage=%u original=%d nested_rows=%u\n",
        status, frontier, instance.stage, found, context.call.row);
    CHECK(!status && found && instance.complete && context.attempts == 1);
    CHECK(context.call.count == 10 && context.call.slot > 2);
    gh_effect_emitter_view nested_view;
    CHECK(!gh_effect_create_emitter_at(effects, context.call.slot, &nested_view));
    CHECK(nested_view.record.bytes == row + 0x2f0 && nested_view.actor == &instance.child.object.view &&
          nested_view.selector == nested_view.actor && nested_view.payload[0x214] == 1);
    CHECK(!gh_effect_create_esp_commit_14(effects,create.slot,create.serial));
    CHECK(!gh_effect_create_esp_at(effects,create.slot,&view) && view.initialized);
    CHECK(!gh_esp14_trans_002f2758(&instance, &view, &frontier));
    const float one=1;
    memcpy((uint8_t *)&work+0x574,&one,4);
    memcpy(&globals.factor_741dc0,&one,4);
    gh_esp_update_bindings update_bind = {
        .storage=producer.storage,.work=&work,.globals=&globals,.atan_coeff=atan_coeff,
        .policy=GH_STATIC_BINARY32_REFERENCE
    };
    const uint32_t previous_angle=word(view.payload+0x264);
    for(unsigned frame=0;frame<3;++frame) {
        gh_esp_update_call update={0};
        status=gh_game_esp_update_002fbed0(&update,&update_bind,create.slot,create.serial);
        printf("update frame=%u status=%d frontier=%08X stage=%u retired=%u\n",frame,status,update.frontier,update.stage,update.retired);
        CHECK(!status&&update.complete&&!update.retired);
        CHECK(!gh_esp14_move_after_base_002f2250(&instance,&ib,&view,1,&frontier));
        CHECK(!gh_esp14_model_move_002b8850(&instance.child,&work,GH_STATIC_BINARY32_REFERENCE,&frontier));
        CHECK(!gh_esp14_trans_002f2758(&instance,&view,&frontier));
    }
    CHECK(word(view.payload+0x264)!=previous_angle);
    CHECK(!gh_esp14_nested_emitter_pose_00302bb4(effects,context.call.slot,&nested_view,
        atan_coeff,GH_STATIC_BINARY32_REFERENCE,&frontier));
    CHECK(!memcmp(nested_view.payload+0x40,instance.child.object.view.bytes+0x80+48,12));
    CHECK(word(nested_view.payload+0x114)&0x800000);
    gh_esp_create_call nested_particle={0};
    CHECK(!gh_esp_create_begin_00300e30(&nested_particle,context.call.slot,nested_view.serial));
    status=gh_game_esp_create_00300e30(&nested_particle,&create_bind);
    printf("nested particle status=%d frontier=%08X stage=%u\n",status,nested_particle.frontier,nested_particle.stage);
    CHECK(!status&&nested_particle.complete);
    gh_effect_esp_view spark;
    CHECK(!gh_effect_create_esp_at(effects,nested_particle.slot,&spark)&&spark.initialized&&
          !spark.type&&spark.actor==nested_view.actor&&spark.selector==nested_view.selector&&
          spark.parent_emitter==(int32_t)context.call.slot&&spark.parent_serial==nested_view.serial);
    float world[4];
    CHECK(!gh_effect_node_world_position_native(effects,0,nested_particle.slot,spark.serial,world,
          GH_STATIC_BINARY32_REFERENCE,&frontier));
    for(unsigned j=0;j<3;j++)CHECK(word((const uint8_t*)world+4*j)==word(nested_view.payload+0x40+4*j));
    gh_esp_update_call nested_update={0};
    status=gh_game_esp_update_002fbed0(&nested_update,&update_bind,nested_particle.slot,spark.serial);
    printf("nested update status=%d frontier=%08X stage=%u\n",status,nested_update.frontier,nested_update.stage);
    CHECK(!status&&nested_update.complete);
    CHECK(spark.texture.bytes&&spark.texture.size>=16);
    float alpha=0;
    CHECK(!gh_esp14_nested_sprite_alpha_002fec38(&spark,128,0.25f,&alpha,&frontier));
    uint8_t *actor=instance.child.object.view.bytes;
    actor[0x250]|=2;
    CHECK(!gh_esp14_nested_sprite_alpha_002fec38(&spark,128,0.25f,&alpha,&frontier)&&alpha==0);
    actor[0x250]&=(uint8_t)~2u;
    spark.payload[0x11c]|=0x80;
    CHECK(!gh_esp14_nested_sprite_alpha_002fec38(&spark,128,0.25f,&alpha,&frontier)&&alpha==32);
    spark.payload[0x11c]&=(uint8_t)~0x80u;
    gh_display_state display={0};
    CHECK(!gh_display_state_from_elf(&display,elf,elf_size));
    CHECK(!gh_display_static_initialize_002a48a8(&display,1,0xffff));
    gh_game_init_state camera_owner={0};
    gh_game_pl_camera camera={0};
    CHECK(!gh_game_camera_construct_pl_00139bc8(&camera,&camera_owner.camera,GH_CAMERA_NATIVE_BINARY32_REFERENCE));
    camera_owner.active_camera=&camera_owner.camera;
    CHECK(!gh_effect_node_world_position_native(effects,0,nested_particle.slot,spark.serial,world,
          GH_STATIC_BINARY32_REFERENCE,&frontier));
    float eye[3]={world[0],world[1],world[2]-5};
    memcpy(camera.fields+0x200,world,12);
    memcpy(camera.fields+0x210,eye,12);
    const float fov=31,up=1;
    memcpy(camera.fields+0x250,&fov,4);
    memcpy(camera.fields+0x3d4,&up,4);
    const int32_t width=512;
    const uint32_t height=0x43e00000,aspect=0x3f800000;
    const uint8_t initialized=1;
    const gh_camera_projection_display projection_display={&width,&height,&aspect,&initialized,GH_CAMERA_NATIVE_BINARY32_REFERENCE};
    CHECK(!gh_game_camera_project_00138740(&camera,&projection_display,&frontier));
    /* Real kind500/44B840 on the native stage-31 dispatch. The supplied
     * camera is deliberately a separate active owner from the room camera. */
    gh_actor_model *fx_model=&instance.child.model;
    CHECK(fx_model->root.original_vtable214==0x44b840&&
          (word(fx_model->root.bytes+0x254)&0x08000000u));
    model_light_probe light_probe={.camera=&camera,.model=fx_model};
    const uint8_t depth_shift=0;
    gh_actor_trans_bindings model_trans_bindings={
        .work=&work,.camera=&camera,.depth_shift=&depth_shift,.elf=elf,.elf_bytes=elf_size,
        .callee=observe_light,.context=&light_probe,.numeric_policy=GH_STATIC_BINARY32_REFERENCE
    };
    uint8_t *work_bytes=(uint8_t*)&work;
    const uint32_t enabled_flags=0x12100000u,ev01_flags=0x02100000u;
    memcpy(work_bytes+0x5e0,&enabled_flags,4);
    gh_actor_trans_call suppressed={0};
    CHECK(!gh_actor_model_trans_0014a3e8(&suppressed,fx_model,&model_trans_bindings,&frontier));
    CHECK(suppressed.complete&&!light_probe.calls&&!frontier);
    memcpy(work_bytes+0x5e0,&ev01_flags,4);
    gh_actor_trans_call allowed={0};
    CHECK(gh_actor_model_trans_0014a3e8(&allowed,fx_model,&model_trans_bindings,&frontier)==GH_ACTOR_MODEL_UNSUPPORTED);
    CHECK(frontier==0x14d828&&allowed.phase==2&&light_probe.calls==1);
    const uint32_t no_gate=0;
    memcpy(work_bytes+0x5e0,&no_gate,4);
    const gh_effect_sprite_camera sprite_camera={&camera,&camera_owner,&display,GH_STATIC_BINARY32_REFERENCE,GH_FRAME_CHOP_REFERENCE};
    gh_effect_sprite_projection sprite={0};
    status=gh_effect_sprite_project_002fe0d8(effects,nested_particle.slot,spark.serial,&sprite_camera,&sprite,&frontier);
    printf("nested projection status=%d frontier=%08X visible=%u depth=%d\n",status,frontier,sprite.visible,sprite.original_depth);
    CHECK(!status&&sprite.visible&&sprite.original_depth>=99&&sprite.original_depth<=100);
    CHECK(sprite.xyz_fixed4[0][0]!=sprite.xyz_fixed4[1][0]&&sprite.xyz_fixed4[0][1]!=sprite.xyz_fixed4[2][1]);
    gh_stage_textures *textures=NULL;
    CHECK(!gh_stage_textures_create(&textures));
    draw_probe draw={.instance=&instance,.model_slot=create.slot,.sprite_slot=nested_particle.slot,
        .depth=(uint32_t)sprite.original_depth};
    gh_esp14_provider provider={.context=&draw,.trans=model_trans};
    gh_effects_trans_bindings draw_bindings={.storage=producer.storage,.work=&work,.globals=&globals,
        .display=&display,.policy=GH_STATIC_BINARY32_REFERENCE,.cvt_policy=GH_FRAME_CHOP_REFERENCE,
        .emit=sprite_trans,.context=&draw,.camera=&camera,.init=&camera_owner,.textures=textures,
        .model_effect=&provider};
    ((uint8_t *)&work)[0x5e6]|=1;
    status=draw_all(&draw_bindings,&frontier);
    printf("attached Trans status=%d pc=%08X model=%u sprites=%u color=%08X particleAlpha=%08X\n",
        status,frontier,draw.model_calls,draw.sprite_calls,draw.rgba,word(spark.payload+0x194));
    CHECK(!status && !frontier && draw.model_calls==1 && draw.sprite_calls==1 && draw.rgba==0x80737373u);
    actor[0x251]|=0x80;
    CHECK(!draw_all(&draw_bindings,&frontier) && draw.model_calls==2 && draw.sprite_calls==1);
    actor[0x251]&=0x7f;
    actor[0x250]|=2;
    CHECK(!draw_all(&draw_bindings,&frontier) && draw.model_calls==3 && draw.sprite_calls==1);
    spark.payload[0x11c]|=0x80;
    CHECK(!draw_all(&draw_bindings,&frontier) && draw.model_calls==4 && draw.sprite_calls==2);
    gh_stage_textures_destroy(textures);
    int collected,borrowed;
    CHECK(!gh_effect_create_actor_has_live_refs(effects,&instance.child.object.view,&borrowed)&&borrowed);
    CHECK(!gh_effect_create_esp_pause_002fbe50(effects,create.slot,create.serial,&frontier));
    CHECK(gh_esp14_destroy_hidden_002f27e8(&instance,NULL,NULL,&frontier)==GH_EFFECT_ARGUMENT);
    /* Original 2FB0E8 promotes paused to hidden before running the dtor on
       a later visit. Explicit phase fixture leaves other scene FX out. */
    set_mask(esp,0x100100,create.slot,1);
    retire_probe retiring={.instance=&instance,.sound={.expect=&instance.child.model,.fail_first=1}};
    const gh_esp14_provider retirement_provider={.context=&retiring,.destroy_hidden=destroy_model};
    gh_game_scene_aux profile={0};
    uint32_t clock=0;
    gh_effects_frame_bindings retirement_frame={
        .storage=producer.storage,.work=&work,.globals=&globals,.profile=&profile,
        .clock=clock_tick,.clock_context=&clock,.atan_coeff=atan_coeff,.policy=GH_STATIC_BINARY32_REFERENCE,
        .model_effect=&retirement_provider
    };
    gh_game_effects_frame frame={.b=retirement_frame,.identity=&frame,.bound=1,.stage=4};
    CHECK(gh_game_effects_frame_update_0030f228(&frame,&retirement_frame)==GH_EFFECT_UNSUPPORTED);
    CHECK(frame.frontier==0x2ce488&&retiring.sound.calls==1&&!instance.child_delete_requested);
    CHECK((word(actor+0x250)&0x80000002u)==2);
    CHECK(!gh_game_effects_frame_update_0030f228(&frame,&retirement_frame));
    CHECK(retiring.destroys==2&&retiring.sound.calls==2);
    CHECK(instance.esp_destroyed&&instance.child_delete_requested&&(word(actor+0x250)&0x80000002u)==0x80000002u);
    CHECK(word(view.payload+0xf0)==0x44b310);
    CHECK(!gh_esp14_destroy_hidden_002f27e8(&instance,detach_sound,&retiring.sound,&frontier)&&retiring.sound.calls==2);
    unsigned reused_slot;
    uint32_t reused_serial;
    int reused;
    CHECK(!gh_effect_create_esp_allocate_002faa88(&producer.storage,2,source.serial,
          &reused_slot,&reused_serial,&reused,&frontier));
    CHECK(reused&&reused_slot==create.slot&&reused_serial!=instance.serial);
    CHECK(!gh_effect_create_esp_abort_partial(effects,reused_slot,reused_serial));
    CHECK(instance.child.object.view.bytes==actor);

    gh_stage_slots room_slots={0},global_slots={0};
    gh_game_objects *room_objects,*global_objects;
    CHECK(!gh_game_objects_create(&room_slots,1,&room_objects));
    CHECK(!gh_game_objects_create(&global_slots,0,&global_objects));
    CHECK(!gh_esp14_child_destroy_002b8908(&instance,room_objects,global_objects,&frontier));
    CHECK(instance.child.model_destroyed&&word(actor+0x214)==0x41eca0);
    CHECK(!gh_esp14_collect_retired(&instance,&collected,&frontier)&&!collected);
    /* Scene 30F650 owns canonical unregister after original virtual delete. */
    CHECK(!gh_room_object_unregister(&pools,instance.child.handle,&instance.child));
    CHECK(!gh_esp14_collect_retired(&instance,&collected,&frontier)&&!collected);
    CHECK(instance.child.object.view.bytes==actor&&spark.actor==&instance.child.object.view);

    /* Restore only the actual nested emitter in the active fixture. Its
       original sign-bit update and paused?hidden loops own retirement. */
    for(unsigned slot=0;slot<256;slot++){
        if(slot!=context.call.slot)set_mask(emitters,0x26000,slot,0);
    }
    for(unsigned tick=0;tick<6&&!collected;++tick){
        status=gh_game_effects_frame_update_0030f228(&frame,&retirement_frame);
        printf("retire tick=%u status=%d frontier=%08X\n",tick,status,frame.frontier);
        CHECK(!status);
        CHECK(!gh_esp14_collect_retired(&instance,&collected,&frontier));
        if(!tick)CHECK(!collected);
    }
    CHECK(collected&&!instance.child.object.ready&&!instance.child.handle);
    CHECK(gh_effect_create_esp_at(effects,nested_particle.slot,&spark)!=GH_EFFECT_OK);
    CHECK(gh_effect_create_emitter_at(effects,context.call.slot,&nested_view)!=GH_EFFECT_OK);
    gh_game_objects_destroy(global_objects);
    gh_game_objects_destroy(room_objects);
    gh_effect_create_forget_on_owner_destroy(effects);
    free(elf);
    CHECK(!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));
    free(emitters);free(esp);free(effects);free(event);free(room);free(core);
    gh_disc_close(disc);
    puts("PASS EV01 row2, shared EFM model spans, native kind500 constructor/registry/model");
    return 0;
}
