#include "game_scene_visibility.h"
#include "game_actor_scroll_links.h"
#include "game_stage.h"
#include "disc.h"
#include "game_scene_dynamic_geometry.h"
#include "game_om_door_geometry.h"
#include "game_esp14.h"
#include "game_event_start.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static unsigned half(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}static uint32_t word(const void*p){uint32_t v;memcpy(&v,p,4);return v;}static float val(const uint8_t*p){float v;memcpy(&v,p,4);return v;}static void put(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
static uint8_t*load(gh_disc*d,const char*path,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,path,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*p=malloc(*n);C(p&&!gh_disc_read(d,i,0,p,*n));return p;}
typedef struct {gh_room_pools*pools;gh_scene_meshes*meshes;gh_title_player_view*player;uint32_t player_handle;unsigned masks,clears,fail_positive;gh_esp14_model *children[2];} context;
static gh_scene_post_status resolve(void*ctx,uint32_t h,gh_title_player_view*out,uint32_t*f)
{
 context*c=ctx;(void)f;void*owned=NULL;
 for(unsigned i=0;i<2;i++){
  gh_esp14_model *child=c->children[i];
  if(child&&h==child->handle){
   if(gh_room_object_lookup(c->pools,h,GH_ESP_MODEL_KIND,child,&owned)||owned!=&child->object)
    return GH_POST_UNSUPPORTED;
   *out=child->object.view;return GH_POST_OK;
  }
 }
 if(h!=c->player_handle||gh_room_object_lookup(c->pools,h,0x100,c,&owned)||owned!=c->player)
  return GH_POST_UNSUPPORTED;
 *out=*c->player;return GH_POST_OK;
}

static gh_game_event_status event_resolve(void *ctx,uint32_t handle,gh_title_player_view *out,uint32_t *f)
{
 return resolve(ctx,handle,out,f)==GH_POST_OK?GH_EVENT_OK:GH_EVENT_UNSUPPORTED;
}

static void kind500_backing_visibility(context *c,gh_scene_visibility_bindings *binding,
    gh_game_pl_camera *camera,gh_boot_work *work,uint8_t *serial,
    const float actor_from[3],const float actor_to[3],uint64_t actor_key)
{
 gh_esp14_model children[2]={0};
 const gh_esp14_model_bindings create={.pools=c->pools,.work=work,.factory_serial=serial};
 uint32_t pc=0;int found;
 for(unsigned i=0;i<2;i++){
  C(!gh_esp14_model_create_0030fa98(&children[i],&create,&found,&pc)&&found&&!pc);
  C(children[i].object.view.size==0x4e0&&children[i].object.view.original_vtable214==0x44b840);
  c->children[i]=&children[i];
 }
 uint8_t *name,*flags,*last_name,*last_flags;
 const gh_room_arena_allocation *first=&children[0].object.arena_allocation;
 const gh_room_arena_allocation *last=&children[1].object.arena_allocation;
 C(!gh_room_arena_model_visibility_field(first,0x538,&name)&&
    !gh_room_arena_model_visibility_field(first,0x5a0,&flags)&&
    !gh_room_arena_model_visibility_field(last,0x538,&last_name)&&
    !gh_room_arena_model_visibility_field(last,0x5a0,&last_flags));
 C(name==children[1].object.view.bytes+0x38&&flags==children[1].object.view.bytes+0xa0);
 memcpy(name,&actor_key,8);
 memcpy(last_name,&(uint64_t){0},8);
 put(flags,0x3f800002u);put(last_flags,0x22u);
 gh_scene_visibility *visibility=NULL;C(!gh_scene_visibility_create(&visibility));
 const float far[]={1000,1000,1000},far_eye[]={1001,1000,1000};
 memcpy(camera->fields+0x200,far,12);memcpy(camera->fields+0x210,far_eye,12);
 C(!gh_scene_visibility_step_00307b28(visibility,binding,&pc)&&!pc);
 C(word(flags)==0x3f800000u&&word(last_flags)==0x20u);
 C(!gh_scene_visibility_next_call(visibility));
 memcpy(camera->fields+0x200,actor_from,12);memcpy(camera->fields+0x210,actor_to,12);
 C(!gh_scene_visibility_step_00307b28(visibility,binding,&pc)&&!pc);
 C(word(flags)==0x3f800002u&&word(last_flags)==0x20u);
 const gh_event_start_bindings event={.pools=c->pools,.meshes=c->meshes,.object_view=event_resolve,.context=c};
 C(!gh_event_clear_object_hidden_00308a38(&event,&pc)&&!pc&&word(flags)==0x3f800000u);
 uint32_t effect250=word(children[0].object.view.bytes+0x250);
 C(!gh_event_hide_scenario_objects_002c5068(&event,1,&pc)&&!pc&&
    word(children[0].object.view.bytes+0x250)==effect250);
 gh_scene_visibility_destroy(visibility);
 for(unsigned i=2;i>0;i--){
  C(!gh_esp14_model_dispose(&children[i-1]));
  c->children[i-1]=NULL;
 }
 puts("PASS R100 original kind500 arena visibility alias and positive named ray");
}
static gh_scene_post_status mask_nodes(void*ctx,uint32_t h,uint32_t a,uint32_t b,uint32_t*f){context*c=ctx;if(b&&c->fail_positive){c->fail_positive--;*f=0x14b6f8;return GH_POST_UNSUPPORTED;}void*owned=NULL;if(gh_room_object_lookup(c->pools,h,GH_ROOM_OBJECT_SCROLL_MESH,c->meshes,&owned))return GH_POST_UNSUPPORTED;gh_scene_mesh_object*o=owned;if(!o->constructor_complete||!o->node)return GH_POST_ARGUMENT;uint8_t*n=o->node;put(n+0x380,(word(n+0x380)&a)|b);if(b)c->masks++;else c->clears++;return GH_POST_OK;}
static int request_for(const gh_game_scene_tables*t,unsigned id,float from[3],float to[3],int actors){const uint8_t*p=t->actor_links[4].view.bytes+(size_t)id*20;unsigned attr=half(p+12);if((size_t)attr*16+16>t->actor_links[1].view.size)return 0;const uint8_t*a=t->actor_links[1].view.bytes+(size_t)attr*16;if(actors?!a[11]:(!a[10]||a[11]))return 0;unsigned base=half(a+8);int found=actors;for(unsigned k=0;k<(actors?0:a[10]);k++){gh_actor_scroll_entry e;if(!gh_scene_actor_scroll_entry_001333b8(t,(uint16_t)(base+k),&e)&&e.resolved&&e.scroll_handle)found=1;}if(!found)return 0;const uint8_t*n=t->actor_links[3].view.bytes+(size_t)half(p+8)*12;double norm[3]={val(n),val(n+4),val(n+8)},length=sqrt(norm[0]*norm[0]+norm[1]*norm[1]+norm[2]*norm[2]);if(length<.1||fabs(norm[0])+fabs(norm[2])<.1)return 0;unsigned count=(word(p+16)&1)?4:3;double center[3]={0};for(unsigned j=0;j<count;j++){const uint8_t*v=t->actor_links[2].view.bytes+(size_t)half(p+j*2)*12;for(unsigned k=0;k<3;k++)center[k]+=val(v+k*4)/count;}for(unsigned k=0;k<3;k++){from[k]=(float)(center[k]+norm[k]/length*.125);to[k]=(float)(center[k]-norm[k]/length*.125);}return 1;}
int main(int argc,char**argv){C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t rn,en=(size_t)gh_disc_boot_elf_size(d);uint8_t*raw=load(d,"st1/r100.dat",&rn),*original=malloc(rn),*elf=malloc(en);C(original&&elf&&!gh_disc_read_boot_elf(d,0,elf,en));memcpy(original,raw,rn);gh_bundle room;C(!gh_bundle_open(raw,rn,&room));gh_heap*heap;gh_room_pools pools={0};C(!gh_heap_create(64*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&room,heap));gh_game_world_light*light=calloc(1,sizeof(*light));C(light&&!gh_game_world_light_construct_002d6d80(light)&&!gh_game_world_light_initialize_with_heap_002d6f40(light,&room,heap));gh_stage_textures*textures;C(!gh_stage_textures_create(&textures));gh_boot_work work={0};gh_game_scene_tables tables={0};gh_scene_meshes meshes={0};uint8_t serial;C(!gh_scene_mesh_factory_serial_from_elf(elf,en,&serial));meshes.factory_serial=&serial;gh_scene_mesh_construction_bindings cb={textures,light,elf,en};C(!gh_scene_meshes_bind_construction(&meshes,&cb)&&!gh_scene_meshes_construct_0031d1b0(&meshes,&tables,&work,&room,&pools,heap)&&meshes.constructed_count==156);C(!gh_scene_actors_0012e670(&tables,&room)&&!gh_scene_actor_scroll_links_native_00307a60(&tables,&pools,&meshes));
 gh_game_init_state*init=malloc(sizeof(*init));C(init);gh_game_init_state_initialize(init);gh_game_pl_camera camera={0};C(!gh_game_camera_construct_pl_00139bc8(&camera,&init->camera,GH_CAMERA_NATIVE_BINARY32_REFERENCE));init->active_camera=&init->camera;init->camera.mode=0;uint8_t*player_bytes=calloc(1,GH_TITLE_PLAYER_BYTES);C(player_bytes);gh_title_player_view player;C(!gh_title_player_construct_00107e40(player_bytes,GH_TITLE_PLAYER_BYTES,&player));uint16_t kind=0x100;memcpy(player_bytes+0x2fe,&kind,2);context c={.pools=&pools,.meshes=&meshes,.player=&player};C(!gh_room_object_register(&pools,&player,&c,0x100,8,&c.player_handle));gh_scene_visibility_bindings b={.scene={&work,&pools,&tables,init,&camera,&player,GH_CAMERA_NATIVE_BINARY32_REFERENCE},.resolve_handle=resolve,.mask_nodes=mask_nodes,.context=&c};
 float from[3],to[3];unsigned selected=0;for(;selected<half(tables.akg.bytes+0x28);selected++)if(request_for(&tables,selected,from,to,0))break;C(selected<half(tables.akg.bytes+0x28));const int modes[]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO};uint32_t f=0;
 for(unsigned mode=0;mode<4;mode++){gh_scene_visibility*s;C(!gh_scene_visibility_create(&s));put(player_bytes+0x5a0,0xau);float high[]={1000,1000,1000},high_to[]={1001,1000,1000};memcpy(camera.fields+0x200,high,12);memcpy(camera.fields+0x210,high_to,12);C(!fesetround(modes[mode])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));uint64_t queries=tables.ray.native_queries;C(!gh_scene_visibility_step_00307b28(s,&b,&f)&&word(player_bytes+0x5a0)==8&&tables.ray.native_queries==queries+6);C(!gh_scene_visibility_next_call(s));memcpy(camera.fields+0x200,from,12);memcpy(camera.fields+0x210,to,12);c.fail_positive=2;gh_scene_post_status e=gh_scene_visibility_step_00307b28(s,&b,&f);if(e!=GH_POST_UNSUPPORTED||f!=0x14b6f8)fprintf(stderr,"positive triangle%u e%d pc%X masks%u\n",selected,e,f,c.masks);C(e==GH_POST_UNSUPPORTED&&f==0x14b6f8);queries=tables.ray.native_queries;C(gh_scene_visibility_step_00307b28(s,&b,&f)==GH_POST_UNSUPPORTED&&f==0x14b6f8&&tables.ray.native_queries==queries);unsigned before=c.masks;C(!gh_scene_visibility_step_00307b28(s,&b,&f)&&c.masks>before);unsigned hidden=0;for(unsigned i=0;i<32;i++)hidden+=tables.scroll_link_words[i]!=0;C(hidden);C(!gh_scene_visibility_next_call(s));memcpy(camera.fields+0x200,high,12);memcpy(camera.fields+0x210,high_to,12);before=c.clears;C(!gh_scene_visibility_step_00307b28(s,&b,&f)&&c.clears>=before+hidden);for(unsigned i=0;i<32;i++)C(!tables.scroll_link_words[i]);C(fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO&&!memcmp(raw,original,rn));gh_scene_visibility_destroy(s);}
 C(!fesetround(FE_TONEAREST));
 /* Exact unsigned kind ranges; only bit2 changes, adjacent flags stay poison. */
 const uint16_t kinds[]={0xff,0x100,0x1fe,0x1ff,0x200,0x2ff,0x300,0x4ff,0x500,0x5ff,0x600};for(unsigned k=0;k<sizeof(kinds)/sizeof(*kinds);k++){memcpy(player_bytes+0x2fe,kinds+k,2);put(player_bytes+0x5a0,0xffffffef);gh_scene_visibility*scope;C(!gh_scene_visibility_create(&scope));C(!gh_scene_visibility_step_00307b28(scope,&b,&f));unsigned eligible=(kinds[k]>0xff&&kinds[k]<0x1ff)||(kinds[k]>0x1ff&&kinds[k]<0x600);C(word(player_bytes+0x5a0)==(eligible?0xffffffed:0xffffffef));gh_scene_visibility_destroy(scope);}kind=0x100;memcpy(player_bytes+0x2fe,&kind,2);
 /* Actual attribute-linked actor name rows, no injected ray reports. */
 float actor_from[3],actor_to[3];unsigned actor_triangle=0;uint64_t actor_key=0;
 for(;actor_triangle<half(tables.akg.bytes+0x28);actor_triangle++){if(!request_for(&tables,actor_triangle,actor_from,actor_to,1))continue;const uint8_t*poly=tables.actor_links[4].view.bytes+(size_t)actor_triangle*20;const uint8_t*a=tables.actor_links[1].view.bytes+(size_t)half(poly+12)*16;gh_actor_scroll_entry entry;if(!gh_scene_actor_scroll_entry_001333b8(&tables,(uint16_t)(half(a+8)+a[10]),&entry)&&!entry.resolved&&entry.current_key){actor_key=entry.current_key;break;}}
 C(actor_key);memcpy(player_bytes+0x538,&actor_key,8);gh_scene_visibility*named;C(!gh_scene_visibility_create(&named));put(player_bytes+0x5a0,0);memcpy(camera.fields+0x200,actor_from,12);memcpy(camera.fields+0x210,actor_to,12);C(!gh_scene_visibility_step_00307b28(named,&b,&f)&&(word(player_bytes+0x5a0)&2));C(!gh_scene_visibility_next_call(named));float miss_from[]={1000,1000,1000},miss_to[]={1001,1000,1000};memcpy(camera.fields+0x200,miss_from,12);memcpy(camera.fields+0x210,miss_to,12);float qa_box[6]={0,0,0,1,2,3};memcpy(player_bytes+0x260,qa_box,24);put(player_bytes+0x260,0x7fc00000);put(player_bytes+0x5a0,0x12);C(gh_scene_visibility_step_00307b28(named,&b,&f)==GH_POST_NUMERIC&&f==0x1f92c0&&word(player_bytes+0x5a0)==0x10);uint64_t final_queries=tables.ray.native_queries;C(gh_scene_visibility_step_00307b28(named,&b,&f)==GH_POST_NUMERIC&&f==0x1f92c0&&tables.ray.native_queries==final_queries);memcpy(player_bytes+0x260,qa_box,24);C(!gh_scene_visibility_step_00307b28(named,&b,&f)&&tables.ray.native_queries==final_queries);gh_scene_visibility_destroy(named);
 /* Real dynamic box producer/update and130700 typed actor identity consumer. */
 gh_dynamic_corner_tables corners={0};C(!gh_dynamic_corners_from_elf(&corners,elf,en)&&!gh_dynamic_corners_initialize_00133670(&corners,1,0xffff));gh_om_door_shared shared={0};C(!gh_om_door_shared_from_elf(&shared,elf,en));gh_scene_dynamic_owner dynamic={0};C(!gh_scene_dynamic_bind(&dynamic,&tables,heap));float center[3],dfrom[3],dto[3];for(unsigned k=0;k<3;k++){center[k]=(from[k]+to[k])*.5f;dfrom[k]=dto[k]=center[k];}dfrom[0]-=3;dto[0]+=3;memcpy(player.alias_f0,center,12);kind=0x303;memcpy(player_bytes+0x2fe,&kind,2);put(player_bytes+0x5b8,word(player_bytes+0x5b8)|4u);float zero[3]={0},size[3]={2,2,2};struct gh_scene_dynamic_box*box;int original_nonnull;C(!gh_scene_dynamic_box_create_0012ead8(&dynamic,&player,player_bytes+0x80,zero,zero,size,shared.attributes,&box,&original_nonnull)&&original_nonnull);C(!gh_scene_dynamic_box_update_0012dcb8(&dynamic,box,&corners,GH_STATIC_BINARY32_REFERENCE,&f));
 for(unsigned m=0;m<4;m++){gh_scene_visibility*s;C(!gh_scene_visibility_create(&s));put(player_bytes+0x5a0,0);memcpy(camera.fields+0x200,dfrom,12);memcpy(camera.fields+0x210,dto,12);C(!fesetround(modes[m])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));gh_scene_post_status e=gh_scene_visibility_step_00307b28(s,&b,&f);if(e)fprintf(stderr,"dynamic e%d pc%X flags%X\n",e,f,word(player_bytes+0x5a0));C(!e&&(word(player_bytes+0x5a0)&2));C(!gh_scene_visibility_next_call(s));float high[3]={1000,1000,1000},high_to[3]={1001,1000,1000};memcpy(camera.fields+0x200,high,12);memcpy(camera.fields+0x210,high_to,12);C(!gh_scene_visibility_step_00307b28(s,&b,&f)&&!(word(player_bytes+0x5a0)&2));C(fegetround()==modes[m]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);gh_scene_visibility_destroy(s);}
 C(!fesetround(FE_TONEAREST));C(!gh_scene_dynamic_box_abort(&dynamic,box)&&!gh_scene_dynamic_release(&dynamic));
 /* Unknown previous slots and skip gate retain their precise observable prefix. */
 gh_scene_visibility*unknown;C(!gh_scene_visibility_create(&unknown));tables.scroll_link_words[0]=0xdeadbeef;put((uint8_t*)&work+0x5d8,0x200000);uint64_t queries=tables.ray.native_queries;C(!gh_scene_visibility_step_00307b28(unknown,&b,&f)&&tables.scroll_link_words[0]==0xdeadbeef&&tables.ray.native_queries==queries);C(!gh_scene_visibility_next_call(unknown));put((uint8_t*)&work+0x5d8,0);C(gh_scene_visibility_step_00307b28(unknown,&b,&f)==GH_POST_UNSUPPORTED&&f==0x14b6f8&&tables.scroll_link_words[0]==0xdeadbeef);tables.scroll_link_words[0]=0;gh_scene_visibility_destroy(unknown);
 kind500_backing_visibility(&c,&b,&camera,&work,&serial,actor_from,actor_to,actor_key);
 C(!memcmp(raw,original,rn));C(!gh_room_object_unregister(&pools,c.player_handle,&c));C(!gh_scene_actors_reset_0012e800(&tables));C(!gh_scene_meshes_release_prefix(&meshes)&&!gh_game_world_light_release_presets(light));free(light);gh_stage_textures_destroy(textures);C(!gh_game_room_pools_release(&pools)&&!gh_heap_destroy(heap));free(player_bytes);free(init);free(original);free(raw);free(elf);gh_disc_close(d);printf("PASS actualR100 156meshes staticpositive triangle%u/realray+32previouscompanion/registry actor flagclear/failedmask resume-noqueryreplay/FEnv/immutableassets; dynamic positive identity/class5B8bit4 consumer + unknownslot/skip; QAactor constructor not fullPlayerInit\n",selected);return 0;}
