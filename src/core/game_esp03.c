#include "game_esp03.h"
#include "id_layout_rotation.h"
#include "game_actor_matrix.h"
#include "game_camera.h"
#include "game_scene_at_geometry.h"
#include "../assets/static_mesh_vu_numeric.h"
#include <math.h>
#include <string.h>

static uint32_t word(const uint8_t *p)
{
    uint32_t v;
    memcpy(&v,p,sizeof(v));
    return v;
}

static void put(uint8_t *p,uint32_t v)
{
    memcpy(p,&v,sizeof(v));
}

static int angle_pair(float angle,float *sine,float *cosine)
{
    return !gh_id_angle_reference_001f7d48(angle,&angle) &&
           !gh_id_sincos_reference(angle,sine,cosine);
}

static void fill_ring(gh_esp03_point *output,float radius)
{
    const float diagonal=radius/1.414f;
    const gh_esp03_point points[9]={
        {-radius,0,0,1},{-diagonal,diagonal,0,1},{0,radius,0,1},
        {diagonal,diagonal,0,1},{radius,0,0,1},{diagonal,-diagonal,0,1},
        {0,-radius,0,1},{-diagonal,-diagonal,0,1},{-radius,0,0,1}
    };
    memcpy(output,points,sizeof(points));
}

/* 2E3FCC: adjacent latitude rings are offset by half a longitude sample. */
static gh_effect_status fill_shells(gh_esp03_point *output)
{
    static const float radii[GH_ESP03_SHELL_COUNT]={0.115f,0.10f,0.075f};
    const float pi=3.1415927f,tau=6.2831855f;
    for(unsigned shell=0;shell<GH_ESP03_SHELL_COUNT;++shell){
        const float radius=radii[shell];
        for(int row=-2;row<4;++row){
            float latitude_sine[2],latitude_cosine[2];
            if(!angle_pair((float)row*pi/6.0f,latitude_sine,latitude_cosine) ||
               !angle_pair((float)(row-1)*pi/6.0f,latitude_sine+1,latitude_cosine+1))
                return GH_EFFECT_UNSUPPORTED;
            const float stagger=((unsigned)row&1u)?0.5f:0.0f;
            for(unsigned col=0;col<GH_ESP03_SHELL_COLUMNS;++col){
                for(unsigned point=0;point<4;++point){
                    float sine,cosine;
                    const float phase=((float)col+0.5f*(float)point+stagger)/6.0f;
                    if(!angle_pair(phase*tau-pi,&sine,&cosine))return GH_EFFECT_UNSUPPORTED;
                    const unsigned ring=point&1u;
                    (*output)[0]=sine*(latitude_cosine[ring]*radius);
                    (*output)[1]=latitude_sine[ring]*radius;
                    (*output)[2]=cosine*(latitude_cosine[ring]*radius);
                    (*output)[3]=1.0f;
                    ++output;
                }
            }
        }
    }
    return GH_EFFECT_OK;
}

gh_effect_status gh_esp03_destroy_002e3500(gh_esp03_active_state *s)
{
    if(!s)return GH_EFFECT_ARGUMENT;
    if(!s->pools)return GH_EFFECT_OK;
    gh_room_arena_allocation *const owned[]={
        s->packet,s->packet+1,&s->arm_points,&s->world_shells,&s->local_shells,&s->local_rings
    };
    for(unsigned i=0;i<sizeof(owned)/sizeof(owned[0]);++i){
        if(!owned[i]->pointer)continue;
        if(gh_room_arena_free_002a9680(s->pools,*owned[i]))return GH_EFFECT_ARGUMENT;
        *owned[i]=(gh_room_arena_allocation){0};
    }
    s->pools=NULL;
    return GH_EFFECT_OK;
}

gh_effect_status gh_esp03_initialize_002e35c0(
    gh_esp03_active_state *s,gh_room_pools *pools,uint32_t work594,uint8_t *esp,
    const uint8_t *est,uint32_t *frontier)
{
    if(!s||!pools||!esp||!est||!frontier)return GH_EFFECT_ARGUMENT;
    *frontier=0x2e35c0;
    if(s->pools||word(esp+0xf0)!=GH_ESP03_VTABLE||word(esp+0x2d4)!=0x3f800000u||
       est[0x18e]!=0||est[0x18d]!=0)return GH_EFFECT_UNSUPPORTED;
    gh_esp03_active_state pending={.pools=pools};
    const uint32_t sizes[]={0x5df0,0x5df0,0x240,0x270,0x1b00,0x2880};
    gh_room_arena_allocation *const owned[]={
        pending.packet,pending.packet+1,&pending.arm_points,&pending.local_rings,
        &pending.local_shells,&pending.world_shells
    };
    for(unsigned i=0;i<sizeof(owned)/sizeof(owned[0]);++i){
        if(gh_room_arena_allocate_002a9538(pools,0x754240,work594,sizes[i],16,owned[i])){
            *frontier=0x30ee08;
            const gh_effect_status retired=gh_esp03_destroy_002e3500(&pending);
            if(retired)*s=pending;
            return retired?retired:GH_EFFECT_BOUNDS;
        }
    }
    gh_esp03_point *rings=pending.local_rings.pointer;
    fill_ring(rings,0.10f);
    fill_ring(rings+9,0.10f);
    fill_ring(rings+18,0.075f);
    fill_ring(rings+27,0.075f);
    for(unsigned i=36;i<39;++i){
        memset(rings[i],0,3*sizeof(float));
        rings[i][3]=1.0f;
    }
    gh_effect_status status=fill_shells(pending.local_shells.pointer);
    if(status){
        *frontier=0x1f7d48;
        const gh_effect_status retired=gh_esp03_destroy_002e3500(&pending);
        if(retired)*s=pending;
        return retired?retired:status;
    }
    put(esp+0x2f0,0x3d99999au);
    put(esp+0x2f4,0x3d99999au);
    put(esp+0x2f8,GH_ESP03_ARM_POINTS);
    put(esp+0x2fc,0x60);
    put(esp+0x300,16);
    put(esp+0x304,8);
    put(esp+0x308,GH_ESP03_ARM_COUNT);
    put(esp+0x310,GH_ESP03_SHELL_LOCAL_POINTS/GH_ESP03_SHELL_COUNT);
    put(esp+0x314,GH_ESP03_SHELL_WORLD_POINTS);
    put(esp+0x318,0x48);
    put(esp+0x31c,GH_ESP03_SHELL_CELLS);
    put(esp+0x320,GH_ESP03_SHELL_COUNT);
    put(esp+0x328,est[0x18d]!=0);
    put(esp+0x32c,est[0x18e]);
    *s=pending;
    *frontier=0;
    return GH_EFFECT_OK;
}

/* 2E4280 stretches each local ring to the two live endpoint joints. */
static gh_effect_status transform_arm(
    gh_esp03_point *dst,const gh_esp03_point *ring,const float origin[3],
    const float end[3],gh_static_transform_policy policy,uint32_t *frontier)
{
    const float x=end[0]-origin[0],y=end[1]-origin[1],z=end[2]-origin[2];
    const float axial=sqrtf(x*x+z*z);
    const float length=sqrtf(x*x+y*y+z*z);
    if(!isfinite(axial)||!isfinite(length))return GH_EFFECT_UNSUPPORTED;
    float angles[3]={0},matrix[16];
    static const float unit[3]={1,1,1};
    if(gh_game_angle_001f8180(y,axial,&angles[0],GH_CAMERA_NATIVE_BINARY32_REFERENCE,frontier)||
       gh_game_angle_001f8180(x,z,&angles[1],GH_CAMERA_NATIVE_BINARY32_REFERENCE,frontier))
        return GH_EFFECT_UNSUPPORTED;
    angles[0]=-angles[0];
    if(gh_actor_matrix_compose_00147a48(origin,angles,unit,0,matrix,policy,frontier))
        return GH_EFFECT_UNSUPPORTED;
    for(unsigned i=0;i<GH_ESP03_ARM_POINTS;++i){
        float local[3]={ring[i][0],ring[i][1],i<GH_ESP03_ARM_POINTS/2?0.0f:length};
        if(gh_scene_at_apply_world_001508d8(matrix,local,dst[i])){
            *frontier=0x30a2b0;
            return GH_EFFECT_UNSUPPORTED;
        }
    }
    return GH_EFFECT_OK;
}

gh_effect_status gh_esp03_move_002e4280(gh_esp03_active_state *s,
    const gh_actor_model *model,gh_static_transform_policy policy,uint32_t *frontier)
{
    if(!s||!model||!frontier)return GH_EFFECT_ARGUMENT;
    *frontier=0x2e4280;
    if(!s->pools||!s->local_rings.pointer||!s->arm_points.pointer||
       !s->local_shells.pointer||!s->world_shells.pointer)return GH_EFFECT_ARGUMENT;
    /* CORE0/21 selects original right forearm 7/8 and last joint 10. */
    const gh_actor_part *parts[3];
    if(gh_actor_model_part_at(model,7,&parts[0])||gh_actor_model_part_at(model,8,&parts[1])||
       gh_actor_model_part_at(model,10,&parts[2]))return GH_EFFECT_UNSUPPORTED;
    gh_esp03_point *rings=s->local_rings.pointer;
    gh_esp03_point *centers=rings+2*GH_ESP03_ARM_POINTS;
    memcpy(centers[0],parts[0]->bytes+0xb0,3*sizeof(float));
    memcpy(centers[1],parts[1]->bytes+0xb0,3*sizeof(float));
    float hand_basis[16];
    memcpy(hand_basis,parts[2]->bytes+0x80,sizeof(hand_basis));
    static const float hand_offset[3]={-0.05f,0,0};
    if(gh_scene_at_apply_world_001508d8(hand_basis,hand_offset,centers[2])){
        *frontier=0x30a2b0;
        return GH_EFFECT_UNSUPPORTED;
    }
    gh_esp03_point *arms=s->arm_points.pointer;
    for(unsigned arm=0;arm<GH_ESP03_ARM_COUNT;++arm){
        const unsigned first=arm*GH_ESP03_ARM_POINTS;
        gh_effect_status status=transform_arm(arms+first,rings+first,centers[arm],
            centers[arm+1],policy,frontier);
        if(status)return status;
    }
    static const unsigned triangle_order[6]={0,1,2,1,2,3};
    const gh_esp03_point *local=s->local_shells.pointer;
    gh_esp03_point *world=s->world_shells.pointer;
    for(unsigned shell=0;shell<GH_ESP03_SHELL_COUNT;++shell){
        for(unsigned cell=0;cell<GH_ESP03_SHELL_CELLS;++cell){
            for(unsigned corner=0;corner<6;++corner){
                for(unsigned axis=0;axis<3;++axis)
                    (*world)[axis]=local[triangle_order[corner]][axis]+centers[shell][axis];
                (*world)[3]=1.0f;
                ++world;
            }
            local+=4;
        }
    }
    s->moved=1;
    *frontier=0;
    return GH_EFFECT_OK;
}

static float number(const uint8_t *p)
{
    float v;
    memcpy(&v,p,sizeof(v));
    return v;
}

static int project_point(const float camera[16],const gh_display_state *display,
    const float position[3],float s,float t,uint32_t rgba,gh_effect_trans_vertex *out)
{
    float clip[4],reciprocal;
    int32_t gs[3];
    if(gh_scene_at_apply_world_001508d8(camera,position,clip)||
       !gh_static_vu_reciprocal(clip[3],&reciprocal))return 0;
    for(unsigned axis=0;axis<3;++axis){
        const volatile float value=clip[axis]*reciprocal;
        if(!gh_static_vu_ftoi4(value,gs+axis))return 0;
    }
    const int32_t x=gs[0]>>4,y=gs[1]>>4;
    if(gs[2]<0||x<0x400-display->screen_width/2||x>0xc00+display->screen_width/2||
       y<display->gs_y_origin-0x400||y>display->gs_y_origin+display->screen_lines+0x400)
        return 0;
    *out=(gh_effect_trans_vertex){
        .x12_4=gs[0],.y12_4=gs[1],.z=(uint32_t)gs[2]>>4,.rgba=rgba,.q=1,.s=s,.t=t
    };
    return 1;
}

gh_effect_status gh_esp03_draw_002e4e70(const gh_esp03_active_state *s,
    const gh_effects_trans_bindings *b,unsigned slot,const gh_effect_esp_view *view,
    gh_effect_trans_packet *out,uint32_t *frontier)
{
    if(!s||!b||!view||!out||!frontier)return GH_EFFECT_ARGUMENT;
    *frontier=0x2e4e70;
    *out=(gh_effect_trans_packet){0};
    const uint8_t *p=view->payload;
    if(!view->actor||!view->actor->bytes||view->actor->size<0x2c0||
       view->type!=3||!view->initialized||word(p+0xf0)!=GH_ESP03_VTABLE)
        return GH_EFFECT_ARGUMENT;
    if(word(view->actor->bytes+0x250)&2u)return GH_EFFECT_OK;
    const gh_display_state *display=b->display;
    if(!b->camera||!b->init||!b->camera->constructed||b->camera->identity!=b->init->active_camera||
       !display||!display->initialized||!b->work||!b->globals||
       b->policy!=GH_STATIC_BINARY32_REFERENCE||b->cvt_policy!=GH_FRAME_CHOP_REFERENCE)
        return GH_EFFECT_UNSUPPORTED;
    if(!s->pools||!s->moved||!s->packet[0].pointer||!s->packet[1].pointer||
       p[0x101]||p[0x244]||p[0x245]!=1||p[0x246]!=1||p[0x247]!=1||
       number(p+0x254)!=0.0f||word(p+0x32c))
        return GH_EFFECT_UNSUPPORTED;

    float camera[16],reference[4];
    const gh_esp03_point *centers=(const gh_esp03_point *)s->local_rings.pointer+
        GH_ESP03_ARM_COUNT*GH_ESP03_ARM_POINTS;
    memcpy(camera,b->camera->fields,sizeof(camera));
    if(gh_scene_at_apply_world_001508d8(camera,centers[1],reference))
        return GH_EFFECT_UNSUPPORTED;
    if(!isfinite(reference[3]))return GH_EFFECT_UNSUPPORTED;
    const unsigned depth=reference[3]<=1?1u:reference[3]>=1020?1020u:(unsigned)reference[3];

    const gh_stage_material *material=NULL;
    gh_effect_status status=gh_effect_sprite_material_00151398(b->textures,view->texture,1,
        &material,frontier);
    if(status)return status;
    uint32_t rgba=0;
    float fade=p[0x24c]?(float)((int)p[0x24c]-(int)p[0x24d])/(float)p[0x24c]:1.0f;
    for(unsigned channel=0;channel<4;++channel){
        float component=128.0f*number(p+0x188+channel*sizeof(float));
        if(!isfinite(component))return GH_EFFECT_UNSUPPORTED;
        if(component<0)component=0;
        if(component>255)component=255;
        if(channel==3&&!(p[0x11c]&0x80u)){
            const uint8_t *actor=view->actor->bytes;
            fade=number(actor+0x24c)*((float)actor[0x2b6]/255.0f)*
                ((float)actor[0x2b7]/255.0f)*((float)actor[0x2bc]/255.0f);
        }
        if(channel==3)component*=fade;
        if(!isfinite(component)||component<0||component>255)return GH_EFFECT_UNSUPPORTED;
        rgba|=(uint32_t)component<<(channel*8);
    }
    const uint32_t flags=word(p+0x11c);
    const float left=flags&0x10000000u?1.0f:0.0f,right=1.0f-left;
    const float top=flags&0x08000000u?1.0f:0.0f,bottom=1.0f-top;
    const float *q=b->globals->sprite_uv_q_61b7a8;
    const float u[]={left*q[0],left*q[2],right*q[1],left*q[2],right*q[1],right*q[3]};
    const float t[]={top*q[0],bottom*q[2],top*q[1],bottom*q[2],top*q[1],bottom*q[3]};
    const unsigned parity=word((const uint8_t *)b->work+0x210)&1u;
    gh_effect_trans_vertex *vertices=s->packet[parity?0:1].pointer;
    unsigned count=0;
    const gh_esp03_point *arms=s->arm_points.pointer;
    const gh_esp03_point *shells=s->world_shells.pointer;
    const unsigned arm_cells=GH_ESP03_ARM_COUNT*(GH_ESP03_ARM_POINTS/2-1);
    for(unsigned cell=0;cell<arm_cells+GH_ESP03_SHELL_COUNT*GH_ESP03_SHELL_CELLS;++cell){
        const float *points[6];
        if(cell<arm_cells){
            const unsigned ring_edges=GH_ESP03_ARM_POINTS/2-1;
            const unsigned base=(cell/ring_edges)*GH_ESP03_ARM_POINTS+cell%ring_edges;
            const unsigned end=base+GH_ESP03_ARM_POINTS/2;
            points[0]=arms[base];points[1]=arms[end];points[2]=arms[base+1];
            points[3]=arms[end];points[4]=arms[base+1];points[5]=arms[end+1];
        }else{
            const unsigned first=(cell-arm_cells)*6;
            for(unsigned corner=0;corner<6;++corner)points[corner]=shells[first+corner];
        }
        int visible=1;
        for(unsigned corner=0;corner<6;++corner){
            if(!project_point(camera,display,points[corner],u[corner],t[corner],rgba,
                vertices+count+corner))visible=0;
        }
        if(visible)count+=6;
    }
    /* The whole original GIF triangle list remains one ordering-table group. */
    if(count){
        *out=(gh_effect_trans_packet){
            .order={.bank=2,.depth=depth,.identity=((uint64_t)view->serial<<32)|((uint64_t)slot<<8)},
            .original_callee=0x2e4e70,.slot=slot,.serial=view->serial,.kind=GH_EFFECT_PASS_SPRITE,
            .source=GH_EFFECT_SURFACE_MATERIAL,.destination=GH_EFFECT_SURFACE_SCENE_COLOR,
            .prim=0x5b,.alpha=UINT64_C(0x8000000048),.test=flags&0x1000u?0x3000c:0x5000c,
            .zbuf=UINT64_C(0x0a0000f8)|((flags&0x80000u)?0:UINT64_C(1)<<32),
            .tex0=material->source_tex0,.writes_tex0=1,.inherited_sampler=1,.writes_clamp=1,
            .restores_scene_target=1,.restore_zbuf=0x0a0000f8,.material=material,
            .triangles=vertices,.triangle_vertices=count
        };
    }
    *frontier=0;
    return GH_EFFECT_OK;
}
