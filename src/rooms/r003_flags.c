#include "r003_flags.h"
#include "r100_scripts.h"
#include "../core/game_actor_matrix.h"
#include "../core/game_om041c.h"
#include "../core/game_scene_at_geometry.h"

#include <fenv.h>
#include <math.h>
#include <string.h>

enum {FLAG_COUNT=6};

static float add(float a,float b){volatile float v=a+b;return v;}
static float mul(float a,float b){volatile float v=a*b;return v;}

static gh_r100_init_status object_view(void *context,uint32_t handle,gh_r100_om_view *out,uint32_t *frontier)
{
    const gh_om_factory *factory=context;
    void *found=NULL;
    if(gh_room_object_lookup(factory->bindings.pools,handle,GH_ROOM_OBJECT_OM,factory,&found)){
        *frontier=0x14fd98;
        return GH_R100_INIT_BOUNDS;
    }
    const gh_om_object *object=found;
    *out=(gh_r100_om_view){object->view.bytes,object->view.size,object};
    return GH_R100_INIT_OK;
}

static int random_range(gh_game_mt *mt,float low,float high,float *out)
{
    uint32_t draw;
    if(gh_game_mt_next_0030f348(mt,&draw))return 0;
    uint32_t bits=gh_frame_jitter_bits_0031d020_word(draw);
    float jitter;
    memcpy(&jitter,&bits,4);
    const float half=mul(add(high,-low),0.5f);
    *out=add(add(mul(half,jitter),low),half);
    return 1;
}

static uint32_t move_flag(gh_om041c_state *flag,unsigned index,gh_game_mt *mt,gh_frame_random *random,
    float wind[3])
{
    const gh_title_player_view *view=&flag->common.model.root;
    float offset[3]={0,0,0.2f},angles[3],position[3],rotation[16],rotated[4];
    if(!random_range(mt,-0.5f,0.5f,offset)||!random_range(mt,0,1,offset+1))return 0x31d050;
    memcpy(angles,view->bytes+0x100,12);
    memcpy(position,view->alias_f0,12);
    uint32_t pc=0;
    if(gh_actor_matrix_rotation_00147728(angles,0,rotation,GH_STATIC_BINARY32_REFERENCE,&pc))
        return pc?pc:0x150830;
    if(gh_scene_at_apply_world_001508d8(rotation,offset,rotated))return 0x150830;

    uint8_t gust;
    const float direction[3]={0,0,-1};
    if(gh_frame_random_byte_0031cef8(random,&gust))return 0x31cef8;
    if(gust<8){
        float center[3],strength;
        if(fesetround(FE_TONEAREST))return 0x150830;
        for(unsigned j=0;j<3;j++)center[j]=add(position[j],rotated[j]);
        if(fesetround(FE_TOWARDZERO))return 0x150830;
        if(!random_range(mt,0.05f,0.15f,&strength))return 0x31d050;
        if(gh_om041c_add_wind_002ae218(flag,center,direction,0.7f,strength))
            return flag->frontier?flag->frontier:0x2ae218;
    }

    float strength=add(wind[0],-mul((float)index,0.5f));
    if(wind[1]<=strength)strength=add(add(wind[1],wind[1]),-strength);
    if(strength<0)strength=0;
    if(strength>wind[1])strength=wind[1];
    if(gh_om041c_add_wind_002ae218(flag,position,direction,3.0f,strength))
        return flag->frontier?flag->frontier:0x2ae218;
    wind[0]=add(wind[0],wind[2]);
    /* REL+AD0 is the original signed integer 6. F13 still holds 0.5
     * at REL+6C4, making the first reset range [0.2,0.5]. */
    if(add(add(wind[1],wind[1]),(float)FLAG_COUNT*0.5f)<wind[0]){
        wind[0]=0;
        if(!random_range(mt,0.2f,0.5f,wind+1)||!random_range(mt,0.0001f,0.05f,wind+2))
            return 0x31d050;
    }
    return 0;
}

gh_r003_status gh_r003_move_flags_000003d8(gh_r003_event *room,const gh_om_factory *factory,
    gh_game_mt *mt,gh_frame_random *random,uint32_t *frontier)
{
    if(!frontier)return GH_R003_ARGUMENT;
    *frontier=0x3d8;
    if(!room||!room->initialized||!room->work.pointer||!factory||!factory->bound||!mt||!random||
       factory->bindings.work!=room->bindings.play.work||random->policy!=GH_FRAME_CHOP_REFERENCE)
        return GH_R003_ARGUMENT;
    static const char *const names[FLAG_COUNT]={"om030","om031","om032","om033","om034","om035"};
    gh_om041c_state *flags[FLAG_COUNT]={0};
    for(unsigned i=0;i<FLAG_COUNT;i++){
        gh_r100_om_view found;
        if(gh_r100_find_om_0014fd98(factory->bindings.pools,object_view,(void *)factory,names[i],&found,frontier))
            return GH_R003_CALLEE;
        if(!found.identity)continue;
        const gh_om_object *object=found.identity;
        if(object->source.type!=0x41c||object->stage!=7||!object->initialization){
            *frontier=0x2ae218;
            return GH_R003_UNSUPPORTED;
        }
        flags[i]=(gh_om041c_state *)object->initialization;
        if(!flags[i]->initialized||flags[i]->common.object!=object){
            *frontier=0x2ae218;
            return GH_R003_ARGUMENT;
        }
    }
    float wind[3];
    memcpy(wind,room->work.pointer,sizeof(wind));
    for(unsigned i=0;i<3;i++)if(!isfinite(wind[i])||wind[i]<0){
        *frontier=0x3d8;
        return GH_R003_UNSUPPORTED;
    }
    fenv_t saved;
    if(fegetenv(&saved)||fesetround(FE_TOWARDZERO)){
        *frontier=0x3d8;
        return GH_R003_UNSUPPORTED;
    }
    uint32_t pc=0;
    for(unsigned i=0;i<FLAG_COUNT&&!pc;i++)if(flags[i]){
        pc=move_flag(flags[i],i,mt,random,wind);
        memcpy(room->work.pointer,wind,sizeof(wind));
    }
    if(fesetenv(&saved)&&!pc)pc=0x3d8;
    *frontier=pc;
    return pc?GH_R003_CALLEE:GH_R003_OK;
}
