#include "game_om355.h"
#include "game_om_base.h"

#include <string.h>

static uint32_t word(const uint8_t *p){uint32_t v;memcpy(&v,p,4);return v;}
static uint16_t half(const uint8_t *p){uint16_t v;memcpy(&v,p,2);return v;}
static void put(uint8_t *p,uint32_t v){memcpy(p,&v,4);}

gh_scene_placement_status gh_om355_construct_001a8348(void *data,size_t size,gh_title_player_view *out)
{
    if(!data||!out)return GH_PLACE_ARGUMENT;
    if(size<GH_OM355_BYTES)return GH_PLACE_BOUNDS;
    const gh_scene_placement_status status=gh_om_base_construct_001b6e90(data,size,out);
    if(status)return status;
    uint8_t *root=data;
    put(root+0x214,0x4277b8);
    put(root+0x600,0);
    out->original_vtable214=0x4277b8;
    return GH_PLACE_OK;
}

/* The six original face records are copied into the original dynamic box
 * constructor. Read the table from the same validated boot ELF as the room. */
static const uint8_t *face_attributes(const void *image,size_t size)
{
    const uint8_t *elf=image;
    if(!elf||size<52||memcmp(elf,"\177ELF\1\1\1",7)||half(elf+18)!=8)return NULL;
    uint32_t ph=word(elf+28);
    unsigned stride=half(elf+42),count=half(elf+44);
    if(stride!=32||count>128||ph>size||(size_t)count*stride>size-ph)return NULL;
    const uint32_t address=0x427758;
    const uint8_t *result=NULL;
    for(unsigned i=0;i<count;i++){
        const uint8_t *segment=elf+ph+(size_t)i*stride;
        uint32_t base=word(segment+8),offset=word(segment+4),extent=word(segment+16);
        if(word(segment)!=1||address<base||(uint64_t)address+96>(uint64_t)base+extent)continue;
        if(result||offset>size||extent>size-offset)return NULL;
        result=elf+offset+(address-base);
    }
    return result;
}

static gh_scene_placement_status fail(gh_om_init_state *s,uint32_t pc,gh_scene_placement_status status)
{
    s->frontier=pc;
    return status;
}

gh_scene_placement_status gh_om355_initialize_001a83e0(gh_om_init_state *s,gh_om_object *object,
    const gh_om_initializer_bindings *b,int *out)
{
    if(!s||!object||object->source.type!=0x355||!b||!out)return GH_PLACE_ARGUMENT;
    *out=0;
    int initialized=0;
    gh_scene_placement_status status=gh_om_common_initialize_001b6fb8(s,object,b,&initialized);
    if(status||!initialized)return fail(s,s->frontier?s->frontier:0x1b6fb8,
        status?status:GH_PLACE_UNSUPPORTED);
    if(s->stage==3){
        if(!b->dynamic)return fail(s,0x12ead8,GH_PLACE_UNSUPPORTED);
        const uint8_t *attributes=face_attributes(b->elf,b->elf_bytes);
        if(!attributes)return fail(s,0x427758,GH_PLACE_BOUNDS);
        gh_om_door_geometry *geometry=&s->geometry_state;
        if(!geometry->box){
            const float offset[3]={0,0.1f,0},rotation[3]={0},size[3]={1.5f,0.2f,1.5f};
            int created=0;
            const gh_scene_dynamic_status result=gh_scene_dynamic_box_create_0012ead8(b->dynamic,
                &object->view,object->view.bytes+0x80,offset,rotation,size,attributes,&geometry->box,&created);
            if(result)return fail(s,b->dynamic->frontier?b->dynamic->frontier:0x12ead8,
                result==GH_DYNAMIC_MEMORY?GH_PLACE_MEMORY:GH_PLACE_UNSUPPORTED);
            geometry->owner=b->dynamic;
            geometry->object=object;
        }
        if(geometry->box&&gh_scene_dynamic_box_bind_slot(b->dynamic,geometry->box,&geometry->box))
            return fail(s,0x12ead8,GH_PLACE_BUSY);
        /* The original actor+600 handle is the same native room box.
         * Its release uses the existing typed slot and room owner. */
        geometry->complete=1;
        s->stage=4;
    }
    *out=1;
    return GH_PLACE_OK;
}
