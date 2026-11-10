#include "game_om_factory.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t*load(gh_disc*d,const char*path,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,path,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*b=malloc(*n);C(b&&!gh_disc_read(d,i,0,b,*n));return b;}
typedef struct {gh_om_scene_placements *placements;gh_bundle_entry *oms;gh_om_factory *factory;gh_scene_placement_status status;int original,done;} bindtask;
static void bind(void*p,int32_t argument){bindtask*t=p;(void)argument;t->status=gh_om_scene_bind_native_001c3cb8(t->placements,t->oms,t->factory,&t->original);t->done=1;}
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
int main(int argc,char**argv){C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t n,object_bytes;uint8_t*raw=load(d,"st1/r100.dat",&n),*resource=load(d,"om/om00.dat",&object_bytes);
 gh_bundle bundle;gh_bundle_entry oms;C(!gh_bundle_open(raw,n,&bundle)&&!gh_bundle_find(&bundle,"OMS",0,&oms)&&oms.size==4480);
 gh_om_scene_placements placement={0};int original=9;C(gh_om_scene_bind_001c3cb8(&placement,&oms,&original)==GH_PLACE_UNSUPPORTED&&!original&&placement.count==69&&placement.version==3&&placement.frontier==0x2bed08);
 unsigned histogram[256]={0};for(unsigned i=0;i<69;i++){gh_om_placement_record r;C(!gh_om_scene_record_at(&placement,i,&r)&&r.bytes==oms.bytes+16+i*64&&r.type>=0x300&&r.type<0x400);histogram[r.type&255]++;}C(histogram[0]==3&&histogram[3]==5&&histogram[4]==10&&histogram[0x24]==10&&histogram[0x5d]==1);
 gh_om_placement_record first;C(!gh_om_scene_record_at(&placement,0,&first)&&first.type==0x300&&first.key==UINT64_C(0x303030726f64));
 gh_game_prepare prepare={0};uint8_t state=0;C(!gh_game_prepare_002a5d40(&prepare,(gh_core_save_view){0},0x100,&state));gh_room_state room;C(!gh_room_state_select_002bebc8(&prepare,0x100,0,&room));
 int used=9;C(!gh_om_room_slot_used_002bed08(&room,first.key,&used)&&!used);memcpy(room.active+0x610+200*16,&first.key,8);room.active[0x618+200*16]=1;C(!gh_om_room_slot_used_002bed08(&room,first.key,&used)&&used);room.active[0x618+200*16]=2;C(!gh_om_room_slot_used_002bed08(&room,first.key,&used)&&!used);C(!gh_om_room_slot_used_002bed08(&room,0,&used)&&!used);
 /* Firstmatchingduplicate wins; used2 mustnot be coercedtrue. */memcpy(room.active+0x610,&first.key,8);room.active[0x618]=0;room.active[0x618+200*16]=1;C(!gh_om_room_slot_used_002bed08(&room,first.key,&used)&&!used);memset(room.active+0x610,0,0x1000);
 gh_heap*heap;C(!gh_heap_create(16*1024*1024,&heap));gh_room_pools pools={0};C(!gh_game_room_pools_initialize_002a5d80(&pools,&bundle,heap));
 gh_stage_slots gs={0},rs={0};gh_game_objects *global,*local;C(!gh_game_objects_create(&gs,0,&global)&&!gh_game_objects_create(&rs,1,&local));C(!gh_stage_slots_reset(&gs,NULL)&&!gh_stage_slots_reset(&rs,heap));
 gh_boot_work work={0};work.room=0x100;uint8_t serial=255;gh_om_factory factory={0};gh_om_factory_bindings b={.pools=&pools,.work=&work,.room_state=&room,.objects=global,.room_objects=local,.factory_serial=&serial};C(!gh_om_factory_bind(&factory,&b,69));
 placement=(gh_om_scene_placements){0};C(gh_om_scene_bind_native_001c3cb8(&placement,&oms,&factory,&original)==GH_PLACE_UNSUPPORTED&&placement.frontier==0x1fe8a0&&!original&&!placement.cursor);
 gh_om_object*o=factory.objects;uint8_t*p=o->view.bytes;C(o->stage==2&&p&&o->allocation.original_global==0x754c10&&!o->registered&&!o->acquired&&serial==0);
 C(o->view.alias_d0==p+0x30&&o->view.alias_f0==p+0xb0&&word(p+0x214)==0x422f90&&word(p+0x4ac)==0x4289f0&&word(p+0x2d0)==0x20000000&&p[0x2ff]==3&&p[0x301]==3&&p[0x2fc]==255);
 C(word(p+0x5b8)==0x10&&word(p+0x5cc)==0x3f800000&&word(p+0x61c)==0x3f800000);
 for(unsigned i=0;i<7;i++)for(unsigned j=0;j<5;j++)C(word(p+0x650+i*0x60+j*16+12)==0x3f800000);
 C(!p[0x8e4]&&!p[0x8ef]&&!p[0x304]);const gh_room_vector*vector;C(!gh_room_registry_view(&pools,0x754c58,&vector)&&!vector->count);
 C(gh_om_scene_bind_native_001c3cb8(&placement,&oms,&factory,&original)==GH_PLACE_UNSUPPORTED&&factory.objects->view.bytes==p&&serial==0);
 /* ConnectactualDVD/fiber continuation ofthealreadyconstructedfirstdoor;
  * no collector/pinnedfixture supplies the globalresource. */
 gh_dvd_jobs dvd;gh_dvd_jobs_init(&dvd,d);gh_task_record32 records[4]={0};gh_task_manager manager={.records=records,.capacity=4,.count=4,.current_index=-1};gh_task_threads*threads;C(!gh_task_threads_create(&manager,&threads));
 gh_object_resource_bindings loads={.dvd=&dvd,.heap=heap,.threads=threads};factory.bindings.resources=&loads;
 bindtask task={&placement,&oms,&factory,0,0,0};int32_t task_slot;C(!gh_task_threads_start(threads,2,bind,&task,0,&task_slot));unsigned ticks=0;
 while(!task.done&&ticks++<12){C(!gh_tasks_run_matching_002d52a0(&manager).status&&!gh_dvd_pump(&dvd));}
 C(task.done&&task.status==GH_PLACE_UNSUPPORTED&&placement.frontier==0x1b7118&&serial==0);
 C(o->stage==4&&o->registered&&o->acquired&&o->resource_manager==global&&o->resource.file_extent_known&&o->resource.file_extent==object_bytes&&o->resource.state==3&&o->resource.use_count==1);
 C(!memcmp(o->resource.bytes,resource,object_bytes));gh_title_player_view view;C(!gh_om_factory_object_view(&factory,o->handle,&view)&&view.bytes==p&&vector->count==1);
 gh_om_factory foreign={0};C(gh_om_factory_object_view(&foreign,o->handle,&view)==GH_PLACE_ARGUMENT);
 C(gh_om_scene_bind_native_001c3cb8(&placement,&oms,&factory,&original)==GH_PLACE_UNSUPPORTED&&serial==0&&vector->count==1);
 C(!gh_om_factory_release(&factory)&&!vector->count);gh_object_view rv;C(!gh_game_objects_find(global,0x300,&rv)&&rv.use_count==0&&rv.state==4);
 C(!gh_task_threads_destroy(threads));
 gh_game_objects_destroy(local);gh_game_objects_destroy(global);gh_disc_close(d);C(!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));free(resource);free(raw);
 puts("PASS actualR100 OMS69/CRoomSave256 query/firstdoor SAMEarena ctor+serial resumability/realmissingresource frontier then actualDVD/fiber/resource/sharedregistry prefix; virtual1B7118 NOTimplemented");return 0;
}
