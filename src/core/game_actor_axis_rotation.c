#include "game_actor_axis_rotation.h"
#include "game_actor_matrix.h"
#include "game_actor_two_bone.h"
#include "id_layout_rotation.h"
#include "sdk_sine.h"

#include <fenv.h>
#include <math.h>
#include <string.h>

gh_actor_model_status gh_actor_part_axis_world_001501d0(float world[16],float radians,
    gh_static_transform_policy policy,uint32_t *frontier)
{
    if(!world||!frontier)return GH_ACTOR_MODEL_ARGUMENT;
    *frontier=0;
    if(policy!=GH_STATIC_BINARY32_REFERENCE||!isfinite(radians)){
        *frontier=0x159fc8;
        return GH_ACTOR_MODEL_UNSUPPORTED;
    }
    fenv_t saved;
    if(feholdexcept(&saved)){
        *frontier=0x159fc8;
        return GH_ACTOR_MODEL_UNSUPPORTED;
    }
    gh_actor_model_status status=GH_ACTOR_MODEL_UNSUPPORTED;
    if(!fesetround(FE_TONEAREST)){
        volatile float half=radians*0.5f;
        float quaternion[4]={0},unused,rotation[16],result[16];
        /* Actual 003BD8B0 supplies (0,1,0,1); quaternion uses its XYZ only. */
        if(gh_sdk_sine_003a19f8(half,&quaternion[1],policy,frontier))goto finish;
        if(gh_id_sincos_reference(half,&unused,&quaternion[3])){
            *frontier=0x1f7fb8;
            goto finish;
        }
        if(gh_actor_quaternion_matrix_001478a8(quaternion,rotation,policy,frontier)||
           gh_actor_matrix_product_00147548(rotation,world,result,policy,frontier))goto finish;
        memcpy(result+12,world+12,12);
        if(fesetenv(&saved)){
            *frontier=0x1501d0;
            return status;
        }
        memcpy(world,result,sizeof(result));
        return GH_ACTOR_MODEL_OK;
    }
finish:
    if(fesetenv(&saved)||!*frontier)*frontier=0x159fc8;
    return status;
}

static float scalar(const uint8_t *bytes)
{
    float value;
    memcpy(&value,bytes,sizeof(value));
    return value;
}

gh_actor_model_status gh_actor_part_world_001501d0(gh_actor_part *part,const uint8_t *parent,
    uint32_t root_flags,gh_static_transform_policy policy,uint32_t *frontier)
{
    if(!part||!part->bytes||!parent||!frontier)return GH_ACTOR_MODEL_ARGUMENT;
    *frontier=0;
    uint8_t *bytes=part->bytes;
    float matrix[16],parent_world[16],inherited[4]={0,0,0,1};
    uint32_t flags;
    memcpy(&flags,bytes+0x154,4);
    memcpy(parent_world,parent+0x80,64);
    int scaled=(root_flags&0x40004u)==0x40000u;
    if(!scaled){
        float local[16];
        memcpy(local,bytes,64);
        if(gh_actor_matrix_product_00147548(parent_world,local,matrix,policy,frontier))
            return GH_ACTOR_MODEL_UNSUPPORTED;
    }else{
        if(!part->alias_d0){*frontier=0x147b18;return GH_ACTOR_MODEL_ARGUMENT;}
        float translate[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
        float inverse[16]={0},scale[16]={0},angles[4]={0,0,0,1},wrapped[4],rotation[16];
        float first[16],second[16];
        memcpy(translate+12,part->alias_d0,12);
        inverse[15]=scale[15]=1;
        fenv_t env;
        if(feholdexcept(&env)){*frontier=0x1501d0;return GH_ACTOR_MODEL_UNSUPPORTED;}
        int valid=!fesetround(FE_TONEAREST);
        for(unsigned i=0;i<3&&valid;i++){
            float parent_scale=scalar(parent+0x120+i*4),local_scale=scalar(bytes+0x110+i*4);
            if(!isfinite(parent_scale)||parent_scale==0||!isfinite(local_scale)){valid=0;break;}
            volatile float angle=scalar(bytes+0x100+i*4)+scalar(bytes+0x130+i*4);
            volatile float ratio=1/parent_scale;
            volatile float value=parent_scale*local_scale;
            angles[i]=angle;inverse[i*5]=ratio;scale[i*5]=inherited[i]=value;
        }
        if(fesetenv(&env)||!valid){*frontier=0x1501d0;return GH_ACTOR_MODEL_UNSUPPORTED;}
        if(gh_actor_euler_adjust_001f7da0(angles,wrapped,policy,frontier)||
           gh_actor_matrix_rotation_00147728(wrapped,0,rotation,policy,frontier)||
           gh_actor_matrix_product_00147548(parent_world,translate,first,policy,frontier)||
           gh_actor_matrix_product_00147548(first,inverse,second,policy,frontier)||
           gh_actor_matrix_product_00147548(second,rotation,first,policy,frontier)||
           gh_actor_matrix_product_00147548(first,scale,matrix,policy,frontier))
            return GH_ACTOR_MODEL_UNSUPPORTED;
    }
    if((flags&0x800u)&&gh_actor_part_axis_world_001501d0(matrix,scalar(bytes+0x150),policy,frontier))
        return GH_ACTOR_MODEL_UNSUPPORTED;
    if(scaled)memcpy(bytes+0x120,inherited,sizeof(inherited));
    memcpy(bytes+0x80,matrix,sizeof(matrix));
    return GH_ACTOR_MODEL_OK;
}
