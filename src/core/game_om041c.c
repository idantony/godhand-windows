#include "game_om041c.h"
#include "game_actor_axis_rotation.h"
#include "game_actor_matrix.h"
#include "game_actor_two_bone.h"
#include "game_om_base.h"
#include "id_layout_rotation.h"

#include <fenv.h>
#include <math.h>
#include <string.h>

enum {POINT_COUNT=15,SPRING_COUNT=30,POINT_BYTES=0x50,SPRING_BYTES=0x18};

static uint32_t word(const uint8_t *p){uint32_t v;memcpy(&v,p,4);return v;}
static uint16_t half(const uint8_t *p){uint16_t v;memcpy(&v,p,2);return v;}
static float value(const uint8_t *p){float v;memcpy(&v,p,4);return v;}
static void put(uint8_t *p,uint32_t v){memcpy(p,&v,4);}
static void write_float(uint8_t *p,float v){memcpy(p,&v,4);}
static float add(float a,float b){volatile float v=a+b;return v;}
static float mul(float a,float b){volatile float v=a*b;return v;}
static float divide(float a,float b){volatile float v=a/b;return v;}
static void vf0(uint8_t *p){memset(p,0,12);put(p+12,0x3f800000);}
static uint8_t *point(uint8_t *root,unsigned i){return root+0x600+i*POINT_BYTES;}
static uint8_t *spring(uint8_t *root,unsigned i){return root+0xab0+i*SPRING_BYTES;}

static gh_scene_placement_status fail(gh_om041c_state *s,uint32_t pc,gh_scene_placement_status status)
{
    s->frontier=pc;
    return status;
}

static const uint8_t *elf_span(const void *image,size_t size,uint32_t address,size_t length)
{
    const uint8_t *elf=image;
    if(!elf||size<52||memcmp(elf,"\177ELF\1\1\1",7))return NULL;
    uint32_t ph=word(elf+28);
    unsigned stride=half(elf+42),count=half(elf+44);
    if(stride!=32||count>128||ph>size||(size_t)count*stride>size-ph)return NULL;
    const uint8_t *result=NULL;
    for(unsigned i=0;i<count;i++){
        const uint8_t *p=elf+ph+(size_t)i*stride;
        uint32_t base=word(p+8),offset=word(p+4),extent=word(p+16);
        if(word(p)!=1||address<base||(uint64_t)address+length>(uint64_t)base+extent)continue;
        if(result||offset>size||extent>size-offset)return NULL;
        result=elf+offset+(address-base);
    }
    return result;
}

gh_scene_placement_status gh_om041c_construct_0017b870(void *data,size_t size,gh_title_player_view *out)
{
    if(!data||!out)return GH_PLACE_ARGUMENT;
    if(size<GH_OM041C_BYTES)return GH_PLACE_BOUNDS;
    gh_scene_placement_status status=gh_om_base_construct_001b6e90(data,size,out);
    if(status)return status;
    uint8_t *root=data,*cloth=root+0xd80;
    put(root+0x214,0x421bb8);
    out->original_vtable214=0x421bb8;
    for(unsigned i=0;i<POINT_COUNT;i++)for(unsigned j=0;j<4;j++)vf0(point(root,i)+16*j);
    /* Original 2AD238. The two guest pointer fields are represented by
     * this object's embedded point and spring records, never native pointers. */
    put(cloth,2);
    put(cloth+4,4);
    put(cloth+8,SPRING_COUNT);
    memset(cloth+12,0,12);
    for(unsigned i=0;i<128;i++)vf0(cloth+0x30+i*0x20);
    return GH_PLACE_OK;
}

static gh_scene_placement_status bind_tables(gh_om041c_state *s,const gh_om_initializer_bindings *b)
{
    const uint8_t *part_bases=elf_span(b->elf,b->elf_bytes,0x421ad0,2);
    const uint8_t *points=elf_span(b->elf,b->elf_bytes,0x421ae0,4*POINT_COUNT);
    const uint8_t *springs=elf_span(b->elf,b->elf_bytes,0x421b20,5*SPRING_COUNT);
    const uint8_t *coefficients=elf_span(b->elf,b->elf_bytes,0x3c238c,24);
    const uint8_t *reset=elf_span(b->elf,b->elf_bytes,0x3bda50,1);
    if(!part_bases||!points||!springs||!coefficients||!reset)return fail(s,0x2ad2b8,GH_PLACE_BOUNDS);
    if(word(coefficients)!=3||*reset)return fail(s,0x2ad590,GH_PLACE_UNSUPPORTED);
    uint32_t seen=0;
    for(unsigned i=0;i<POINT_COUNT;i++){
        const uint8_t *p=points+4*i;
        if(p[0]>2||p[1]>4||!p[2]||p[3]>1)return fail(s,0x2ad2b8,GH_PLACE_BOUNDS);
        unsigned index=3*p[1]+p[0];
        if(seen&(1u<<index))return fail(s,0x2ad2b8,GH_PLACE_BOUNDS);
        seen|=1u<<index;
    }
    for(unsigned i=0;i<SPRING_COUNT;i++){
        const uint8_t *p=springs+5*i;
        if(p[0]>2||p[1]>4||p[2]>2||p[3]>4||p[4]>1)
            return fail(s,0x2ad2b8,GH_PLACE_BOUNDS);
    }
    for(unsigned i=1;i<6;i++)if(!isfinite(value(coefficients+4*i)))
        return fail(s,0x2ad2b8,GH_PLACE_BOUNDS);
    if(value(coefficients+16)<0||value(coefficients+20)<=0)return fail(s,0x2ad748,GH_PLACE_BOUNDS);
    s->part_bases=part_bases;
    s->point_setup=points;
    s->spring_setup=springs;
    memcpy(s->spring_coefficients,coefficients+4,8);
    s->gravity=value(coefficients+12);
    s->drag=value(coefficients+16);
    s->force_cap=value(coefficients+20);
    return GH_PLACE_OK;
}

/* 2AD2B8 resets point forces and initializes the original connected pairs.
 * Pair endpoints remain indices into the same embedded points. */
static void reset_cloth(gh_om041c_state *s,uint8_t *root)
{
    const float total_per_weight=divide(5.0f,62.0f);
    for(unsigned i=0;i<POINT_COUNT;i++){
        const uint8_t *p=s->point_setup+4*i;
        uint8_t *out=point(root,3*p[1]+p[0]);
        float mass=mul((float)p[2],total_per_weight);
        write_float(out+0x40,mass);
        write_float(out+0x44,divide(1.0f,mass));
        out[0x48]=p[3];
        for(unsigned j=1;j<=3;j++)memset(out+16*j,0,12);
    }
    for(unsigned i=0;i<SPRING_COUNT;i++){
        const uint8_t *p=s->spring_setup+5*i;
        uint8_t *out=spring(root,i);
        const uint8_t *a=point(root,3*p[1]+p[0]),*b=point(root,3*p[3]+p[2]);
        float diff[3];
        for(unsigned j=0;j<3;j++)diff[j]=add(value(a+4*j),-value(b+4*j));
        float square=add(add(mul(diff[0],diff[0]),mul(diff[1],diff[1])),mul(diff[2],diff[2]));
        memcpy(out+8,s->spring_coefficients,8);
        write_float(out+0x10,sqrtf(square));
        out[0x14]=p[4];
    }
}

static gh_scene_placement_status pose(gh_om041c_state *s)
{
    gh_actor_model *model=&s->common.model;
    uint8_t *root=model->root.bytes;
    const gh_static_transform_policy policy=GH_STATIC_BINARY32_REFERENCE;
    uint32_t pc=0,flags=word(root+0x250);
    if(!model->ready||!model->root.alias_f0||model->external_parts||model->external_owner)
        return fail(s,0x17c028,GH_PLACE_BOUNDS);
    for(unsigned column=0;column<2;column++)
        if((unsigned)s->part_bases[column]+4>=model->part_count)return fail(s,0x17c028,GH_PLACE_BOUNDS);
    fenv_t saved;
    if(fegetenv(&saved))return fail(s,0x17c028,GH_PLACE_UNSUPPORTED);
    if(fesetround(FE_TONEAREST))return fail(s,0x17c028,GH_PLACE_UNSUPPORTED);
    if(!(flags&0x800u)){
        float position[3],angles[3],scale[3],world[16];
        memcpy(position,model->root.alias_f0,12);
        memcpy(angles,root+0x100,12);
        memcpy(scale,root+0x110,12);
        if(gh_actor_matrix_compose_00147a48(position,angles,scale,root[0x140],world,policy,&pc)){
            if(!pc)pc=0x14a2b0;
            goto finish;
        }
        memcpy(root+0x80,world,64);
        memcpy(root+0x120,root+0x110,12);
    }
    for(unsigned row=0;row<5;row++)for(unsigned column=0;column<2;column++){
        gh_actor_part *part=&model->parts[s->part_bases[column]+row];
        float angle[4]={0,0,0,1},wrapped[4],position[3],scale[3],local[16];
        uint8_t *bytes=part->bytes;
        if(!bytes||!part->alias_d0){pc=0x150138;goto finish;}
        for(unsigned i=0;i<3;i++)angle[i]=add(value(bytes+0x100+4*i),value(bytes+0x130+4*i));
        memcpy(position,part->alias_d0,12);
        memcpy(scale,bytes+0x110,12);
        if(gh_actor_euler_adjust_001f7da0(angle,wrapped,policy,&pc)||
           gh_actor_matrix_compose_00147a48(position,wrapped,scale,0,local,policy,&pc)){
            if(!pc)pc=0x150138;
            goto finish;
        }
        memcpy(bytes,local,64);
        const uint8_t *parent=part->parent_is_model?root:
            part->parent>=0&&(unsigned)part->parent<model->part_count?model->parts[part->parent].bytes:NULL;
        if(!parent||gh_actor_part_world_001501d0(part,parent,flags,policy,&pc)){
            if(!pc)pc=0x1501d0;
            goto finish;
        }
        memcpy(point(root,3*row+2*column),bytes+0xb0,12);
    }
    for(unsigned row=0;row<5;row++){
        uint8_t *left=point(root,3*row),*middle=point(root,3*row+1),*right=point(root,3*row+2);
        for(unsigned i=0;i<3;i++){
            float l=value(left+4*i),r=value(right+4*i);
            write_float(middle+4*i,add(l,mul(add(r,-l),0.5f)));
        }
    }
    reset_cloth(s,root);
finish:
    if(fesetenv(&saved)&&!pc)pc=0x17c028;
    return pc?fail(s,pc,GH_PLACE_UNSUPPORTED):GH_PLACE_OK;
}

gh_scene_placement_status gh_om041c_initialize_0017b940(gh_om041c_state *s,gh_om_object *object,
    const gh_om_initializer_bindings *b,int *out)
{
    if(!s||!object||object->source.type!=0x41c||!b||!out)return GH_PLACE_ARGUMENT;
    *out=0;
    s->frontier=0;
    int initialized=0;
    gh_scene_placement_status status=gh_om_common_initialize_001b6fb8(&s->common,object,b,&initialized);
    if(status||!initialized)return fail(s,s->common.frontier?s->common.frontier:0x1b6fb8,
        status?status:GH_PLACE_UNSUPPORTED);
    if(!s->initialized){
        status=bind_tables(s,b);
        if(status)return status;
        status=pose(s);
        if(status)return status;
        for(unsigned column=0;column<2;column++)for(unsigned row=0;row<4;row++){
            uint8_t *part=s->common.model.parts[s->part_bases[column]+row].bytes;
            put(part+0x154,word(part+0x154)|0x18u);
        }
        s->initialized=1;
    }
    *out=1;
    return GH_PLACE_OK;
}

gh_scene_placement_status gh_om041c_set_position_0017c3a0(gh_om041c_state *s,const void *position)
{
    if(!s||!s->initialized||!position)return GH_PLACE_ARGUMENT;
    s->frontier=0;
    uint8_t *root=s->common.model.root.bytes;
    memmove(root+0x490,position,12);
    memmove(s->common.model.root.alias_f0,root+0x490,12);
    return pose(s);
}

gh_scene_placement_status gh_om041c_set_euler_0017c408(gh_om041c_state *s,const void *angle)
{
    if(!s||!s->initialized||!angle)return GH_PLACE_ARGUMENT;
    s->frontier=0;
    memmove(s->common.model.root.bytes+0x100,angle,12);
    return pose(s);
}

gh_scene_placement_status gh_om041c_abort(gh_om041c_state *s)
{
    if(!s)return GH_PLACE_ARGUMENT;
    gh_scene_placement_status status=gh_om_initialize_abort(&s->common);
    if(!status)memset(s,0,sizeof(*s));
    return status;
}

gh_scene_placement_status gh_om041c_add_wind_002ae218(gh_om041c_state *s,const float center[3],
    const float direction[3],float radius,float strength)
{
    if(!s||!s->initialized||!center||!direction)return GH_PLACE_ARGUMENT;
    s->frontier=0;
    if(!isfinite(radius)||radius<=0||!isfinite(strength))
        return fail(s,0x2ae218,GH_PLACE_UNSUPPORTED);
    for(unsigned i=0;i<3;i++)if(!isfinite(center[i])||!isfinite(direction[i]))
        return fail(s,0x2ae218,GH_PLACE_UNSUPPORTED);
    fenv_t saved;
    if(fegetenv(&saved))return fail(s,0x2ae218,GH_PLACE_UNSUPPORTED);
    uint8_t *root=s->common.model.root.bytes;
    float updated[POINT_COUNT][3];
    uint32_t mask=0,pc=0;
    if(fesetround(FE_TOWARDZERO)){pc=0x2ae218;goto finish;}
    const float radius_squared=mul(radius,radius);
    if(fesetround(FE_TONEAREST)){pc=0x2ae218;goto finish;}
    for(unsigned i=0;i<POINT_COUNT;i++){
        const uint8_t *p=point(root,i);
        if(p[0x48])continue;
        float diff[3];
        for(unsigned j=0;j<3;j++)diff[j]=add(value(p+4*j),-center[j]);
        const float d2=add(add(mul(diff[0],diff[0]),mul(diff[1],diff[1])),mul(diff[2],diff[2]));
        if(!isfinite(d2)){pc=0x1f7e28;break;}
        if(d2>=radius_squared)continue;
        if(fesetround(FE_TOWARDZERO)){pc=0x2ae218;break;}
        const float ratio=divide(sqrtf(d2),radius);
        if(fesetround(FE_TONEAREST)){pc=0x2ae218;break;}
        float attenuation;
        if(gh_id_cos_reference_001f7fb8(ratio,&attenuation)){pc=0x1f7fb8;break;}
        for(unsigned j=0;j<3;j++)
            updated[i][j]=add(value(p+0x10+4*j),mul(mul(direction[j],strength),attenuation));
        mask|=1u<<i;
    }
finish:
    if(fesetenv(&saved)&&!pc)pc=0x2ae218;
    if(pc)return fail(s,pc,GH_PLACE_UNSUPPORTED);
    for(unsigned i=0;i<POINT_COUNT;i++)
        if(mask&(1u<<i))memcpy(point(root,i)+0x10,updated[i],12);
    return GH_PLACE_OK;
}

static float dot3(const float a[3],const float b[3])
{
    return add(add(mul(a[0],b[0]),mul(a[1],b[1])),mul(a[2],b[2]));
}

/* Actual 2AD748 force order is gravity, velocity drag, the 30 ordered
 * springs, then one force limit per unpinned point. */
static uint32_t solve_forces(const gh_om041c_state *s,const uint8_t *root,
    uint8_t points[POINT_COUNT][POINT_BYTES])
{
    for(unsigned i=0;i<POINT_COUNT;i++){
        uint8_t *p=points[i];
        memset(p+0x30,0,12);
        if(p[0x48])continue;
        float velocity[3];
        memcpy(velocity,p+0x10,12);
        const float speed_squared=dot3(velocity,velocity),speed=sqrtf(speed_squared);
        const float mass=value(p+0x40);
        if(!isfinite(speed)||!isfinite(mass)||mass<=0)return 0x2ad748;
        float force[3]={0,mul(s->gravity,mass),0};
        if(speed>0){
            const float reciprocal=divide(1,speed);
            for(unsigned axis=0;axis<3;axis++){
                const float unit=mul(velocity[axis],reciprocal);
                force[axis]=add(force[axis],-mul(mul(unit,speed_squared),s->drag));
            }
        }
        memcpy(p+0x30,force,12);
    }
    for(unsigned i=0;i<SPRING_COUNT;i++){
        const uint8_t *cfg=s->spring_setup+5*i;
        const uint8_t *link=spring((uint8_t *)root,i);
        uint8_t *a=points[3*cfg[1]+cfg[0]],*b=points[3*cfg[3]+cfg[2]];
        float separation[3],relative_speed[3];
        for(unsigned axis=0;axis<3;axis++){
            separation[axis]=add(value(a+4*axis),-value(b+4*axis));
            relative_speed[axis]=add(value(a+0x10+4*axis),-value(b+0x10+4*axis));
        }
        const float distance=sqrtf(dot3(separation,separation));
        if(!isfinite(distance)||distance<=0)return 0x1f7df8;
        const float elastic=mul(add(distance,-value(link+0x10)),value(link+8));
        const float damped=mul(divide(dot3(relative_speed,separation),distance),value(link+0xc));
        const float amount=-add(elastic,damped),inverse=divide(1,distance);
        for(unsigned axis=0;axis<3;axis++){
            const float f=mul(mul(separation[axis],inverse),amount);
            if(!isfinite(f))return 0x2ad748;
            if(!a[0x48])write_float(a+0x30+4*axis,add(value(a+0x30+4*axis),f));
            if(!b[0x48])write_float(b+0x30+4*axis,add(value(b+0x30+4*axis),-f));
        }
    }
    const float cap_squared=mul(s->force_cap,s->force_cap);
    for(unsigned i=0;i<POINT_COUNT;i++){
        uint8_t *p=points[i];
        if(p[0x48])continue;
        float force[3];
        memcpy(force,p+0x30,12);
        float squared=dot3(force,force);
        if(!isfinite(squared))return 0x2ad748;
        if(squared>cap_squared){
            float ratio=divide(s->force_cap,sqrtf(squared));
            for(unsigned axis=0;axis<3;axis++)write_float(p+0x30+4*axis,mul(force[axis],ratio));
        }
    }
    return 0;
}

static uint32_t integrate_cloth(gh_om041c_state *s,uint8_t *root)
{
    uint8_t points[POINT_COUNT][POINT_BYTES];
    memcpy(points,point(root,0),sizeof(points));
    const float dt=divide(0x1.111112p-5f,3.0f); /* original f12 bits 3D088889 */
    for(unsigned step=0;step<3;step++){
        uint32_t pc=solve_forces(s,root,points);
        if(pc)return pc;
        for(unsigned i=0;i<POINT_COUNT;i++){
            uint8_t *p=points[i];
            if(p[0x48])continue;
            const float inverse=value(p+0x44);
            if(!isfinite(inverse)||inverse<=0)return 0x2ad590;
            for(unsigned axis=0;axis<3;axis++){
                const float accel=mul(value(p+0x30+4*axis),inverse);
                const float velocity=add(value(p+0x10+4*axis),mul(accel,dt));
                const float position=add(value(p+4*axis),mul(velocity,dt));
                if(!isfinite(position)||!isfinite(velocity))return 0x2ad590;
                write_float(p+0x20+4*axis,accel);
                write_float(p+0x10+4*axis,velocity);
                write_float(p+4*axis,position);
            }
        }
    }
    memcpy(point(root,0),points,sizeof(points));
    return 0;
}

static uint32_t root_world(gh_actor_model *model,gh_static_transform_policy policy)
{
    uint8_t *root=model->root.bytes;
    if(word(root+0x250)&0x800u)return 0;
    float position[3],angles[3],scale[3],matrix[16];
    memcpy(position,model->root.alias_f0,12);
    memcpy(angles,root+0x100,12);
    memcpy(scale,root+0x110,12);
    uint32_t pc=0;
    if(gh_actor_matrix_compose_00147a48(position,angles,scale,root[0x140],matrix,policy,&pc))
        return pc?pc:0x14a2b0;
    memcpy(root+0x80,matrix,64);
    memcpy(root+0x120,root+0x110,12);
    return 0;
}

static uint32_t seed_anchors(gh_om041c_state *s,gh_static_transform_policy policy)
{
    gh_actor_model *model=&s->common.model;
    uint8_t *root=model->root.bytes;
    uint32_t pc=root_world(model,policy);
    if(pc)return pc;
    for(unsigned column=0;column<2;column++){
        unsigned index=s->part_bases[column];
        gh_actor_part *part=model->parts+index;
        if(gh_actor_part_rebuild_00150138(model,index,policy,&pc))return pc?pc:0x150138;
        if(gh_actor_part_world_001501d0(part,root,word(root+0x250),policy,&pc))
            return pc?pc:0x1501d0;
        memcpy(point(root,2*column),part->bytes+0xb0,12);
    }
    return 0;
}

static uint32_t orient_bone(const gh_motion_ik_globals *globals,const float parent[16],
    const float delta[3],float quaternion[4],gh_static_transform_policy policy)
{
    float q[4],swing[4];
    uint32_t pc=0;
    if(gh_actor_matrix_quaternion_0015a190(parent,q,globals,policy,&pc))return pc?pc:0x15a190;
    const float distance=sqrtf(dot3(delta,delta));
    if(!isfinite(distance))return 0x101e28;
    float direction[3]={0};
    if(distance>0){
        const float inverse=divide(1,distance);
        for(unsigned i=0;i<3;i++)direction[i]=mul(delta[i],inverse);
    }
    /* 15A878 aligns the authored bone's (0,-1,0) to the cloth segment. */
    float dot=-direction[1];
    if(dot < -1.0f)dot=-1.0f;
    if(dot > 1.0f)dot=1.0f;
    const float twice_cos=sqrtf(add(add(add(dot,1),dot),1));
    if(!isfinite(twice_cos)||twice_cos<=0)return 0x15a878;
    const float inv_cos=divide(1,twice_cos);
    swing[0]=mul(-direction[2],inv_cos);
    swing[1]=0;
    swing[2]=mul(direction[0],inv_cos);
    swing[3]=mul(twice_cos,0.5f);

    const float norm=add(dot3(q,q),mul(q[3],q[3]));
    if(!isfinite(norm)||norm<=0)return 0x15a190;
    const float inverse=divide(1,norm);
    for(unsigned i=0;i<3;i++)q[i]=mul(-q[i],inverse);
    q[3]=mul(q[3],inverse);
    for(unsigned i=0;i<3;i++){
        const unsigned j=(i+1)%3,k=(i+2)%3;
        const float cross=add(mul(q[j],swing[k]),-mul(q[k],swing[j]));
        quaternion[i]=add(add(cross,mul(q[i],swing[3])),mul(q[3],swing[i]));
    }
    quaternion[3]=add(add(add(mul(q[3],swing[3]),-mul(q[0],swing[0])),
        -mul(q[1],swing[1])),-mul(q[2],swing[2]));
    return 0;
}

static uint32_t solve_bones(gh_om041c_state *s,const gh_motion_ik_globals *globals,
    gh_static_transform_policy policy)
{
    gh_actor_model *model=&s->common.model;
    uint8_t *root=model->root.bytes;
    uint32_t flags=word(root+0x250);
    for(unsigned column=0;column<2;column++)for(unsigned row=0;row<5;row++){
        const unsigned index=s->part_bases[column]+row;
        gh_actor_part *part=model->parts+index;
        const uint8_t *parent=row?model->parts[index-1].bytes:root;
        float local[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
        memcpy(local+12,part->alias_d0,12);
        uint32_t pc=0;
        if(row<4){
            float delta[3],parent_world[16],quaternion[4],rotation[16],adjusted[16];
            const uint8_t *a=point(root,3*row+2*column),*b=point(root,3*(row+1)+2*column);
            for(unsigned axis=0;axis<3;axis++)delta[axis]=add(value(b+4*axis),-value(a+4*axis));
            memcpy(parent_world,parent+0x80,64);
            pc=orient_bone(globals,parent_world,delta,quaternion,policy);
            if(pc)return pc;
            if(gh_actor_quaternion_matrix_001478a8(quaternion,rotation,policy,&pc)||
               gh_actor_matrix_product_00147548(local,rotation,adjusted,policy,&pc))return pc?pc:0x147bb0;
            memcpy(local,adjusted,64);
        }
        /* Each authored column is an independent chain. The original tip's
         * temporary rotation is overwritten by 17BFAC's translation-only
         * pass before any later consumer, and no bone inherits the tip. */
        memcpy(part->bytes,local,64);
        if(gh_actor_part_world_001501d0(part,parent,flags,policy,&pc))return pc?pc:0x1501d0;
    }
    return 0;
}

gh_scene_placement_status gh_om041c_move_0017baf0(gh_om041c_state *s,
    const gh_motion_ik_globals *globals,uint8_t *phase)
{
    if(!s||!s->initialized||!phase||*phase>2)return GH_PLACE_ARGUMENT;
    s->frontier=0;
    if(*phase==2)return GH_PLACE_OK;
    if(!globals||!globals->initialized)return fail(s,0x15a190,GH_PLACE_UNSUPPORTED);
    gh_actor_model *model=&s->common.model;
    const gh_static_transform_policy policy=GH_STATIC_BINARY32_REFERENCE;
    if(model->part_count!=10||s->part_bases[0]!=0||s->part_bases[1]!=5)
        return fail(s,0x17baf0,GH_PLACE_UNSUPPORTED);
    for(unsigned i=0;i<10;i++){
        const gh_actor_part *part=model->parts+i;
        if(!part->bytes||!part->alias_d0||(i%5?part->parent_is_model||part->parent!=(int)i-1:!part->parent_is_model))
            return fail(s,0x17baf0,GH_PLACE_UNSUPPORTED);
    }
    uint32_t pc=0;
    if(!*phase){
        pc=seed_anchors(s,policy);
        if(!pc)pc=integrate_cloth(s,model->root.bytes);
        if(pc)return fail(s,pc,GH_PLACE_UNSUPPORTED);
        *phase=1;
    }
    pc=solve_bones(s,globals,policy);
    if(pc)return fail(s,pc,GH_PLACE_UNSUPPORTED);
    *phase=2;
    return GH_PLACE_OK;
}
