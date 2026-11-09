#include "../../src/platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);return 1;} }while(0)
static int contains(const unsigned char *data,size_t n,const char *needle) {
    size_t i,len=strlen(needle)+1;
    for(i=0;i+len<=n;i++)if(!memcmp(data+i,needle,len))return 1;
    return 0;
}
int main(int argc,char **argv) {
    gh_disc *d=NULL;
    gh_disc_entry e;
    size_t i,index,other,overlays=0;
    unsigned char raw[1720],ref[1721],sentinel=0x7b;
    FILE *f;
    gh_disc_error err;
    CHECK(argc==3);
    err=gh_disc_open(argv[1],&d);
    if(err){fprintf(stderr,"open: %s\n",gh_disc_error_string(err));return 1;}
    CHECK(gh_disc_count(d)==1445);
    for(i=0;i<gh_disc_count(d);i++) {
        CHECK(gh_disc_entry_at(d,i,&e)==GH_DISC_OK);
        CHECK(gh_disc_find(d,e.path,&other)==GH_DISC_OK && other==i);
        if(strlen(e.path)>=4 && !strcmp(e.path+strlen(e.path)-4,".rel"))overlays++;
    }
    CHECK(overlays==81);
    CHECK(gh_disc_find(d,"dll/debugoptimize/r207.rel",&index)==GH_DISC_OK);
    CHECK(gh_disc_find(d,"DLL\\DEBUGOPTIMIZE\\R207.REL",&other)==GH_DISC_OK && index==other);
    CHECK(gh_disc_entry_at(d,index,&e)==GH_DISC_OK);
    CHECK(e.afs_index==1221 && e.size==1720 && e.iso_offset==UINT64_C(1112633344));
    CHECK(gh_disc_read(d,index,0,raw,sizeof(raw))==GH_DISC_OK);
    CHECK(!memcmp(raw,"SNR2",4));
    CHECK(contains(raw,sizeof(raw),"R207Init__Fv"));
    CHECK(contains(raw,sizeof(raw),"R207Move__Fv"));
    f=fopen(argv[2],"rb");CHECK(f);
    CHECK(fread(ref,1,sizeof(ref),f)==sizeof(raw));CHECK(!ferror(f));fclose(f);
    CHECK(!memcmp(raw,ref,sizeof(raw)));
    CHECK(gh_disc_find(d,"../r207.rel",&other)==GH_DISC_UNSAFE_PATH);
    CHECK(gh_disc_find(d,"dll/../r207.rel",&other)==GH_DISC_UNSAFE_PATH);
    CHECK(gh_disc_find(d,"C:\\r207.rel",&other)==GH_DISC_UNSAFE_PATH);
    CHECK(gh_disc_find(d,"/dll/r207.rel",&other)==GH_DISC_UNSAFE_PATH);
    CHECK(gh_disc_find(d,"dll//r207.rel",&other)==GH_DISC_UNSAFE_PATH);
    CHECK(gh_disc_find(d,"absent.rel",&other)==GH_DISC_NOT_FOUND);
    CHECK(gh_disc_entry_at(d,SIZE_MAX,&e)==GH_DISC_BOUNDS);
    CHECK(gh_disc_read(d,SIZE_MAX,0,&sentinel,1)==GH_DISC_BOUNDS);
    CHECK(gh_disc_read(d,index,1720,&sentinel,1)==GH_DISC_BOUNDS);
    CHECK(gh_disc_read(d,index,UINT64_MAX,&sentinel,1)==GH_DISC_BOUNDS);
    CHECK(sentinel==0x7b);
    CHECK(gh_disc_read(d,index,1720,NULL,0)==GH_DISC_OK);
    CHECK(gh_disc_read(d,index,0,NULL,1)==GH_DISC_ARGUMENT);
    gh_disc_close(d);gh_disc_close(NULL);
    CHECK(gh_disc_open(NULL,&d)==GH_DISC_ARGUMENT && !d);
    puts("PASS native ISO/FST/AFS: 1445 paths, 81 overlays; r207 exact bytes and symbols; unsafe paths and bounded reads");
    return 0;
}
