#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../../src/core/mc_file_save.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);} } while(0)

static uint32_t word(const uint8_t *bytes)
{
    return (uint32_t)bytes[0]|(uint32_t)bytes[1]<<8|(uint32_t)bytes[2]<<16|(uint32_t)bytes[3]<<24;
}

static void stage_write(gh_memory_card *card,unsigned slot,const uint8_t *record)
{
    int done;
    gh_mc_result result;
    CHECK(gh_memory_card_request_save_write_open(card,slot)==GH_MC_OK);
    CHECK(gh_memory_card_poll(card,&done,&result)==GH_MC_OK&&done&&result.result==0);
    CHECK(gh_memory_card_request_save_write(card,record)==GH_MC_OK);
    CHECK(gh_memory_card_poll(card,&done,&result)==GH_MC_OK&&done&&result.result==GH_MC_SAVE_RECORD_BYTES);
}

static void commit_write(gh_memory_card *card)
{
    int done;
    gh_mc_result result;
    CHECK(gh_memory_card_request_save_close(card)==GH_MC_OK);
    CHECK(gh_memory_card_poll(card,&done,&result)==GH_MC_OK&&done&&result.result==0);
}

static void transfer(gh_memory_card *card,unsigned slot,const uint8_t *record)
{
    stage_write(card,slot,record);
    commit_write(card);
}

static void check_missing(const char *path)
{
    CHECK(GetFileAttributesA(path)==INVALID_FILE_ATTRIBUTES);
    DWORD error=GetLastError();
    CHECK(error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND);
}

static void check_file(const char *path,const uint8_t *record)
{
    uint8_t bytes[GH_MC_SAVE_RECORD_BYTES];
    FILE *file=fopen(path,"rb");
    CHECK(file&&fread(bytes,1,sizeof(bytes),file)==sizeof(bytes));
    CHECK(!memcmp(bytes,record,sizeof(bytes)));
    CHECK(fgetc(file)==0x5a&&fgetc(file)==EOF&&!ferror(file)&&!fclose(file));
}

int main(int argc,char **argv)
{
    char scratch[1200],profile[1400],folder[1500],path[1600],fresh_folder[1500],fresh_path[1600];
    static const char suffix[]="\\memory-card-tests\\scratch";
    CHECK(argc==2&&GetFullPathNameA(argv[1],sizeof(scratch),scratch,NULL));
    size_t length=strlen(scratch);
    CHECK(length>=sizeof(suffix)-1&&!_stricmp(scratch+length-(sizeof(suffix)-1),suffix));
    CHECK(snprintf(profile,sizeof(profile),"%s\\save-%lu",scratch,(unsigned long)GetCurrentProcessId())>0);
    CHECK(CreateDirectoryA(profile,NULL));
    CHECK(snprintf(folder,sizeof(folder),"%s\\BASLUS-21503GODHD019",profile)>0);
    CHECK(snprintf(path,sizeof(path),"%s\\BASLUS-21503GODHD019",folder)>0);
    CHECK(snprintf(fresh_folder,sizeof(fresh_folder),"%s\\BASLUS-21503GODHD018",profile)>0);
    CHECK(snprintf(fresh_path,sizeof(fresh_path),"%s\\BASLUS-21503GODHD018",fresh_folder)>0);

    static uint8_t payload[GH_CHECKPOINT_BYTES],core[GH_CORE_SAVE_BYTES],player_raw[GH_TITLE_PLAYER_BYTES];
    static uint8_t record[GH_MC_SAVE_RECORD_BYTES],old_record[GH_MC_SAVE_RECORD_BYTES];
    static uint8_t history[20*GH_MC_SAVE_HEADER_BYTES];
    static gh_game_init_state init;
    static gh_game_prepare prepare;
    static gh_mc_selected_load read;
    gh_game_checkpoint checkpoint={0};
    gh_boot_work work={.room=0x100,.language=2};
    gh_game_door door={.position_bits={0x3f800000,0x40000000,0x40400000},
                      .yaw_bits=0x40800000,.target_room=0x102};
    for(unsigned i=0;i<24;i++)door.backup[i]=(uint8_t)(0x80+i);
    gh_title_player_view player={.bytes=player_raw,.size=sizeof(player_raw),.alias_f0=player_raw+0xb0,
        .initial_memory=GH_TITLE_MEMORY_PROVIDED_BYTES};
    const gh_checkpoint_bindings bind={.work=&work,.save={core,sizeof(core)},.prepare=&prepare,
        .init=&init,.player=&player,.destination=payload,.destination_size=sizeof(payload)};
    const gh_mc_save_metadata meta={.mode=0,.output_mode=1,.event_room_override=1,.event_room=0x103,
        .clock_result=1,.clock={0,0x32,0x11,0x10,0,0x20,0x11,0x26}};
    core[0x10]=5;core[0x1f]=3;
    player_raw[0x54a]=81;
    prepare.fields[0x1660]=0x44;prepare.global_0061a994=0x87654321;
    init.world_time=0x12345678;init.world_active_005e85f8=1;
    init.heat[7]=0x42;
    work.pad588[2]=0x60;
    uint32_t frontier;
    gh_mc_save_metadata bad_mode=meta;bad_mode.mode=1;
    CHECK(gh_mc_save_serialize_00006758(&checkpoint,&bind,&door,&bad_mode,record,&frontier)==
          GH_MC_LOAD_UNSUPPORTED&&frontier==0x2be998&&!checkpoint.captured&&!core[8]);
    CHECK(gh_mc_save_serialize_00006758(&checkpoint,&bind,&door,&meta,record,&frontier)==
          GH_MC_LOAD_VERIFIED&&!frontier&&checkpoint.captured);
    const uint8_t expected_header[32]={0xcd,0xcc,0xcc,0x3d,0,0,0,0,1,3,0,0,3,1,5,0,3,0,
        0,0x32,0x11,0x10,0,0x20,0x11,0x26,0,0,0,0,0,0};
    CHECK(!memcmp(record,expected_header,sizeof(expected_header)));
    CHECK(word(record+0x20)==0x12345678&&word(record+0x24)==0x87654321&&word(record+0x28)==1);
    CHECK(word(record+0x2c)==0x01000102&&word(core+0xc)==0x0100);
    CHECK(word(record+0xa4)==81&&word(record+0x190)==1&&word(record+0x194)==1&&word(record+0x198)==2);
    CHECK(record[0x14c0]==0x44&&word(record+0x178)==0x91909392);
    CHECK(word(record+0xb0f0)==0x3f800000&&word(record+0xb100)==0x40800000&&word(record+0xb108)==0x42000000);
    CHECK(!checkpoint.tail.position[0]&&!checkpoint.tail.rotation[1]);
    for(size_t i=0xb118;i<sizeof(record);++i)CHECK(!record[i]);

    gh_memory_card *card;
    CHECK(gh_memory_card_open(profile,&card)==GH_MC_OK);
    CHECK(gh_memory_card_request_save_write_open(card,20)==GH_MC_ARGUMENT);
    transfer(card,19,record);
    memcpy(old_record,record,sizeof(old_record));
    FILE *file=fopen(path,"ab");
    CHECK(file&&fputc(0x5a,file)==0x5a&&!fclose(file));

    /* Original 8C80 fallback uses the newest present metadata record, then
     * increments its seconds byte even when the RTC reports failure. */
    gh_mc_save_metadata fallback=meta;
    fallback.clock_result=0;fallback.history=history;fallback.history_bytes=sizeof(history);
    fallback.history_mask=(1u<<1)|(1u<<5);
    memcpy(history+0x400+0x12,meta.clock,8);
    memcpy(history+0x1400+0x12,meta.clock,8);
    history[0x1400+0x19]=0x27;
    work.room=0x21;
    CHECK(gh_mc_save_serialize_00006758(&checkpoint,&bind,&door,&fallback,record,&frontier)==
          GH_MC_LOAD_VERIFIED&&!frontier);
    CHECK(record[0x13]==0x33&&record[0x19]==0x27);
    CHECK(word(record+0x2c)==0x00219190&&!memcmp(record+0xb0f0,door.backup,12));
    CHECK(!memcmp(record+0xb100,door.backup+12,4));
    CHECK(memcmp(old_record,record,sizeof(record)));

    stage_write(card,19,record);
    check_file(path,old_record);
    CHECK(gh_memory_card_end(card)==GH_MC_OK);
    check_file(path,old_record);
    CHECK(gh_memory_card_begin(card)==GH_MC_OK);

    gh_memory_card *competing_card;
    CHECK(gh_memory_card_open(profile,&competing_card)==GH_MC_OK);
    stage_write(card,19,record);
    int done;
    gh_mc_result result;
    CHECK(gh_memory_card_request_save_write_open(competing_card,19)==GH_MC_OK);
    CHECK(gh_memory_card_poll(competing_card,&done,&result)==GH_MC_IO&&!done);
    CHECK(gh_memory_card_abort(card)==GH_MC_OK);
    check_file(path,old_record);
    stage_write(competing_card,19,record);
    check_file(path,old_record);
    CHECK(gh_memory_card_abort(competing_card)==GH_MC_OK);
    gh_memory_card_close(competing_card);
    CHECK(gh_memory_card_begin(card)==GH_MC_OK);

    stage_write(card,19,record);
    gh_memory_card_close(card);
    check_file(path,old_record);
    CHECK(gh_memory_card_open(profile,&card)==GH_MC_OK);

    stage_write(card,18,record);
    check_missing(fresh_path);
    CHECK(gh_memory_card_abort(card)==GH_MC_OK);
    check_missing(fresh_path);
    CHECK(gh_memory_card_begin(card)==GH_MC_OK);

    stage_write(card,19,record);
    HANDLE reader=CreateFileA(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    CHECK(reader!=INVALID_HANDLE_VALUE);
    CHECK(gh_memory_card_request_save_close(card)==GH_MC_OK);
    CHECK(gh_memory_card_poll(card,&done,&result)==GH_MC_IO&&!done);
    check_file(path,old_record);
    CHECK(CloseHandle(reader));
    CHECK(gh_memory_card_abort(card)==GH_MC_OK);
    CHECK(gh_memory_card_begin(card)==GH_MC_OK);

    stage_write(card,19,record);
    check_file(path,old_record);
    commit_write(card);
    CHECK(gh_mc_selected_load_begin(&read,card,19)==GH_MC_LOAD_PENDING);
    CHECK(gh_mc_selected_load_step(&read,&frontier)==GH_MC_LOAD_PENDING);
    CHECK(gh_mc_selected_load_step(&read,&frontier)==GH_MC_LOAD_PENDING);
    CHECK(gh_mc_selected_load_step(&read,&frontier)==GH_MC_LOAD_VERIFIED&&!frontier);
    CHECK(!memcmp(read.record,record,sizeof(record)));
    check_file(path,record);
    gh_memory_card_close(card);
    if(GetFileAttributesA(fresh_folder)!=INVALID_FILE_ATTRIBUTES)CHECK(RemoveDirectoryA(fresh_folder));
    CHECK(DeleteFileA(path)&&RemoveDirectoryA(folder)&&RemoveDirectoryA(profile));
    puts("PASS original checkpoint layout, atomic replacement, rollback, writer exclusion and retry");
    return 0;
}
