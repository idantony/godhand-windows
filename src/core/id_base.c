/* Copyright (c) 2026 Lucas Picoli and god-hand-decomp contributors.
 * MIT notice: LICENSE.upstream. Source pin eaae18d0dfab1f9a0df276ae5ca1581ad326faf3.
 * Actual patched ELF and original MC data evidence: analysis/id-base-semantics.json.
 */
#include "id_base.h"
#include "id_path.h"
#include <stdlib.h>
#include <string.h>
#include <float.h>
_Static_assert(sizeof(float)==4 && FLT_RADIX==2 && FLT_MANT_DIG==24,"binary32 fields");
enum {POOL_COUNT=640,JOB_COUNT=100,SOURCE_SIZE=0x2e0,ENTRY_SIZE=0xac};
struct gh_id_runtime {
    gh_id_state *manager;
    int default_manager;
    uint32_t live_objects;
    gh_id_path_policy path_policy;
    gh_id_entry entries[POOL_COUNT];
    gh_id_job_view jobs[JOB_COUNT];
    gh_id_base *job_owners[JOB_COUNT]; /*native lifetime only, never guestdata*/
    gh_id_entry fallback;
};
static uint32_t u32(const unsigned char *p)
{return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint16_t u16(const unsigned char *p) {return (uint16_t)((uint16_t)p[0]|(uint16_t)p[1]<<8);}
static int16_t s16(const unsigned char *p)
{uint16_t v=u16(p);return (int16_t)(v<0x8000?(int32_t)v:(int32_t)v-0x10000);}
static int8_t s8(unsigned char v) {return (int8_t)(v<128?(int)v:(int)v-256);}
static float f32(const unsigned char *p) {uint32_t v=u32(p);float f;memcpy(&f,&v,4);return f;}
static void put32(unsigned char *p,uint32_t n)
{p[0]=(unsigned char)n;p[1]=(unsigned char)(n>>8);p[2]=(unsigned char)(n>>16);p[3]=(unsigned char)(n>>24);}
static int runtime_valid(const gh_id_runtime *r)
{return r && r->manager && r->manager->pool_used<=POOL_COUNT && r->manager->job_count<=JOB_COUNT;}
gh_id_error gh_id_runtime_set_path_policy(gh_id_runtime *r,gh_id_path_policy policy)
{
    if(!runtime_valid(r) || (policy!=GH_ID_PATH_EXACT && policy!=GH_ID_PATH_REFERENCE_NEAREST))return GH_ID_ARGUMENT;
    if(r->live_objects)return GH_ID_BUSY;
    r->path_policy=policy;return GH_ID_OK;
}

gh_id_error gh_id_lookup_002acd78(const gh_id_state *m,uint32_t slot,uint32_t n,gh_id_view *out)
{
    const gh_bundle_entry *table;uint32_t count,offset,i;size_t end;
    if(!m || !out)return GH_ID_ARGUMENT;
    memset(out,0,sizeof(*out));
    if(slot==0x21 || !n)return GH_ID_NOT_FOUND;
    if(slot>=32)return GH_ID_BOUNDS;
    if(!m->pair[slot].data.bytes)return GH_ID_NOT_FOUND;
    table=&m->pair[slot].data;
    if(table->size<4)return GH_ID_BOUNDS;
    count=u32(table->bytes);
    if(count>(table->size-4)/4)return GH_ID_BOUNDS;
    if(n>count)return GH_ID_NOT_FOUND;
    offset=u32(table->bytes+(size_t)n*4);
    if(!offset)return GH_ID_NOT_FOUND;
    if(offset<4+(size_t)count*4 || offset>=table->size)return GH_ID_BOUNDS;
    end=table->size;
    /* Zero/duplicate offsets are allowed; nearest larger offset bounds the
     * selected payload without assuming a fully monotone sparse table. */
    for(i=1;i<=count;++i) {
        uint32_t next=u32(table->bytes+(size_t)i*4);
        if(next && (next<4+(size_t)count*4 || next>table->size))return GH_ID_BOUNDS;
        if(next>offset && next<end)end=next;
    }
    out->bytes=table->bytes+offset;out->offset=offset;out->size=end-offset;
    return GH_ID_OK;
}
gh_id_error gh_id_runtime_create(gh_id_state *manager,int is_default,gh_id_runtime **out)
{
    gh_id_runtime *r;
    if(!manager || !out)return GH_ID_ARGUMENT;
    *out=0;
    if(!manager->initialized || manager->job_count || manager->pool_used)return GH_ID_BUSY;
    r=calloc(1,sizeof(*r));if(!r)return GH_ID_MEMORY;
    r->manager=manager;r->default_manager=!!is_default;*out=r;return GH_ID_OK;
}
gh_id_error gh_id_runtime_destroy(gh_id_runtime *r)
{
    if(!r)return GH_ID_ARGUMENT;
    if(r->live_objects)return GH_ID_BUSY;
    /* Host bookkeeping only. Does not silently reset original pool counters;
     * callers must reinitialize manager before attaching a fresh runtime. */
    free(r);return GH_ID_OK;
}
gh_id_error gh_id_pool_allocate_002ace48(gh_id_runtime *r,uint32_t n,int32_t tag,int32_t arg,gh_id_pool_run *out)
{
    gh_id_job_view *job;unsigned char *wire;uint32_t start;
    if(!runtime_valid(r) || !out)return GH_ID_ARGUMENT;
    memset(out,0,sizeof(*out));
    if(!n)return GH_ID_NOT_FOUND;
    if(n>POOL_COUNT-r->manager->pool_used || r->manager->job_count>=JOB_COUNT)return GH_ID_CAPACITY;
    start=r->manager->pool_used;job=&r->jobs[r->manager->job_count];r->job_owners[r->manager->job_count]=NULL;
    job->tag=tag;job->arg=arg;job->start=start;job->wire_count=s8((unsigned char)n);job->active=1;
    wire=r->manager->jobs+(size_t)r->manager->job_count*0x10;
    put32(wire,(uint32_t)tag);put32(wire+4,(uint32_t)arg);
    /* Guest pointer word+8 is NOT populated with a truncated host pointer.
     * Its semantic target lives in the typed native job/start mapping. */
    put32(wire+8,0);wire[0xc]=(unsigned char)n;wire[0xd]=1;
    ++r->manager->job_count;r->manager->pool_used+=n;
    memset(r->manager->pool+(size_t)start*ENTRY_SIZE,0,(size_t)n*ENTRY_SIZE);
    memset(r->entries+start,0,(size_t)n*sizeof(*r->entries));
    out->entries=r->entries+start;out->start=start;out->count=n;return GH_ID_OK;
}
gh_id_error gh_id_pool_release_002acf30(gh_id_runtime *r,gh_id_pool_run run,int32_t tag,int *freed)
{
    gh_id_job_view *last;int32_t start;
    if(!runtime_valid(r) || !freed)return GH_ID_ARGUMENT;
    *freed=0;
    if(!r->manager->pool_used || !r->manager->job_count)return GH_ID_OK;
    last=&r->jobs[r->manager->job_count-1];
    if(last->tag!=tag)return GH_ID_OK;
    start=(int32_t)r->manager->pool_used-(int32_t)last->wire_count;
    if(start<0 || start>=POOL_COUNT || run.start!=(uint32_t)start || run.entries!=r->entries+start)return GH_ID_OK;
    --r->manager->job_count;r->manager->pool_used=(uint32_t)start;*freed=1;return GH_ID_OK;
}
gh_id_error gh_id_runtime_job(const gh_id_runtime *r,uint32_t i,gh_id_job_view *out)
{if(!runtime_valid(r) || !out)return GH_ID_ARGUMENT;if(i>=r->manager->job_count)return GH_ID_BOUNDS;*out=r->jobs[i];return GH_ID_OK;}
void gh_id_base_clear_002a9df8(gh_id_base *o)
{
    static const float identity[]={0,0,0,1};
    if(!o)return;
    memset(&o->source_records,0,sizeof(o->source_records));memset(&o->packed,0,sizeof(o->packed));
    o->entries=0;o->entry_count=0;o->stop=0;o->hide=0;o->frame=0;o->playing=0;o->b1d=0;o->b1e=0;
    memcpy(o->vector_a,identity,sizeof(identity));memcpy(o->vector_b,identity,sizeof(identity));
    o->f40=0.01f;o->h44=0x80;o->mode=6;o->initialized=0;o->owns_run=0;o->first_unported=0;o->path_policy=GH_ID_PATH_EXACT;
}
static void init_entry(gh_id_entry *e,const unsigned char *s,uint32_t index)
{
    unsigned j;
    e->source=s;e->source_index=index;e->seed=0xbc614e;e->kind=s[0x66];e->flags=u32(s+4);
    e->pos_x=f32(s+8);e->pos_y=f32(s+0xc);e->scale_x=f32(s+0x28);e->scale_y=f32(s+0x2c);
    e->rotation_speed=f32(s+0x24);e->color_base=u32(s+0x50);e->f68=f32(s+0x14);e->f6c=f32(s+0x18);
    for(j=0;j<4;++j)e->f70[j]=f32(s+0x40+j*4);
    e->b89=s[0x54];e->b28=s[1];e->parent_ref=s8(s[2]);e->id=s8(s[3]);
    e->b8a=s[0x55];e->b8b=s[0x56];e->b8d=s[0x58];e->h8e=s16(s+0x5a);
    e->f80=f32(s+0x60);e->layer=s8(s[0x59]);e->b8c=s8(s[0x65]);
    e->parts[0]=s+0x130;e->parts[1]=s+0x70;e->parts[2]=s+0x1b4;e->parts[3]=s+0x218;e->parts[4]=s+0x27c;
    e->message=0;
}
static void link_parents(gh_id_base *o)
{
    uint32_t i,j;
    for(i=0;i<o->entry_count;++i) {
        gh_id_entry *e=o->entries+i;
        if(e->parent_ref<0) {e->parent=0;continue;}
        if(e->parent_ref==e->b28) {e->parent=0;return;} /* original returns, not continue */
        for(j=0;j<o->entry_count;++j)if(i!=j && e->parent_ref==o->entries[j].b28)e->parent=o->entries+j;
    }
}
static void flag(gh_id_entry *e,uint32_t bit,int on)
{if(on)e->flags|=bit;else e->flags&=~bit;}
static gh_id_error reset_anim(gh_id_base *o,gh_id_random_u32 random,void *context)
{
    uint32_t i;
    o->frame=0;o->b1d=0;o->b1e=0;
    for(i=0;i<o->entry_count;++i) {
        gh_id_entry *e=o->entries+i;const unsigned char *s=e->source;
        if(e->flags&0x40) {
            if(!random) {o->first_unported=0x31cfe0;return GH_ID_UNSUPPORTED;}
            e->jitter[0]=(int16_t)(random(context)%9999);e->jitter[1]=(int16_t)(random(context)%9999);
            e->jitter[3]=(int16_t)(random(context)%9999);e->jitter[4]=(int16_t)(random(context)%9999);
            e->jitter[2]=(int16_t)(random(context)%9999);
        } else memset(e->jitter,0,sizeof(e->jitter));
        e->jitter[0]=0;e->flags&=~UINT32_C(0x4000);
        gh_id_error path_error=o->path_policy==GH_ID_PATH_REFERENCE_NEAREST?
            gh_id_path_reset_reference_nearest(e):gh_id_path_reset_002abc30(e);
        if(path_error){o->first_unported=0x2abc30;return path_error;}
        e->jitter[2]=0;if(s8(s[0x5e])==0)flag(e,0x1000,s16(s+0x1b0)<2);
        e->jitter[3]=0;if(s8(s[0x5f])==0)flag(e,0x800,s16(s+0x214)<2);
        e->jitter[4]=0;if(s8(s[0x64])==0)flag(e,0x400,s16(s+0x278)<2);
    }
    return GH_ID_OK;
}
gh_id_error gh_id_base_initialize_002a9ea0(gh_id_runtime *r,gh_id_base *o,int32_t resource,
    int32_t id,gh_id_random_u32 random,void *context,int *original_return)
{
    gh_id_view view;gh_id_pool_run run;gh_id_error error;uint32_t raw_count,count=0,i,next=0;
    if(!runtime_valid(r) || !o || !original_return)return GH_ID_ARGUMENT;
    *original_return=-1;
    if(o->owns_run || o->entries)return GH_ID_BUSY;
    gh_id_base_clear_002a9df8(o);o->id=id;o->resource_number=resource;o->mode=6;o->path_policy=r->path_policy;
    error=gh_id_lookup_002acd78(r->manager,(uint32_t)resource,(uint32_t)id+1,&view);
    if(error) {if(error==GH_ID_NOT_FOUND)*original_return=0;return error;}
    if(view.size<16 || u32(view.bytes)!=0x504449) {*original_return=1;return GH_ID_FORMAT;}
    raw_count=u32(view.bytes+4);
    if(raw_count>(view.size-16)/SOURCE_SIZE)return GH_ID_BOUNDS;
    for(i=0;i<raw_count;++i)if(!r->default_manager || !(u32(view.bytes+16+(size_t)i*SOURCE_SIZE+4)&0x200))++count;
    o->entry_count=count;
    if(!count) {*original_return=1;return GH_ID_FORMAT;}
    error=gh_id_pool_allocate_002ace48(r,count,resource,id,&run);
    if(error) {*original_return=1;return error;}
    o->entries=run.entries;o->owns_run=1;++r->live_objects;r->job_owners[r->manager->job_count-1]=o;
    o->source_records.bytes=view.bytes+16;o->source_records.size=(size_t)raw_count*SOURCE_SIZE;
    o->source_records.offset=view.offset+16;
    for(i=0;i<raw_count;++i) {
        const unsigned char *s=view.bytes+16+(size_t)i*SOURCE_SIZE;
        if(r->default_manager && (u32(s+4)&0x200))continue;
        init_entry(o->entries+next,s,i);++next;
    }
    link_parents(o);
    error=reset_anim(o,random,context);if(error)return error;
    o->initialized=1;*original_return=1;return GH_ID_OK;
}
gh_id_error gh_id_base_release_002aa078(gh_id_runtime *r,gh_id_base *o,int *original_return,int *reclaimed)
{
    gh_id_pool_run run;gh_id_error error;uintptr_t address,base;
    if(!runtime_valid(r) || !o || !original_return || !reclaimed)return GH_ID_ARGUMENT;
    *original_return=0;*reclaimed=0;
    if(!o->entries)return GH_ID_OK;
    address=(uintptr_t)o->entries;base=(uintptr_t)r->entries;
    if(address<base || address-base>=sizeof(r->entries) || (address-base)%sizeof(*r->entries))return GH_ID_ARGUMENT;
    run.entries=o->entries;run.start=(uint32_t)((address-base)/sizeof(*r->entries));run.count=o->entry_count;
    error=gh_id_pool_release_002acf30(r,run,o->resource_number,reclaimed);if(error)return error;
    for(unsigned i=0;i<JOB_COUNT;i++)if(r->job_owners[i]==o)r->job_owners[i]=NULL;
    memset(&o->source_records,0,sizeof(o->source_records));memset(&o->packed,0,sizeof(o->packed));o->entries=0;
    if(o->owns_run) {o->owns_run=0;--r->live_objects;}
    o->initialized=0;*original_return=1;return GH_ID_OK;
}
gh_id_error gh_id_base_get_work_002aa410(gh_id_runtime *r,gh_id_base *o,int32_t id,gh_id_entry **out,int *fallback)
{
    uint32_t i;
    if(!runtime_valid(r) || !o || !out)return GH_ID_ARGUMENT;
    if(o->entries)for(i=0;i<o->entry_count;++i)
        if((o->entries[i].flags&UINT32_C(0x80000000)) && o->entries[i].id==id) {
            *out=o->entries+i;if(fallback)*fallback=0;return GH_ID_OK;
        }
    *out=&r->fallback;if(fallback)*fallback=1;return GH_ID_OK;
}
static gh_id_error display_children(gh_id_base *o,gh_id_entry *parent,int show,uint32_t depth)
{
    uint32_t i;
    if(depth>o->entry_count)return GH_ID_FORMAT;
    for(i=0;i<o->entry_count;++i) {
        gh_id_entry *child=o->entries+i;
        if(child!=parent && child->parent==parent) {
            flag(child,UINT32_C(0x20000000),!show);
            if(display_children(o,child,show,depth+1)!=GH_ID_OK)return GH_ID_FORMAT;
        }
    }
    return GH_ID_OK;
}
gh_id_error gh_id_base_set_family_display(gh_id_runtime *r,gh_id_base *o,int32_t id,int show)
{
    gh_id_entry *entry,*parent;uint32_t count=0;gh_id_error error;
    if(show!=0 && show!=1)return GH_ID_ARGUMENT;
    error=gh_id_base_get_work_002aa410(r,o,id,&entry,0);if(error)return error;
    parent=entry->parent;
    while(parent) {
        if(++count>o->entry_count)return GH_ID_FORMAT;
        flag(parent,UINT32_C(0x20000000),!show);parent=parent->parent;
    }
    return display_children(o,entry,show,0);
}
gh_id_error gh_id_base_restart_anim(gh_id_base *o,gh_id_random_u32 random,void *context)
{
    gh_id_error error;
    if(!o)return GH_ID_ARGUMENT;
    error=reset_anim(o,random,context);if(error)return error;
    o->playing=1;return GH_ID_OK;
}

gh_id_error gh_id_entry_restore_source_002aa2d0(gh_id_base*o,gh_id_entry*e){
 if(!o||!e||!o->entries||!o->source_records.bytes)return GH_ID_ARGUMENT;
 uintptr_t address=(uintptr_t)e,first=(uintptr_t)o->entries,records=(uintptr_t)o->source_records.bytes;
 if(address<first||(address-first)%sizeof(*e)||(address-first)/sizeof(*e)>=o->entry_count)return GH_ID_ARGUMENT;
 uintptr_t source=(uintptr_t)e->source;
 if(source<records||source-records>o->source_records.size||SOURCE_SIZE>o->source_records.size-(source-records)||(source-records)%SOURCE_SIZE)return GH_ID_BOUNDS;
 if((source-records)/SOURCE_SIZE!=e->source_index)return GH_ID_BOUNDS;
 init_entry(e,e->source,e->source_index);return GH_ID_OK;
}

gh_id_error gh_id_base_detach_for_compaction(gh_id_runtime *r,gh_id_base *base)
{
    if(!runtime_valid(r)||!base)return GH_ID_ARGUMENT;
    if(!base->owns_run)return base->entries?GH_ID_FORMAT:GH_ID_OK;
    if(!base->entries||!r->live_objects)return GH_ID_FORMAT;
    for(unsigned i=0;i<r->manager->job_count;i++)if(r->job_owners[i]==base){
        const gh_id_job_view *job=&r->jobs[i];
        if(!job->active||job->tag==1||job->tag!=base->resource_number||
           job->start>=r->manager->pool_used||job->wire_count<=0||
           base->entry_count!=(uint32_t)job->wire_count||
           base->entry_count>r->manager->pool_used-job->start||
           base->entries!=r->entries+job->start)
            return GH_ID_FORMAT;
        r->job_owners[i]=NULL;
        --r->live_objects;
        base->source_records=(gh_id_view){0};
        base->packed=(gh_id_view){0};
        base->entries=NULL;
        base->owns_run=0;
        base->initialized=0;
        return GH_ID_OK;
    }
    return GH_ID_FORMAT;
}

gh_id_error gh_id_pool_compact_002ad108(gh_id_runtime*r,int*original){
 if(!runtime_valid(r)||!original)return GH_ID_ARGUMENT;*original=0;
 if(!r->manager->pool_used)return GH_ID_OK;
 unsigned count=r->manager->job_count;if(count>JOB_COUNT||r->manager->pool_used>POOL_COUNT)return GH_ID_FORMAT;
 int seen_other=0,blocked=0,found=0;unsigned last=0;int64_t total=0;
 for(unsigned i=0;i<count;i++){
  const gh_id_job_view*j=r->jobs+i;if(j->tag==1){found=1;last=i;total+=j->wire_count;if(seen_other)blocked=1;}else seen_other=1;
 }
 if(blocked)return GH_ID_OK; /*actualreturn0; interiorforeignresource blocks*/
 /* Original all-non1/signed-byte overflow domains can manufacture impossible
  * native pool bounds. Preserve them as explicitunsupported, not danglingIDs. */
 if(!found)return GH_ID_UNSUPPORTED;if(total<0||total>POOL_COUNT)return GH_ID_BOUNDS;
 if((uint64_t)r->jobs[last].start+(uint32_t)(uint8_t)r->jobs[last].wire_count!=(uint64_t)total)return GH_ID_FORMAT;
 for(unsigned i=last+1;i<count;i++)if(r->job_owners[i])return GH_ID_BUSY;
 r->manager->job_count=last+1;r->manager->pool_used=(uint32_t)total;for(unsigned i=last+1;i<count;i++)r->jobs[i].active=0;
 *original=1;return GH_ID_OK;
}
