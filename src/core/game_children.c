/* Native readable recovery of1612C8/15FD98 and their9vtable-derivedmethods.
 * Exactmapping and unchangedbytes:game-children-evidence.json. */
#include "game_subscreen_save.h"
#include "id_jobs.h"
#include <stdlib.h>
#include <string.h>
_Static_assert(offsetof(gh_boot_work,system_flags)==0x58c,"747A2C Work58C");
struct gh_game_children {uint8_t bytes[0xf18];const void *room_jump;
 gh_game_child_view pairs[9];gh_id_base bases[18];uint32_t count;
 gh_r207_allocation save_map_asset;
 gh_id_entry *save_map_entries[28];
 uint32_t save_map_job;
 uint16_t save_map_message,save_map_room,save_map_target;
 uint8_t save_map_bound,save_map_inited,save_map_active;
};
static void w16(uint8_t *p,size_t o,uint16_t v){p[o]=(uint8_t)v;p[o+1]=(uint8_t)(v>>8);}
static void w32(uint8_t *p,size_t o,uint32_t v){for(unsigned i=0;i<4;i++)p[o+i]=(uint8_t)(v>>(i*8));}
static void vec0(uint8_t *p){w32(p,0,0);w32(p,4,0);w32(p,8,0);w32(p,12,0x3f800000);}
static void construct_base(uint8_t *p){
 /*2A9DC0 storesVF0 vectors;2A9DF8 replaces them with actual3BD880 vec(0,0,0,1). */
 memset(p,0,16);p[0x18]=p[0x19]=0;w16(p,0x1a,0);p[0x1c]=p[0x1d]=p[0x1e]=0;
 vec0(p+0x20);vec0(p+0x30);w16(p,0x44,0x80);p[0x46]=6;w32(p,0x40,0x3c23d70a);
}
static const uint16_t base_offsets[]={0x60,0x160,0x1e0,0x290,0x310,0x480,0x5a0,0x620,0x730,0x880,0x960,0x9c0,0xa80,0xae0,0xba0,0xc40,0xd10,0xe50};
static const uint16_t offsets[]={0x60,0x310,0x620,0x730,0x880,0x9c0,0xba0,0xc40,0xae0};
static const uint8_t kinds[]={0,1,2,3,4,5,8,7,6};
static const uint32_t vtables[]={0x41f6c8,0x41ff30,0x41f388,0x41f5e0,0x41fb20,0x41fbe8,0x41fc60,0x41fdf8,0x41f200};
static const uint32_t methods[]={0x164ef8,0x16c238,0x161bb0,0x1627a0,0x166618,0x1673b0,0x167f70,0x168528,0x161688};
static const uint16_t sizes[]={0x2b0,0x310,0x110,0x150,0x140,0x120,0xa0,0x290,0xc0};
gh_child_status gh_game_children_create(const void *room_jump,gh_game_children **out){
 gh_game_children *s;if(!out)return GH_CHILD_ARGUMENT;*out=0;s=calloc(1,sizeof(*s));if(!s)return GH_CHILD_MEMORY;
 for(unsigned i=0;i<18;i++){construct_base(s->bytes+base_offsets[i]);gh_id_base_clear_002a9df8(&s->bases[i]);}
 vec0(s->bytes+0x580);vec0(s->bytes+0x830);vec0(s->bytes+0x840);vec0(s->bytes+0xe30);
 s->room_jump=room_jump; /*2BE968 original pointer; never truncatehostaddress*/
 for(unsigned i=0;i<9;i++)w32(s->bytes,offsets[i]+0x70,vtables[i]);
 *out=s;return GH_CHILD_OK;
}
const void *gh_game_children_room_jump(const gh_game_children *s){return s?s->room_jump:0;}
void gh_game_children_destroy(gh_game_children *s){free(s);}
static void initialize_child(uint8_t *p,unsigned kind){
 /*1614D0 commonbody; its repeated derivedbyteclears have no extra effects. */
 p[0x5d]=0;w32(p,0x60,0xffffffffu);p[0x58]=p[0x59]=0;w32(p,0x50,0);p[0x64]=0;
 p[0x5a]=p[0x5b]=p[0x5c]=0;
 if(kind==0)w32(p,0x298,0);else if(kind==1)w32(p,0x264,0);else if(kind==8)p[0x90]=0;
}
gh_child_status gh_game_children_initialize_0015fd98(gh_game_children *s){
 if(!s)return GH_CHILD_ARGUMENT;
 memset(s->bytes,0,0x48);memset(s->pairs,0,sizeof(s->pairs));s->count=0;
 for(unsigned i=0;i<9;i++){
  gh_game_child_view *v=&s->pairs[s->count++];
  v->bytes=s->bytes+offsets[i];v->size=sizes[i];v->original_offset=offsets[i];v->kind=kinds[i];
  v->vtable_address=vtables[i];v->init_entry=methods[i];
  for(unsigned j=0;j<18;j++)if(base_offsets[j]==offsets[i])v->native_base=&s->bases[j];
 }
 for(unsigned i=0;i<9;i++){initialize_child(s->bytes+s->pairs[i].original_offset,s->pairs[i].kind);s->pairs[i].initialized=1;}
 w16(s->bytes,0x50,0);w32(s->bytes,0x48,0xffffffffu);w32(s->bytes,0xed0,0);s->bytes[0xed4]=0;
 return gh_game_children_room_reset_0015fee0(s);
}
gh_child_status gh_game_children_room_reset_0015fee0(gh_game_children*s){
 if(!s)return GH_CHILD_ARGUMENT;for(unsigned i=0;i<9;i++)w32(s->bytes,0xf08-i*4,0xffffffffu);s->bytes[0xf15]=0;return GH_CHILD_OK;
}
gh_child_status gh_game_child_at(const gh_game_children *s,uint32_t n,gh_game_child_view *out){
 if(!s || !out)return GH_CHILD_ARGUMENT;if(n>=s->count)return GH_CHILD_BOUNDS;*out=s->pairs[n];return GH_CHILD_OK;
}
gh_child_status gh_game_children_manager_view(const gh_game_children *s,const uint8_t **data,size_t *size){
 if(!s || !data || !size)return GH_CHILD_ARGUMENT;*data=s->bytes;*size=sizeof(s->bytes);return GH_CHILD_OK;
}

gh_child_status gh_game_children_id_base(gh_game_children *s,uint32_t offset,gh_id_base **out){
 if(!s || !out)return GH_CHILD_ARGUMENT;*out=0;
 for(unsigned i=0;i<18;i++)if(base_offsets[i]==offset){*out=&s->bases[i];return GH_CHILD_OK;}
 return GH_CHILD_BOUNDS;
}

static uint32_t r32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint16_t r16(const uint8_t*p){return (uint16_t)(p[0]|(unsigned)p[1]<<8);}
static gh_child_status child_stop(uint32_t*f,uint32_t pc,gh_child_status e){if(f)*f=pc;return e;}
gh_child_status gh_game_children_active_00161288(const gh_game_children*s,const gh_game_child_view**out){
 if(!s||!out)return GH_CHILD_ARGUMENT;*out=NULL;if(s->count!=9)return GH_CHILD_BOUNDS;
 uint32_t kind=r32(s->bytes+0x48);if(kind==UINT32_MAX)return GH_CHILD_OK;
 for(unsigned i=0;i<s->count;i++)if(s->pairs[i].kind==kind){*out=&s->pairs[i];break;}return GH_CHILD_OK;
}
gh_child_status gh_game_children_alpha_step_00160c48(gh_game_children*s,uint32_t*f){
 if(f)*f=0;if(!s)return GH_CHILD_ARGUMENT;uint8_t*p=s->bytes;unsigned phase=p[0xed4];
 if(phase==0||phase==1)return GH_CHILD_OK;
 uint32_t count=r32(p+0xed8);
 if(phase==2||phase==3){
  if(count>4)return child_stop(f,0x160c48,GH_CHILD_UNSUPPORTED);
  uint32_t alpha=phase==2?count*32:128-count*32;
  w32(p,0xed0,(r32(p+0xed0)&0x00ffffffu)|(alpha<<24));
  if(count==4)p[0xed4]=(uint8_t)(phase==2?0:1);
 }
 w32(p,0xed8,count+1);return GH_CHILD_OK;
}
gh_child_status gh_game_children_move_001601d8(gh_game_children*s,gh_boot_work*work,uint32_t*f){
 const gh_game_child_view*active=NULL;if(f)*f=0;if(!s||!work)return GH_CHILD_ARGUMENT;
 uint8_t*raw=(uint8_t*)work,*p=s->bytes;
 /* ActualLW747A2C = Work+58C, distinctfromflags584/systemflags5D8. */
 if(work->system_flags&0x80000000u)return GH_CHILD_OK;
 gh_child_status e=gh_game_children_active_00161288(s,&active);if(e)return e;
 if(active&&active->bytes[0x58])return GH_CHILD_OK;
 uint16_t state=r16(p+0x50);
 switch(state){
 case 0:break;
 case 2:if(active)return child_stop(f,0x1601d8,GH_CHILD_UNSUPPORTED);w16(p,0x50,3);break;
 case 3:w16(p,0x50,4);break;
 case 5:
  if(!(r32(raw+0x5dc)&0x100000u)&&active)return child_stop(f,0x1601d8,GH_CHILD_UNSUPPORTED);
  break;
 case 7:
  if(r32(p+0x4c)==0xb)work->flags|=0x8000;
  if(active)return child_stop(f,0x1601d8,GH_CHILD_UNSUPPORTED);
  w32(p,0x48,r32(p+0x4c));break;
 case 9:
  if(active)return child_stop(f,0x1601d8,GH_CHILD_UNSUPPORTED);
  w32(raw,0x5e0,r32(p+0xee4));w32(p,0x48,UINT32_MAX);w16(p,0x50,0x15);break;
 default:
  if(state<0x16)return child_stop(f,0x1601d8,GH_CHILD_UNSUPPORTED);
  break; /* actualout-of-range state also visits160C48 */
 }
 e=gh_game_children_alpha_step_00160c48(s,f);if(e)return e;
 if(work->flags&0x20000u)return child_stop(f,0x2c7850,GH_CHILD_UNSUPPORTED);
 return GH_CHILD_OK;
}

gh_child_status gh_game_children_cover_00160d50(const gh_game_children*s,gh_render_draw_env*env,gh_child_cover_submit submit,void*context,uint32_t*f){
 if(f)*f=0;if(!s||!env)return GH_CHILD_ARGUMENT;
 if(!env->ready)return child_stop(f,0x2c54e8,GH_CHILD_UNSUPPORTED);
 if(!submit)return child_stop(f,0x15b880,GH_CHILD_UNSUPPORTED);
 const gh_child_cover_quad quad={{{-1,-1,0,1},{513,-1,0,1},{-1,449,0,1},{513,449,0,1}},r32(s->bytes+0xed0),1,6,5,1,UINT64_C(0x8000000044),UINT64_C(0x3001d)};
 int emitted=-1;uint32_t pc=0;gh_child_status e=submit(context,&quad,&emitted,&pc);
 if(e)return child_stop(f,pc?pc:0x15b880,e);
 if(emitted!=0&&emitted!=1)return GH_CHILD_ARGUMENT;
 if(gh_render_draw_init_002c54e8(env)!=GH_RENDER_STEP_OK)return child_stop(f,0x2c54e8,GH_CHILD_UNSUPPORTED);
 env->scalar.bank=7;env->scalar.depth=1;return GH_CHILD_OK;
}
static gh_child_status save_map_trans_001640e8(const gh_game_children *s,
    const gh_game_child_view *child,gh_save_map_trans_submit submit,void *context,
    uint32_t *frontier)
{
    const uint8_t variant=child->bytes[0x64];
    if(variant==1&&child->bytes[0x5b]==1)
        return child_stop(frontier,0x163c58,GH_CHILD_UNSUPPORTED);
    /* 164978 receives 00567D70 and immediately returns. Door map variants
     * 2 and 3 proceed directly to the IDBase Trans at 2AAC28. */
    if(!s->save_map_active||!s->save_map_inited||(variant!=2&&variant!=3)||
       !child->native_base||!child->native_base->initialized)
        return child_stop(frontier,0x1640e8,GH_CHILD_UNSUPPORTED);
    uint32_t original_frontier=0;
    gh_child_status status=submit(context,child->native_base,s->save_map_message,&original_frontier);
    return status?child_stop(frontier,original_frontier?original_frontier:0x2aac28,status):GH_CHILD_OK;
}

gh_child_status gh_game_children_draw_with_save_map_00160b90(const gh_game_children *s,
    const gh_boot_work *w,gh_render_draw_env *env,gh_child_cover_submit submit,
    gh_save_map_trans_submit map_trans,void *context,uint32_t *f)
{
 if(f)*f=0;if(!s||!w)return GH_CHILD_ARGUMENT;
 if((w->system_flags&0x80000000u)||(r32((const uint8_t*)w+0x5dc)&0x100000u))return GH_CHILD_OK;
 gh_child_status e=gh_game_children_cover_00160d50(s,env,submit,context,f);if(e)return e;
 const gh_game_child_view*active=NULL;e=gh_game_children_active_00161288(s,&active);if(e)return e;
 if(!active||active->bytes[0x59])return GH_CHILD_OK;
 uint16_t state=r16(s->bytes+0x50);
 if(state==4||state==5||state==6||state==8||state==10||state==14||state==17){
  if(r32(active->bytes+0x70)!=active->vtable_address)return child_stop(f,0x160c20,GH_CHILD_UNSUPPORTED);
  static const uint32_t targets[]={0x165e88,0x16dc18,0x162278,0x1640e8,0x166b60,0x1678b8,0x1682a8,0x16acd0,0x161970};
  for(unsigned i=0;i<s->count;i++)if(active==&s->pairs[i]){
   if(i==3&&map_trans)return save_map_trans_001640e8(s,active,map_trans,context,f);
   return child_stop(f,targets[i],GH_CHILD_UNSUPPORTED);
  }
  return child_stop(f,0x160c20,GH_CHILD_UNSUPPORTED);
 }
 return GH_CHILD_OK;
}

gh_child_status gh_game_children_draw_00160b90(const gh_game_children *s,
    const gh_boot_work *w,gh_render_draw_env *env,gh_child_cover_submit submit,
    void *context,uint32_t *f)
{
    return gh_game_children_draw_with_save_map_00160b90(s,w,env,submit,NULL,context,f);
}

gh_child_status gh_game_children_quit_001612a8(gh_game_children*s,gh_boot_work*w,uint32_t*f){
 if(f)*f=0;if(!s||!w)return GH_CHILD_ARGUMENT;
 if(r32((const uint8_t*)w+0x5e4)&0x8000000)return child_stop(f,0x15ff10,GH_CHILD_UNSUPPORTED);
 return GH_CHILD_OK;
}

static gh_id_base *save_map_base(gh_game_children *s)
{
    return s->pairs[3].native_base;
}

gh_child_status gh_game_children_save_map_open_00160020(gh_game_children *s,
    const gh_save_map_bindings *b,int *started,uint32_t *frontier)
{
    if(frontier)*frontier=0;
    if(!s||!b||!b->work||!started||s->count!=9)return GH_CHILD_ARGUMENT;
    *started=0;
    uint8_t *w=(uint8_t*)b->work,*p=s->bytes;
    const uint32_t flags=r32(w+0x5e4);
    if((flags&UINT32_C(0x30200040))||(b->work->flags&8)||r16(p+0x50))
        return GH_CHILD_OK;
    if(!b->player||!b->player->ready||!b->player->view.bytes||b->player->view.size<0x550)
        return child_stop(frontier,0x2db6b0,GH_CHILD_UNSUPPORTED);
    s->save_map_room=b->map_room;s->save_map_target=b->target_room;
    p[0xedc]=0x11;
    w32(p,0xee0,r32(w+0x5d8));
    w32(p,0xee4,r32(w+0x5e0));
    p[0xf15]=1;
    w32(p,0xee8,3);
    w32(p,0x48,3);
    p[0x794]=2;
    w16(p,0x50,1);
    w32(b->player->view.bytes,0x54c,0x40a00000);
    w32(w,0x5e4,flags|0x08000000);
    w32(w,0x5e0,r32(w+0x5e0)|0x02000000);
    w32(w,0x5d8,r32(w+0x5d8)|0xf8040000u);
    s->save_map_active=1;
    *started=1;
    return GH_CHILD_OK;
}

static gh_child_status save_map_resource(gh_game_children *s,const gh_save_map_bindings *b,
    int *ready,uint32_t *frontier)
{
    *ready=0;
    if(!b->dvd||!b->dvd->disc||!b->subscreen_heap||!b->manager||!b->id_runtime||!b->font||
       !b->packed||!b->language||b->work->language!=1)
        return child_stop(frontier,0x161500,GH_CHILD_UNSUPPORTED);
    const char *path="id/local/US/idsubscrmap_US.idd";
    if(!s->save_map_job){
        if(b->manager->pair[7].bound)return child_stop(frontier,0x2accf8,GH_CHILD_UNSUPPORTED);
        s->save_map_job=gh_dvd_read_alloc(b->dvd,path,&s->save_map_asset,b->subscreen_heap,0,0,0,0);
        if(!s->save_map_job)return child_stop(frontier,0x200cb0,GH_CHILD_MEMORY);
        return GH_CHILD_OK;
    }
    if(gh_dvd_check(b->dvd,s->save_map_job))return GH_CHILD_OK;
    size_t index;
    gh_disc_entry file;
    if((b->dvd->failed_id==s->save_map_job&&b->dvd->last_error==GH_DVD_IO)||
       gh_disc_find(b->dvd->disc,path,&index)!=GH_DISC_OK||gh_disc_entry_at(b->dvd->disc,index,&file)!=GH_DISC_OK||
       !s->save_map_asset.pointer||file.size>SIZE_MAX)
        return child_stop(frontier,0x201160,GH_CHILD_UNSUPPORTED);
    gh_id_base *base=save_map_base(s);
    if(!s->save_map_bound){
        const gh_bundle_entry data={.bytes=s->save_map_asset.pointer,.size=(size_t)file.size};
        if(gh_id_set_data_002accf8(b->manager,7,&data)!=GH_MENU_OK)
            return child_stop(frontier,0x2accf8,GH_CHILD_UNSUPPORTED);
        s->save_map_bound=1;
    }
    int original;
    if(!base->initialized){
        if(gh_id_base_initialize_002a9ea0(b->id_runtime,base,7,0,NULL,NULL,&original)!=GH_ID_OK||original!=1)
            return child_stop(frontier,0x2a9ea0,GH_CHILD_UNSUPPORTED);
        if(gh_id_base_restart_anim(base,0,0)!=GH_ID_OK)
            return child_stop(frontier,0x2abbe8,GH_CHILD_UNSUPPORTED);
    }
    if(gh_packed_message_bind(base,b->manager,b->font,b->packed,b->language,1,7,1,&original)!=GH_ID_OK||original!=1)
        return child_stop(frontier,0x2a9f30,GH_CHILD_UNSUPPORTED);
    for(unsigned i=0;i<28;++i)
        if(gh_id_base_get_work_002aa410(b->id_runtime,base,(int)i,&s->save_map_entries[i],NULL)!=GH_ID_OK)
            return child_stop(frontier,0x2aa410,GH_CHILD_UNSUPPORTED);
    *ready=1;
    return GH_CHILD_OK;
}

static void save_map_show(gh_id_entry *entry,int show)
{
    if(show)entry->flags&=~UINT32_C(0x08000000);
    else entry->flags|=UINT32_C(0x08000000);
}

static gh_child_status save_map_init(gh_game_children *s,const gh_save_map_bindings *b,
    int *ready,uint32_t *frontier)
{
    uint8_t *p=s->bytes+0x730;
    if(!s->save_map_job){
        const uint8_t *w=(const uint8_t*)b->work;
        w32(p,0x68,r32(w+0x5d8));
        w32(p,0x6c,r32(w+0x5e0));
        w16(p,0x134,0);
        w16(p,0x136,0);
    }
    gh_child_status status=save_map_resource(s,b,ready,frontier);
    if(status||!*ready)return status;
    if((p[0x64]!=2&&p[0x64]!=3)||(b->work->flags&0x40)||!b->save.bytes||b->save.size<0xa8)
        return child_stop(frontier,0x1627d8,GH_CHILD_UNSUPPORTED);
    gh_id_entry **entries=s->save_map_entries;
    const uint32_t unlock=r32(b->save.bytes+0xa4);
    const int debug=(r32((const uint8_t*)b->work+0x59c)&0x80000)!=0;
    static const uint8_t marker[]={0,1,2,3,4,5,7,8,0};
    for(unsigned chapter=1;chapter<=8;++chapter){
        const int unlocked=debug||(unlock&(1u<<chapter));
        save_map_show(entries[chapter+8],unlocked);
        if(unlocked&&marker[chapter])save_map_show(entries[marker[chapter]],1);
        if(unlocked&&chapter==5)save_map_show(entries[6],1);
    }
    static const float positions[9][2]={{254,289},{102,266},{167,195},{134,92},
        {235,126},{320,190},{320,88},{408,88},{455,47}};
    const unsigned chapter=s->save_map_room>>8;
    unsigned message=chapter?chapter*16+0x2013+(s->save_map_room&255):0x20a3;
    if(message>0x20a3)message=0x20a3;
    if(chapter<=8){entries[0]->pos_x=positions[chapter][0];entries[0]->pos_y=positions[chapter][1];}
    memcpy(p+0x128,&entries[0]->pos_x,4);
    memcpy(p+0x12c,&entries[0]->pos_y,4);
    s->save_map_message=(uint16_t)message;
    entries[25]->message=entries[26]->message=0x2022;
    w16(p,0x130,0);
    w32(p,0x60,3);
    s->save_map_inited=1;
    return GH_CHILD_OK;
}

static gh_child_status save_map_update(gh_game_children *s,const gh_save_map_bindings *b,
    uint32_t *frontier)
{
    uint8_t *p=s->bytes+0x730;
    gh_id_entry **e=s->save_map_entries;
    const uint16_t selected=r16(p+0x130);
    if(!s->save_map_inited||selected>=4)return GH_CHILD_ARGUMENT;
    e[24]->seed=r32(b->save.bytes+0x20);
    if(r32((const uint8_t*)b->work+0x594)&0x02000000){
        e[24]->seed=999999;
        w32(b->save.bytes,0x20,e[24]->seed);
    }
    save_map_show(e[25],1);
    save_map_show(e[17],1);
    for(unsigned i=19;i<=22;++i)save_map_show(e[i],i==19u+selected);
    static const float selection_y[]={22,66,110,156};
    e[18]->pos_y=selection_y[selected];
    p[0x132]=(uint8_t)(selected>=2);
    if(selected<2){
        memcpy(&e[0]->pos_x,p+0x128,4);
        memcpy(&e[0]->pos_y,p+0x12c,4);
    }else{
        e[0]->pos_x=255;
        e[0]->pos_y=283;
    }
    gh_id_frontier id_frontier={0};
    if(gh_id_jobs_move_reference_rotation(save_map_base(s),NULL,NULL,&id_frontier)!=GH_ID_OK)
        return child_stop(frontier,id_frontier.address?id_frontier.address:0x2aaa40,GH_CHILD_UNSUPPORTED);
    return GH_CHILD_OK;
}

static gh_child_status save_map_sound(const gh_save_map_bindings *b,uint16_t program,uint32_t *frontier)
{
    if(!b->sound)return child_stop(frontier,0x2cb8a0,GH_CHILD_UNSUPPORTED);
    return b->sound(b->sound_context,program,frontier);
}

static gh_child_status save_map_input(gh_game_children *s,const gh_save_map_bindings *b,uint32_t *frontier)
{
    uint8_t *p=s->bytes+0x730;
    const uint8_t *w=(const uint8_t*)b->work;
    const uint64_t repeat=(uint64_t)r32(w+0x1b0)|((uint64_t)r32(w+0x1b4)<<32);
    int selection=r16(p+0x130);
    gh_child_status status;
    if(repeat&UINT64_C(0x1000001000000)){
        if((status=save_map_sound(b,0x15f,frontier)))return status;
        --selection;
    }
    if(repeat&UINT64_C(0x2000002000000)){
        if((status=save_map_sound(b,0x15f,frontier)))return status;
        ++selection;
    }
    if(selection<0)selection=3;
    else if(selection>3)selection=0;
    w16(p,0x130,(uint16_t)selection);
    if(!(r32(w+0x1a0)&0x10000000u))return GH_CHILD_OK;
    if(selection==1&&b->work->room==0x21)
        return child_stop(frontier,0x1645e0,GH_CHILD_UNSUPPORTED);
    if((status=save_map_sound(b,0x15e,frontier)))return status;
    w32(p,0x60,selection==0?0xc:UINT32_MAX);
    if(selection==0)w16(p,0x134,0xffff);
    else if(selection==1)w16(p,0x134,s->save_map_target);
    else{
        w16(p,0x134,0);
        w16(p,0x136,(uint16_t)(selection==2));
    }
    return GH_CHILD_OK;
}

static gh_child_status save_map_release(gh_game_children *s,const gh_save_map_bindings *b,
    uint32_t *frontier)
{
    uint8_t *w=(uint8_t*)b->work,*p=s->bytes+0x730;
    w32(w,0x5d8,r32(p+0x68));
    w32(w,0x5e0,r32(p+0x6c));
    int original,reclaimed;
    gh_id_base *base=save_map_base(s);
    if(base->entries&&gh_id_base_release_002aa078(b->id_runtime,base,&original,&reclaimed)!=GH_ID_OK)
        return child_stop(frontier,0x2aa078,GH_CHILD_UNSUPPORTED);
    if(s->save_map_bound){
        memset(&b->manager->pair[7],0,sizeof(b->manager->pair[7]));
        s->save_map_bound=0;
    }
    if(s->save_map_asset.pointer&&gh_heap_release(b->subscreen_heap,s->save_map_asset)!=GH_MEM_OK)
        return child_stop(frontier,0x2a94f0,GH_CHILD_MEMORY);
    memset(&s->save_map_asset,0,sizeof(s->save_map_asset));
    memset(s->save_map_entries,0,sizeof(s->save_map_entries));
    memset(b->packed,0,sizeof(*b->packed));
    s->save_map_job=0;
    s->save_map_inited=0;
    w32(p,0x60,UINT32_MAX);
    p[0x64]=0;
    return GH_CHILD_OK;
}

static gh_child_status save_map_fade(const gh_save_map_bindings *b,unsigned frames,int reverse,uint32_t *frontier)
{
    if(gh_screen_fade_start_00307208(b->fade,&b->fade_context,0,reverse?8:10,0,
       reverse?0xff000000:0,reverse?0:0xff000000,(int32_t)frames)!=GH_FADE_OK)
        return child_stop(frontier,0x307208,GH_CHILD_UNSUPPORTED);
    return GH_CHILD_OK;
}

gh_child_status gh_game_children_save_map_move_001601d8(gh_game_children *s,
    const gh_save_map_bindings *b,uint32_t *frontier)
{
    if(frontier)*frontier=0;
    if(!s||!b||!b->work||!b->fade||!b->cursor||s->count!=9)return GH_CHILD_ARGUMENT;
    uint8_t *w=(uint8_t*)b->work,*p=s->bytes;
    const uint16_t state=r16(p+0x50);
    if(!state||b->work->system_flags&0x80000000u)return GH_CHILD_OK;
    if(!s->save_map_active)return child_stop(frontier,0x1601d8,GH_CHILD_UNSUPPORTED);
    if(r32(p+0x48)==3&&p[0x788])return GH_CHILD_OK;
    if((r32(p+0x48)!=3&&state!=12&&state!=13&&state!=21)||
       (p[0xedc]&0x7f)!=0x11||(b->work->flags&0x40))
        return child_stop(frontier,0x1601d8,GH_CHILD_UNSUPPORTED);
    gh_child_status status;
    uint8_t fade=gh_screen_fade_status_00307718(b->fade);
    switch(state){
    case 1:
        for(unsigned i=0;i<s->count;++i){
            uint8_t *child=p+s->pairs[i].original_offset;
            memset(child+0x5a,0,4);
        }
        p[0xed4]=0;
        w16(p,0x50,2);
        break;
    case 2: {
        int ready;
        status=save_map_init(s,b,&ready,frontier);
        if(status||!ready)return status;
        status=save_map_update(s,b,frontier);
        if(status)return status;
        w16(p,0x50,3);
        break;
    }
    case 3:
        w16(p,0x50,4);
        break;
    case 4:
        if(!p[0xed4]){
            if((status=save_map_fade(b,15,1,frontier)))return status;
            p[0xed4]=1;
            fade=gh_screen_fade_status_00307718(b->fade);
        }
        if(!fade){
            w16(p,0x50,5);
            p[0xed4]=0;
            p[0xedc]&=0x7f;
        }
        break;
    case 5:
        if(r32(w+0x5dc)&0x100000)return GH_CHILD_OK;
        if((status=save_map_input(s,b,frontier)))return status;
        if((status=save_map_update(s,b,frontier)))return status;
        if(r32(p+0x790)==0xc){
            w32(p,0x4c,0xc);
            w16(p,0x50,10);
        }else if(r32(p+0x790)==UINT32_MAX)w16(p,0x50,8);
        break;
    case 8:case 10:
        if(!p[0xed4]){
            if((status=save_map_fade(b,15,0,frontier)))return status;
            p[0xed4]=1;
            fade=gh_screen_fade_status_00307718(b->fade);
        }
        if(fade&4){
            p[0xed4]=0;
            if(state==8)w16(p,0x50,9);
            else{
                if((status=save_map_release(s,b,frontier)))return status;
                w32(p,0x48,0xc);
                w16(p,0x50,12);
            }
        }
        break;
    case 9:
        if((status=save_map_release(s,b,frontier)))return status;
        w32(w,0x5e0,r32(p+0xee4));
        w32(p,0x48,UINT32_MAX);
        w16(p,0x50,21);
        break;
    case 12:
        if(!(r32(w+0x594)&0x800)&&!(r32(w+0x598)&0x80000040u)){
            if(!r16(p+0x52)){
                if(gh_scene_cursor_open_save_002bf1a0(b->cursor,b->work,0)!=GH_AUX_OK)
                    return child_stop(frontier,0x2bf1a0,GH_CHILD_UNSUPPORTED);
                w16(p,0x52,1);
            }
            if(r32(w+0x5e8)&0x40000000)return GH_CHILD_OK;
            b->work->flags&=~2u;
            w16(p,0x52,0);
        }
        w16(p,0x50,13);
        break;
    case 13:
        p[0x794]=3;
        w16(p,0x50,2);
        w32(p,0x48,3);
        break;
    case 21:
        w16(p,0x50,0); /* Door's launch arg 0x11 skips this final fade. */
        s->save_map_active=0;
        w32(w,0x5e4,r32(w+0x5e4)&~0x08000000u);
        w32(w,0x5d8,r32(p+0xee0));
        p[0xed4]=0;
        break;
    default:
        return child_stop(frontier,0x1601d8,GH_CHILD_UNSUPPORTED);
    }
    return gh_game_children_alpha_step_00160c48(s,frontier);
}

gh_child_status gh_game_children_save_map_quit_0015ff10(gh_game_children *s,
    const gh_save_map_bindings *b,uint32_t *frontier)
{
    if(frontier)*frontier=0;
    if(!s||!b||!b->work||s->count!=9)return GH_CHILD_ARGUMENT;
    if(!s->save_map_active)return GH_CHILD_OK;
    uint8_t *w=(uint8_t*)b->work,*p=s->bytes;
    const uint32_t mask=r32(w+0x5e0)&0x40000;
    if(s->save_map_job){
        if(!b->dvd||!b->id_runtime||!b->manager||!b->subscreen_heap||!b->packed)
            return GH_CHILD_ARGUMENT;
        if(gh_dvd_check(b->dvd,s->save_map_job)&&!gh_dvd_cancel(b->dvd,s->save_map_job))
            return child_stop(frontier,0x201160,GH_CHILD_UNSUPPORTED);
        gh_child_status status=save_map_release(s,b,frontier);
        if(status)return status;
    }
    w32(w,0x5d8,r32(p+0xee0));
    w32(w,0x5e0,(r32(p+0xee4)&~0x40000u)|mask);
    w32(w,0x5e4,r32(w+0x5e4)&~0x08000000u);
    w32(p,0x48,UINT32_MAX);
    p[0xed4]=0;
    w16(p,0x50,0);
    s->save_map_active=0;
    return gh_game_children_room_reset_0015fee0(s);
}

gh_child_status gh_game_children_save_map_room(const gh_game_children *s,uint16_t *room)
{
    if(!s||!room||!s->save_map_active)return GH_CHILD_ARGUMENT;
    *room=s->save_map_room;
    return GH_CHILD_OK;
}

gh_child_status gh_game_children_save_map_view(const gh_game_children *s,gh_save_map_view *out)
{
    if(!s||!out||s->count!=9)return GH_CHILD_ARGUMENT;
    const uint8_t *child=s->bytes+0x730;
    const uint16_t state=r16(s->bytes+0x50);
    *out=(gh_save_map_view){.base=s->pairs[3].native_base,.chapter_message=s->save_map_message,
        .selected=r16(child+0x130),.target=r16(child+0x134),.exit_kind=r16(child+0x136),
        .variant=child[0x64],.active=s->save_map_active,.visible=(uint8_t)(s->save_map_inited&&(state==4||state==5||state==8||state==10))};
    return GH_CHILD_OK;
}
