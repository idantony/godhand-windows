#include "game_om041c.h"
#include "game_actor_axis_rotation.h"
#include "game_actor_matrix.h"
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
    const uint8_t *coefficients=elf_span(b->elf,b->elf_bytes,0x3c2390,8);
    if(!part_bases||!points||!springs||!coefficients)return fail(s,0x2ad2b8,GH_PLACE_BOUNDS);
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
    if(!isfinite(value(coefficients))||!isfinite(value(coefficients+4)))
        return fail(s,0x2ad2b8,GH_PLACE_BOUNDS);
    s->part_bases=part_bases;
    s->point_setup=points;
    s->spring_setup=springs;
    memcpy(s->spring_coefficients,coefficients,8);
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
