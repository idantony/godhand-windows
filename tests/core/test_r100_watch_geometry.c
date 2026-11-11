#include "r100_watch_geometry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(int argc,char **argv)
{
 CHECK(argc==2);
 gh_disc *disc;
 CHECK(!gh_disc_open(argv[1],&disc));
 size_t i;
 gh_disc_entry file;
 CHECK(!gh_disc_find(disc,"st1/r100.dat",&i)&&!gh_disc_entry_at(disc,i,&file));
 size_t elf_size=(size_t)gh_disc_boot_elf_size(disc);
 uint8_t *elf=malloc(elf_size),*data=malloc((size_t)file.size);
 CHECK(elf&&data&&!gh_disc_read_boot_elf(disc,0,elf,elf_size)&&!gh_disc_read(disc,i,0,data,(size_t)file.size));
 gh_bundle room;
 gh_game_scene_tables tables={0};
 CHECK(!gh_bundle_open(data,(size_t)file.size,&room)&&!gh_scene_actors_0012e670(&tables,&room));
 const float p[4]={100,20,100,1},e[4]={100,20,102,1};
 const float pi=0x1.921fb6p+1f;
 uint64_t count=tables.ray.native_queries;
 int payload=-1;
 uint32_t pc=99;
 CHECK(!fesetround(FE_UPWARD)&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));
 CHECK(!gh_r100_at1_geometry(elf,elf_size,&tables,p,e,pi,&payload,&pc)&&payload&&!pc);
 CHECK(tables.ray.native_queries==count);
 CHECK(!gh_r100_at1_geometry(elf,elf_size,&tables,p,e,pi*.25f,&payload,&pc)&&payload&&!pc);
 CHECK(tables.ray.native_queries==count);
 CHECK(!gh_r100_at1_geometry(elf,elf_size,&tables,p,e,0,&payload,&pc)&&!payload&&!pc);
 CHECK(tables.ray.native_queries==count+1&&tables.ray.mask174==7&&!tables.ray.flags178&&
       tables.ray.mode181&&tables.ray.fromb0[1]==0x41a80000&&tables.ray.toc0[1]==0x41a80000);
 count=tables.ray.native_queries;
 CHECK(!gh_r100_at1_geometry(elf,elf_size,&tables,p,p,0,&payload,&pc)&&payload&&!pc&&tables.ray.native_queries==count);
 CHECK(fegetround()==FE_UPWARD&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);
 CHECK(!fesetround(FE_TONEAREST));
 /* Find a real R100 vertical segment hitting AKG with the exact mask.
    Coincident XZ faces only at the original negative quarter-turn. */
 int found=0;
 for(int x=-30;x<=30&&!found;x+=2)for(int z=-30;z<=30&&!found;z+=2){
  gh_scene_ray_request ray={.from={(float)x,4,(float)z,1},.to={(float)x,-4,(float)z,1},.mask=7,.segmented=1};
  int hit;
  if(gh_scene_ray_query_00131e10(&tables,&ray,&hit,&pc)||!hit)continue;
  float enemy[4]={(float)x,3,(float)z,1},player[4]={(float)x,-5,(float)z,1};
  count=tables.ray.native_queries;
  CHECK(!gh_r100_at1_geometry(elf,elf_size,&tables,player,enemy,-pi*.5f,&payload,&pc)&&payload&&!pc);
  CHECK(tables.ray.native_queries>count);
  found=1;
 }
 CHECK(found&&!gh_scene_actors_reset_0012e800(&tables));
 gh_disc_close(disc);free(data);free(elf);
 puts("PASS real R100 REL2920 direction cutoff, clear retry, segmented blocked payload, lifted Y and FEnv");
 return 0;
}
