#include "../../src/core/game_scene_setup.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
static uint32_t get32(const uint8_t *p){uint32_t n;memcpy(&n,p,4);return n;}
typedef struct {gh_boot_work *work;gh_scene_scenario_view scenario;uint8_t *em;unsigned calls;int bad;} context;
static gh_scene_status before(void *p,uint32_t *f){context*c=p;(void)f;c->calls++;if(*c->em||c->scenario.bytes[0x14]!=0xa5)c->bad=1;c->work->flags|=0x200;return GH_SCENE_OK;}
static gh_scene_status init(void *p,uint32_t*f){context*c=p;(void)f;c->calls++;if(get32((uint8_t*)c->work+0x610)!=0xffff||c->scenario.manager->flags!=0xfffffffcu||c->scenario.bytes[0x108]||get32(c->scenario.bytes+0xec))c->bad=1;return GH_SCENE_OK;}
int main(void){uint8_t raw[0x114],em=7;gh_boot_work w;gh_task_manager m;uint32_t f;context c;
 memset(&w,0,sizeof(w));memset(&m,0,sizeof(m));memset(raw,0xa5,sizeof(raw));m.flags=0xffffffffu;
 gh_scene_scenario_view s={raw,sizeof(raw),&m};c=(context){&w,s,&em,0,0};
 CHECK(gh_scene_before_objects_002c33d8(s,&w,&em,(gh_scene_hook){GH_SCENE_HOOK_NATIVE,before,&c},&f)==GH_SCENE_OK);CHECK(c.calls==1&&!c.bad&&raw[0x14]==0);
 memset(raw,0xa5,sizeof(raw));w.flags=0;em=9;
 CHECK(gh_scene_before_objects_002c33d8(s,&w,&em,(gh_scene_hook){GH_SCENE_HOOK_UNRESOLVED,0,0},&f)==GH_SCENE_UNSUPPORTED);CHECK(!em&&raw[0x14]==0xa5&&f==0x2c3400);
 CHECK(gh_scene_initialize_room_002c3440(s,&w,(gh_scene_hook){GH_SCENE_HOOK_NATIVE,init,&c},&f)==GH_SCENE_OK);CHECK(c.calls==2&&!c.bad);
 /* Every untouched byte, including padding and guest manager storage, survives. */
 for(unsigned i=0;i<sizeof(raw);i++){int touched=(i>=0x112)||(i>=0x104&&i<0x109)||(i>=0xec&&i<0x104)||(i>=0xe8&&i<0xea);if(!touched)CHECK(raw[i]==0xa5);}
 CHECK(raw[0x112]==0xff&&raw[0x113]==0xff&&raw[0x109]==0xa5);
 memset(raw,0xa5,sizeof(raw));m.flags=7;((uint8_t*)&w)[0x590]=1;
 CHECK(gh_scene_initialize_room_002c3440(s,&w,(gh_scene_hook){GH_SCENE_HOOK_UNRESOLVED,0,0},&f)==GH_SCENE_OK);CHECK(m.flags==4&&f==0&&c.calls==2);
 ((uint8_t*)&w)[0x590]=0;
 CHECK(gh_scene_initialize_room_002c3440(s,&w,(gh_scene_hook){GH_SCENE_HOOK_UNRESOLVED,0,0},&f)==GH_SCENE_UNSUPPORTED);CHECK(f==0x2c34d8);
 s.size=0x113;raw[0x108]=7;CHECK(gh_scene_initialize_room_002c3440(s,&w,(gh_scene_hook){0},&f)==GH_SCENE_ARGUMENT);CHECK(raw[0x108]==7);
 puts("game_scene_setup original ordering/preservation/hook gates PASS");return 0;}
