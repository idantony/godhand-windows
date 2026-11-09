/* Independent native implementation of verified ISO9660/AFS/FST layouts.
 * Format evidence: LucasPicoli/god-hand-decomp scripts/afs.py (MIT), commit
 * eaae18d0dfab1f9a0df276ae5ca1581ad326faf3. No upstream parser code copied.
 * Local evidence: analysis/asset-layout.json and reference/GODHAND.FST.
 * FST size_hint is not authoritative: sizes always come from AFS slots. */
#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200809L
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#ifdef _WIN32
#include <windows.h>
#define SEEK64 _fseeki64
#define TELL64 _ftelli64
#else
#include <sys/types.h>
#define SEEK64 fseeko
#define TELL64 ftello
#endif

struct gh_disc {
    FILE *file;
    uint64_t size, afs_base, afs_size;
    uint32_t slots;
    unsigned char *afs, *fst, *visited;
    size_t fst_size, string_start, entry_start, count, capacity;
    gh_disc_entry *entries;
};
static uint32_t le32(const unsigned char *p) {
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static uint32_t be32(const unsigned char *p) {
    return (uint32_t)p[3] | (uint32_t)p[2]<<8 | (uint32_t)p[1]<<16 | (uint32_t)p[0]<<24;
}
static int fits(uint64_t off, uint64_t len, uint64_t total) {
    return off <= total && len <= total-off;
}
static gh_disc_error read_at(gh_disc *d, uint64_t off, void *buf, size_t n) {
    if (!fits(off,n,d->size) || off > INT64_MAX) return GH_DISC_BOUNDS;
    if (SEEK64(d->file,(int64_t)off,SEEK_SET)) return GH_DISC_IO;
    return fread(buf,1,n,d->file)==n ? GH_DISC_OK : GH_DISC_IO;
}
static int safe_path(const char *p) {
    const char *start;
    if (!p || !*p) return 0;
    start=p;
    for (;;) {
        unsigned char c=(unsigned char)*p;
        if (c==':' || (c && c<32)) return 0;
        if (!c || c=='/' || c=='\\') {
            size_t n=(size_t)(p-start);
            if (!n || (n==1 && start[0]=='.') || (n==2 && start[0]=='.' && start[1]=='.')) return 0;
            if (!c) return 1;
            start=p+1;
        }
        ++p;
    }
}
static unsigned char canon(unsigned char c) {
    if (c=='\\') return '/';
    return c>='A' && c<='Z' ? (unsigned char)(c+32) : c;
}
static int same_path(const char *a,const char *b) {
    while (*a && *b && canon((unsigned char)*a)==canon((unsigned char)*b)) {++a;++b;}
    return !*a && !*b;
}
static gh_disc_error add_entry(gh_disc *d,const char *path,uint32_t slot) {
    size_t i,n=strlen(path)+1;
    char *copy;
    gh_disc_entry *e;
    uint32_t off,len;
    if (slot>=d->slots) return GH_DISC_BOUNDS;
    for (i=0;i<d->count;i++) if(same_path(path,d->entries[i].path)) return GH_DISC_FORMAT;
    if(d->count==d->capacity) {
        size_t cap=d->capacity?d->capacity*2:128;
        void *p;
        if(cap>100000) return GH_DISC_FORMAT;
        p=realloc(d->entries,cap*sizeof(*d->entries));
        if(!p) return GH_DISC_MEMORY;
        d->entries=p;d->capacity=cap;
    }
    copy=malloc(n);if(!copy)return GH_DISC_MEMORY;memcpy(copy,path,n);
    off=le32(d->afs+8+slot*8);len=le32(d->afs+12+slot*8);
    e=&d->entries[d->count++];e->path=copy;e->afs_index=slot;e->iso_offset=d->afs_base+off;e->size=len;
    return GH_DISC_OK;
}
static gh_disc_error walk(gh_disc *d,uint32_t start,const char *prefix,unsigned depth) {
    size_t pos=start,table_size=d->fst_size-d->entry_start;
    if(depth>16 || start%12 || !fits(start,12,table_size))return GH_DISC_FORMAT;
    for(;;pos+=12) {
        const unsigned char *e;
        const char *name,*end;
        uint32_t no,val;
        char path[4096];
        size_t pn=strlen(prefix),nn;
        gh_disc_error err;
        if(!fits(pos,12,table_size) || d->visited[pos/12])return GH_DISC_FORMAT;
        d->visited[pos/12]=1;e=d->fst+d->entry_start+pos;
        if(e[0]==3)return GH_DISC_OK;
        if(e[0]!=1 && e[0]!=2)return GH_DISC_FORMAT;
        no=le32(e+4);val=le32(e+8);
        if(no>=d->entry_start-d->string_start)return GH_DISC_BOUNDS;
        name=(const char*)d->fst+d->string_start+no;
        end=memchr(name,0,d->entry_start-d->string_start-no);
        if(!end)return GH_DISC_FORMAT;
        nn=(size_t)(end-name);
        if(!safe_path(name) || strchr(name,'/') || strchr(name,'\\'))return GH_DISC_UNSAFE_PATH;
        if(pn+nn+2>sizeof(path))return GH_DISC_BOUNDS;
        memcpy(path,prefix,pn);if(pn)path[pn++]='/';memcpy(path+pn,name,nn+1);
        err=e[0]==1?walk(d,val,path,depth+1):add_entry(d,path,val);
        if(err)return err;
    }
}
void gh_disc_close(gh_disc *d) {
    size_t i;if(!d)return;
    if(d->file)fclose(d->file);
    for(i=0;i<d->count;i++)free((void*)d->entries[i].path);
    free(d->entries);free(d->afs);free(d->fst);free(d->visited);free(d);
}
gh_disc_error gh_disc_open(const char *path,gh_disc **out) {
    gh_disc *d;
    unsigned char pvd[2048],head[8],*root=NULL;
    uint64_t root_off=0,fst_off=0;
    uint32_t root_size=0;
    size_t pos,i;
    int64_t total;
    gh_disc_error err=GH_DISC_FORMAT;
    if(!out)return GH_DISC_ARGUMENT;
    *out=NULL;
    if(!path || !*path)return GH_DISC_ARGUMENT;
    d=calloc(1,sizeof(*d));if(!d)return GH_DISC_MEMORY;
#ifdef _WIN32
    {
        int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,NULL,0);
        wchar_t *wide=n>0?malloc((size_t)n*sizeof(*wide)):NULL;
        if(!wide){err=n>0?GH_DISC_MEMORY:GH_DISC_ARGUMENT;goto fail;}
        MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,wide,n);
        d->file=_wfopen(wide,L"rb");free(wide);
    }
#else
    d->file=fopen(path,"rb");
#endif
    if(!d->file){err=GH_DISC_IO;goto fail;}
    if(SEEK64(d->file,0,SEEK_END) || (total=TELL64(d->file))<0){err=GH_DISC_IO;goto fail;}
    d->size=(uint64_t)total;
    for(i=16;i<80;i++) {
        err=read_at(d,i*2048,pvd,sizeof(pvd));if(err)goto fail;
        if(pvd[0]==1 && !memcmp(pvd+1,"CD001",5))break;
    }
    err=GH_DISC_FORMAT;
    if(i==80 || pvd[128]!=0 || pvd[129]!=8 || pvd[130]!=8 || pvd[131]!=0)goto fail;
    if(pvd[156]<34)goto fail;
    root_off=(uint64_t)le32(pvd+158)*2048;root_size=le32(pvd+166);
    if(le32(pvd+158)!=be32(pvd+162) || root_size!=be32(pvd+170) || root_size>32*1024*1024)goto fail;
    root=malloc(root_size);if(!root){err=GH_DISC_MEMORY;goto fail;}
    err=read_at(d,root_off,root,root_size);if(err)goto fail;
    for(pos=0;pos<root_size;) {
        unsigned n=root[pos],nl;
        const unsigned char *r=root+pos;
        uint32_t lba,len;
        char name[256];char *semi;
        if(!n){pos=(pos/2048+1)*2048;continue;}
        err=GH_DISC_FORMAT;
        if(n<34 || !fits(pos,n,root_size) || n>2048-pos%2048)goto fail;
        nl=r[32];if(nl>n-33)goto fail;
        lba=le32(r+2);len=le32(r+10);
        if(lba!=be32(r+6)||len!=be32(r+14))goto fail;
        if(!fits((uint64_t)lba*2048,len,d->size)){err=GH_DISC_BOUNDS;goto fail;}
        if(!(r[25]&2)) {
            memcpy(name,r+33,nl);name[nl]=0;semi=strchr(name,';');if(semi)*semi=0;
            if(same_path(name,"_DATA.AFS") || same_path(name,"GODHAND.FST")) {
                if(r[25]&0x80)goto fail;
                if(same_path(name,"_DATA.AFS")) {
                    if(d->afs_base)goto fail;
                    d->afs_base=(uint64_t)lba*2048;d->afs_size=len;
                }else {if(fst_off)goto fail;fst_off=(uint64_t)lba*2048;d->fst_size=len;}
            }
        }
        pos+=n;
    }
    free(root);root=NULL;err=GH_DISC_FORMAT;
    if(!d->afs_base || !fst_off || d->fst_size<16 || d->fst_size>4*1024*1024)goto fail;
    err=read_at(d,d->afs_base,head,8);if(err)goto fail;
    err=GH_DISC_FORMAT;
    if(memcmp(head,"AFS\0",4) || !(d->slots=le32(head+4)) || d->slots>100000)goto fail;
    if(!fits(0,8+(uint64_t)d->slots*8,d->afs_size)){err=GH_DISC_BOUNDS;goto fail;}
    d->afs=malloc(8+(size_t)d->slots*8);d->fst=malloc(d->fst_size);
    if(!d->afs||!d->fst){err=GH_DISC_MEMORY;goto fail;}
    err=read_at(d,d->afs_base,d->afs,8+(size_t)d->slots*8);if(err)goto fail;
    for(i=0;i<d->slots;i++) {
        uint32_t off=le32(d->afs+8+i*8),len=le32(d->afs+12+i*8);
        size_t j;
        if(!fits(off,len,d->afs_size) || (len && off<8+d->slots*8)){err=GH_DISC_BOUNDS;goto fail;}
        for(j=0;len && j<i;j++) {
            uint32_t other=le32(d->afs+8+j*8),other_len=le32(d->afs+12+j*8);
            if(other_len && (uint64_t)off<(uint64_t)other+other_len &&
               (uint64_t)other<(uint64_t)off+len){err=GH_DISC_FORMAT;goto fail;}
        }
    }
    err=read_at(d,fst_off,d->fst,d->fst_size);if(err)goto fail;
    err=GH_DISC_FORMAT;
    if(memcmp(d->fst,"FST\0",4)||le32(d->fst+4)!=0x3f800000)goto fail;
    d->string_start=le32(d->fst+8);d->entry_start=le32(d->fst+12);
    if(d->string_start<16 || d->string_start>=d->entry_start || d->entry_start>d->fst_size)goto fail;
    d->visited=calloc((d->fst_size-d->entry_start)/12+1,1);
    if(!d->visited){err=GH_DISC_MEMORY;goto fail;}
    err=walk(d,0,"",0);if(err)goto fail;
    *out=d;return GH_DISC_OK;
fail:
    free(root);gh_disc_close(d);return err;
}
size_t gh_disc_count(const gh_disc *d){return d?d->count:0;}
gh_disc_error gh_disc_entry_at(const gh_disc *d,size_t i,gh_disc_entry *out) {
    if(!d||!out)return GH_DISC_ARGUMENT;
    if(i>=d->count)return GH_DISC_BOUNDS;
    *out=d->entries[i];return GH_DISC_OK;
}
gh_disc_error gh_disc_find(const gh_disc *d,const char *path,size_t *index) {
    size_t i;if(!d||!index)return GH_DISC_ARGUMENT;
    if(!safe_path(path))return GH_DISC_UNSAFE_PATH;
    for(i=0;i<d->count;i++)if(same_path(path,d->entries[i].path)){*index=i;return GH_DISC_OK;}
    return GH_DISC_NOT_FOUND;
}
gh_disc_error gh_disc_read(gh_disc *d,size_t i,uint64_t off,void *buffer,size_t n) {
    if(!d||(!buffer&&n))return GH_DISC_ARGUMENT;
    if(i>=d->count || !fits(off,n,d->entries[i].size))return GH_DISC_BOUNDS;
    if(!n)return GH_DISC_OK;
    return read_at(d,d->entries[i].iso_offset+off,buffer,n);
}
const char *gh_disc_error_string(gh_disc_error e) {
    static const char *const names[]={"success","invalid argument","file I/O failure","unsupported or malformed disc format","offset or index out of bounds","resource not found","allocation failed","unsafe resource path"};
    return (unsigned)e<sizeof(names)/sizeof(names[0])?names[e]:"unknown disc error";
}
