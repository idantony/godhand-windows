#include "mc_file_save.h"
#include <string.h>

static uint16_t read16(const uint8_t *bytes)
{
    return (uint16_t)((uint16_t)bytes[0]|(uint16_t)bytes[1]<<8);
}

static void write_le(uint8_t *bytes,uint64_t value,unsigned count)
{
    for(unsigned i=0;i<count;++i)bytes[i]=(uint8_t)(value>>(8*i));
}

/* 9280 stores only the original unlock bits in the blank file header. */
static uint32_t save_unlocks(const gh_boot_work *work,const uint8_t *record)
{
    static const uint32_t masks[10]={
        0x80000000,0x40000000,0x20000000,0x10000000,0x08000000,
        0x04000000,0x02000000,0x01000000,0x00400000,0x00200000
    };
    uint32_t work_flags;
    memcpy(&work_flags,work->pad588,4);
    uint32_t flags=read16(record+0x30)!=0;
    for(unsigned i=0;i<10;++i)
        if(work_flags&masks[i])flags|=UINT32_C(1)<<i;
    return flags;
}

/* 8C80 compares the six unsigned RTC fields in chronological order and
 * keeps the earliest slot on a tie, defaulting to slot zero. */
static uint64_t clock_order(const uint8_t *clock)
{
    static const unsigned indexes[]={1,2,3,5,6,7};
    uint64_t value=0;
    for(unsigned i=0;i<6;++i)value|=(uint64_t)clock[indexes[i]]<<(8*i);
    return value;
}

gh_mc_load_status gh_mc_save_serialize_00006758(gh_game_checkpoint *checkpoint,
    const gh_checkpoint_bindings *bind,const gh_game_door *door,const gh_mc_save_metadata *meta,
    uint8_t out[GH_MC_SAVE_RECORD_BYTES],uint32_t *frontier)
{
    if(!frontier)return GH_MC_LOAD_ARGUMENT;
    *frontier=0;
    if(!checkpoint||!bind||!bind->work||!door||!meta||!out)return GH_MC_LOAD_ARGUMENT;
    if(meta->mode==1){*frontier=0x2be998;return GH_MC_LOAD_UNSUPPORTED;}
    const int fallback=meta->clock_result==0||meta->clock[0]!=0;
    if(fallback&&(!meta->history||meta->history_bytes<20*GH_MC_SAVE_HEADER_BYTES||
                  (meta->history_mask&~UINT32_C(0xfffff))))return GH_MC_LOAD_ARGUMENT;
    gh_checkpoint_status status=gh_game_checkpoint_full_002c05b0(
        checkpoint,bind,door->backup,meta->output_mode);
    if(status){
        *frontier=checkpoint->frontier?checkpoint->frontier:0x2c05b0;
        return status==GH_CHECKPOINT_UNSUPPORTED?GH_MC_LOAD_UNSUPPORTED:GH_MC_LOAD_ARGUMENT;
    }

    /* 6758 rewrites the capture destination, leaving live CoreSave and
     * the global 755880 pose snapshot as they were produced by 2C05B0. */
    uint8_t *payload=bind->destination;
    uint16_t room=(uint16_t)door->target_room;
    if(bind->work->room==0||bind->work->room==0x21){
        memcpy(payload+0xb0d0,door->backup,12);
        memcpy(payload+0xb0e0,door->backup+12,4);
        room=read16(door->backup+16);
    }else{
        memcpy(payload+0xb0d0,door->position_bits,12);
        write_le(payload+0xb0e0,door->yaw_bits,4);
    }
    write_le(payload+0xc,room,2);
    write_le(payload+0xe,(uint16_t)bind->work->room,2);
    memset(out,0,GH_MC_SAVE_RECORD_BYTES);
    memcpy(out+0x20,payload,GH_CHECKPOINT_BYTES);
    write_le(out,UINT32_C(0x3dcccccd),4);
    write_le(out+4,meta->mode,4);
    write_le(out+8,save_unlocks(bind->work,out),4);
    write_le(out+0xc,meta->event_room_override?meta->event_room:room,2);
    write_le(out+0xe,read16(out+0x30),2);
    out[0x10]=out[0x3f];

    if(fallback){
        unsigned newest=0;
        uint64_t last=0;
        for(unsigned slot=0;slot<20;++slot){
            if(!(meta->history_mask&(UINT32_C(1)<<slot)))continue;
            const uint8_t *candidate=meta->history+slot*GH_MC_SAVE_HEADER_BYTES+0x12;
            uint64_t order=clock_order(candidate);
            if(order>last){last=order;newest=slot;}
        }
        memcpy(out+0x12,meta->history+newest*GH_MC_SAVE_HEADER_BYTES+0x12,8);
        ++out[0x13];
    }else{
        memcpy(out+0x12,meta->clock,8);
    }
    write_le(out+0xb110,gh_mc_save_checksum_00008c08(out),8);
    return GH_MC_LOAD_VERIFIED;
}
