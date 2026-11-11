#include "game_motion_camera.h"
#include "game_camera_envelope.h"
#include "game_event_data_prefix.h"
#include "game_event_env_frame.h"
#include "game_event_link.h"
#include "game_effects.h"
#include "game_event_effect_play.h"
#include "game_esp_create.h"
#include "game_esp_update.h"
#include "game_effects_frame.h"
#include "game_effects_trans.h"

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

static gh_game_event_status register_effects(void *context, const uint8_t *data, size_t size, uint32_t *frontier) {
    gh_game_effects *effects = context;
    gh_effect_status status = gh_game_effects_register_event_002fa0b0(effects, data, size);
    if (status) *frontier = effects->frontier;
    return status == GH_EFFECT_OK ? GH_EVENT_OK : GH_EVENT_CALLEE;
}

static uint32_t word(const uint8_t *p) {
    uint32_t result;
    memcpy(&result, p, sizeof(result));
    return result;
}

static unsigned live_emitters(const gh_game_effects *effects)
{
    unsigned live = 0;
    for (unsigned i = 0; i < 256; ++i) {
        gh_effect_emitter_view view;
        if (!gh_effect_create_emitter_at(effects, i, &view)) {
            CHECK(view.initialized && view.serial && !view.selector);
            ++live;
        }
    }
    return live;
}

static void advance(gh_event_timing *timing, gh_event_components *components,
    gh_event_component_view env, unsigned cut, unsigned frame)
{
    uint32_t frontier, bits;
    float value = (float)frame;
    memcpy(&bits, &value, sizeof(bits));
    put_word(env.bytes + 0x18, cut);
    put_word(env.bytes + 0x1c, bits);
    CHECK(!gh_event_env_update_frame_000059e0(timing, components, &frontier) && !frontier);
    CHECK(word(env.bytes + 0x10) == cut && word(env.bytes + 0x14) == bits);
}

static gh_effect_status lookup_absent(void *context, uint16_t kind, uint8_t occurrence,
    const gh_title_player_view **actor, const gh_actor_model **model, uint32_t *frontier)
{
    unsigned *seen = context;
    CHECK((kind == 0x600 || kind == 0x60a) && occurrence == 0);
    ++*seen;
    *actor = NULL;
    *model = NULL;
    *frontier = 0;
    return GH_EFFECT_OK;
}

typedef struct {
    unsigned particle;
    unsigned accepted;
    unsigned attempts;
} capture_probe;

static gh_effect_status check_type12_packet(void *context, const gh_effect_trans_packet *packet)
{
    capture_probe *probe = context;
    ++probe->attempts;
    CHECK(packet->order.bank == 4 && packet->order.depth == 10 && packet->slot == 0);
    CHECK(packet->inherited_sampler && !packet->writes_clamp && packet->writes_tex0);
    CHECK(packet->frame_mask == 0xff000000 && packet->zbuf == UINT64_C(0x13a0000f8));
    CHECK(packet->restores_scene_target && !packet->restore_frame_mask && packet->restore_zbuf == 0x3a0000f8);
    CHECK(packet->alpha == UINT64_C(0x8000000044));
    if (!probe->accepted) {
        CHECK(packet->kind == GH_EFFECT_PASS_SCENE_WARP && packet->original_callee == 0x2f02b8);
        CHECK(packet->source == GH_EFFECT_SURFACE_3C00 && packet->destination == GH_EFFECT_SURFACE_SCENE_COLOR);
        CHECK(packet->prim == 0x5d && packet->test == 0x5000c && !packet->texture_flush_after);
        CHECK(packet->writes_texa && packet->texa == 0x80);
        CHECK(packet->tex0 == UINT64_C(0x2000000620113c00) && packet->quad_count == 1);
        const gh_effect_trans_quad *fan = packet->quads;
        CHECK(fan->vertex_count == 6);
        const uint16_t x[2][6] = {{31037,30710,30361,31364,31713,30710},
            {34479,35019,33746,33938,35211,35019}};
        const uint16_t y[2][6] = {{34131,34787,33847,33474,34414,34787},
            {35127,35924,35752,34330,34502,35924}};
        for (unsigned i = 0; i < 6; ++i) {
            const gh_effect_trans_vertex *v = fan->vertices + i;
            CHECK(v->x12_4 == x[probe->particle][i] && v->y12_4 == y[probe->particle][i]);
            CHECK(v->q == 1.0f && v->z == 0xfff0);
            uint32_t rgba = i ? 0x00808080 : probe->particle ? 0x05808080 : 0x66808080;
            CHECK(v->rgba == rgba);
        }
        CHECK(word((const uint8_t *)&fan->vertices[0].s) == (probe->particle ? 0x3f358000 : 0x3e940000));
        CHECK(word((const uint8_t *)&fan->vertices[0].t) == (probe->particle ? 0x3f3a0000 : 0x3f1b0000));
        CHECK(word((const uint8_t *)&fan->vertices[1].s) == (probe->particle ? 0x3f480937 : 0x3e83126f));
        CHECK(word((const uint8_t *)&fan->vertices[1].t) == (probe->particle ? 0x3f548937 : 0x3f310937));
    } else {
        CHECK(probe->accepted == 1);
        CHECK(packet->kind == GH_EFFECT_PASS_SCENE_CAPTURE && packet->original_callee == 0x2fa620);
        CHECK(packet->source == GH_EFFECT_SURFACE_SCENE_COLOR && packet->destination == GH_EFFECT_SURFACE_3C00);
        CHECK(packet->prim == 0x116 && packet->test == 0x30000 && packet->texture_flush_after);
        CHECK(!(packet->prim & 0x40) && !packet->writes_texa);
        CHECK(packet->tex0 == UINT64_C(0x2000000664020e00) && packet->quad_count == 8);
        for (unsigned i = 0; i < 8; ++i) {
            const gh_effect_trans_quad *strip = packet->quads + i;
            CHECK(strip->vertex_count == 2);
            for (unsigned j = 0; j < 2; ++j) {
                const gh_effect_trans_vertex *v = strip->vertices + j;
                CHECK(v->x12_4 == 28672 + (int)(i + j) * 32 * 16);
                CHECK(v->y12_4 == 29184 + (int)j * 448 * 16);
                CHECK(v->u12_4 == 8 + (i + j) * 64 * 16 && v->v12_4 == 4 + j * 448 * 16);
                CHECK(v->rgba == 0x80808080 && v->z == 0xffff);
            }
        }
        if (probe->attempts == 2) return GH_EFFECT_UNSUPPORTED;
    }
    ++probe->accepted;
    return GH_EFFECT_OK;
}

static void check_type12_move(const gh_effect_release_bindings *storage, gh_boot_work *work,
    gh_effects_frame_globals *globals, const gh_display_state *display)
{
    gh_game_mt random = {0};
    gh_game_mt_seed_0030f2b0(&random, 42);
    gh_esp_create_bindings create_bindings = {.storage = *storage, .globals = globals,
        .work = work, .random = &random, .policy = GH_STATIC_BINARY32_REFERENCE};
    gh_esp_update_bindings update_bindings = {.storage = *storage, .work = work,
        .globals = globals, .policy = GH_STATIC_BINARY32_REFERENCE};
    uint8_t *esp = storage->esp;
    for (unsigned i = 0; i < 2; ++i) {
        gh_effect_emitter_view source;
        gh_effect_esp_view particle;
        gh_esp_create_call create = {0};
        uint32_t frontier;
        CHECK(!gh_effect_create_emitter_at(storage->effects, i, &source));
        CHECK(!gh_esp_create_begin_00300e30(&create, i, source.serial));
        CHECK(!gh_game_esp_create_00300e30(&create, &create_bindings));
        CHECK(create.complete && create.allocated && !create.frontier && create.slot == 0);
        CHECK(!gh_effect_create_esp_at(storage->effects, 0, &particle));
        CHECK(particle.initialized && particle.type == 0x12 && particle.globals == globals &&
            particle.target120_known_null && particle.parent_emitter == -1);
        CHECK(word(esp + 0x2d0) == 0x3bc49ba6 && word(esp + 0x2d4) == 0x3bc49ba6);
        CHECK(word(esp + 0x2e0) == 0x00010000 && !word(esp + 0x2c8) &&
            !word(esp + 0x2cc) && !word(esp + 0x2dc));
        CHECK(word(esp + 0x194) == (i ? 0 : 0x3f4ccccd));
        uint32_t initial_x = word(esp + 0x40);
        float initial_y;
        memcpy(&initial_y, esp + 0x44, sizeof(initial_y));
        for (unsigned frame = 1; frame <= 60; ++frame) {
            gh_esp_update_call move = {0};
            CHECK(!gh_game_esp_update_002fbed0(&move, &update_bindings, 0, create.serial));
            CHECK(!move.frontier && move.complete && !move.retired);
            if (frame == 1) {
                capture_probe probe = {.particle = i};
                gh_game_effects_trans trans = {0};
                gh_effects_trans_bindings bindings = {.storage = *storage, .work = work,
                    .globals = globals, .display = display, .policy = GH_STATIC_BINARY32_REFERENCE,
                    .cvt_policy = GH_FRAME_CHOP_REFERENCE, .emit = check_type12_packet, .context = &probe};
                CHECK(!gh_effects_trans_begin_002fb310(&trans));
                CHECK(gh_game_effects_trans_002fb310(&trans, &bindings) == GH_EFFECT_UNSUPPORTED);
                CHECK(trans.frontier == 0x2fa620 && trans.enqueued == 1 && trans.pending);
                CHECK(!gh_game_effects_trans_002fb310(&trans, &bindings));
                CHECK(!trans.frontier && trans.complete && trans.enqueued == 2);
                CHECK(probe.accepted == 2 && probe.attempts == 3);
            }
            float expected_y = initial_y - (float)frame;
            CHECK(word(esp + 0x40) == initial_x && !memcmp(esp + 0x44, &expected_y, 4));
            CHECK(esp[0x224] == frame && esp[0x22e] == frame);
            CHECK(word(esp + 0x2d4) == 0x3bc49ba6 && !word(esp + 0x2d8));
            CHECK(word(esp + 0x188) == 0x3f800000 && word(esp + 0x18c) == 0x3f800000 &&
                word(esp + 0x190) == 0x3f800000);
            if (frame == 18 || frame == 42)
                CHECK(word(esp + 0x194) == 0x3f4ccccd);
        }
        CHECK(!word(esp + 0x194));
        gh_esp_update_call last = {0};
        CHECK(!gh_game_esp_update_002fbed0(&last, &update_bindings, 0, create.serial));
        CHECK(last.retired && last.complete && word(esp + 0x100080) == 0x80000000u);
        put_word(esp + 0x100100, 0x80000000u);
        CHECK(!gh_effect_create_esp_destroy_hidden_native(storage->effects, 0, create.serial, &frontier));
        CHECK(!frontier && word(esp + 0xf0) == 0x44b310);
        CHECK(gh_effect_create_esp_at(storage->effects, 0, &particle) == GH_EFFECT_UNSUPPORTED);
        put_word(esp + 0x100000, 0);
        put_word(esp + 0x100080, 0);
        put_word(esp + 0x100100, 0);
    }
}

typedef struct {
    gh_effect_view source;
    const gh_stage_material *material;
    unsigned row,packets;
} type0_sprite_probe;

static gh_effect_status check_type0_packet(void *context,const gh_effect_trans_packet *p)
{
    type0_sprite_probe *probe=context;
    CHECK(p->kind==GH_EFFECT_PASS_SPRITE && p->original_callee==0x2fec38);
    CHECK(p->order.bank==4 && p->order.depth==10 && p->quad_count==1);
    CHECK(p->prim==0x54 && p->test==0x5000d && p->alpha==UINT64_C(0x8000000048));
    CHECK(p->frame_mask==0xff000000 && p->zbuf==UINT64_C(0x13a0000f8));
    CHECK(p->writes_clamp && !p->clamp && p->inherited_sampler);
    CHECK(p->writes_tex0 && p->tex0==UINT64_C(0x20000005dc148000));
    const gh_stage_material *m=p->material;
    CHECK(m && m->source==probe->source.bytes && m->source_bytes==probe->source.size);
    CHECK(m->width==128 && m->height==128 && m->psm==20 && m->draw_tcc && !m->draw_tfx);
    CHECK(m->cache_id && m->rgba_bytes==128*128*4);
    if(probe->packets++){
        CHECK(probe->material==m);
        return GH_EFFECT_OK;
    }
    probe->material=m;
    CHECK(probe->source.size==8320);
    const uint8_t *raw=probe->source.bytes;
    for(unsigned i=0;i<128*128;i++){
        unsigned index=(raw[0x40+i/2]>>((i&1)*4))&15;
        CHECK(!memcmp(m->rgba_gs+i*4,raw+0x2040+index*4,4));
    }
    const int x[2][2]={{36678,41432},{36741,39933}};
    const int y[2][2]={{35313,37487},{35480,37273}};
    CHECK(p->quads[0].vertex_count==4);
    for(unsigned i=0;i<4;i++){
        const gh_effect_trans_vertex *v=p->quads[0].vertices+i;
        CHECK(v->x12_4==x[probe->row][i&1] && v->y12_4==y[probe->row][i>>1]);
        CHECK(v->z==0xfff0 && v->q==1 && v->s==(float)(i&1) && v->t==(float)(1-(i>>1)));
        CHECK(v->rgba==(probe->row?0x40667980u:0x05667980u));
    }
    return GH_EFFECT_OK;
}

static void check_type0_particles(const gh_effect_release_bindings *storage,gh_boot_work *work,
    gh_effects_frame_globals *globals,const gh_display_state *display)
{
    gh_game_mt random={0};
    gh_game_mt_seed_0030f2b0(&random,42);
    gh_stage_textures *textures=NULL;
    CHECK(!gh_stage_textures_create(&textures));
    gh_esp_create_bindings create_bindings={.storage=*storage,.globals=globals,.work=work,
       .random=&random,.policy=GH_STATIC_BINARY32_REFERENCE};
    gh_esp_update_bindings update_bindings={.storage=*storage,.globals=globals,.work=work,
       .policy=GH_STATIC_BINARY32_REFERENCE};
    uint64_t previous_material=0;
    for(unsigned row=0;row<2;row++){
        gh_effect_emitter_view source;
        CHECK(!gh_effect_create_emitter_at(storage->effects,3+row,&source));
        const uint8_t *r=source.record.bytes;
        CHECK(!r[0x133] && !r[0x136] && (int8_t)r[0xbe]==-4 && r[0x12e]==(row?76:132));
        CHECK(word(r+4)==0x800000u);
        gh_esp_create_call create={0};
        CHECK(!gh_esp_create_begin_00300e30(&create,3+row,source.serial));
        CHECK(!gh_game_esp_create_00300e30(&create,&create_bindings));
        CHECK(create.complete && create.allocated && create.slot==0 && !create.frontier);
        gh_effect_esp_view p;
        CHECK(!gh_effect_create_esp_at(storage->effects,create.slot,&p));
        CHECK(p.initialized && !p.type && word(p.payload+0xf0)==0x44fad0 &&
              p.target120_known_null && p.parent_emitter==-1 && p.texture.bytes);
        type0_sprite_probe probe={.source=p.texture,.row=row};
        gh_effects_trans_bindings draw={.storage=*storage,.work=work,.globals=globals,.display=display,
           .policy=GH_STATIC_BINARY32_REFERENCE,.cvt_policy=GH_FRAME_CHOP_REFERENCE,
           .emit=check_type0_packet,.context=&probe,.textures=textures};
        const unsigned end=row?97:86;
        for(unsigned frame=1;frame<=end;frame++){
            gh_esp_update_call update={0};
            CHECK(!gh_game_esp_update_002fbed0(&update,&update_bindings,create.slot,create.serial));
            CHECK(update.complete && !update.frontier && update.retired==(frame==end));
            if(update.retired)continue;
            gh_game_effects_trans trans={0};
            CHECK(!gh_effects_trans_begin_002fb310(&trans));
            CHECK(!gh_game_effects_trans_002fb310(&trans,&draw));
            CHECK(trans.complete && !trans.frontier);
            if(frame<(row?4u:2u))CHECK(!probe.packets);
            else if(frame==(row?4u:2u))CHECK(probe.packets==1);
        }
        CHECK(probe.packets>1 && probe.material->cache_id>previous_material);
        previous_material=probe.material->cache_id;
        CHECK(word(storage->esp+0x100080)==0x80000000);
        put_word(storage->esp+0x100100,0x80000000);
        uint32_t frontier;
        CHECK(!gh_effect_create_esp_destroy_hidden_native(storage->effects,0,create.serial,&frontier));
        CHECK(!frontier && word(p.payload+0xf0)==0x44b310);
        put_word(storage->esp+0x100000,0);
        put_word(storage->esp+0x100080,0);
        put_word(storage->esp+0x100100,0);
    }
    gh_stage_textures_destroy(textures);
}

static void check_type13(const gh_effect_release_bindings *storage,gh_boot_work *work,
    gh_effects_frame_globals *globals,const gh_display_state *display,gh_event_link *link,
    const gh_event_data_prefix *prefix,const uint8_t *elf,size_t elf_bytes)
{
    uint32_t frontier=0;
    static gh_game_init_state init;
    gh_game_init_state_initialize(&init);
    gh_game_motion_camera camera={0};
    CHECK(!gh_game_motion_camera_construct_00138be8(&camera,GH_CAMERA_NATIVE_BINARY32_REFERENCE));
    CHECK(!gh_event_camera_init_play_00004bd8(&link->components,&init,&camera,&frontier));
    gh_event_camera_track track;
    CHECK(!gh_event_camera_select_00004c08(link,prefix,&track,&frontier));
    CHECK(track.apply_motion && track.frame==0.0f);
    CHECK(!gh_game_motion_camera_set_001390b8(&camera,&track));
    gh_camera_projection_display camera_display={&display->screen_width,&display->screen_height_bits,
        &display->pixel_aspect_bits,&display->initialized,GH_CAMERA_NATIVE_BINARY32_REFERENCE};
    CHECK(!gh_game_motion_camera_move_00138600(&camera,&init,work,&camera_display,&frontier));

    gh_effect_emitter_view source;
    CHECK(!gh_effect_create_emitter_at(storage->effects,8,&source));
    CHECK(source.record.bytes[0x133]==0x13 && source.record.bytes[0x16c]==0 && source.record.bytes[0x170]==12);
    gh_effect_view est;
    CHECK(!gh_effect_est_at_00306030(storage->effects,0,13,&est) && word(est.bytes)==0x545345u);
    CHECK(word(est.bytes+4)==11 && est.size==8288);
    gh_game_mt random={0};
    gh_game_mt_seed_0030f2b0(&random,42);
    gh_esp_create_bindings cb={.storage=*storage,.globals=globals,.work=work,.random=&random,
        .policy=GH_STATIC_BINARY32_REFERENCE};
    gh_esp_create_call create={0};
    CHECK(!gh_esp_create_begin_00300e30(&create,8,source.serial));
    globals->flare_hold_3c3bb0=1;
    globals->flare_intensity_3c3bb4=0x3f800000;
    globals->flare_active_61b670=1;
    globals->flare_setup_61b674=0x3f800000;
    CHECK(!gh_game_esp_create_00300e30(&create,&cb) && create.complete && !create.frontier);
    CHECK(!globals->flare_hold_3c3bb0 && !globals->flare_intensity_3c3bb4 &&
          !globals->flare_active_61b670 && !globals->flare_setup_61b674);
    gh_effect_esp_view p;
    CHECK(!gh_effect_create_esp_at(storage->effects,create.slot,&p));
    CHECK(p.type==0x13 && p.initialized && word(p.payload+0xf0)==0x44ec68 && p.parent_emitter==8);
    CHECK(!word(p.payload+0x2b0) && word(p.payload+0x2b4)==12);
    gh_camera_envelope envelope={0};
    CHECK(!gh_game_camera_envelope_from_elf(&envelope,elf,elf_bytes,&frontier));
    gh_esp_update_bindings ub={.storage=*storage,.globals=globals,.work=work,.policy=GH_STATIC_BINARY32_REFERENCE,
       .camera=&camera.camera,.active_camera=init.active_camera,.display=display,.atan_coeff=envelope.atan_coeff};
    gh_esp_update_call update={0};
    CHECK(!gh_game_esp_update_002fbed0(&update,&ub,create.slot,create.serial));
    CHECK(update.complete && !update.frontier && globals->flare_active_61b670==1);
    CHECK(!globals->flare_intensity_3c3bb4 && !globals->flare_setup_61b674 &&
          !globals->flare_hold_3c3bb0);
    CHECK(globals->flare_position_61b660[0]==0x43a9f988u &&
          globals->flare_position_61b660[1]==0x420c2d40u &&
          globals->flare_position_61b660[2]==0x3f7fffffu &&
          globals->flare_depth_61b672==0x01bf);
    CHECK(word(storage->esp+0x100000)==0x80000000u);
    gh_effects_trans_bindings tb={.storage=*storage,.globals=globals,.work=work,.display=display,
        .policy=GH_STATIC_BINARY32_REFERENCE,.cvt_policy=GH_FRAME_CHOP_REFERENCE};
    gh_game_effects_trans trans={0};
    CHECK(!gh_effects_trans_begin_002fb310(&trans));
    CHECK(!gh_game_effects_trans_002fb310(&trans,&tb)&&trans.complete&&!trans.enqueued);
    globals->flare_intensity_3c3bb4=0x3f800000u;
    gh_esp_update_call positive={0};
    CHECK(gh_game_esp_update_002fbed0(&positive,&ub,create.slot,create.serial)==GH_EFFECT_UNSUPPORTED);
    CHECK(positive.frontier==0x2fab88 && positive.stage==17 && !positive.complete);
    CHECK(globals->flare_intensity_3c3bb4==0x3f733333u && p.payload[0x224]==2);
    CHECK(gh_game_esp_update_002fbed0(&positive,&ub,create.slot,create.serial)==GH_EFFECT_UNSUPPORTED);
    CHECK(positive.frontier==0x2fab88 && globals->flare_intensity_3c3bb4==0x3f733333u &&
          p.payload[0x224]==2 && word(storage->esp+0x100000)==0x80000000u);
    CHECK(!gh_effect_create_esp_abort_native(storage->effects,create.slot,create.serial));
}

int main(int argc, char **argv) {
    CHECK(argc == 2);
    gh_disc *disc = NULL;
    CHECK(gh_disc_open(argv[1], &disc) == GH_DISC_OK);
    const size_t elf_bytes = (size_t)gh_disc_boot_elf_size(disc);
    uint8_t *elf = malloc(elf_bytes);
    CHECK(elf && gh_disc_read_boot_elf(disc, 0, elf, elf_bytes) == GH_DISC_OK);
    gh_display_state display = {0};
    CHECK(!gh_display_state_from_elf(&display, elf, elf_bytes));
    CHECK(!gh_display_static_initialize_002a48a8(&display, 1, 0xffff));
    gh_effects_frame_globals globals = {0};
    CHECK(!gh_effects_frame_globals_from_elf(&globals, elf, elf_bytes));
    gh_game_event event = {0};
    CHECK(gh_game_event_from_elf(&event, elf, elf_bytes) == GH_EVENT_OK);
    gh_game_event_config config = {0};
    CHECK(!gh_game_event_config_from_elf(&config, elf, elf_bytes));
    CHECK(!gh_game_event_init_environment_002975f8(&event, &config, 1, elf, elf_bytes));
    CHECK(config.flags == 8);
    event.fields.data_number = 1;

    gh_heap *heap = NULL;
    gh_boot_work work = {0};
    gh_event_module module = {0};
    gh_event_resource resource = {0};
    gh_dvd_jobs dvd;
    CHECK(gh_heap_create(2 * 1024 * 1024, &heap) == GH_MEM_OK);
    gh_dvd_jobs_init(&dvd, disc);
    CHECK(gh_game_event_module_begin_00297378(&module, &event, &work, &dvd, heap) == GH_EVENT_OK);
    int ready = 0;
    for (unsigned i = 0; i < 8 && !ready; ++i) {
        CHECK(gh_dvd_pump(&dvd) == GH_DVD_OK);
        CHECK(gh_game_event_module_poll_00297428(&module, &ready) == GH_EVENT_OK);
    }
    CHECK(ready && module.loaded);

    gh_event_link link = {0};
    CHECK(!gh_game_event_link_002be890(&link, &module, elf, elf_bytes));
    gh_event_components *components = &link.components;
    CHECK(gh_event_components_construct_00297680(components, 1) == GH_EVENT_OK);
    CHECK(gh_game_event_resource_begin_00297540(&resource, &event, &dvd, heap) == GH_EVENT_OK);
    ready = 0;
    for (unsigned i = 0; i < 1024 && !ready; ++i) {
        CHECK(gh_dvd_pump(&dvd) == GH_DVD_OK);
        CHECK(gh_game_event_resource_poll_002975d0(&resource, &ready) == GH_EVENT_OK);
    }
    CHECK(ready && resource.loaded && resource.file_bytes == 873872);

    gh_event_data_prefix prefix = {0};
    uint32_t frontier;
    CHECK(!gh_event_data_bind_prefix_002977e8(&prefix, components, &resource, &frontier));
    CHECK(!gh_event_actor_cuts_bind_00297864(&prefix, &frontier));
    size_t core_size, room_size;
    uint8_t *core_data = load(disc, "etc/core.dat", &core_size);
    uint8_t *room_data = load(disc, "st1/r100.dat", &room_size);
    gh_bundle room;
    CHECK(gh_bundle_open(room_data, room_size, &room) == GH_BUNDLE_OK);
    gh_game_effects *effects = calloc(1, sizeof(*effects));
    uint8_t *esp = calloc(1, 0x100180), *emitters = calloc(1, 0x26060);
    CHECK(effects && esp && emitters);
    CHECK(gh_game_effects_initialize_00305a68(effects, esp, 0x100180, emitters, 0x26060,
                                             core_data, core_size, &room) == GH_EFFECT_OK);
    CHECK(!gh_event_data_bind_tail_00297880(&prefix, components, register_effects, effects, &frontier));
    gh_event_sequence_view track;
    CHECK(!gh_event_sequence_at(&prefix, 3, &track) && !track.records && !track.count);
    CHECK(!gh_event_sequence_at(&prefix, 4, &track) && !track.records && !track.count);
    CHECK(!gh_event_sequence_at(&prefix, 5, &track) && !track.records && !track.count);
    CHECK(effects->event_registered_41c && effects->event_data.bytes == prefix.data + word(prefix.data + 0x1c));
    CHECK(!word(prefix.data + 0x18) && !word(prefix.data + 0x20) && !word(prefix.data + 0x50));

    gh_event_component_view env, vertex, effect;
    CHECK(!gh_event_component_at(components, 0, &env));
    CHECK(!gh_event_component_at(components, 3, &vertex));
    CHECK(!gh_event_component_at(components, 4, &effect));
    CHECK(!gh_event_vertex_init_absent_0000a200(&prefix, &frontier) && !frontier);
    CHECK(!memcmp(vertex.bytes, "VERTEX", sizeof("VERTEX")));
    gh_event_effect_play state = {0};
    gh_game_effects wrong = {.ready = 1};
    CHECK(gh_event_effect_init_00005000(&state, &prefix, &config, &work, &wrong, &frontier) == GH_EVENT_ARGUMENT);
    CHECK(frontier == 0x5000 && !effect.bytes[0]);
    put_word((uint8_t *)&work + 0x5e4, 1);
    CHECK(!gh_event_effect_init_00005000(&state, &prefix, &config, &work, effects, &frontier));
    CHECK(!frontier && !memcmp(effect.bytes, "EFFECT", sizeof("EFFECT")));
    CHECK(word((uint8_t *)&work + 0x5e4) == 0x10001);
    gh_event_timing timing = {0};
    int original = 0;
    CHECK(!gh_event_timing_prepare_000054c0(&timing, components, heap, &original) && original);
    put_word(env.bytes + 0x10, UINT32_MAX);
    put_word(env.bytes + 0x14, 0xbf800000);
    gh_game_mt rng;
    gh_game_mt_seed_0030f2b0(&rng, 123);
    gh_effect_create_bindings producer = {
        .storage = {.effects = effects, .esp = esp, .esp_bytes = 0x100180,
                    .emitters = emitters, .emitter_bytes = 0x26060},
        .work = &work, .random = &rng
    };
    int relative;
    unsigned produced = 0, frames = 0, queries = 0;
    const unsigned expected[10] = {9, 4, 1, 4, 0, 1, 0, 1, 3, 1};
    for (unsigned cut = 0; cut < prefix.camera_count; ++cut) {
        int present;
        CHECK(!gh_effect_event_cut_present_002fa128(effects, cut, &present) && present == (cut != 6));
        uint16_t length;
        CHECK(!gh_event_camera_cut_length_00004e60(&prefix, cut, &length));
        for (unsigned frame = 0; frame < length; ++frame) {
            advance(&timing, components, env, cut, frame);
            CHECK(!gh_event_vertex_move_absent_0000a220(&prefix, &frontier) && !frontier);
            if (!frame && cut == 2) {
                CHECK(gh_event_effect_move_00005050(&state, &producer, &frontier, &relative) == GH_EVENT_UNSUPPORTED);
                CHECK(!relative && frontier == 0x297c50 && state.stage == 3 && state.create.row == 1);
                CHECK(live_emitters(effects) == produced + 1);
                const gh_game_mt before = rng;
                for (unsigned retry = 0; retry < 3; ++retry) {
                    CHECK(gh_event_effect_move_00005050(&state, &producer, &frontier, &relative) == GH_EVENT_UNSUPPORTED);
                    CHECK(frontier == 0x297c50 && live_emitters(effects) == produced + 1);
                    CHECK(!memcmp(&rng, &before, sizeof(before)));
                }
                producer.event_actor = lookup_absent;
                producer.event_actor_context = &queries;
            }
            CHECK(!gh_event_effect_move_00005050(&state, &producer, &frontier, &relative) && !frontier);
            if (!frame) {
                for (unsigned i = 0; i < produced; ++i)
                    CHECK(word(emitters + i * 0x260 + 0x114) & 0x40000000u);
                for (unsigned i = produced; i < produced + expected[cut]; ++i) {
                    gh_effect_emitter_view view;
                    CHECK(!gh_effect_create_emitter_at(effects, i, &view));
                    CHECK(!view.actor && !view.selector && view.initialized);
                    CHECK(word(view.payload + 0x110) == 0x2500 + cut);
                    CHECK(word(view.payload + 0x118) == 6 && word(view.payload + 0x1e8) == 0xe);
                    CHECK(word(view.payload + 0x18c) == (6 | (cut + 1) << 16));
                    CHECK(!(word(view.payload + 0x114) & 0x40000000u));
                }
                produced += expected[cut];
                if (!cut) {
                    uint32_t serials[2];
                    memset(esp, 0xa5, 0x800);
                    memset(esp + 0x100080, 0xe0, 1);
                    esp[0x100083] = esp[0x100103] = 0xe0;
                    for (unsigned i = 0; i < 2; ++i) {
                        gh_effect_emitter_view source;
                        gh_effect_esp_view particle;
                        unsigned slot;
                        int allocated;
                        CHECK(!gh_effect_create_emitter_at(effects, i, &source));
                        CHECK(source.record.bytes[0x133] == 0x12);
                        CHECK(!gh_effect_create_esp_allocate_002faa88(&producer.storage, i, source.serial,
                            &slot, &serials[i], &allocated, &frontier));
                        CHECK(!frontier && allocated && slot == i && serials[i] > source.serial);
                        CHECK(!gh_effect_create_esp_at(effects, slot, &particle));
                        CHECK(particle.serial == serials[i] && particle.emitter_slot == i &&
                            particle.emitter_serial == source.serial && particle.record.bytes == source.record.bytes);
                        CHECK(particle.type == 0x12 && !particle.initialized && !particle.actor &&
                            !particle.selector && particle.parent_emitter == -1 && particle.target120_known_null);
                        const uint8_t *payload = particle.payload;
                        CHECK(payload == esp + i * 0x400 && particle.payload_size == 0x400);
                        CHECK(word(payload + 0xf0) == 0x44eb70 && payload[0x102] == 0x12);
                        CHECK(word(payload + 0x11c) == 0x06000000 && word(payload + 0x20) == 0x3f800000);
                        CHECK(word(payload + 0x34) == 0x80000000 && word(payload + 0x3c) == 0x3f800000);
                        CHECK(payload[0x238] == 0xff && payload[0x239] == 0xff && payload[0x245] == 1);
                        CHECK(!word(payload + 0x2b0) && !word(payload + 0x3fc));
                        CHECK(word(esp + 0x100000) == (i ? 0xc0000000u : 0x80000000u));
                    }
                    CHECK(serials[1] > serials[0] && word(esp + 0x100080) == 0x200000e0u);
                    CHECK(word(esp + 0x100100) == 0x20000000u);
                    for (unsigned i = 0; i < 2; ++i)
                        CHECK(!gh_effect_create_esp_abort_partial(effects, i, serials[i]));
                    CHECK(!word(esp + 0x100000));
                    memset(esp + 0x100080, 0, 0x100);
                    check_type12_move(&producer.storage, &work, &globals, &display);
                    check_type0_particles(&producer.storage, &work, &globals, &display);
                    check_type13(&producer.storage,&work,&globals,&display,&link,&prefix,elf,elf_bytes);
                }
                advance(&timing, components, env, cut, frame);
                CHECK(!gh_event_effect_move_00005050(&state, &producer, &frontier, &relative));
                CHECK(!frontier && live_emitters(effects) == produced);
            }
            ++frames;
        }
    }
    CHECK(frames == 1209 && produced == 24 && queries == 3 && live_emitters(effects) == produced);
    CHECK(!gh_event_timing_abort(&timing));
    gh_effect_create_forget_on_owner_destroy(effects);
    free(effects);
    free(esp);
    free(emitters);
    free(core_data);
    free(room_data);
    CHECK(!gh_game_event_resource_abort(&resource));
    CHECK(!gh_game_event_module_abort(&module));
    CHECK(!gh_heap_used(heap) && !gh_heap_destroy(heap));
    gh_disc_close(disc);
    free(elf);
    puts("PASS actual EV01 1209 frames, 24 emitters, Vertex absence, type12 capture/warp, type0 lifecycles, and type13 actual-camera zero emission");
    return 0;
}
