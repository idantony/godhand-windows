#include "game_enemy_2a2.h"
#include "disc.h"
#include <fenv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr,"line %d: %s\n",__LINE__,#expr); return 1; } } while (0)

static void put_word(uint8_t *bytes,uint32_t value)
{
    memcpy(bytes,&value,sizeof(value));
}

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

int main(int argc,char **argv)
{
    CHECK(argc==2);
    uint8_t data[GH_EM2A2_BYTES+16], expected[sizeof(data)];
    memset(data,0xa5,sizeof(data));
    memcpy(expected,data,sizeof(data));
    gh_title_player_view parent,view={0};
    CHECK(gh_enemy2a2_construct_00284a28(data,GH_EM2A2_BYTES-1,&view)==GH_PLAYER_BOUNDS);
    CHECK(!view.bytes&&!memcmp(expected,data,sizeof(data)));
    CHECK(!gh_enemy_base_construct_0028eb00(expected,GH_EM2A2_BYTES,&parent));
    CHECK(!gh_enemy2a2_construct_00284a28(data,GH_EM2A2_BYTES,&view));
    static const size_t vectors[]={0x1540,0x1570,0x1580,0x1590,0x15a0,0x15f0};
    for (unsigned i=0;i<sizeof(vectors)/sizeof(vectors[0]);i++) {
        memset(expected+vectors[i],0,12);
        put_word(expected+vectors[i]+12,0x3f800000);
    }
    expected[0x1561]=0xff;
    put_word(expected+0x214,0x448ca0);
    put_word(expected+0x4ac,0x448a00);
    CHECK(!memcmp(expected,data,sizeof(data))&&view.alias_f0==data+0xb0&&view.alias_d0==data+0x30);
    CHECK(view.original_vtable214==0x448ca0&&view.original_kind4ac==0x448a00);

    gh_heap *heap,*small;
    CHECK(!gh_heap_create(32*1024*1024,&heap)&&!gh_heap_create(16,&small));
    gh_boot_work work={.room=0x100};
    gh_game_title_player actor={0};
    int nonnull=9;
    CHECK(!gh_enemy2a2_allocate_00313820(&actor,small,&work,&nonnull)&&!nonnull&&!gh_heap_used(small));
    CHECK(!gh_heap_destroy(small));
    put_word((uint8_t *)&work+0x594,0x100000);
    CHECK(!gh_enemy2a2_allocate_00313820(&actor,heap,&work,&nonnull)&&!nonnull&&!gh_heap_used(heap));
    put_word((uint8_t *)&work+0x594,0);

    gh_disc *disc;
    CHECK(!gh_disc_open(argv[1],&disc));
    size_t room_extent,asset_extent;
    uint8_t *room=load(disc,"st1/r100.dat",&room_extent),*asset=load(disc,"em/ema2.dat",&asset_extent);
    CHECK(room&&asset&&asset_extent==778784);
    gh_bundle room_bundle;
    CHECK(!gh_bundle_open(room,room_extent,&room_bundle));
    gh_room_pools pools={0};
    CHECK(!gh_game_room_pools_initialize_002a5d80(&pools,&room_bundle,heap));
    const size_t baseline=gh_heap_used(heap);
    gh_object_view resource={.bytes=asset,.size=asset_extent,.file_extent=asset_extent,
        .file_extent_known=1,.kind=0x2a2};
    const size_t md_offsets[]={0xee20,0x32ee0,0x577e0},tm3_offsets[]={0x320,0x243e0,0x48460};
    const int modes[]={FE_TONEAREST,FE_TOWARDZERO,FE_DOWNWARD,FE_UPWARD};
    uint32_t fog=0;
    for (unsigned mode=0;mode<4;mode++) {
        CHECK(!fesetround(modes[mode]));
        for (unsigned variant=0;variant<3;variant++) {
            CHECK(!gh_enemy2a2_allocate_00313820(&actor,heap,&work,&nonnull)&&nonnull&&actor.ready);
            CHECK(actor.view.initial_memory==GH_TITLE_MEMORY_ORIGINAL_FACTORY_ZERO&&actor.view.bytes[0x160f]==0);
            CHECK((uintptr_t)actor.view.bytes%16==0&&gh_heap_used(heap)==baseline+GH_EM2A2_BYTES);
            CHECK(gh_enemy2a2_allocate_00313820(&actor,heap,&work,&nonnull)==GH_PLAYER_BUSY&&!nonnull);
            /* These are factory class and Init request fields. The fixture
             * does not register an actor or advance the encounter. */
            memcpy(actor.view.bytes+0x2fe,&(uint16_t){0x2a2},2);
            put_word(actor.view.bytes+0x564,0x2a2+variant);
            gh_enemy_model model={0};
            int original=0;
            gh_actor_model_status status=gh_enemy2a2_model_00288da8(&model,&actor.view,&resource,
                &work,&pools,heap,&fog,GH_STATIC_BINARY32_REFERENCE,&original);
            if (status) fprintf(stderr,"variant %u status %d frontier %08x\n",variant,status,model.frontier);
            CHECK(!status&&original==1&&model.model.ready&&model.occurrence==variant);
            CHECK(model.geometry.offset==md_offsets[variant]&&model.texture.offset==tm3_offsets[variant]);
            uint32_t model_offset;
            memcpy(&model_offset,actor.view.bytes+0x424,4);
            CHECK(model_offset==25u*variant&&fegetround()==modes[mode]);
            const size_t used=gh_heap_used(heap);
            CHECK(!gh_enemy2a2_model_00288da8(&model,&actor.view,&resource,&work,&pools,heap,&fog,
                GH_STATIC_BINARY32_REFERENCE,&original)&&original==1&&gh_heap_used(heap)==used);
            gh_object_view truncated=resource;
            truncated.file_extent=model.geometry.offset+model.geometry.size-1;
            CHECK(gh_enemy2a2_model_00288da8(&model,&actor.view,&truncated,&work,&pools,heap,&fog,
                GH_STATIC_BINARY32_REFERENCE,&original)==GH_ACTOR_MODEL_BOUNDS&&
                model.frontier==0x31ccf0&&!original&&gh_heap_used(heap)==used);
            CHECK(!gh_enemy_model_release_prefix(&model)&&!gh_game_title_player_release(&actor));
            CHECK(gh_heap_used(heap)==baseline&&!pools.blocks[0].used_count&&!pools.blocks[1].used_count);
        }
    }
    CHECK(!fesetround(FE_TONEAREST));
    CHECK(!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));
    gh_disc_close(disc);
    free(asset);
    free(room);
    puts("PASS original EMA2 constructor, shared heap gate and all three real model variants in four FEnv modes");
    return 0;
}
