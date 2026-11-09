#include "mc_file_load.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(void){
 gh_heap *heap;gh_mc_boot_checks s;gh_boot_work w={0};uint32_t frontier,flags;
 C(gh_heap_create(0x10000,&heap)==GH_MEM_OK && gh_mc_boot_checks_create(&s,heap)==GH_MC_BOOT_OK);
 s.phase=2;C(gh_mc_file_load_00000730(&s,&w,0,&frontier)==GH_MC_BOOT_OK);
 C(!frontier && s.phase==3 && s.error==2 && !s.frame && !s.subphase && !s.done && s.load_index==20);
 memcpy(&flags,w.pad588,4);C(!flags && !(w.flags&4));
 s.phase=2;s.load_index=0;s.present_mask=1u<<7;
 C(gh_mc_file_load_00000730(&s,&w,0,&frontier)==GH_MC_BOOT_UNSUPPORTED && frontier==0x3350 && s.load_index==7);
 /* Real record bytes after loads would be aggregated even when older selected
  * bits are below current index. This verifies every mapping, not fake I/O. */
 s.load_index=20;unsigned char *p=s.icon_work.pointer;p[8]=0xff;p[9]=3;p[0x30]=99;
 C(gh_mc_file_load_00000730(&s,&w,0,&frontier)==GH_MC_BOOT_OK && s.error==10 && s.phase==3 && (w.flags&4));
 memcpy(&flags,w.pad588,4);C(flags==0xff600000);
 s.phase=2;C(gh_mc_file_load_00000730(&s,&w,1,&frontier)==GH_MC_BOOT_OK && s.done && s.phase==2);
 C(gh_mc_boot_checks_release(&s,heap)==GH_MC_BOOT_OK && gh_heap_destroy(heap)==GH_MEM_OK);
 puts("PASS original empty-file scan and twenty-record system flag aggregation; selected file open stays explicit frontier, no fabricated read");return 0;
}
