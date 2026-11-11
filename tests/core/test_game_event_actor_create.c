#include "game_event_actor_bust.h"
#include "actor_world_queue.h"
#include <fenv.h>
#include <math.h>
#include "game_event_actor_create.h"
#include "game_effects_frame.h"
#include "game_actor_two_bone.h"
#include "game_effects_trans.h"
#include "game_motion_camera.h"
#include "game_camera_envelope.h"
#include "game_ray_static.h"
#include "game_event_accessory_move.h"
#include "game_event_pendulum.h"
#include "game_event_actor_factory.h"
#include "game_event_actor_extra.h"
#include "game_event_actor_play.h"
#include "game_event_actor_release.h"
#include "game_event_release_stage.h"
#include "game_actor_motion_ik.h"
#include "game_player_node_cache.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); exit(1); } \
} while (0)

static void put_word(uint8_t *p, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(value >> (i * 8));
}

static uint8_t *load(gh_disc *disc, const char *path, size_t *bytes) {
    size_t index;
    gh_disc_entry entry;
    CHECK(gh_disc_find(disc, path, &index) == GH_DISC_OK);
    CHECK(gh_disc_entry_at(disc, index, &entry) == GH_DISC_OK && entry.size <= SIZE_MAX);
    *bytes = (size_t)entry.size;
    uint8_t *data = malloc(*bytes);
    CHECK(data && gh_disc_read(disc, index, 0, data, *bytes) == GH_DISC_OK);
    return data;
}

typedef struct {
    gh_room_pools *pools;
    unsigned calls;
    uint32_t fail;
    const uint32_t *handles;
    const uint16_t *kinds;
    size_t count;
    gh_game_player_factory *player;
    gh_game_player_child *child;
} object_scope;

static gh_game_event_status object_view(void *opaque, uint32_t handle, gh_title_player_view *out,
                                       uint32_t *frontier) {
    object_scope *scope = opaque;
    ++scope->calls;
    if (handle == scope->fail) {
        *frontier = 0x123456;
        return GH_EVENT_UNSUPPORTED;
    }
    if (scope->player && handle == scope->player->handle) {
        const gh_title_player_view *view;
        if (gh_game_player_factory_scalar_view(scope->player, handle, &view)) return GH_EVENT_ARGUMENT;
        *out = *view;
        return GH_EVENT_OK;
    }
    if (scope->child && handle == scope->child->handle) {
        void *object = NULL;
        if (gh_room_object_lookup(scope->pools, handle, GH_ROOM_OBJECT_PLAYER_CHILD,
            &scope->child->object, &object) || object != &scope->child->object) return GH_EVENT_ARGUMENT;
        *out = scope->child->object.view;
        return GH_EVENT_OK;
    }
    for (size_t i = 0; i < scope->count; ++i) {
        if (handle != scope->handles[i]) continue;
        void *object = NULL;
        if (gh_room_object_lookup(scope->pools, handle, scope->kinds[i], scope, &object)) {
            return GH_EVENT_ARGUMENT;
        }
        *out = *(gh_title_player_view *)object;
        return GH_EVENT_OK;
    }
    return GH_EVENT_ARGUMENT;
}

static uint32_t word(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

typedef struct {
    gh_event_actor_factory *actor;
    unsigned calls;
} event_move_shadow_probe;

static gh_game_event_status probe_event_move_shadow(void *opaque, gh_event_actor_factory *actor,
                                                     uint32_t *frontier) {
    event_move_shadow_probe *probe = opaque;
    CHECK(actor == probe->actor && actor->shadow.initialized);
    CHECK(word(actor->object.view.bytes + 0x4dc) == 3 && *frontier == 0x14efa8);
    if (++probe->calls == 1) {
        *frontier = 0x31a600;
        return GH_EVENT_UNSUPPORTED;
    }
    return GH_EVENT_OK;
}

static void check_event_hand_variants(gh_event_actor_factory *actor, const uint8_t *elf,
                                      size_t elf_bytes, const gh_player_shared_vector *shared) {
    static const uint8_t names[2][3][8] = {
        {"RH_00", "RH_01", "RH_02"}, {"LH_00", "LH_01", "LH_02"}
    };
    CHECK(elf_bytes >= 0x34c508 && !memcmp(elf + 0x34c4d8, names, sizeof(names)));
    uint8_t *root = actor->object.view.bytes;
    CHECK(actor->kind == 0x630 && root[0x2f4] == 4 && root[0x4d8] == 1);
    CHECK(!root[0x1390] && !root[0x2fa0] && !root[0x2ff0] && actor->model.part_count >= 28);
    const gh_actor_part *driver;
    CHECK(!gh_actor_model_part_at(&actor->model, 27, &driver));
    uint32_t authored[2] = {word(driver->bytes + 0x100), word(driver->bytes + 0x108)};
    gh_player_mesh_reference meshes[2][3];
    uint32_t original[2][3] = {{0}};
    unsigned present = 0;
    for (unsigned hand = 0; hand < 2; ++hand) {
        for (unsigned mesh = 0; mesh < 3; ++mesh) {
            int found;
            CHECK(!gh_actor_model_find_mesh_0014b730(&actor->model, names[hand][mesh], 8,
                &meshes[hand][mesh], &found));
            if (!found) continue;
            present |= 1u << (3 * hand + mesh);
            original[hand][mesh] = word(meshes[hand][mesh].bytes + 0x380);
        }
    }
    CHECK(actor->model.part_count == 30 && present == 0x1f && !authored[0] && !authored[1]);
    fenv_t environment;
    CHECK(!feholdexcept(&environment));
    CHECK(!fesetround(FE_UPWARD) && !feraiseexcept(FE_DIVBYZERO));
    const uint32_t samples[][2] = {
        {authored[0], authored[1]}, {0x3e860a91, 0x3edf66f2}, {0x3e860a91, 0x3edf66f2},
        {0x3eb2b8c2, 0x3e32b8c2}, {0x3f060a91, 0x3e32b8c1},
        {0xbe860a91, 0x3f060a92}
    };
    const unsigned expected[][2] = {{0, 0}, {1, 2}, {1, 2}, {2, 1}, {2, 0}, {0, 0}};
    for (unsigned sample = 0; sample < sizeof(samples) / sizeof(*samples); ++sample) {
        put_word(driver->bytes + 0x100, samples[sample][0]);
        put_word(driver->bytes + 0x108, samples[sample][1]);
        uint32_t frontier = 0;
        gh_event_accessory_move move = {0};
        event_move_shadow_probe shadow = {.actor = actor, .calls = 1};
        CHECK(gh_game_event_simple_move_002b6930(&move, actor, shared, probe_event_move_shadow,
              &shadow, &frontier) == GH_EVENT_OK && !frontier && !move.stage);
        CHECK(fegetround() == FE_UPWARD && fetestexcept(FE_ALL_EXCEPT) == FE_DIVBYZERO);
        for (unsigned hand = 0; hand < 2; ++hand) {
            for (unsigned mesh = 0; mesh < 3; ++mesh) {
                if (!meshes[hand][mesh].bytes) continue;
                uint32_t flags = word(meshes[hand][mesh].bytes + 0x380);
                CHECK(flags == ((original[hand][mesh] & ~1u) | (mesh != expected[sample][hand])));
            }
        }
    }
    CHECK(!fesetenv(&environment));
    put_word(driver->bytes + 0x100, authored[0]);
    put_word(driver->bytes + 0x108, authored[1]);
    uint32_t frontier = 0;
    gh_event_accessory_move move = {0};
    event_move_shadow_probe shadow = {.actor = actor, .calls = 1};
    CHECK(gh_game_event_simple_move_002b6930(&move, actor, shared, probe_event_move_shadow,
          &shadow, &frontier) == GH_EVENT_OK && !frontier && !move.stage);
}

static void check_event_display_base_texture(gh_event_actor_factory *actor, const uint8_t *elf,
                                             size_t elf_bytes) {
    gh_actor_model *model = &actor->model;
    CHECK(actor->kind == 0x631 && actor->object.view.bytes[0x2f4] == 4 &&
          model->part_count == 30 && model->script.mesh_count == 4);
    const gh_actor_node_packets *packet;
    gh_model_mesh mesh;
    CHECK(!gh_actor_model_packet_at(model, 0, &packet) && packet->stream_count == 5);
    CHECK(!gh_model_mesh_at(&model->script, 0, &mesh) && !memcmp(mesh.name, "body", 5));
    static const uint8_t slots[] = {1, 255, 3, 4, 2};
    for (unsigned stream = 0; stream < sizeof(slots); ++stream)
        CHECK(packet->streams[stream].input.material == slots[stream]);
    const gh_skinned_mesh_stream *ff = &packet->streams[1].input;
    CHECK(ff->header_offset == 0x1ec0 && ff->vertices == 14);
    for (unsigned vertex = 0; vertex < ff->vertices; ++vertex)
        CHECK(!word(ff->bytes + ff->uv_offset + vertex * 4));
    /* Patched ELF PT_LOAD zero fill covers the original 747BB8 + FF*B0. */
    CHECK(elf_bytes > 0x54 && word(elf + 0x1c) == 0x34 && word(elf + 0x34) == 1);
    uint32_t load_address = word(elf + 0x3c);
    CHECK((uint64_t)load_address + word(elf + 0x44) <= 0x752b08 &&
          (uint64_t)load_address + word(elf + 0x48) >= 0x752b10);
    gh_stage_textures *textures;
    CHECK(!gh_stage_textures_create(&textures));
    gh_actor_materials materials = {0};
    uint32_t frontier = 0;
    CHECK(!gh_actor_materials_0014b4d0(model, 0, textures, &materials, &frontier));
    CHECK(!frontier && materials.ready && materials.count == sizeof(slots));
    for (unsigned stream = 0; stream < sizeof(slots); ++stream) {
        const gh_actor_stream_material *material = &materials.streams[stream];
        CHECK(material->source_member == slots[stream] && material->selected_member == slots[stream]);
        if (stream == 1) {
            CHECK(material->samples_display_base0 && !material->written &&
                  !material->material && !material->tex0);
        } else {
            CHECK(!material->samples_display_base0 && material->written &&
                  material->material && (material->tex0 & (UINT64_C(1) << 34)));
        }
    }
    CHECK(!gh_actor_model_update_hierarchy_00149eb8(model, GH_STATIC_BINARY32_REFERENCE));
    /* Same model's owned palette plus a numeric camera/light fixture. */
    gh_game_pl_camera camera = {.constructed = 1};
    for (unsigned i = 0; i < 4; ++i) {
        put_word(camera.fields + i * 20, i == 0 ? 0xbf800000 : 0x3f800000);
        put_word(camera.fields + 0xc0 + i * 20, i == 3 ? 0x3f800000 : 0x3a83126f);
    }
    CHECK(!gh_actor_palette_0014c7e8(model, &camera, GH_STATIC_BINARY32_REFERENCE, &frontier));
    gh_actor_draw_command command = {.kind = GH_ACTOR_DRAW_GEOMETRY, .model = model,
        .node_index = 0, .parity = materials.parity, .bank = 2, .depth = 10,
        .selected_program = 2, .packet = packet, .materials = materials, .packet_group = 1};
    CHECK(!gh_actor_model_program2_inputs(model, 0, &command.inputs, &frontier));
    gh_actor_world_queue *queue;
    CHECK(!gh_actor_world_queue_create(&queue));
    size_t index = SIZE_MAX;
    gh_static_transform_frontier render_frontier;
    CHECK(!gh_actor_world_queue_append(queue, &command, &index, &render_frontier) && index == 0);
    gh_actor_world_group group;
    CHECK(!gh_actor_world_queue_group_at(queue, 0, &group));
    CHECK(group.packets == packet->batch_count);
    unsigned sampled = 0;
    for (size_t i = 0; i < group.packets; ++i) {
        gh_actor_world_packet batch;
        CHECK(!gh_actor_world_queue_packet_at(queue, 0, i, &batch));
        if (batch.stream == 1) {
            CHECK(batch.samples_display_base0 && !batch.material && !batch.tex0 &&
                  batch.primitive == 0x7c && batch.vertex_count == 14 && batch.triangles == 8);
            ++sampled;
        } else {
            CHECK(!batch.samples_display_base0 && batch.material);
        }
    }
    CHECK(sampled == 1);
    command.materials.streams[1].samples_display_base0 = 0;
    index = SIZE_MAX;
    CHECK(gh_actor_world_queue_append(queue, &command, &index, &render_frontier) ==
          GH_ACTOR_WORLD_UNSUPPORTED && render_frontier.ee_entry == 0x151368 &&
          index == SIZE_MAX && gh_actor_world_queue_count(queue) == 1);
    gh_actor_world_queue_destroy(queue);
    gh_stage_textures_destroy(textures);
}

static gh_effect_status lookup_el00(void *context,uint16_t kind,uint8_t code,
    const gh_title_player_view **view,const gh_actor_model **model,uint32_t *frontier){
    const gh_event_actor_factory *actor=context;
    CHECK(kind==0x600&&!code&&actor->kind==kind&&actor->model.ready);
    *view=&actor->object.view;
    *model=&actor->model;
    *frontier=0;
    return GH_EFFECT_OK;
}

static uint32_t effect_clock(void *context){
    uint32_t *clock=context;
    return ++*clock;
}

typedef struct {
    gh_game_effects *effects;
    unsigned rows;
    int world;
} ground_packet_probe;

static gh_effect_status verify_ground_packet(void *context,const gh_effect_trans_packet *p){
    ground_packet_probe *probe=context;
    gh_effect_esp_view particle;
    CHECK(!gh_effect_create_esp_at(probe->effects,p->slot,&particle)&&particle.serial==p->serial&&
      (probe->world?particle.emitter_slot<=3:(particle.emitter_slot>=1&&particle.emitter_slot<=3)));
    CHECK(p->kind==GH_EFFECT_PASS_SPRITE&&p->original_callee==0x2fec38&&
      p->material&&p->material->rgba_gs&&p->quad_count==1&&p->quads[0].vertex_count==4&&p->order.bank==2);
    probe->rows|=1u<<(particle.emitter_slot-(probe->world?0:1));
    return GH_EFFECT_OK;
}

static void check_event_ground_particles(gh_disc *disc,const uint8_t *elf,size_t elf_bytes,
    const gh_bundle *room,const gh_event_data_prefix *prefix,gh_event_actor_factory *actor){
    size_t core_size;
    uint8_t *core=load(disc,"etc/core.dat",&core_size);
    gh_game_effects *effects=calloc(1,sizeof(*effects));
    gh_game_init_state *init=calloc(1,sizeof(*init));
    uint8_t *esp=calloc(1,0x100180),*emitters=calloc(1,0x26060);
    CHECK(effects&&init&&esp&&emitters&&!gh_game_effects_initialize_00305a68(effects,
      esp,0x100180,emitters,0x26060,core,core_size,room));
    size_t offset=word(prefix->data+0x1c);
    CHECK(offset&&offset<prefix->bytes&&!gh_game_effects_register_event_002fa0b0(effects,
      prefix->data+offset,prefix->bytes-offset));
    gh_game_init_state_initialize(init);
    gh_game_mt_seed_0030f2b0(&init->random,123);
    gh_boot_work work={0};
    put_word((uint8_t *)&work+0x5e4,0x10040);
    put_word((uint8_t *)&work+0x574,0x3f800000);
    gh_effect_release_bindings storage={.effects=effects,.esp=esp,.esp_bytes=0x100180,
      .emitters=emitters,.emitter_bytes=0x26060};
    gh_effect_create_bindings producer={.storage=storage,.work=&work,.random=&init->random,
      .event_actor=lookup_el00,.event_actor_context=actor};
    gh_effect_create_call cut={0};
    CHECK(!gh_effect_create_event_cut_begin_002fa158(&cut,2)&&
      !gh_game_effect_create_00306528(&cut,&producer)&&cut.complete&&cut.count==4);
    /* Only these three source rows run in the isolated fixture. */
    put_word(emitters+0x26000,0x70000000);
    gh_effects_frame_globals globals={0};
    gh_camera_envelope envelope={0};
    gh_game_scene_tables actors={0};
    uint32_t frontier,clock=0;
    CHECK(!gh_effects_frame_globals_from_elf(&globals,elf,elf_bytes));
    globals.factor_741dc0=0x3f800000;
    CHECK(!gh_game_camera_envelope_from_elf(&envelope,elf,elf_bytes,&frontier));
    CHECK(!gh_scene_actors_0012e670(&actors,room));
    gh_game_scene_aux aux={0};
    gh_effects_frame_bindings b={.storage=storage,.work=&work,.init=init,.globals=&globals,
      .profile=&aux,.clock=effect_clock,.clock_context=&clock,.atan_coeff=envelope.atan_coeff,
      .policy=GH_STATIC_BINARY32_REFERENCE,.actors=&actors};
    gh_display_state display={0};
    gh_game_motion_camera camera={0};
    gh_event_camera_cut camera_cut;
    gh_event_camera_track track={.frame=13,.apply_motion=1};
    CHECK(!gh_display_state_from_elf(&display,elf,elf_bytes));
    CHECK(!gh_display_static_initialize_002a48a8(&display,1,0xffff));
    CHECK(!gh_game_motion_camera_construct_00138be8(&camera,GH_CAMERA_NATIVE_BINARY32_REFERENCE));
    CHECK(!gh_event_camera_init_play_00004bd8((gh_event_components *)prefix->components,init,&camera,&frontier));
    CHECK(!gh_event_camera_cut_at(prefix,2,&camera_cut)&&camera_cut.primary);
    CHECK(!gh_motion_open_mtb3(prefix->data,prefix->bytes,camera_cut.primary_offset,&track.track));
    CHECK(!gh_game_motion_camera_set_001390b8(&camera,&track));
    gh_camera_projection_display cd={&display.screen_width,&display.screen_height_bits,
      &display.pixel_aspect_bits,&display.initialized,GH_CAMERA_NATIVE_BINARY32_REFERENCE};
    CHECK(!gh_game_motion_camera_move_00138600(&camera,init,&work,&cd,&frontier));
    gh_stage_textures *textures;
    CHECK(!gh_stage_textures_create(&textures));
    ground_packet_probe drawn={.effects=effects};
    gh_effects_trans_bindings trans_b={.storage=storage,.work=&work,.globals=&globals,.display=&display,
      .policy=GH_STATIC_BINARY32_REFERENCE,.cvt_policy=GH_FRAME_CHOP_REFERENCE,.emit=verify_ground_packet,
      .context=&drawn,.camera=&camera.camera,.init=init,.textures=textures};
    gh_game_effects_frame frame={0};
    unsigned observed=0,spawned=0;
    const unsigned delays[]={13,14,24};
    for(unsigned tick=0;tick<240;tick++){
      gh_effect_status status=gh_game_effects_frame_update_0030f228(&frame,&b);
      if(status)fprintf(stderr,"EL00 type1A tick%u status%u PC%x emitter=%u stage%u ESPstep=%u\n",
        tick,status,frame.frontier,frame.emitter_index,frame.emitter_stage,frame.esp_stage);
      CHECK(!status);
      track.frame=(float)tick;
      if(track.frame>(float)(track.track.frames-1))track.frame=(float)(track.track.frames-1);
      CHECK(!gh_game_motion_camera_set_001390b8(&camera,&track));
      CHECK(!gh_game_motion_camera_move_00138600(&camera,init,&work,&cd,&frontier));
      gh_game_effects_trans trans={0};
      status=gh_game_effects_trans_002fb310(&trans,&trans_b);
      if(status)fprintf(stderr,"EL00 type1A Trans tick%u status%u PC%x slot%u stage%u\n",
        tick,status,trans.frontier,trans.esp_index,trans.call_stage);
      CHECK(!status);
      for(unsigned row=0;row<3;row++)if(tick==delays[row]){
        gh_effect_emitter_view emitter;
        CHECK(!gh_effect_create_emitter_at(effects,row+1,&emitter));
        CHECK(emitter.actor==&actor->object.view&&emitter.model==&actor->model&&emitter.part&&
          emitter.record.bytes[0x133]==0x1a&&(word(emitter.payload+0x114)&0x800000u));
        CHECK(!memcmp(emitter.payload+0x40,emitter.part->bytes+0xb0,12));
        float root_matrix[16],expected_euler[3];
        memcpy(root_matrix,actor->object.view.bytes+0x80,64);
        memcpy(root_matrix+12,emitter.part->bytes+0xb0,12);
        CHECK(!gh_actor_matrix_extract_euler_00147c88(root_matrix,envelope.atan_coeff,
          expected_euler,GH_STATIC_BINARY32_REFERENCE,&frontier));
        CHECK(!memcmp(emitter.payload+0x30,expected_euler,12));
        observed|=1u<<row;
      }
      for(unsigned slot=0;slot<1024;slot++){
        gh_effect_esp_view particle;
        if(gh_effect_create_esp_at(effects,slot,&particle))continue;
        CHECK(particle.initialized&&particle.type==0x1a&&particle.actor==&actor->object.view&&
          !particle.selector&&particle.parent_emitter==-1&&particle.emitter_slot>=1&&particle.emitter_slot<=3);
        spawned|=1u<<(particle.emitter_slot-1);
      }
    }
    CHECK(observed==7&&spawned==7&&(drawn.rows&1));
    int borrowed;
    CHECK(!gh_effect_create_actor_has_live_refs(effects,&actor->object.view,&borrowed)&&!borrowed);
    gh_stage_textures_destroy(textures);
    gh_scene_ray_triangle_state_release(&actors);
    gh_effect_create_forget_on_owner_destroy(effects);
    free(emitters);free(esp);free(init);free(effects);free(core);
    puts("PASS EV01 cut2 actual EL00 part emitter transforms, three type1A lifecycles and visible sprite");
}
static void check_event_world_particles(gh_disc *disc,const uint8_t *elf,size_t elf_bytes,
    const gh_bundle *room,const gh_event_data_prefix *prefix,gh_event_actor_factory *actor){
    size_t core_size;
    uint8_t *core=load(disc,"etc/core.dat",&core_size);
    gh_game_effects *effects=calloc(1,sizeof(*effects));
    gh_game_init_state *init=calloc(1,sizeof(*init));
    uint8_t *esp=calloc(1,0x100180),*emitters=calloc(1,0x26060);
    CHECK(effects&&init&&esp&&emitters&&!gh_game_effects_initialize_00305a68(effects,
      esp,0x100180,emitters,0x26060,core,core_size,room));
    size_t offset=word(prefix->data+0x1c);
    CHECK(offset&&offset<prefix->bytes&&!gh_game_effects_register_event_002fa0b0(effects,
      prefix->data+offset,prefix->bytes-offset));
    gh_game_init_state_initialize(init);
    gh_game_mt_seed_0030f2b0(&init->random,123);
    gh_boot_work work={0};
    put_word((uint8_t *)&work+0x5e4,0x10040);
    put_word((uint8_t *)&work+0x574,0x3f800000);
    gh_effect_release_bindings storage={.effects=effects,.esp=esp,.esp_bytes=0x100180,
      .emitters=emitters,.emitter_bytes=0x26060};
    gh_effect_create_bindings producer={.storage=storage,.work=&work,.random=&init->random,
      .event_actor=lookup_el00,.event_actor_context=actor};
    gh_effect_create_call cut={0};
    CHECK(!gh_effect_create_event_cut_begin_002fa158(&cut,3)&&
      !gh_game_effect_create_00306528(&cut,&producer)&&cut.complete&&cut.count==4);
    /* Only these three source rows run in the isolated fixture. */

    gh_effects_frame_globals globals={0};
    gh_camera_envelope envelope={0};
    gh_game_scene_tables actors={0};
    uint32_t frontier,clock=0;
    CHECK(!gh_effects_frame_globals_from_elf(&globals,elf,elf_bytes));
    globals.factor_741dc0=0x3f800000;
    CHECK(!gh_game_camera_envelope_from_elf(&envelope,elf,elf_bytes,&frontier));
    CHECK(!gh_scene_actors_0012e670(&actors,room));
    gh_game_scene_aux aux={0};
    gh_effects_frame_bindings b={.storage=storage,.work=&work,.init=init,.globals=&globals,
      .profile=&aux,.clock=effect_clock,.clock_context=&clock,.atan_coeff=envelope.atan_coeff,
      .policy=GH_STATIC_BINARY32_REFERENCE,.actors=&actors};
    gh_display_state display={0};
    gh_game_motion_camera camera={0};
    gh_event_camera_cut camera_cut;
    gh_event_camera_track track={.frame=0,.apply_motion=1};
    CHECK(!gh_display_state_from_elf(&display,elf,elf_bytes));
    CHECK(!gh_display_static_initialize_002a48a8(&display,1,0xffff));
    CHECK(!gh_game_motion_camera_construct_00138be8(&camera,GH_CAMERA_NATIVE_BINARY32_REFERENCE));
    CHECK(!gh_event_camera_init_play_00004bd8((gh_event_components *)prefix->components,init,&camera,&frontier));
    CHECK(!gh_event_camera_cut_at(prefix,3,&camera_cut)&&camera_cut.primary);
    CHECK(!gh_motion_open_mtb3(prefix->data,prefix->bytes,camera_cut.primary_offset,&track.track));
    CHECK(!gh_game_motion_camera_set_001390b8(&camera,&track));
    gh_camera_projection_display cd={&display.screen_width,&display.screen_height_bits,
      &display.pixel_aspect_bits,&display.initialized,GH_CAMERA_NATIVE_BINARY32_REFERENCE};
    CHECK(!gh_game_motion_camera_move_00138600(&camera,init,&work,&cd,&frontier));
    gh_stage_textures *textures;
    CHECK(!gh_stage_textures_create(&textures));
    ground_packet_probe drawn={.effects=effects,.world=1};
    gh_effects_trans_bindings trans_b={.storage=storage,.work=&work,.globals=&globals,.display=&display,
      .policy=GH_STATIC_BINARY32_REFERENCE,.cvt_policy=GH_FRAME_CHOP_REFERENCE,.emit=verify_ground_packet,
      .context=&drawn,.camera=&camera.camera,.init=init,.textures=textures};
    gh_game_effects_frame frame={0};
    unsigned spawned=0;
    for(unsigned tick=0;tick<240;tick++){
      gh_effect_status status=gh_game_effects_frame_update_0030f228(&frame,&b);
      if(status)fprintf(stderr,"EL00 type1A tick%u status%u PC%x emitter=%u stage%u ESPstep=%u\n",
        tick,status,frame.frontier,frame.emitter_index,frame.emitter_stage,frame.esp_stage);
      CHECK(!status);
      track.frame=(float)tick;
      if(track.frame>(float)(track.track.frames-1))track.frame=(float)(track.track.frames-1);
      CHECK(!gh_game_motion_camera_set_001390b8(&camera,&track));
      CHECK(!gh_game_motion_camera_move_00138600(&camera,init,&work,&cd,&frontier));
      gh_game_effects_trans trans={0};
      status=gh_game_effects_trans_002fb310(&trans,&trans_b);
      if(status)fprintf(stderr,"EL00 type1A Trans tick%u status%u PC%x slot%u stage%u\n",
        tick,status,trans.frontier,trans.esp_index,trans.call_stage);
      CHECK(!status);
      for(unsigned slot=0;slot<1024;slot++){
        gh_effect_esp_view particle;
        if(gh_effect_create_esp_at(effects,slot,&particle))continue;
        CHECK(particle.initialized&&particle.type==0&&!particle.actor&&
          !particle.selector&&particle.emitter_slot<=3);
        spawned|=1u<<particle.emitter_slot;
      }
    }
    CHECK(spawned==15&&drawn.rows==15);
    int borrowed;
    CHECK(!gh_effect_create_actor_has_live_refs(effects,&actor->object.view,&borrowed)&&!borrowed);
    gh_stage_textures_destroy(textures);
    gh_scene_ray_triangle_state_release(&actors);
    gh_effect_create_forget_on_owner_destroy(effects);
    free(emitters);free(esp);free(init);free(effects);free(core);
    puts("PASS EV01 cut3 actual four class0 world sprite lifecycles");
}

typedef struct {
    gh_event_release_stage stage;
    gh_event_release_bindings bindings;
    uint32_t sites[12];
    size_t count;
    int block_enemy;
} release_fixture;

static gh_game_event_status release_test_leaf(void *context,uint32_t site,uint32_t *frontier,int *relative)
{
    release_fixture *f=context;
    CHECK(f->count<sizeof(f->sites)/sizeof(*f->sites));
    f->sites[f->count++]=site;
    *frontier=0;*relative=0;
    if(site==0x2972b0){
        const gh_game_event_prefix *p=&f->bindings.event->fields;
        CHECK(!p->flags&&!p->state&&!p->phase&&!p->unknown06&&!p->unknown07&&!p->data_number&&!p->text_token);
        CHECK(f->bindings.config->flags==8);
        if(f->block_enemy){*frontier=0x294ca0;return GH_EVENT_UNSUPPORTED;}
    }
    return GH_EVENT_OK;
}

static int release_test_stage(void *context,gh_game_event *event,int *done)
{
    release_fixture *f=context;
    CHECK(event==f->bindings.event);
    *done=0;
    uint32_t frontier;
    int relative;
    gh_game_event_status status=gh_event_release_00297038(&f->stage,&f->bindings,&frontier,&relative);
    if(status)event->frontier=frontier;
    return status;
}

int main(int argc, char **argv) {
    CHECK(argc == 2 || (argc == 3 && !strcmp(argv[2], "--ev02")));
    const int ev02 = argc == 3;
    gh_disc *disc = NULL;
    CHECK(gh_disc_open(argv[1], &disc) == GH_DISC_OK);
    const size_t elf_bytes = (size_t)gh_disc_boot_elf_size(disc);
    uint8_t *elf = malloc(elf_bytes);
    CHECK(elf && gh_disc_read_boot_elf(disc, 0, elf, elf_bytes) == GH_DISC_OK);
    gh_game_event event = {0};
    CHECK(gh_game_event_from_elf(&event, elf, elf_bytes) == GH_EVENT_OK);
    event.fields.data_number = 1;

    gh_heap *heap = NULL;
    gh_boot_work work = {0};
    gh_event_module module = {0};
    gh_event_resource resource = {0};
    gh_dvd_jobs dvd;
    CHECK(gh_heap_create(32 * 1024 * 1024, &heap) == GH_MEM_OK);
    gh_dvd_jobs_init(&dvd, disc);
    CHECK(gh_game_event_module_begin_00297378(&module, &event, &work, &dvd, heap) == GH_EVENT_OK);
    int ready = 0;
    for (unsigned i = 0; i < 8 && !ready; ++i) {
        CHECK(gh_dvd_pump(&dvd) == GH_DVD_OK);
        CHECK(gh_game_event_module_poll_00297428(&module, &ready) == GH_EVENT_OK);
    }
    CHECK(ready && module.loaded);

    gh_event_components components = {0};
    CHECK(gh_event_components_bind(&components, &module, elf, elf_bytes) == GH_EVENT_OK);
    CHECK(gh_event_components_construct_00297680(&components, 1) == GH_EVENT_OK);
    CHECK(gh_game_event_resource_begin_00297540(&resource, &event, &dvd, heap) == GH_EVENT_OK);
    ready = 0;
    for (unsigned i = 0; i < 1024 && !ready; ++i) {
        CHECK(gh_dvd_pump(&dvd) == GH_DVD_OK);
        CHECK(gh_game_event_resource_poll_002975d0(&resource, &ready) == GH_EVENT_OK);
    }
    CHECK(ready && resource.loaded && resource.file_bytes == 873872);

    gh_event_component_view actor;
    CHECK(gh_event_component_at(&components, 2, &actor) == GH_EVENT_OK);
    gh_event_data_prefix prefix = {0};
    uint32_t frontier = 0;
    CHECK(gh_event_data_bind_prefix_002977e8(&prefix, &components, &resource, &frontier) == GH_EVENT_OK);
    CHECK(gh_event_actor_cuts_bind_00297864(&prefix, &frontier) == GH_EVENT_OK);
    CHECK(prefix.actor_count == 9 && prefix.camera_count == 10);

    size_t room_bytes, el_bytes;
    uint8_t *room = load(disc, ev02 ? "st0/r003.dat" : "st1/r100.dat", &room_bytes);
    uint8_t *el = load(disc, "el/el00.dat", &el_bytes);
    gh_bundle room_bundle;
    CHECK(gh_bundle_open(room, room_bytes, &room_bundle) == GH_BUNDLE_OK);
    gh_room_pools pools = {0};
    CHECK(gh_game_room_pools_initialize_002a5d80(&pools, &room_bundle, heap) == GH_POOL_OK);
    const uint16_t kinds[] = {0x100, 0x309, 0x376, 0x500, 0x1};
    uint8_t raw[5][0x300] = {{0}};
    gh_title_player_view views[5] = {{0}};
    uint32_t handles[5];
    object_scope scope = {.pools = &pools, .handles = handles, .kinds = kinds, .count = 5};
    for (size_t i = 0; i < 5; ++i) {
        views[i].bytes = raw[i];
        views[i].size = sizeof(raw[i]);
        raw[i][0x2fe] = i == 4 ? 0 : (uint8_t)kinds[i];
        raw[i][0x2ff] = (uint8_t)(kinds[i] >> 8);
        put_word(raw[i] + 0x250, 0x200400);
        CHECK(gh_room_object_register(&pools, &views[i], &scope, kinds[i], 1, &handles[i]) == GH_POOL_OK);
    }

    gh_stage_slots holders = {0};
    gh_game_objects *objects;
    CHECK(gh_game_objects_create(&holders, 0, &objects) == GH_OBJECT_OK);
    CHECK(gh_stage_slots_reset(&holders, NULL) == GH_STAGE_OK);
    int original = 0;
    CHECK(gh_game_objects_add_pinned_001fece0(objects, 0x600, el, el_bytes, &original) == GH_OBJECT_OK && original);

    gh_task_record32 tasks[4] = {{0}};
    gh_task_manager manager = {.records = tasks, .capacity = 4, .count = 4, .current_index = -1};
    gh_task_threads *threads;
    CHECK(gh_task_threads_create(&manager, &threads) == GH_THREAD_OK);
    uint8_t save[GH_CORE_SAVE_BYTES] = {0};
    save[0xae] = 6;
    gh_event_actor_create state = {0};
    gh_event_actor_create_bindings bindings = {
        .components = &components, .data = &prefix, .heap = heap, .pools = &pools,
        .objects = objects, .global_threads = threads, .save = {save, sizeof(save)},
        .object_view = object_view, .context = &scope
    };
    const size_t before = gh_heap_used(heap);
    scope.fail = handles[2];
    put_word(actor.bytes, 2);
    CHECK(gh_event_actor_create_start_002979f0(&state, &bindings, &frontier) == GH_EVENT_UNSUPPORTED);
    CHECK(frontier == 0x123456 && state.object_cursor == 2 && scope.calls == 3);
    CHECK(gh_heap_used(heap) == before + 648 && word(raw[0] + 0x250) == 0x208400);
    CHECK(word(raw[1] + 0x250) == 0x200400 && word(raw[2] + 0x250) == 0x200400);
    const uint8_t *row;
    size_t cut;
    CHECK(gh_event_actor_create_work_at(&state, 8, &row, &cut) == GH_EVENT_OK);
    CHECK(row == (uint8_t *)state.work.pointer + 8 * 72 && cut == 0x9f0 && !word(row + 0x44));
    CHECK(gh_event_actor_create_work_at(&state, 9, &row, &cut) == GH_EVENT_BOUNDS);

    scope.fail = 0;
    CHECK(gh_event_actor_create_start_002979f0(&state, &bindings, &frontier) == GH_EVENT_OK);
    CHECK(!frontier && scope.calls == 6 && state.task_slot == 2 && tasks[2].running == 2);
    CHECK(word(raw[2] + 0x250) == 0x208400 && word(raw[3] + 0x250) == 0x208400);
    CHECK(word(raw[4] + 0x250) == 0x200400 && !word(actor.bytes));
    CHECK(gh_event_actor_create_task_status(&state, &frontier) == GH_EVENT_OK && !frontier);
    CHECK(gh_tasks_run_matching_002d52a0(&manager).status == GH_TASK_OK);
    CHECK(gh_event_actor_create_task_status(&state, &frontier) == GH_EVENT_UNSUPPORTED && frontier == 0x312a80);
    CHECK(gh_event_actor_create_work_at(&state, 0, &row, &cut) == GH_EVENT_OK && cut == 0x130);
    CHECK(word(row + 4) == 0x600 && word(row + 0x40) == 0x6f6 && row[0x24] == 1);
    CHECK(state.pending.loaded_resource.bytes == el && state.pending.loaded_resource.size == el_bytes);
    CHECK(state.pending.source_kind == 0x600 && state.pending.resource_kind == 0x600 &&
          state.pending.create_kind == 0x6f6 && state.pending.create_argument == 0xffff);
    CHECK(gh_event_actor_creation_poll_00297a68(&components, &ready) == GH_EVENT_OK && !ready);
    CHECK(gh_event_actor_create_abort(&state) == GH_EVENT_OK && gh_heap_used(heap) == before && !tasks[2].running);

    CHECK(gh_stage_slots_reset(&holders, NULL) == GH_STAGE_OK);
    gh_stage_slots room_holders = {0};
    gh_game_objects *room_objects;
    CHECK(gh_game_objects_create(&room_holders, 1, &room_objects) == GH_OBJECT_OK);
    CHECK(gh_stage_slots_reset(&room_holders, heap) == GH_STAGE_OK);
    gh_event_actor_factory factory = {0};
    uint8_t serial = 255;
    const uint32_t fog = 0x325476;
    gh_event_actor_factory_bindings factory_bindings = {
        .pools = &pools, .global_objects = objects, .room_objects = room_objects,
        .work = &work, .factory_serial = &serial,
        .resources = {.dvd = &dvd, .heap = heap, .threads = threads},
        .packet_heap = heap, .fog_rgb = &fog
    };
    CHECK(gh_event_actor_factory_bind(&factory, &factory_bindings) == GH_EVENT_OK);
    gh_event_actor_create_request request = {
        .source_kind = 0x600, .resource_kind = 0x600, .create_kind = 0x600, .create_argument = 0xffff
    };
    put_word((uint8_t *)&work + 0x594, 0x100000);
    int nonnull = -1;
    CHECK(gh_event_actor_factory_run(&factory, &request, &nonnull, &frontier) == GH_EVENT_OK);
    CHECK(!nonnull && !frontier && !factory.stage && serial == 255 && gh_heap_used(heap) == before);
    put_word((uint8_t *)&work + 0x594, 0);
    save[0xae] = 0;
    bindings.factory = gh_event_actor_factory_run;
    bindings.factory_context = &factory;
    bindings.initialize = gh_event_actor_factory_initialize;
    CHECK(gh_event_actor_create_start_002979f0(&state, &bindings, &frontier) == GH_EVENT_OK);
    CHECK(gh_tasks_run_matching_002d52a0(&manager).status == GH_TASK_OK);
    CHECK(factory.stage == 2 && factory.load.stage == 1 && !factory.handle && !serial);
    CHECK(gh_event_actor_create_task_status(&state, &frontier) == GH_EVENT_OK && !frontier);
    CHECK(!word(actor.bytes));
    for (unsigned i = 0; i < 1024 && !factory.handle; ++i) {
        CHECK(gh_dvd_pump(&dvd) == GH_DVD_OK);
        CHECK(gh_tasks_run_matching_002d52a0(&manager).status == GH_TASK_OK);
    }
    CHECK(factory.handle && factory.stage == 5 && factory.global_reference && !factory.room_reference);
    CHECK(factory.resource.file_extent == el_bytes && !memcmp(factory.resource.bytes, el, el_bytes));
    gh_game_event_status task_status = gh_event_actor_create_task_status(&state, &frontier);
    if (task_status != GH_EVENT_UNSUPPORTED || frontier != 0x297a48) {
        fprintf(stderr, "Actor task status %u frontier %06x model stage %u detail %u init stage %u\n",
            task_status, frontier, factory.model.stage, factory.model.detail, factory.init_stage);
    }
    CHECK(task_status == GH_EVENT_UNSUPPORTED && frontier == 0x297a48);
    CHECK(state.first_actor_handle == factory.handle && factory.init_stage == 4 && state.frontier_rel == 0x1a30);
    CHECK(factory.model.ready && factory.model.parts_ready && factory.model.root.bytes == factory.object.view.bytes);
    const unsigned model_parts = factory.model.parts_allocated;
    CHECK(model_parts == 32 && factory.model.part_count == model_parts);
    CHECK(factory.shadow.object == &factory.object.view && factory.shadow.pattern_original == 0x3c3f68);
    CHECK(factory.geometry.bytes && factory.texture.bytes);
    CHECK(gh_event_actor_creation_poll_00297a68(&components, &ready) == GH_EVENT_OK && !ready);
    gh_title_player_view published;
    CHECK(gh_event_actor_factory_view(&factory, factory.handle, &published) == GH_EVENT_OK);
    CHECK(published.size == 0x30e0 && published.original_vtable214 == 0x44b528 && published.bytes[0x2fc] == 255);
    CHECK(published.bytes[0x2ff] == 6 && word(published.bytes + 0x2d0) == 0x800000);
    CHECK((word(published.bytes + 0x250) & 0x2018000) == 0x2018000 &&
          word(published.bytes + 0x13ac) == 0x3f800000);
    CHECK(word(published.bytes + 0x13cc) == 0 && word(published.bytes + 0x30dc) == 0x3f800000);
    CHECK(!word(published.bytes + 0x304) && !word(published.bytes + 0x2c8));
    CHECK(word(published.bytes + 0x4dc) == 3 && (word(published.bytes + 0x4a8) & 0x20000000u));
    CHECK(published.bytes[0x2f4] == 1 && (word(published.bytes + 0x2f0) & 0x1000u));
    CHECK(!word(published.alias_f0) && !word(published.bytes + 0x100) && word(published.bytes + 0x110) == 0x3f800000);
    CHECK(pools.vectors[0].count == 6 && pools.vectors[3].count == 1);
    CHECK(word((uint8_t *)pools.vectors[0].allocation.pointer + 20) == factory.handle &&
          word(pools.vectors[3].allocation.pointer) == factory.handle);
    CHECK(gh_event_actor_factory_view(&factory, handles[0], &published) == GH_EVENT_ARGUMENT);

    size_t pl00_bytes, pl01_bytes;
    uint8_t *pl00 = load(disc, "pl/pl00.dat", &pl00_bytes);
    uint8_t *pl01 = load(disc, "pl/pl01.dat", &pl01_bytes);
    CHECK(gh_game_objects_add_pinned_001fece0(objects, 0x100, pl00, pl00_bytes, &original) == GH_OBJECT_OK && original);
    CHECK(gh_game_objects_add_pinned_001fece0(objects, 0x101, pl01, pl01_bytes, &original) == GH_OBJECT_OK && original);
    CHECK(gh_game_objects_set_file_extent(objects, 0x101, pl01, pl01_bytes) == GH_OBJECT_OK);
    gh_game_title_player player = {0};
    gh_game_player_factory parent = {0};
    gh_player_factory_bindings parent_bindings = {
        .player = &player, .pools = &pools, .global_objects = objects, .room_objects = room_objects,
        .work = &work, .disc = disc, .save = {save, sizeof(save)}, .factory_serial = &serial
    };
    CHECK(gh_game_player_factory_create_0030fa98(&parent, &parent_bindings, &original) == GH_PLAYER_OK && original);
    gh_actor_model parent_model = {0};
    gh_bundle parent_bundle;
    gh_bundle_entry parent_md, parent_texture;
    CHECK(gh_bundle_open(pl00, pl00_bytes, &parent_bundle) == GH_BUNDLE_OK);
    CHECK(gh_bundle_find(&parent_bundle, "MD", 0, &parent_md) == GH_BUNDLE_OK);
    CHECK(gh_bundle_find(&parent_bundle, "TM3", 0, &parent_texture) == GH_BUNDLE_OK);
    CHECK(gh_actor_model_bind_packet_inputs(&parent_model, heap, &fog) == GH_ACTOR_MODEL_OK);
    CHECK(gh_actor_model_initialize_00149738(&parent_model, &player.view, &work, &pools, &parent_md,
        &parent_texture, 0, 0, GH_STATIC_BINARY32_REFERENCE, &original) == GH_ACTOR_MODEL_OK && original);

    gh_game_player_child child = {0};
    gh_player_child_bindings child_bindings = {
        .pools = &pools, .global_objects = objects, .room_objects = room_objects, .work = &work,
        .factory_serial = &serial, .resources = factory_bindings.resources
    };
    CHECK(gh_game_player_child_create_0030fa98(&child, &child_bindings, &original) == GH_PLAYER_OK && original);
    CHECK(gh_actor_model_bind_packet_inputs(&child.model, heap, &fog) == GH_ACTOR_MODEL_OK);
    CHECK(gh_game_player_child_initialize_00100378(&child, &original) == GH_PLAYER_UNSUPPORTED);
    CHECK(child.frontier == 0x100420 && child.model.ready);
    gh_player_child_attachment attachment = {0};
    CHECK(gh_game_player_child_attach_00100ed8(&attachment, &child, &parent, &parent_model,
        17, bindings.save) == GH_PLAYER_OK);
    CHECK(attachment.attached && parent_model.external_borrowers == 1);
    put_word(child.object.view.bytes + 0x250, 0x8002);
    put_word(child.object.view.bytes + 0x2f0, 0x20);
    gh_event_weapon_slot weapon = {0};
    gh_event_actor_extra extra = {0};
    gh_event_actor_extra_bindings extra_bindings = {
        .factory = &factory, .player = &player.view, .child = &child, .attachment = &attachment,
        .weapon_slot = &weapon, .save = bindings.save
    };
    CHECK(gh_event_actor_extra_bind(&extra, &extra_bindings) == GH_EVENT_OK);
    CHECK(gh_event_actor_extra_run(&extra, &state.pending, factory.handle, &original, &frontier) == GH_EVENT_UNSUPPORTED);
    CHECK(frontier == 0x1cf160 && !original && !extra.mesh_cursor && !parent_model.external_borrowers);
    CHECK(child.model.external_parent == &factory.model && factory.model.external_borrowers == 1);
    CHECK(attachment.parent_part == factory.model.parts + 17 && child.model.external_owner == &attachment);
    CHECK(word(child.object.view.bytes + 0x250) == 0x10000 && word(child.object.view.bytes + 0x2f0) == 0x21);
    CHECK(word(state.pending.work + 0x34) == 0xffff && !word(state.pending.work + 0x30));
    CHECK(gh_event_actor_factory_abort(&factory) == GH_EVENT_CALLEE && factory.handle);
    weapon.known = 1;
    CHECK(gh_event_actor_extra_run(&extra, &state.pending, factory.handle, &original, &frontier) == GH_EVENT_OK);
    CHECK(original && !frontier && extra.mesh_cursor == 3 && word(state.pending.work + 0x30) == 0x101);
    const uint8_t hidden_names[][8] = {"RH_01", "LH_01", "LH_02"};
    for (size_t i = 0; i < 3; ++i) {
        gh_player_mesh_reference ref;
        CHECK(gh_actor_model_find_mesh_0014b730(&factory.model, hidden_names[i], 8, &ref, &original) == GH_PLAYER_NODES_OK);
        CHECK(original && (word(ref.bytes + 0x380) & 1));
    }
    CHECK(gh_event_actor_extra_abort(&extra) == GH_EVENT_OK);
    CHECK(child.model.external_parent == &parent_model && attachment.parent_model == &parent_model &&
          parent_model.external_borrowers == 1 && !factory.model.external_borrowers);
    CHECK(word(child.object.view.bytes + 0x250) == 0x8002 && word(child.object.view.bytes + 0x2f0) == 0x20);
    CHECK(gh_event_actor_create_abort(&state) == GH_EVENT_OK);
    CHECK(gh_event_actor_factory_abort(&factory) == GH_EVENT_OK);
    CHECK(pools.vectors[0].count == 7 && pools.vectors[3].count == 2);

    if (ev02) {
        CHECK(!gh_game_event_resource_abort(&resource));
        event.fields.data_number = 2;
        CHECK(!gh_event_components_construct_00297680(&components, 2));
        CHECK(!gh_game_event_resource_begin_00297540(&resource, &event, &dvd, heap));
        ready = 0;
        for (unsigned i = 0; i < 1024 && !ready; ++i) {
            CHECK(!gh_dvd_pump(&dvd));
            CHECK(!gh_game_event_resource_poll_002975d0(&resource, &ready));
        }
        CHECK(ready && resource.loaded);
        prefix = (gh_event_data_prefix){0};
        CHECK(!gh_event_data_bind_prefix_002977e8(&prefix, &components, &resource, &frontier));
        CHECK(prefix.actor_count == 22 && prefix.camera_count == 28);
        CHECK(!gh_event_actor_cuts_bind_00297864(&prefix, &frontier));
    }
    const size_t roster_capacity = ev02 ? GH_EVENT_ACTOR_ROSTER_CAPACITY : 9;
    gh_event_actor_factory *rows = calloc(roster_capacity, sizeof(*rows));
    CHECK(rows);
    gh_event_actor_roster roster = {0};
    CHECK(gh_event_actor_roster_bind(&roster, &factory_bindings, rows, roster_capacity) == GH_EVENT_OK);
    gh_game_math_globals math = {0};
    CHECK(gh_game_math_globals_from_elf(&math, elf, elf_bytes) == GH_MATH_GLOBALS_OK);
    CHECK(gh_game_math_static_initialize_003079c8(&math, &work, 1, 0xffff) == GH_MATH_GLOBALS_OK);
    extra_bindings.factory = &rows[0];
    extra_bindings.roster = &roster;
    extra_bindings.creation = &state;
    extra_bindings.math = &math;
    extra_bindings.boot_elf = elf;
    extra_bindings.boot_elf_size = elf_bytes;
    CHECK(gh_event_actor_extra_bind(&extra, &extra_bindings) == GH_EVENT_OK);
    gh_event_component_view env;
    CHECK(gh_event_component_at(&components, 0, &env) == GH_EVENT_OK);
    put_word(env.bytes + 0x28, 0x40000000); // Original gate skips earlier, already tested suspension.
    bindings.factory = gh_event_actor_roster_run;
    bindings.initialize = gh_event_actor_roster_initialize;
    bindings.factory_context = &roster;
    bindings.extra = gh_event_actor_extra_run;
    bindings.extra_context = &extra;
    bindings.row_limit = roster_capacity;
    bindings.finish = gh_event_actor_extra_finish;
    CHECK(gh_event_actor_create_start_002979f0(&state, &bindings, &frontier) == GH_EVENT_OK);
    task_status = GH_EVENT_OK;
    ready = 0;
    for (unsigned i = 0; i < 1024 && !task_status && !ready; ++i) {
        CHECK(gh_dvd_pump(&dvd) == GH_DVD_OK);
        CHECK(gh_tasks_run_matching_002d52a0(&manager).status == GH_TASK_OK);
        task_status = gh_event_actor_create_task_status(&state, &frontier);
        CHECK(gh_event_actor_creation_poll_00297a68(&components, &ready) == GH_EVENT_OK);
    }
    if (ev02) {
        if (task_status != GH_EVENT_UNSUPPORTED || state.actor_cursor != 11)
            fprintf(stderr, "EV02 ActorCreate status %u frontier %08x REL %04x row %zu model detail %u\n",
                task_status, frontier, state.frontier_rel, state.actor_cursor, state.actor_cursor < roster_capacity ? rows[state.actor_cursor].model.detail : 0);
        CHECK(task_status == GH_EVENT_UNSUPPORTED && frontier == 0x297a48 &&
              state.frontier_rel == 0x04d0 && state.actor_cursor == 11 && !ready);
        for (unsigned i = 0; i < 11; ++i)
            CHECK(state.actor_handles[i] && rows[i].handle == state.actor_handles[i] && rows[i].init_stage == 4);
        CHECK(rows[0].kind == 0x600 && rows[1].kind == 0x601 && rows[2].kind == 0x602 && rows[3].kind == 0x605);
        CHECK(rows[6].kind == 0x60a && rows[10].kind == 0x631 && extra.accessory_handle &&
              extra.olivia_accessory.actor_index == 6 && extra.accessory_parts.parent == &rows[6].model);
        CHECK(rows[4].kind == 0x606 && rows[5].kind == 0x607);
        CHECK(!word(rows[4].object.view.bytes + 0x4dc) && !word(rows[5].object.view.bytes + 0x4dc));
        static const uint8_t parts[][4] = {{0, 1, 2, 0}, {0, 1, 17, 18}};
        static const uint8_t parents[][4] = {{2, 3, 4, 0}, {15, 16, 9, 10}};
        for (unsigned i = 0; i < 2; ++i) {
            CHECK(word(rows[4 + i].object.view.bytes + 0x30a4) == 2 - i);
            CHECK(extra.row_parts[4 + i].parent == &rows[3].model);
            for (unsigned j = 0; j < i + 3; ++j) {
                const uint8_t *world;
                CHECK(!gh_actor_model_external_parts_at(&rows[4 + i].model, parts[i][j], &world));
                CHECK(world == rows[3].model.parts[parents[i][j]].bytes);
            }
        }
        CHECK(rows[3].shadow.initialized && word(rows[3].object.view.bytes + 0x4dc) == 3);
        size_t el05_bytes;
        uint8_t *el05 = load(disc, "el/el05.dat", &el05_bytes);
        CHECK(rows[3].resource.file_extent == el05_bytes && !memcmp(rows[3].resource.bytes, el05, el05_bytes));
        free(el05);
        gh_player_mesh_reference right_hand;
        const uint8_t right_hand_name[8] = "RH_01";
        CHECK(!gh_actor_model_find_mesh_0014b730(&rows[3].model, right_hand_name, 8, &right_hand, &original) &&
              original && (word(right_hand.bytes + 0x380) & 1));
        CHECK(!gh_event_actor_create_work_at(&state, 3, &row, &cut));
        CHECK(word(row + 0x30) == 0x102 && word(row + 0x34) == 0xffff && !word(row + 0x38));
        CHECK(child.model.external_parent == &rows[3].model && attachment.parent_model == &rows[3].model &&
              attachment.parent_part == &rows[3].model.parts[17] && rows[0].model.external_borrowers == 2 &&
              rows[3].model.external_borrowers == 3);
        CHECK((word(child.object.view.bytes + 0x250) & 0x18002) == 0x10000 &&
              (word(child.object.view.bytes + 0x2f0) & 1));
        CHECK(!gh_event_actor_create_abort(&state));
        CHECK(!gh_event_actor_extra_abort(&extra));
        CHECK(child.model.external_parent == &parent_model && attachment.parent_model == &parent_model &&
              parent_model.external_borrowers == 1 && !rows[3].model.external_borrowers);
        CHECK(!gh_event_actor_roster_abort(&roster));
        free(rows);
        gh_game_player_child_attachment_clear(&attachment);
        CHECK(!gh_game_player_child_release_prefix(&child));
        CHECK(!gh_actor_model_release(&parent_model));
        CHECK(!gh_game_player_factory_release_prefix(&parent));
        CHECK(!gh_task_threads_destroy(threads));
        gh_game_objects_destroy(room_objects);
        gh_game_objects_destroy(objects);
        for (size_t i = 0; i < 5; ++i) CHECK(!gh_room_object_unregister(&pools, handles[i], &scope));
        CHECK(!gh_game_room_pools_release(&pools));
        CHECK(!gh_game_event_resource_abort(&resource));
        CHECK(!gh_game_event_module_abort(&module));
        CHECK(!gh_heap_used(heap) && !gh_heap_destroy(heap));
        free(room);
        free(el);
        free(pl00);
        free(pl01);
        free(elf);
        gh_disc_close(disc);
        puts("PASS R003 EV02 EL00 through EL31 creation, jacket and EL06/07 borrows, rollback; EL34 strict");
        return 0;
    }
    if (task_status || state.actor_cursor != 9) {
        fprintf(stderr, "EL roster status %u frontier %06x REL %04x row %zu details %u/%u\n",
            task_status, frontier, state.frontier_rel, state.actor_cursor, rows[1].model.detail, rows[2].model.detail);
    }
    CHECK(!task_status && !frontier && !state.frontier_rel && state.actor_cursor == 9 && ready);
    CHECK(word(actor.bytes) == 1 && rows[0].model.external_borrowers == 3 && !state.task_live);
    CHECK(gh_event_actor_creation_end_normal_000012a8(&components, &original) == GH_EVENT_OK && original);
    CHECK(extra.row_parts[0].count == 3 && extra.row_parts[1].count == 4);
    CHECK(pools.vectors[0].count == 17 && pools.vectors[3].count == 12);
    CHECK(word((uint8_t *)pools.vectors[0].allocation.pointer + 16 * 4) == child.handle);
    CHECK(rows[3].kind == 0x60a && rows[3].shadow.initialized && rows[3].object.view.bytes[0x2fa0] == 1);
    {
        gh_event_actor_factory *el0a = &rows[3];
        CHECK(el0a->model.part_count == 30);
        const gh_actor_part *origin, *bust;
        CHECK(!gh_actor_model_part_at(&el0a->model, 0, &origin));
        CHECK(!gh_actor_model_part_at(&el0a->model, 29, &bust));
        uint8_t *root = el0a->object.view.bytes;
        uint8_t saved[0x40], origin_xyz[12], matrix[64], euler_x[4], env_frame[4];
        memcpy(saved, root + 0x2fb0, sizeof(saved));
        memcpy(origin_xyz, origin->bytes + 0xb0, sizeof(origin_xyz));
        memcpy(matrix, bust->bytes + 0x80, sizeof(matrix));
        memcpy(euler_x, bust->bytes + 0x100, sizeof(euler_x));
        memcpy(env_frame, env.bytes + 0x14, sizeof(env_frame));
        uint32_t flags = word(root + 0x2f0);
        CHECK(!gh_event_actor_bust_002b6a40(el0a, &components, &frontier) && !frontier);
        CHECK(!memcmp(root + 0x2fd0, origin_xyz, 12) && !memcmp(root + 0x2fe0, origin_xyz, 12));

        /* Explicit numeric pose on the actual actor and component owners.
         * World translation (1,2,3) maps the input offset to local y=0. */
        const float identity_translated[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 1,2,3,1};
        const float current[3] = {2,8,4}, old[3] = {1,1,3};
        const float offset[4] = {1,2,3,1};
        const float phase = 1, zero = 0, angle = 0.125f, velocity = 0.125f;
        memcpy(origin->bytes + 0xb0, current, 12);
        memcpy(bust->bytes + 0x80, identity_translated, 64);
        uint32_t no_reset = flags & ~1u;
        memcpy(root + 0x2f0, &no_reset, 4);
        const int modes[] = {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO};
        for (unsigned mode = 0; mode < 4; ++mode) {
            CHECK(!fesetround(modes[mode]));
            for (unsigned reset = 0; reset < 2; ++reset) {
                memcpy(env.bytes + 0x14, reset ? &zero : &phase, 4);
                memcpy(root + 0x2fb0, offset, 16);
                memcpy(root + 0x2fc0, &velocity, 4);
                memcpy(root + 0x2fe0, old, 12);
                memcpy(bust->bytes + 0x100, &angle, 4);
                CHECK(!gh_event_actor_bust_002b6a40(el0a, &components, &frontier) && !frontier);
                CHECK(fegetround() == modes[mode]);
                CHECK(word(root + 0x2fb0) == 0x3f800000u && word(root + 0x2fb4) == 0x40000000u &&
                      word(root + 0x2fb8) == 0x4041eb85u && word(root + 0x2fbc) == 0x3f800000u);
                CHECK(word(bust->bytes + 0x100) == (reset ? 0 : 0x3e800000u));
                float actual_velocity;
                memcpy(&actual_velocity, root + 0x2fc0, 4);
                CHECK(fabsf(actual_velocity - (reset ? 0.0f : -0.025f)) < 1e-7f);
            }
        }
        CHECK(!fesetround(FE_TONEAREST));
        memcpy(root + 0x2fb0, saved, sizeof(saved));
        memcpy(root + 0x2f0, &flags, 4);
        memcpy(origin->bytes + 0xb0, origin_xyz, sizeof(origin_xyz));
        memcpy(bust->bytes + 0x80, matrix, sizeof(matrix));
        memcpy(bust->bytes + 0x100, euler_x, sizeof(euler_x));
        memcpy(env.bytes + 0x14, env_frame, sizeof(env_frame));
        puts("PASS actual EL0A part29 Bust update, motion and reset branches, four FEnv modes");
    }

    CHECK(extra.accessory_handle && extra.olivia_accessory.kind == 0x60d && extra.olivia_accessory.model.ready);
    CHECK(extra.olivia_accessory.init_stage == 2 && extra.olivia_accessory.object.view.bytes[0x2b5] == 0xfd);
    CHECK(word(extra.olivia_accessory.object.view.bytes + 0x30a4) == 4 && extra.accessory_parts.count == 7);
    size_t accessory_bytes;
    uint8_t *accessory = load(disc, "el/el0d.dat", &accessory_bytes);
    CHECK(extra.olivia_accessory.resource.file_extent == accessory_bytes &&
          !memcmp(extra.olivia_accessory.resource.bytes, accessory, accessory_bytes));
    free(accessory);
    CHECK(gh_event_actor_create_work_at(&state, 3, &row, &cut) == GH_EVENT_OK);
    CHECK(word(row + 0x30) == 0x60d && (row[0x24] & 8));
    CHECK(rows[4].kind == 0x60b && rows[4].model.ready && extra.pendulum_parent == &rows[3].model);
    CHECK(word(rows[4].object.view.bytes + 0x1394) == 0x2a0 && rows[4].object.view.bytes[0x1390] == 1);
    CHECK(word(rows[4].object.view.bytes + 0x2f20) == 10 && rows[4].object.view.bytes[0x2b5] == 0xfe);
    CHECK(extra.pendulum_profile.loaded && extra.pendulum_profile.original_scratch == 0x61af10);
    const uint8_t *pendulum = rows[4].object.view.bytes + 0x13a0;
    CHECK(word(pendulum + 38 * 0xa0 + 4) == 0xbd75c28f &&
          word(pendulum + 38 * 0xa0 + 0x60) == 0x3d75c28f);
    CHECK(word(pendulum + 31 * 0xa0 + 0x60) == 0);
    static const uint16_t ids[] = {0x600, 0x601, 0x602, 0x60a, 0x60b, 0x60c, 0x630, 0x631, 0x667};
    for (unsigned i = 0; i < sizeof(ids) / sizeof(*ids); ++i) {
        CHECK(rows[i].kind == ids[i] && rows[i].handle == state.actor_handles[i] && rows[i].model.ready);
        CHECK(gh_event_actor_create_work_at(&state, i, &row, &cut) == GH_EVENT_OK);
        CHECK(word(row + 4) == ids[i] && word(row + 0x40) == ids[i]);
    }
    gh_event_actor_play play = {0};
    gh_event_actor_play_bindings play_bindings = {.creation = &state, .roster = &roster, .extra = &extra};
    CHECK(gh_event_actor_play_init_00001468(&play, &play_bindings, &frontier) == GH_EVENT_OK);
    CHECK(gh_event_actor_play_move_000014c8(&play, &play_bindings, &frontier) == GH_EVENT_OK && !play.active);
    CHECK(!(word(extra.olivia_accessory.object.view.bytes + 0x250) & 0x8000));
    put_word(env.bytes + 0x28, 0);
    put_word(env.bytes + 0x10, 0);
    put_word(env.bytes + 0x14, 0);
    CHECK(gh_event_actor_play_move_000014c8(&play, &play_bindings, &frontier) == GH_EVENT_OK && !frontier);
    for (unsigned i = 0; i < 9; ++i) {
        CHECK(gh_event_actor_create_work_at(&state, i, &row, &cut) == GH_EVENT_OK);
        CHECK(row[9] == 0xff && !word(row + 0x10) && !word(row + 0x1c));
        CHECK((word(rows[i].object.view.bytes + 0x250) & 0x8002) == 0x8002);
    }
    CHECK((word(extra.olivia_accessory.object.view.bytes + 0x250) & 0x8002) == 0x8002);
    CHECK((word(child.object.view.bytes + 0x250) & 0x8002) == 2);
    put_word(env.bytes + 0x10, 1);
    gh_event_actor_cut first_active;
    CHECK(gh_event_actor_cut_at(&prefix, 0, 1, &first_active) == GH_EVENT_OK && first_active.lanes[0].primary);
    CHECK(gh_event_actor_play_move_000014c8(&play, &play_bindings, &frontier) == GH_EVENT_UNSUPPORTED);
    CHECK(frontier == 0x157568 && !play.frontier_relative && play.active && !play.row && play.row_stage == 2);
    CHECK(play.rows[0].primary_offset == first_active.lanes[0].primary_offset && !play.rows[0].frame_bits);
    CHECK(rows[0].object.view.bytes[0x2f4] == 4 && !(word(rows[0].object.view.bytes + 0x250) & 0x8002));
    CHECK(word(rows[1].object.view.bytes + 0x250) & 0x8000);
    CHECK(gh_event_actor_play_move_000014c8(&play, &play_bindings, &frontier) == GH_EVENT_UNSUPPORTED &&
          frontier == 0x157568 && play.row_stage == 2);
    gh_event_actor_play motion_play = {0};
    gh_actor_motion *motions = calloc(9, sizeof(*motions));
    CHECK(motions);
    const float rate = 1;
    gh_motion_ik_globals ik = {0};
    CHECK(gh_motion_ik_globals_from_elf(&ik, elf, elf_bytes) == GH_ACTOR_MOTION_OK);
    play_bindings.motions = motions;
    play_bindings.motion_capacity = 9;
    play_bindings.rate = &rate;
    play_bindings.ik_globals = &ik;
    put_word(env.bytes + 0x34, 0x298088);
    CHECK(gh_event_actor_play_init_00001468(&motion_play, &play_bindings, &frontier) == GH_EVENT_OK);
    put_word(env.bytes + 0x10, 0);
    CHECK(gh_event_actor_play_move_000014c8(&motion_play, &play_bindings, &frontier) == GH_EVENT_OK);
    put_word(env.bytes + 0x10, 1);
    gh_game_event_status motion_result = gh_event_actor_play_move_000014c8(&motion_play, &play_bindings, &frontier);
    if (motion_result) fprintf(stderr, "cut1 Motion status=%u pc=%x row=%zu stage=%u detail=%u\n",
        motion_result, frontier, motion_play.row, motion_play.row_stage, motions[motion_play.row].detail);
    CHECK(motion_result == GH_EVENT_OK);
    CHECK(motions[0].complete && motions[0].motion.offset == first_active.lanes[0].primary_offset &&
          word(rows[0].object.view.bytes + 0x430) == 0x3f800000);
    /* The first actual EV01 cut-1 EL00 record changes this actor to state4.
     * Main ELF 44B528 +54/+58/+5c prove Move and its state4 selector. */
    CHECK(elf_bytes > 0x34c588 && word(elf + 0x34c57c) == 0x2b6930 &&
          word(elf + 0x34c580) == 0 && word(elf + 0x34c584) == 0x14ea50);
    gh_event_actor_factory *el00 = &rows[0];
    uint8_t *el00_root = el00->object.view.bytes;
    CHECK(el00->kind == 0x600 && el00_root[0x2f4] == 4 && el00_root[0x2f5] == 0);
    CHECK(!el00_root[0x4e0] && !el00_root[0x1390] && !el00_root[0x2fa0] &&
          !el00_root[0x2ff0] && !el00_root[0x4d8]);
    gh_event_accessory_move object_move = {0};
    event_move_shadow_probe shadow_probe = {.actor = el00};
    CHECK(gh_game_event_simple_move_002b6930(&object_move, el00, NULL, probe_event_move_shadow,
          &shadow_probe, &frontier) == GH_EVENT_UNSUPPORTED);
    CHECK(frontier == 0x31a600 && object_move.actor == el00 && object_move.stage == 3 &&
          object_move.texture_node == el00->model.script.mesh_count && shadow_probe.calls == 1);
    CHECK(gh_game_event_simple_move_002b6930(&object_move, el00, NULL, probe_event_move_shadow,
          &shadow_probe, &frontier) == GH_EVENT_OK);
    CHECK(!frontier && !object_move.actor && !object_move.stage && shadow_probe.calls == 2 &&
          !memcmp(el00_root + 0x490, el00->model.root.alias_f0, 12));
    el00_root[0x2f5] = 1;
    CHECK(gh_game_event_simple_move_002b6930(&object_move, el00, NULL, probe_event_move_shadow,
          &shadow_probe, &frontier) == GH_EVENT_UNSUPPORTED && frontier == 0x14ead8 &&
          !object_move.stage && shadow_probe.calls == 2);
    el00_root[0x2f5] = 0;
    gh_player_shared_vector shared = {0};
    CHECK(gh_player_shared_vector_from_elf(&shared, elf, elf_bytes) == GH_PLAYER_OK);
    static const unsigned attached[] = {1, 2, 4, 5};
    for (unsigned i = 0; i < sizeof(attached) / sizeof(*attached); ++i) {
        gh_event_actor_factory *member = rows + attached[i];
        uint8_t *root = member->object.view.bytes;
        gh_event_accessory_move move = {0};
        const int wide = i & 1;
        static const unsigned narrow_parts[] = {0, 1, 2}, wide_parts[] = {0, 1, 17, 18};
        const unsigned *parts = wide ? wide_parts : narrow_parts;
        const gh_event_actor_factory *parent = rows + (i < 2 ? 0 : 3);
        CHECK(root[0x4e0] && word(root + 0x30a4) == (wide ? 1u : 2u));
        CHECK(word(root + 0x30b4) == UINT32_MAX && !word(root + 0x4dc));
        for (unsigned j = 0; j < (wide ? 4u : 3u); ++j) {
            const gh_actor_part *part;
            CHECK(!gh_actor_model_part_at(&member->model, parts[j], &part));
            memset(part->alias_d0, 0xcc, 12);
            memset(part->bytes + 0xe0, 0xcc, 12);
        }
        gh_game_event_status result = gh_game_event_simple_move_002b6930(&move, member, &shared, NULL, NULL, &frontier);
        if (i == 2) {
            CHECK(result == GH_EVENT_UNSUPPORTED && frontier == 0x2df860 && move.stage == 4);
        } else {
            CHECK(result == GH_EVENT_OK && !frontier && !move.stage);
        }
        CHECK(!memcmp(member->model.root.alias_f0, parent->model.root.bytes + 0xb0, 16));
        CHECK((word(root + 0x250) & 0x10u) == (word(parent->model.root.bytes + 0x250) & 0x10u));
        CHECK(!memcmp(root + 0x100, parent->model.root.bytes + 0x100, 12));
        CHECK(!memcmp(root + 0x240, parent->model.root.bytes + 0x240, 16));
        for (unsigned j = 0; j < (wide ? 4u : 3u); ++j) {
            const gh_actor_part *part;
            CHECK(!gh_actor_model_part_at(&member->model, parts[j], &part));
            CHECK(!memcmp(part->alias_d0, shared.bytes, 12));
            CHECK(!memcmp(part->bytes + 0xe0, shared.bytes, 12));
        }
    }
    CHECK(motions[8].complete && !motions[6].bound && !motions[7].bound);
    CHECK(!(word(child.object.view.bytes + 0x250) & 0x8002));
    CHECK(!(word(extra.olivia_accessory.object.view.bytes + 0x250) & 0x8002));
    gh_cloth_templates cloth_templates = {0};
    gh_cloth_runtime_globals cloth_globals = {0};
    CHECK(gh_cloth_templates_from_elf(&cloth_templates, elf, elf_bytes) == GH_CLOTH_OK);
    CHECK(gh_cloth_templates_initialize_00100fb8(&cloth_templates, 1, 0xffff) == GH_CLOTH_OK);
    CHECK(gh_cloth_runtime_from_elf(&cloth_globals, elf, elf_bytes) == GH_CLOTH_OK);
    CHECK(gh_cloth_runtime_initialize_00107de0(&cloth_globals, 1, 0xffff) == GH_CLOTH_OK);
    gh_game_mt random;
    gh_game_mt_seed_0030f2b0(&random, 5489);
    gh_game_mt expected_random = random;
    uint32_t expected_word;
    CHECK(!gh_game_mt_next_0030f348(&expected_random, &expected_word));
    const gh_cloth_step_bindings cloth = {
        .templates = &cloth_templates, .globals = &cloth_globals, .random = &random,
        .matrix_policy = GH_STATIC_BINARY32_REFERENCE, .random_policy = GH_FRAME_CHOP_REFERENCE
    };
    gh_event_pendulum_call pendulum_call = {0};
    gh_game_event_status pendulum_status =
        gh_game_event_pendulum_002df860(&pendulum_call, &rows[4], &extra, &cloth, &shared, &frontier);
    if (pendulum_status) fprintf(stderr, "EL0B 104010 status=%u frontier=%x stage=%u\n",
        pendulum_status, frontier, pendulum_call.prefix.stage);
    CHECK(pendulum_status == GH_EVENT_OK && !frontier && pendulum_call.finish.complete);
    CHECK(pendulum_call.prefix.geometry == &cloth_templates.geometry && cloth_templates.geometry.count == 6);
    CHECK(pendulum_call.prefix.random_words[0] == expected_word && !memcmp(&random, &expected_random, sizeof(random)));
    CHECK(pendulum_call.prefix.operands.model == &rows[4].model && pendulum_call.prefix.operands.parent == &rows[3].model);
    CHECK(pendulum_call.distances.iterations == 1);
    CHECK(gh_game_event_pendulum_002df860(&pendulum_call, &rows[4], &extra, &cloth, &shared, &frontier) == GH_EVENT_OK);
    CHECK(!memcmp(&random, &expected_random, sizeof(random)));
    put_word(env.bytes + 0x14, 0x3f800000);
    CHECK(gh_event_actor_play_move_000014c8(&motion_play, &play_bindings, &frontier) == GH_EVENT_OK);
    CHECK(word(rows[0].object.view.bytes + 0x430) == 0x40000000);
    unsigned played = 2;
    for (unsigned scene_cut = 1; scene_cut < prefix.camera_count; ++scene_cut) {
        uint16_t length;
        CHECK(gh_event_camera_cut_length_00004e60(&prefix, scene_cut, &length) == GH_EVENT_OK);
        put_word(env.bytes + 0x10, scene_cut);
        for (unsigned frame = scene_cut == 1 ? 2 : 0; frame < length; ++frame) {
            float value = (float)frame;
            uint32_t raw_frame;
            memcpy(&raw_frame, &value, 4);
            put_word(env.bytes + 0x14, raw_frame);
            motion_result = gh_event_actor_play_move_000014c8(&motion_play, &play_bindings, &frontier);
            if (motion_result) fprintf(stderr, "cut%u frame%u row%zu status%u pc%x detail%u\n",
                scene_cut, frame, motion_play.row, motion_result, frontier, motions[motion_play.row].detail);
            CHECK(motion_result == GH_EVENT_OK);
            /* Run the original EL00 Move hierarchy after ActorPlay samples
             * each frame. Cuts 5 and 8 contain large unwrapped Euler channels. */
            unsigned probe_part = scene_cut == 5 ? 14 : 8;
            int check_wrap = !frame && (scene_cut == 5 || scene_cut == 8);
            const uint8_t *angles = el00->model.parts[probe_part].bytes + 0x100;
            if (check_wrap) {
                static const uint32_t cut5[] = {0x42d59fa9, 0xbf36bdc8, 0xc2d5a25c};
                static const uint32_t cut8[] = {0x4300c643, 0x4024a0c7, 0x4300d461};
                const uint32_t *expected = scene_cut == 5 ? cut5 : cut8;
                for (unsigned axis = 0; axis < 3; ++axis)
                    CHECK(word(angles + axis * 4) == expected[axis]);
            }
            CHECK(gh_actor_model_update_hierarchy_00149eb8(&el00->model,
                  GH_STATIC_BINARY32_REFERENCE) == GH_ACTOR_MODEL_OK);
            if (check_wrap) {
                static const uint32_t cut5[] = {0xbb180000, 0xbf36bdc8, 0xbb418000};
                static const uint32_t cut8[] = {0x40471680, 0x4024a0c7, 0xc04781c0};
                const uint32_t *expected = scene_cut == 5 ? cut5 : cut8;
                for (unsigned axis = 0; axis < 3; ++axis)
                    CHECK(word(angles + axis * 4) == expected[axis]);
            }
            if (scene_cut == 8 && !frame) {
                check_event_hand_variants(&rows[6], elf, elf_bytes, &shared);
                check_event_display_base_texture(&rows[7], elf, elf_bytes);
            }
            if(scene_cut==2&&frame==13)
                check_event_ground_particles(disc,elf,elf_bytes,&room_bundle,&prefix,&rows[0]);
            if(scene_cut==3&&frame==0)
                check_event_world_particles(disc,elf,elf_bytes,&room_bundle,&prefix,&rows[0]);
            ++played;
        }
    }
    CHECK(played > 100);
    CHECK(rows[6].object.view.bytes[0x4d8] == 1 && rows[7].object.view.bytes[0x4d8] == 1);
    CHECK(rows[8].kind == 0x667 && rows[8].model.ready && !rows[8].object.view.bytes[0x4d8]);
    gh_actor_external_parent_set rejected = {0};
    const gh_actor_external_part_pair cycle = {0, 0};
    CHECK(gh_actor_model_bind_external_parts(&rejected, &rows[0].model, &rows[1].model, &cycle, 1) ==
          GH_ACTOR_MODEL_ARGUMENT && !rejected.count && rows[0].model.external_borrowers == 3);
    const char *el_paths[] = {"el/el01.dat", "el/el02.dat"};
    const uint8_t parts[][4] = {{0, 1, 2, 0}, {0, 1, 17, 18}};
    const uint8_t parents[][4] = {{2, 3, 4, 0}, {15, 16, 9, 10}};
    for (unsigned i = 0; i < 2; ++i) {
        size_t bytes;
        uint8_t *file = load(disc, el_paths[i], &bytes);
        gh_event_actor_factory *member = &rows[i + 1];
        CHECK(member->model.ready && member->model.part_count && !member->shadow.initialized);
        CHECK(member->resource.file_extent == bytes && !memcmp(member->resource.bytes, file, bytes));
        CHECK(gh_event_actor_factory_view(member, state.actor_handles[i + 1], &published) == GH_EVENT_OK);
        CHECK(!word(published.bytes + 0x4dc) && word(published.bytes + 0x30a4) == 2 - i);
        CHECK(word(published.bytes + 0x30b4) == UINT32_MAX && published.bytes[0x4e0] == 1);
        for (unsigned j = 0; j < i + 3; ++j) {
            const uint8_t *world;
            CHECK(gh_actor_model_external_parts_at(&member->model, parts[i][j], &world) == GH_ACTOR_MODEL_OK);
            CHECK(world == rows[0].model.parts[parents[i][j]].bytes);
        }
        CHECK(gh_actor_model_update_hierarchy_00149eb8(&member->model, GH_STATIC_BINARY32_REFERENCE) == GH_ACTOR_MODEL_OK);
        free(file);
    }
    scope.player = &parent;
    scope.child = &child;
    gh_sound_control sound = {.initialized = 1, .recent_initialized = 1};
    sound.recent[10] = (gh_sound_recent){.bank = -1, .program = -1, .key = 0x1234};
    sound.recent[9].bank = 2;
    gh_event_actor_release release = {0};
    gh_event_actor_release_bindings release_bindings = {&state, &roster, &extra, &motion_play, &sound, NULL};
    CHECK(gh_event_actor_quit_00001588(&release, &release_bindings, &frontier) == GH_EVENT_OK && !frontier);
    gh_game_event_config release_config={0};
    CHECK(!gh_game_event_config_from_elf(&release_config,elf,elf_bytes));
    CHECK(!gh_game_event_config_set_00299320(&release_config,1,elf,elf_bytes));
    CHECK(release_config.flags==8);
    gh_event_timing release_timing={0};
    CHECK(!gh_event_timing_prepare_000054c0(&release_timing,&components,heap,&original)&&original);
    gh_screen_fade release_fade={0};
    gh_display_state release_display={0};
    CHECK(!gh_display_state_from_elf(&release_display,elf,elf_bytes));
    CHECK(!gh_display_static_initialize_002a48a8(&release_display,1,0xffff));
    const uint8_t release_background[3]={17,34,51};
    uint16_t screen_state=0xabcd;
    release_fixture release_path={.block_enemy=1};
    release_path.bindings=(gh_event_release_bindings){&event,&work,&release_config,&release,&release_timing,
        &release_fade,&release_display,release_test_leaf,&release_path,release_background,&screen_state};
    event.fields.state=4;
    event.fields.phase=0;
    event.fields.flags|=0x181;
    CHECK(!gh_game_event_set_stage(&event,4,release_test_stage,&release_path));
    CHECK(!gh_game_event_update_00296530(&event)&&event.fields.phase==1&&release.release_started);
    for(unsigned phase=2;phase<=8;++phase)
        CHECK(!gh_game_event_update_00296530(&event)&&event.fields.phase==phase);
    CHECK(!gh_game_event_update_00296530(&event)&&event.fields.phase==8&&!release_path.stage.in_cleanup);
    CHECK(gh_event_actor_start_release_00297a88(&release, &frontier) == GH_EVENT_OK && !frontier);
    CHECK(child.model.external_parent == &parent_model && attachment.parent_model == &parent_model);
    CHECK((word(child.object.view.bytes + 0x2f0) & 1) && !(word(child.object.view.bytes + 0x250) & 0x10000));
    CHECK(!motions[0].bound && !extra.accessory_handle && !state.actor_handles[0] && !state.actor_handles[8]);
    size_t before_unload = gh_heap_used(heap);
    const gh_object_resource_bindings *resource_bindings = &factory_bindings.resources;
    for (unsigned i = 0; i < 3; ++i) {
        gh_object_view disposing;
        CHECK(gh_game_objects_find(objects, 0x601, &disposing) == GH_OBJECT_OK &&
              disposing.use_count == 0 && disposing.state == 5 + i);
        CHECK(gh_event_actor_wait_release_00297aa8(&release, &ready, &frontier) == GH_EVENT_OK && !ready);
        CHECK(gh_game_objects_update_bound_001fe748(objects, resource_bindings, &frontier) == GH_OBJECT_OK);
    }
    CHECK(gh_event_actor_wait_release_00297aa8(&release, &ready, &frontier) == GH_EVENT_OK && ready);
    CHECK(gh_heap_used(heap) < before_unload && sound.recent[10].bank == 3 && !sound.recent[10].program);
    CHECK(sound.recent[10].key == 0x1234 && sound.recent[9].bank == 2);
    for (unsigned i = 0; i < sizeof(ids) / sizeof(*ids); ++i) {
        CHECK(gh_game_objects_is_unloaded_001fefd0(objects, ids[i], &ready) == GH_OBJECT_OK && ready);
    }
    CHECK(gh_game_objects_is_unloaded_001fefd0(objects, 0x60d, &ready) == GH_OBJECT_OK && ready);
    CHECK(gh_game_objects_update_bound_001fe748(objects, resource_bindings, &frontier) == GH_OBJECT_OK);
    size_t before_work = gh_heap_used(heap);
    scope.fail = handles[2];
    CHECK(gh_event_actor_release_work_00297ad0(&release, &frontier) == GH_EVENT_UNSUPPORTED);
    CHECK(frontier == 0x123456 && release.unsuspend_cursor == 2 && release.work_released && !state.work.pointer);
    CHECK(gh_heap_used(heap) == before_work - 648 && !word(actor.bytes + 4));
    CHECK(!(word(raw[0] + 0x250) & 0x8000) && (word(raw[2] + 0x250) & 0x8000));
    scope.fail = 0;
    CHECK(gh_event_actor_release_work_00297ad0(&release, &frontier) == GH_EVENT_OK && !frontier);
    CHECK(!(word(raw[2] + 0x250) & 0x8000) && !(word(raw[3] + 0x250) & 0x8000));
    CHECK(gh_heap_used(heap) == before_work - 648);
    CHECK(gh_event_actor_release_work_00297ad0(&release, &frontier) == GH_EVENT_OK);
    CHECK(gh_game_event_update_00296530(&event)==GH_EVENT_CALLEE&&event.frontier==0x294ca0&&event.release_resume);
    CHECK(!release_timing.allocation.pointer&&!word(env.bytes+8));
    const uint32_t ordered[]={0x2970dc,0x297158,0x2971f4,0x297244,0x29724c,0x2972b0};
    CHECK(release_path.count==sizeof(ordered)/sizeof(*ordered)&&!memcmp(ordered,release_path.sites,sizeof(ordered)));
    release_path.block_enemy=0;
    CHECK(!gh_game_event_update_00296530(&event)&&!event.release_resume&&release_path.stage.complete);
    CHECK(release_path.count==8&&release_path.sites[6]==0x2972b0&&release_path.sites[7]==0x297358);
    CHECK(word((uint8_t *)&work+0x60c)==0x332211&&!(word((uint8_t *)&work+0x5e0)&0x2000000u));
    CHECK(release_fade.state==9&&release_fade.kind==0&&release_fade.time==15&&
        release_fade.color_from==0xff000000u&&!release_fade.color_to&&screen_state==15);
    CHECK(!gh_game_event_update_00296530(&event)&&release_path.count==8);
    puts("PASS EV01 297038 release delay, actual actor/Env owners, 297660 before enemy gate and post-reset retry");
    CHECK(gh_event_actor_create_abort(&state) == GH_EVENT_OK);
    CHECK(gh_event_actor_extra_abort(&extra) == GH_EVENT_OK);
    CHECK(gh_event_actor_roster_abort(&roster) == GH_EVENT_OK);
    free(motions);
    CHECK(pools.vectors[0].count == 7 && pools.vectors[3].count == 2);
    free(rows);
    gh_game_player_child_attachment_clear(&attachment);
    CHECK(!attachment.attached && !parent_model.external_borrowers);
    CHECK(gh_game_player_child_release_prefix(&child) == GH_PLAYER_OK);
    CHECK(gh_actor_model_release(&parent_model) == GH_ACTOR_MODEL_OK);
    CHECK(gh_game_player_factory_release_prefix(&parent) == GH_PLAYER_OK);

    CHECK(gh_task_threads_destroy(threads) == GH_THREAD_OK);
    gh_game_objects_destroy(room_objects);
    gh_game_objects_destroy(objects);
    for (size_t i = 0; i < 5; ++i) CHECK(gh_room_object_unregister(&pools, handles[i], &scope) == GH_POOL_OK);
    CHECK(gh_game_room_pools_release(&pools) == GH_POOL_OK);
    CHECK(gh_game_event_resource_abort(&resource) == GH_EVENT_OK);
    CHECK(gh_game_event_module_abort(&module) == GH_EVENT_OK);
    CHECK(!gh_heap_used(heap) && gh_heap_destroy(heap) == GH_MEM_OK);
    free(room);
    free(el);
    free(pl00);
    free(pl01);
    free(elf);
    gh_disc_close(disc);
    printf("PASS EV01 creation, %u positive-cut ActorMove frames, normal actor deletion/holder unload/work release\n", played);
    return 0;
}
