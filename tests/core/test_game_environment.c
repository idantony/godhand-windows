#include "game_scene_tables.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t *read_room(gh_disc*d,const char*path,size_t*n){size_t i;gh_disc_entry e;
 C(!gh_disc_find(d,path,&i)&&!gh_disc_entry_at(d,i,&e)&&e.size<=SIZE_MAX);*n=(size_t)e.size;
 uint8_t*p=malloc(*n);C(p&&!gh_disc_read(d,i,0,p,*n));return p;}
int main(int argc,char**argv){gh_disc*d;C(argc==2&&!gh_disc_open(argv[1],&d));
 size_t n;uint8_t*image=read_room(d,"st1/r100.dat",&n),*backup=malloc(n);C(backup);memcpy(backup,image,n);
 gh_bundle b;gh_bundle_entry expected;C(!gh_bundle_open(image,n,&b)&&!gh_bundle_find(&b,"ENV",0,&expected)&&expected.size==5184);
 gh_game_scene_tables s={0};gh_boot_work w,before;memset(&w,0xa5,sizeof(w));before=w;
 C(!gh_scene_environment_002a71b0(&s,&w,&b)&&!s.frontier&&s.environment.bytes==expected.bytes&&s.environment.size==expected.size);
 uint32_t token;memcpy(&token,(uint8_t*)&w+0x608,4);C(token==GH_SCENE_ENVIRONMENT_NATIVE_HANDLE);
 memcpy((uint8_t*)&before+0x608,&token,4);C(!memcmp(&before,&w,sizeof(w))&&!memcmp(backup,image,n));
 /* Retry binds the same original view; it neither decodes TIM2 nor allocates. */
 C(!gh_scene_environment_002a71b0(&s,&w,&b)&&s.environment.bytes==expected.bytes);
 gh_bundle malformed=b;malformed.size=12;gh_scene_table_view old=s.environment;before=w;
 C(gh_scene_environment_002a71b0(&s,&w,&malformed)==GH_TABLE_BOUNDS&&!memcmp(&w,&before,sizeof(w))&&s.environment.bytes==old.bytes);
 size_t empty_n;uint8_t*empty=read_room(d,"st0/r020.dat",&empty_n);gh_bundle empty_b;C(!gh_bundle_open(empty,empty_n,&empty_b));
 C(!gh_scene_environment_002a71b0(&s,&w,&empty_b)&&!s.environment.bytes&&!s.environment.size);memcpy(&token,(uint8_t*)&w+0x608,4);C(!token);
 free(empty);free(backup);free(image);gh_disc_close(d);puts("PASS actualR100 ENV view/nativehandle/immutabledata/no-neighbor-writes/R020absence/malformedbounds");return 0;
}
