#include "game_effects_trans.h"
#include "game_camera_envelope.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while(0)

static uint32_t word(const void *p){uint32_t n;memcpy(&n,p,4);return n;}
static float real(const void *p){float n;memcpy(&n,p,4);return n;}

static uint8_t *load(gh_disc *disc,const char *path,size_t *size)
{
    size_t index;
    gh_disc_entry entry;
    CHECK(!gh_disc_find(disc,path,&index) && !gh_disc_entry_at(disc,index,&entry) && entry.size<=SIZE_MAX);
    *size=(size_t)entry.size;
    uint8_t *bytes=malloc(*size);
    CHECK(bytes && !gh_disc_read(disc,index,0,bytes,*size));
    return bytes;
}

typedef struct {
    unsigned accepted, retries;
    uint32_t rgba;
    float radius;
} packet_proof;

static gh_effect_status packet(void *context,const gh_effect_trans_packet *p)
{
    packet_proof *proof=context;
    const unsigned i=proof->accepted;
    CHECK(i<6 && p->serial && p->order.bank==4 && p->order.depth==10 &&
          (p->order.identity&255)==i && p->frame_mask==0xff000000u && p->restores_scene_target &&
          p->texture_flush_after);
    if(i==3 && !proof->retries++){
        CHECK(p->kind==GH_EFFECT_PASS_SCENE_CAPTURE);
        return GH_EFFECT_UNSUPPORTED;
    }
    if(i&1){
        CHECK(p->kind==GH_EFFECT_PASS_SCENE_CAPTURE && p->original_callee==0x2fa620 &&
              p->source==GH_EFFECT_SURFACE_SCENE_COLOR && p->destination==GH_EFFECT_SURFACE_3C00 &&
              p->quad_count==8 && p->quads[0].vertices[0].v12_4==4 &&
              p->restore_zbuf==0x3a0000f8 && !p->writes_clamp);
    }else{
        CHECK(p->kind==GH_EFFECT_PASS_SCENE_WARP && p->original_callee==0x2e9d48 &&
              p->source==(i==4?GH_EFFECT_SURFACE_DISPLAY_BASE0:GH_EFFECT_SURFACE_3C00) &&
              p->destination==GH_EFFECT_SURFACE_SCENE_COLOR && p->prim==0x5d &&
              p->test==0x5000c && p->alpha==UINT64_C(0x8000000044) &&
              p->writes_texa && p->texa==0x80 && p->writes_clamp && !p->clamp &&
              !p->restore_zbuf && p->quad_count==1 && p->quads[0].vertex_count==6);
        const gh_effect_trans_vertex *v=p->quads[0].vertices;
        const unsigned x[]={256,0,512,512,0,0},y[]={176,0,0,448,448,0};
        for(unsigned k=0;k<6;++k)
            CHECK(v[k].x12_4==(int)(1792+x[k])*16 && v[k].y12_4==(int)(1824+y[k])*16 &&
                  v[k].z==0xfff0 && v[k].q==1 && v[k].rgba==proof->rgba);
        CHECK(v[0].s==256.0f/512.0f+1.0f/512.0f && v[0].t==176.0f/512.0f+1.0f/512.0f);
        CHECK(v[1].s==proof->radius+1.0f/512.0f && v[1].t==proof->radius+1.0f/512.0f);
        CHECK(v[3].s==1.0f-proof->radius+1.0f/512.0f && v[3].t==448.0f/512.0f-proof->radius+1.0f/512.0f);
    }
    ++proof->accepted;
    return GH_EFFECT_OK;
}

static uint32_t clock_word(void *p){return ++*(uint32_t *)p;}

static void verify_impact_scheduler(const gh_effect_release_bindings *storage,gh_boot_work *work,
    gh_effects_frame_globals *globals,gh_camera_envelope *envelope,gh_display_state *display,
    gh_game_pl_camera *camera)
{
    gh_game_init_state *init=calloc(1,sizeof(*init));
    CHECK(init);
    gh_game_init_state_initialize(init);
    init->active_camera=camera->identity;
    gh_game_mt_seed_0030f2b0(&init->random,123);
    /* Isolate actual impact effects from the independent ambient scan. */
    const uint32_t flags=0x40,rate=0x3f800000;
    memcpy((uint8_t *)work+0x5e4,&flags,4);
    memcpy((uint8_t *)work+0x574,&rate,4);
    gh_effect_create_call impact={0};
    const uint32_t xyz[]={0,0,0};
    const gh_effect_create_bindings producer={.storage=*storage,.work=work,.random=&init->random,
        .atan_coeff=envelope->atan_coeff};
    CHECK(!gh_effect_create_player_impact_begin_00306838(&impact,xyz) &&
          !gh_game_effect_create_00306528(&impact,&producer) && impact.complete && impact.count==4);
    gh_game_scene_aux profile={0};
    uint32_t clock=0;
    const gh_effects_frame_bindings bindings={.storage=*storage,.work=work,.init=init,.globals=globals,
        .profile=&profile,.clock=clock_word,.clock_context=&clock,.atan_coeff=envelope->atan_coeff,
        .policy=GH_STATIC_BINARY32_REFERENCE,.camera=camera,.display=display};
    gh_game_effects_frame frame={0};
    unsigned mask=0,checked=0;
    const float marker=0.125f,far_eye=1000.0f,origin=0.0f;
    for(unsigned tick=0;tick<70;++tick){
        gh_effect_status status=gh_game_effects_frame_update_0030f228(&frame,&bindings);
        if(status)fprintf(stderr,"Impact FX tick %u status %d frontier %08x emitter %u ESP %u\n",
            tick,status,frame.frontier,frame.emitter_index,frame.esp_index);
        CHECK(!status);
        for(unsigned slot=0;slot<1024;++slot){
            gh_effect_esp_view particle;
            if(gh_effect_create_esp_at(storage->effects,slot,&particle)||!particle.initialized)continue;
            mask|=1u<<particle.type;
            if(particle.type!=0x18||checked>=4)continue;
            CHECK((word(particle.payload+0x2b0)&0xffff)==0xffff);
            if(!checked){
                CHECK(camera->fields[0x278]==2 && !camera->fields[0x264]);
                CHECK(!memcmp(camera->fields+0x268,particle.record.bytes+0x13c,16));
                camera->fields[0x278]=1;
                memcpy(camera->fields+0x274,&marker,4);
            }else if(checked==1){
                CHECK(camera->fields[0x278]==2 && real(camera->fields+0x274)==marker);
                camera->fields[0x278]=0;
                memcpy(camera->fields+0x210,&far_eye,4);
            }else if(checked==2){
                /* Actual row radius is zero, so even this distant eye is eligible. */
                CHECK(!word(particle.payload+0x2b8) && camera->fields[0x278]==2 &&
                      real(camera->fields+0x274)!=marker);
                int near=1;
                uint32_t frontier=0;
                CHECK(!gh_esp18_shake_range_002f3e68(storage->effects,slot,particle.serial,camera,1,
                    GH_STATIC_BINARY32_REFERENCE,&near,&frontier) && !near);
                memcpy(camera->fields+0x210,&origin,4);
            }else{
                CHECK(camera->fields[0x278]==2 && real(camera->fields+0x274)!=marker);
            }
            ++checked;
        }
    }
    CHECK(mask==0x1000081 && checked==4);
    free(init);
}

int main(int argc,char **argv)
{
    CHECK(argc==2);
    gh_disc *disc;
    CHECK(!gh_disc_open(argv[1],&disc));
    const size_t elf_size=(size_t)gh_disc_boot_elf_size(disc);
    uint8_t *elf=malloc(elf_size);
    CHECK(elf && !gh_disc_read_boot_elf(disc,0,elf,elf_size));
    size_t core_size,room_size;
    uint8_t *core=load(disc,"etc/core.dat",&core_size),*room_bytes=load(disc,"st1/r100.dat",&room_size);
    gh_bundle room;
    CHECK(!gh_bundle_open(room_bytes,room_size,&room));
    gh_game_effects *effects=calloc(1,sizeof(*effects));
    uint8_t *esp=calloc(1,0x100180),*emitters=calloc(1,0x26060);
    CHECK(effects && esp && emitters && !gh_game_effects_initialize_00305a68(effects,esp,0x100180,
           emitters,0x26060,core,core_size,&room));
    const gh_effect_release_bindings storage={.effects=effects,.esp=esp,.esp_bytes=0x100180,
        .emitters=emitters,.emitter_bytes=0x26060};
    gh_effects_frame_globals globals={0};
    gh_camera_envelope envelope={0};
    gh_display_state display={0};
    uint32_t frontier;
    CHECK(!gh_effects_frame_globals_from_elf(&globals,elf,elf_size) &&
          !gh_game_camera_envelope_from_elf(&envelope,elf,elf_size,&frontier) &&
          !gh_display_state_from_elf(&display,elf,elf_size) &&
          !gh_display_static_initialize_002a48a8(&display,1,0xffff));
    gh_boot_work work={0};
    gh_game_mt rng;
    gh_game_mt_seed_0030f2b0(&rng,123);
    gh_effect_create_call impact={0};
    const uint32_t xyz[]={0,0,0};
    const gh_effect_create_bindings producer={.storage=storage,.work=&work,.random=&rng,
        .atan_coeff=envelope.atan_coeff};
    CHECK(!gh_effect_create_player_impact_begin_00306838(&impact,xyz) &&
          !gh_game_effect_create_00306528(&impact,&producer) && impact.complete && impact.count==4);
    gh_effect_emitter_view source;
    CHECK(!gh_effect_create_emitter_at(effects,0,&source) && source.record.bytes[0x133]==7 && !source.actor);
    gh_esp_create_call particle={0};
    const gh_esp_create_bindings create={.storage=storage,.random=&rng,.work=&work,
        .policy=GH_STATIC_BINARY32_REFERENCE,.globals=&globals,.atan_coeff=envelope.atan_coeff};
    CHECK(!gh_esp_create_begin_00300e30(&particle,0,source.serial));
    gh_effect_status status=gh_game_esp_create_00300e30(&particle,&create);
    if(status)fprintf(stderr,"ESP07 Init %d at %08x stage%u\n",status,particle.frontier,particle.stage);
    CHECK(!status && particle.complete);
    gh_effect_esp_view live;
    CHECK(!gh_effect_create_esp_at(effects,particle.slot,&live) && live.initialized &&
          live.record.bytes==source.record.bytes && live.parent_emitter==0);
    uint8_t *p=live.payload;
    CHECK(word(p+0xf0)==0x44e770 && !(word(p+0x11c)&0x1000000u) && p[0x2cc]==30 && p[0x2ce]==3);
    CHECK(real(p+0x2b0)==2.5f*0.01f && real(p+0x2b4)==real(p+0x2b0) &&
          !word(p+0x2bc) && !word(p+0x2c8) && !word(p+0x2d0) && !globals.distortion_ready_61b644);

    gh_game_effects_trans trans={0};
    gh_effects_trans_bindings draw={.storage=storage,.work=&work,.globals=&globals,
        .display=&display,.policy=GH_STATIC_BINARY32_REFERENCE,.cvt_policy=GH_FRAME_CHOP_REFERENCE};
    CHECK(!gh_game_effects_trans_002fb310(&trans,&draw) && trans.complete && !trans.enqueued);
    gh_game_camera identity={0};
    gh_game_pl_camera camera={0};
    CHECK(!gh_game_camera_construct_pl_00139bc8(&camera,&identity,GH_CAMERA_NATIVE_BINARY32_REFERENCE));
    /* Controlled GS projection matrix on the original camera owner. */
    float matrix[16]={0};
    memcpy(camera.fields,matrix,sizeof(matrix));
    const gh_esp_update_bindings update={.storage=storage,.work=&work,.globals=&globals,
        .atan_coeff=envelope.atan_coeff,.policy=GH_STATIC_BINARY32_REFERENCE,
        .camera=&camera,.active_camera=&identity,.display=&display};
    gh_esp_update_call move={0};
    status=gh_game_esp_update_002fbed0(&move,&update,particle.slot,particle.serial);
    if(status!=GH_EFFECT_UNSUPPORTED || move.frontier!=0x138470)
        fprintf(stderr,"ESP07 Move %d at %08x stage%u\n",status,move.frontier,move.stage);
    CHECK(status==GH_EFFECT_UNSUPPORTED && move.frontier==0x138470 && move.stage==16);
    const uint32_t age=word(p+0x228);
    matrix[12]=2048;
    matrix[13]=2000;
    matrix[15]=1;
    memcpy(camera.fields,matrix,sizeof(matrix));
    CHECK(!gh_game_esp_update_002fbed0(&move,&update,particle.slot,particle.serial) && move.complete);
    CHECK(real(p+0x2c0)==256 && real(p+0x2c4)==176 &&
          word(p+0x228)==age && globals.distortion_ready_61b644);
    CHECK(!gh_effects_trans_begin_002fb310(&trans));
    CHECK(gh_game_effects_trans_002fb310(&trans,&draw)==GH_EFFECT_UNSUPPORTED &&
          trans.frontier==0x2e9d48 && !trans.enqueued);
    packet_proof proof={.radius=real(p+0x2b4)};
    for(unsigned j=0;j<4;++j)proof.rgba|=(uint32_t)(real(p+0x188+4*j)*128.0f)<<(8*j);
    draw.emit=packet;
    draw.context=&proof;
    CHECK(gh_game_effects_trans_002fb310(&trans,&draw)==GH_EFFECT_UNSUPPORTED &&
          trans.frontier==0x2fa620 && trans.enqueued==3 && trans.pending);
    CHECK(!gh_game_effects_trans_002fb310(&trans,&draw) && trans.complete &&
          trans.enqueued==6 && proof.accepted==6 && proof.retries==2);
    CHECK(!gh_effect_create_esp_abort_native(effects,particle.slot,particle.serial));
    gh_effect_create_forget_on_owner_destroy(effects);
    memset(esp,0,0x100180);
    memset(emitters,0,0x26060);
    memset(effects,0,sizeof(*effects));
    CHECK(!gh_game_effects_initialize_00305a68(effects,esp,0x100180,emitters,0x26060,core,core_size,&room));
    verify_impact_scheduler(&storage,&work,&globals,&envelope,&display,&camera);
    gh_effect_create_forget_on_owner_destroy(effects);
    puts("PASS actual bank0/index3 all classes, 70 scheduled Moves, continuous ESP18 camera refresh/range");
    free(emitters);
    free(esp);
    free(effects);
    free(room_bytes);
    free(core);
    free(elf);
    gh_disc_close(disc);
    puts("PASS R100 actual player impact ESP07 allocator, Init, Move, 3 fans/3 captures, ordered Draw retry");
    return 0;
}
