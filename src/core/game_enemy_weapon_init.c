/* Original 001C9268, bounded to the real kind-363 weapon class. */
#include "game_enemy_weapon.h"
#include "game_actor_model_nodes.h"
#include "game_actor_external_parent.h"
#include "game_event_player_reset.h"
#include <string.h>

enum {
    RESOURCES_READY=1, MODEL_READY, RECEIVER_CREATED, RECEIVER_SHAPED,
    RECEIVER_DISABLED, ATTACKER_CREATED, ATTACKER_SHAPED, ATTACKER_DISABLED,
    HIDE_LAYER, COMPLETE
};

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

static gh_actor_model_status stop(gh_enemy_weapon_init *s,uint32_t pc,gh_actor_model_status error)
{
    s->frontier=pc;
    return error;
}

static gh_actor_model_status hit_error(gh_enemy_weapon_init *s,gh_hitarea_status error,
    uint32_t pc,uint32_t fallback)
{
    return stop(s,pc?pc:fallback,error==GH_HITAREA_MEMORY?GH_ACTOR_MODEL_MEMORY:
        error==GH_HITAREA_BOUNDS?GH_ACTOR_MODEL_BOUNDS:GH_ACTOR_MODEL_UNSUPPORTED);
}

gh_actor_model_status gh_enemy_weapon_initialize_001c9268(gh_enemy_weapon_init *s,
    gh_enemy_weapon *weapon,const gh_hitarea_shape_bindings *shapes,int *original)
{
    if(!s||!weapon||!shapes||!original||!weapon->handle||!weapon->object.view.bytes||
        !weapon->bindings.init||!weapon->bindings.packet_heap||!weapon->bindings.fog_rgb||
        !shapes->pairs||!shapes->math||!shapes->math->initialized)
        return GH_ACTOR_MODEL_ARGUMENT;
    *original=0;
    if(s->weapon&&(s->weapon!=weapon||s->shapes.pairs!=shapes->pairs||
        s->shapes.math!=shapes->math||s->shapes.policy!=shapes->policy))
        return GH_ACTOR_MODEL_BUSY;
    gh_title_player_view *object=&weapon->object.view;
    uint8_t *p=object->bytes;
    if(object->size<GH_ENEMY_WEAPON_363_BYTES||object->original_vtable214!=0x42aa98||
        (word(p+0x2fc)>>16)!=0x363)return stop(s,0x1c9268,GH_ACTOR_MODEL_UNSUPPORTED);
    void *identity=NULL;
    if(gh_room_object_lookup(weapon->bindings.pools,weapon->handle,GH_ROOM_OBJECT_ENEMY_WEAPON,
        weapon,&identity)||identity!=&weapon->object)return GH_ACTOR_MODEL_BUSY;
    s->weapon=weapon;
    s->shapes=*shapes;
    s->frontier=0;

    if(!s->stage){
        gh_object_view resource;
        if(!weapon->resource_owner||gh_game_objects_find(weapon->resource_owner,0x363,&resource)||
            resource.bytes!=weapon->resource.bytes||!resource.file_extent_known||
            resource.file_extent>resource.size)return stop(s,0x31ccf0,GH_ACTOR_MODEL_BOUNDS);
        gh_bundle bundle;
        if(gh_bundle_open(resource.bytes,resource.file_extent,&bundle))
            return stop(s,0x31ccf0,GH_ACTOR_MODEL_BOUNDS);
        if(gh_bundle_find(&bundle,"MD",0,&s->geometry))
            return stop(s,0x149738,GH_ACTOR_MODEL_UNSUPPORTED);
        gh_bundle_error result=gh_bundle_find(&bundle,"TM3",0,&s->texture);
        if(result==GH_BUNDLE_NOT_FOUND){
            s->texture_mode=1;
            result=gh_bundle_find(&bundle,"T3L",0,&s->texture);
            if(result==GH_BUNDLE_NOT_FOUND){
                s->texture_mode=0;
                result=gh_bundle_find(&bundle,"T32",0,&s->texture);
            }
        }
        if(result&&result!=GH_BUNDLE_NOT_FOUND)
            return stop(s,0x31ccf0,GH_ACTOR_MODEL_BOUNDS);
        gh_actor_model_status status=gh_actor_model_bind_packet_inputs(&s->model,
            weapon->bindings.packet_heap,weapon->bindings.fog_rgb);
        if(status)return stop(s,0x149738,status);
        s->stage=RESOURCES_READY;
    }

    if(s->stage==RESOURCES_READY){
        int initialized;
        gh_actor_model_status status=gh_actor_model_initialize_00149738(&s->model,object,
            weapon->bindings.work,weapon->bindings.pools,&s->geometry,&s->texture,0,
            s->texture_mode,weapon->bindings.policy,&initialized);
        if(status)return stop(s,s->model.frontier,status);
        if(!initialized)return stop(s,0x312708,GH_ACTOR_MODEL_UNSUPPORTED);
        if(word(p+0x5e0))return stop(s,0x1b7de8,GH_ACTOR_MODEL_UNSUPPORTED);
        p[0x140]=0;
        put(p+0x5b8,word(p+0x5b8)|0x40);
        put(p+0x254,word(p+0x254)|0x88000000u);
        put(p+0x4a8,word(p+0x4a8)|0x10000000);
        half(p+0x570,0xffff);
        put(p+0x574,2);
        put(p+0x3a8,0x3f800000);
        put(p+0x600,0);
        put(p+0x604,0);
        put(p+0x630,0);
        put(p+0x668,60);
        s->stage=MODEL_READY;
    }

    const gh_actor_part *root;
    if(gh_actor_model_part_at(&s->model,0,&root)||!root->bytes)
        return stop(s,0x1fdaf0,GH_ACTOR_MODEL_BOUNDS);
    for(unsigned i=0;i<2;i++){
        unsigned first=i?RECEIVER_DISABLED:MODEL_READY;
        unsigned created=i?ATTACKER_CREATED:RECEIVER_CREATED;
        unsigned shaped=i?ATTACKER_SHAPED:RECEIVER_SHAPED;
        unsigned disabled=i?ATTACKER_DISABLED:RECEIVER_DISABLED;
        uint32_t pc=0;
        if(s->stage==first){
            int present;
            gh_hitarea_status status=gh_hitarea_object_create_001fe0e8(weapon->bindings.init,
                object,(uint8_t)(1-i),&s->hit[i],&present,&pc);
            if(status)return hit_error(s,status,pc,0x1fe0e8);
            if(!present){
                if(!i)return stop(s,0x1fdaf0,GH_ACTOR_MODEL_UNSUPPORTED);
                s->stage=HIDE_LAYER;
                break;
            }
            if(!s->hit[i].fallback)s->hit_owned|=(uint8_t)(1u<<i);
            s->stage=(uint8_t)created;
        }
        if(s->stage==created){
            const uint32_t xyz[3]={0};
            gh_hitarea_status status=gh_hitarea_add_sphere_001fdaf0(&s->hit[i],shapes,
                root->bytes+0x80,xyz,i?0x3ecccccd:0x3e99999a,&s->shape_index[i],&pc);
            if(status)return hit_error(s,status,pc,0x1fdaf0);
            s->stage=(uint8_t)shaped;
        }
        if(s->stage==shaped){
            gh_hitarea_status status=gh_hitarea_object_set_active_001fd668(&s->hit[i],0,&pc);
            if(status)return hit_error(s,status,pc,0x1fd668);
            if(i){
                half(s->hit[i].bytes+0x46,0x2f);
                put(s->hit[i].bytes+0x4c,50);
                memcpy(s->hit[i].bytes+0x10,shapes->math->zero_vector,12);
                put(s->hit[i].bytes+0x40,UINT32_MAX);
                half(s->hit[i].bytes+0x44,0);
            }
            s->stage=(uint8_t)disabled;
        }
    }

    if(s->stage==ATTACKER_DISABLED||s->stage==HIDE_LAYER){
        for(unsigned i=0;i<s->model.script.mesh_count;i++){
            const gh_actor_node_packets *node;
            gh_actor_model_status status=gh_actor_model_packet_at(&s->model,i,&node);
            if(status)return stop(s,0x14b6f8,status);
            if(node->node[0x40c]==0x40){
                status=gh_actor_model_node_or_flags(&s->model,i,0x380,1);
                if(status)return stop(s,0x1b7280,status);
            }
        }
        p[0x2f4]=0;
        p[0x2f5]=0;
        p[0x2f6]=2;
        p[0x2f7]=0;
        put(p+0x5b8,word(p+0x5b8)&~0x40u);
        p[0x688]=0;
        put(p+0x68c,0);
        s->stage=COMPLETE;
    }
    *original=1;
    return GH_ACTOR_MODEL_OK;
}

gh_actor_model_status gh_enemy_weapon_attach_0026ab20(gh_enemy_weapon_init *s,
    const gh_title_player_view *parent,gh_actor_model *parent_model,
    gh_game_scene_tables *actors,gh_collision_scroll_mutation *scroll)
{
    if(!s||s->stage!=COMPLETE||!s->weapon||!parent||!parent->bytes||parent->size<=0x17c3||
        !parent->alias_f0||!parent_model||parent_model->root.bytes!=parent->bytes||!actors||!scroll)
        return GH_ACTOR_MODEL_ARGUMENT;
    if(s->attach_stage&&(s->parent!=parent||s->parent_model!=parent_model||
        s->actors!=actors||s->scroll!=scroll))return GH_ACTOR_MODEL_BUSY;
    if(word(parent->bytes+0x564)!=0x202||parent->bytes[0x17c3])
        return stop(s,0x26acdc,GH_ACTOR_MODEL_UNSUPPORTED);

    gh_title_player_view *weapon=&s->weapon->object.view;
    uint8_t *p=weapon->bytes;
    const gh_static_transform_policy policy=s->weapon->bindings.policy;
    const unsigned parent_index=16;
    const gh_actor_part *hand,*child;
    if(gh_actor_model_part_at(parent_model,parent_index,&hand)||
        gh_actor_model_part_at(&s->model,0,&child)||!hand->bytes||!child->bytes||!child->alias_d0)
        return stop(s,0x1cf160,GH_ACTOR_MODEL_UNSUPPORTED);
    s->frontier=0;

    if(!s->attach_stage){
        memmove(weapon->alias_f0,parent->alias_f0,12);
        memmove(p+0x490,weapon->alias_f0,12);
        memset(p+0x100,0,12);
        s->parent=parent;
        s->parent_model=parent_model;
        s->actors=actors;
        s->scroll=scroll;
        s->attach_stage=1;
    }
    if(s->attach_stage==1){
        gh_actor_model_status status=gh_actor_model_bind_external_parent(&s->model,0,parent_model,
            parent_index,s);
        if(status)return stop(s,0x1cf160,status);
        const uint32_t xyz[]={0x3e99999a,0xbd8f5c29,0x3f19999a};
        const uint32_t angles[]={0,0x3eb2b8c2,0};
        memcpy(child->alias_d0,xyz,12);
        memcpy(child->bytes+0x100,angles,12);
        put(p+0x604,parent_index);
        memcpy(p+0x610,xyz,12);
        memcpy(p+0x620,angles,12);
        p[0x2f4]=0;
        p[0x2f5]=1;
        p[0x2f6]=0;
        p[0x2f7]=0;
        p[0x664]=0;
        s->attach_stage=2;
    }
    if(s->attach_stage==2){
        const gh_event_weapon_attachment attachment={weapon,&s->model,parent,parent_model,parent_index};
        uint32_t pc=0;
        gh_game_event_status status=gh_event_weapon_follow_001cf3a8(&attachment,policy,&pc);
        if(status)return stop(s,pc?pc:0x1cf3a8,GH_ACTOR_MODEL_UNSUPPORTED);
        s->attach_stage=3;
    }
    if(s->attach_stage==3){
        gh_actor_model_status status=gh_actor_model_update_hierarchy_00149eb8(&s->model,policy);
        if(status)return stop(s,s->model.frontier?s->model.frontier:0x149eb8,status);
        s->attach_stage=4;
    }
    if(s->attach_stage==4){
        char name[9]={0};
        memcpy(name,p+0x540,8);
        uint32_t changed;
        /* Original 1331B8 argument 0 sets triangle flag bit2. */
        if(gh_collision_scroll_set_enabled_001332d0(actors,scroll,name,0,&changed))
            return stop(s,0x1331b8,GH_ACTOR_MODEL_UNSUPPORTED);
        s->attach_stage=5;
    }
    return GH_ACTOR_MODEL_OK;
}

gh_actor_model_status gh_enemy_weapon_release_init(gh_enemy_weapon_init *s)
{
    if(!s)return GH_ACTOR_MODEL_ARGUMENT;
    if(s->attach_stage>=2&&s->model.external_owner){
        gh_actor_model_status status=gh_actor_model_clear_external_parent(&s->model,s);
        if(status)return stop(s,0x1cf160,status);
    }
    for(unsigned i=2;i>0;i--){
        gh_hitarea_object *hit=&s->hit[i-1];
        if(!(s->hit_owned&(1u<<(i-1))))continue;
        uint32_t pc=0;
        gh_hitarea_status status=gh_hitarea_shapes_abort_object(hit,&pc);
        if(status)return hit_error(s,status,pc,0x1fd580);
        status=gh_hitarea_object_release_empty_001fe278(hit,&pc);
        if(status)return hit_error(s,status,pc,0x1fe148);
        s->hit_owned&=(uint8_t)~(1u<<(i-1));
        *hit=(gh_hitarea_object){0};
    }
    gh_actor_model_status status=gh_actor_model_release(&s->model);
    if(status)return stop(s,0x149738,status);
    *s=(gh_enemy_weapon_init){0};
    return GH_ACTOR_MODEL_OK;
}
