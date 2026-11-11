#include "game_enemy_weapon.h"
#include "game_om_base.h"
#include <string.h>

enum { ALLOCATED=1, STAMPED, RESOURCE_BOUND, PUBLISHED, ORIGINAL_NULL };

static uint32_t word(const uint8_t *bytes)
{
    uint32_t result;
    memcpy(&result,bytes,4);
    return result;
}

static void put(uint8_t *bytes,uint32_t value)
{
    memcpy(bytes,&value,4);
}

static void half(uint8_t *bytes,uint16_t value)
{
    memcpy(bytes,&value,2);
}

static void vf0(uint8_t *bytes)
{
    memset(bytes,0,12);
    put(bytes+12,0x3f800000);
}

gh_actor_model_status gh_om_weapon_construct_001c9140(void *data,size_t size,gh_title_player_view *view)
{
    if(!data||!view)return GH_ACTOR_MODEL_ARGUMENT;
    if(size<GH_ENEMY_WEAPON_363_BYTES)return GH_ACTOR_MODEL_BOUNDS;
    if(gh_om_base_construct_001b6e90(data,size,view))return GH_ACTOR_MODEL_ARGUMENT;
    uint8_t *bytes=data;
    put(bytes+0x214,0x42aa98);
    view->original_vtable214=0x42aa98;
    static const uint16_t vectors[]={0x610,0x620,0x640,0x6a0};
    for(unsigned i=0;i<sizeof(vectors)/sizeof(vectors[0]);i++)vf0(bytes+vectors[i]);
    for(unsigned i=0;i<14;i++)vf0(bytes+0x6b0+i*16);
    return GH_ACTOR_MODEL_OK;
}

static gh_actor_model_status stop(gh_enemy_weapon *weapon,uint32_t pc,gh_actor_model_status status)
{
    weapon->frontier=pc;
    return status;
}

static int same_bindings(const gh_enemy_entry_bindings *a,const gh_enemy_entry_bindings *b)
{
    return a->pools==b->pools&&a->work==b->work&&a->factory_serial==b->factory_serial&&
        a->room_objects==b->room_objects&&a->global_objects==b->global_objects&&
        a->resources.heap==b->resources.heap&&a->resources.dvd==b->resources.dvd&&
        a->resources.threads==b->resources.threads;
}

static gh_actor_model_status resource_status(gh_object_status status)
{
    switch(status){
    case GH_OBJECT_MEMORY: return GH_ACTOR_MODEL_MEMORY;
    case GH_OBJECT_BOUNDS: return GH_ACTOR_MODEL_BOUNDS;
    case GH_OBJECT_BUSY: return GH_ACTOR_MODEL_BUSY;
    default: return GH_ACTOR_MODEL_UNSUPPORTED;
    }
}

gh_actor_model_status gh_enemy_weapon_create_kind_0030fa98(gh_enemy_weapon *weapon,
    const gh_enemy_entry_bindings *binding,uint16_t kind,int *nonnull)
{
    if(!weapon||!binding||!binding->pools||!binding->work||!binding->factory_serial||
        !binding->room_objects||!binding->global_objects||!nonnull)return GH_ACTOR_MODEL_ARGUMENT;
    *nonnull=0;
    if(weapon->stage&&(weapon->kind!=kind||!same_bindings(&weapon->bindings,binding)))
        return GH_ACTOR_MODEL_BUSY;
    if(kind<0x361||kind>0x363)return stop(weapon,0x30fb70,GH_ACTOR_MODEL_UNSUPPORTED);
    weapon->frontier=0;
    if(weapon->stage==ORIGINAL_NULL)return GH_ACTOR_MODEL_OK;

    if(!weapon->stage){
        const gh_room_vector *all,*named;
        if(gh_room_registry_view(binding->pools,0x754c30,&all)||
            gh_room_registry_view(binding->pools,0x754c58,&named))
            return stop(weapon,0x30fb70,GH_ACTOR_MODEL_ARGUMENT);
        weapon->bindings=*binding;
        weapon->kind=kind;
        uint32_t flags=word((const uint8_t*)binding->work+0x594);
        if(all->count>=all->capacity||named->count>=named->capacity||(flags&0x10000u)){
            weapon->stage=ORIGINAL_NULL;
            return GH_ACTOR_MODEL_OK;
        }

        gh_room_arena_allocation allocation;
        gh_room_pool_status status=gh_room_arena_allocate_002a9538(binding->pools,0x754c10,
            flags,GH_ENEMY_WEAPON_363_BYTES,16,&allocation);
        if(status)return stop(weapon,0x316150,GH_ACTOR_MODEL_MEMORY);
        if(!allocation.pointer){weapon->stage=ORIGINAL_NULL;return GH_ACTOR_MODEL_OK;}
        uint8_t *bytes=allocation.pointer;
        memset(bytes,0,GH_ENEMY_WEAPON_363_BYTES);
        weapon->object.arena_allocation=allocation;
        weapon->object.allocation_domain=GH_PLAYER_ALLOCATION_ROOM_ARENA;
        weapon->stage=ALLOCATED;
    }

    if(weapon->stage==ALLOCATED){
        uint8_t *bytes=weapon->object.arena_allocation.pointer;
        if(gh_om_weapon_construct_001c9140(bytes,GH_ENEMY_WEAPON_363_BYTES,&weapon->object.view))
            return stop(weapon,0x1c9140,GH_ACTOR_MODEL_ARGUMENT);
        weapon->object.view.initial_memory=GH_TITLE_MEMORY_ORIGINAL_FACTORY_ZERO;
        bytes[0x2fc]=(*binding->factory_serial)++;
        half(bytes+0x2fe,kind);
        half(bytes+0x300,kind);
        put(bytes+0x2d0,0x20000000);
        weapon->stage=STAMPED;
    }

    if(weapon->stage==STAMPED){
        if(!weapon->resource_owner && !weapon->load.stage){
            gh_object_status status=gh_game_objects_acquire_001fea48(binding->room_objects,kind,&weapon->resource);
            if(status==GH_OBJECT_OK)weapon->resource_owner=binding->room_objects;
            else if(status!=GH_OBJECT_MISSING)return stop(weapon,0x1fea48,resource_status(status));
        }
        if(!weapon->resource.bytes && weapon->resource_owner!=binding->global_objects){
            if(weapon->resource_owner==binding->room_objects)
                return stop(weapon,0x1fea48,GH_ACTOR_MODEL_UNSUPPORTED);
            uint32_t frontier=0;
            gh_object_status status=gh_game_objects_load_wait_001fe8a0(&weapon->load,
                binding->global_objects,kind,&binding->resources,1,&weapon->resource,&frontier);
            if(status)return stop(weapon,frontier?frontier:0x1fe8a0,resource_status(status));
            if(!weapon->resource.bytes)return stop(weapon,0x312778,GH_ACTOR_MODEL_UNSUPPORTED);
            weapon->resource_owner=binding->global_objects;
            uint8_t *bytes=weapon->object.view.bytes;
            put(bytes+0x250,word(bytes+0x250)|0x02000000u);
        }
        weapon->stage=RESOURCE_BOUND;
    }

    if(weapon->stage==RESOURCE_BOUND){
        gh_room_pool_status status=gh_room_object_register(binding->pools,&weapon->object,
            weapon,GH_ROOM_OBJECT_ENEMY_WEAPON,9,&weapon->handle);
        if(status)return stop(weapon,0x30fcc0,GH_ACTOR_MODEL_BUSY);
        weapon->stage=PUBLISHED;
    }
    weapon->frontier=0x1c9268;
    *nonnull=1;
    return GH_ACTOR_MODEL_OK;
}

gh_actor_model_status gh_enemy_weapon_create_0030fa98(gh_enemy_weapon *weapon,
    const gh_enemy_entry_bindings *binding,int *nonnull)
{
    return gh_enemy_weapon_create_kind_0030fa98(weapon,binding,0x363,nonnull);
}

gh_actor_model_status gh_enemy_weapon_release_factory(gh_enemy_weapon *weapon)
{
    if(!weapon)return GH_ACTOR_MODEL_ARGUMENT;
    if(weapon->handle){
        if(gh_room_object_unregister(weapon->bindings.pools,weapon->handle,weapon))
            return stop(weapon,0x30fcc0,GH_ACTOR_MODEL_BUSY);
        weapon->handle=0;
    }
    if(weapon->resource_owner){
        if(gh_game_objects_release_reference_001ff0b0(weapon->resource_owner,weapon->kind))
            return stop(weapon,0x1ff0b0,GH_ACTOR_MODEL_BUSY);
        weapon->resource_owner=NULL;
    }
    if(gh_game_title_player_release(&weapon->object))return stop(weapon,0x2a9680,GH_ACTOR_MODEL_BUSY);
    *weapon=(gh_enemy_weapon){0};
    return GH_ACTOR_MODEL_OK;
}
