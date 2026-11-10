#include "game_event_link.h"
#include <string.h>
static uint32_t w(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static unsigned h(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static const uint8_t*elf_at(const uint8_t*b,size_t n,uint32_t va,size_t bytes,int memory_only){
 if(!b||n<52||memcmp(b,"\177ELF\1\1\1",7)||h(b+18)!=8)return NULL;size_t ph=w(b+28);unsigned stride=h(b+42),count=h(b+44);if(stride<32||count>128||ph>n||(size_t)count*stride>n-ph)return NULL;
 const uint8_t*out=NULL;for(unsigned i=0;i<count;i++){const uint8_t*p=b+ph+(size_t)i*stride;uint32_t base=w(p+8),files=w(p+16),mem=w(p+20),off=w(p+4);if(w(p)!=1||va<base||(uint64_t)va+bytes>(uint64_t)base+(memory_only?mem:files))continue;if(files>mem||off>n||files>n-off||out)return NULL;/* memory-only identity needs no dereference, includingBSS. */out=memory_only?b:b+off+(va-base);}return out;
}
static const char*rel_name(const uint8_t*b,size_t n,uint32_t off){if(off>=n)return NULL;size_t remaining=n-off;if(remaining>128)remaining=128;return memchr(b+off,0,remaining)?(const char*)b+off:NULL;}
static const char*main_name(const uint8_t*b,size_t n,uint32_t va){for(size_t length=1;length<=128;length++){const uint8_t*p=elf_at(b,n,va,length,0);if(!p)return NULL;if(!p[length-1])return (const char*)p;}return NULL;}
static int main_resolve(const uint8_t*b,size_t n,const char*name,uint32_t*address){
 const uint8_t*head=elf_at(b,n,0x40a400,60,0);if(!head||memcmp(head,"SNR2",4))return 0;uint32_t base=w(head+12),count=w(head+16);if(count>4096)return 0;const uint8_t*symbols=elf_at(b,n,base,(size_t)count*12,0);if(!symbols)return 0;unsigned found=0;
 for(uint32_t i=0;i<count;i++){const uint8_t*p=symbols+(size_t)i*12;if(!w(p))continue;const char*s=main_name(b,n,w(p));if(!s)return 0;if(strcmp(s,name))continue;if((p[10]!=2&&p[10]!=3)||!w(p+4)||!elf_at(b,n,w(p+4),1,1))return 0;*address=w(p+4);found++;}return found==1;
}
static gh_game_event_status fail(gh_event_link*s,uint32_t pc,int relative,gh_game_event_status e){s->frontier=pc;s->frontier_relative=(uint8_t)relative;return e;}
static int code(const uint8_t*b,size_t n,uint32_t off,const uint32_t*words,size_t count){if(off>n||count>(n-off)/4)return 0;for(size_t i=0;i<count;i++)if(w(b+off+i*4)!=words[i])return 0;return 1;}
gh_game_event_status gh_game_event_link_002be890(gh_event_link*s,const gh_event_module*m,const void*elf,size_t en){
 static const uint32_t init[]={0x3402ffff,0x14a20007,0x00000000,0x10800005,0x3c020001,0x2442cf20,0xf8400010,0xf8400020,0xf8400030,0x03e00008,0x00000000,0x00000000};
 static const uint32_t ctor[]={0x27bdfff0,0x24040001,0xffbf0000,0x0c0011a2,0x3405ffff,0xdfbf0000,0x03e00008,0x27bd0010};
 static const uint32_t traversal[]={0x3c070001,0x27bdffd0,0x8ce5ca94,0x2402ffff,0xffbf0020,0x24e3ca94,0xffb10010,0x14a20011,0xffb00000,0x8c620004,0x1040000e,0x0000282d,0x24060001,0x00c0282d,0x24e4ca94,0x00000000,0x64a30001,0x0060302d,0x0006103c,0x0002103f,0x00021080,0x00441021,0x8c430000,0x5460fff8,0x00c0282d,0x0005883c,0x0011883f,0x1220000b,0x3c020001,0x00111880,0x2442ca94,0x00628021,0x8e020000,0x00000000,0x2631ffff,0x0040f809,0x2610fffc,0x5620fffc,0x8e020000,0xdfbf0020,0xdfb10010,0xdfb00000,0x03e00008,0x27bd0030};
 if(!s||!m||!m->loaded||!m->allocation.pointer||!elf)return GH_EVENT_ARGUMENT;
 if(s->bound)return s->module==m&&s->components.module==m->allocation.pointer&&s->module_job==m->job?GH_EVENT_OK:GH_EVENT_ARGUMENT;
 const uint8_t*b=m->allocation.pointer;size_t n=m->file_bytes;if(n<60||memcmp(b,"SNR2",4)||w(b+44)!=n)return fail(s,0x394dd8,0,GH_EVENT_FORMAT);
 uint32_t symbols=w(b+12),count=w(b+16),relocs=w(b+4),numrel=w(b+8);if(count!=393||numrel!=518||symbols>n||count>(n-symbols)/12||relocs>n||numrel>(n-relocs)/12)return fail(s,0x394dd8,0,GH_EVENT_FORMAT);
 /* Metadata staging precedes nativeconstructor writes or publication. */
 for(uint32_t i=0;i<count;i++){const uint8_t*p=b+symbols+(size_t)i*12;const char*name=w(p)?rel_name(b,n,w(p)):"";if(!name)return fail(s,0x394dd8,0,GH_EVENT_FORMAT);s->symbols[i]=(gh_event_symbol_binding){.name=name,.index=(uint16_t)i,.original_address=w(p+4),.first_reloc_site=UINT32_MAX};if(!i&&!name[0]&&!w(p+4)&&!p[10])continue;if(p[10]!=1&&p[10]!=2&&p[10]!=3)return fail(s,0x394dd8,0,GH_EVENT_FORMAT);s->symbols[i].imported=p[10]==1;}
 for(uint32_t i=0;i<numrel;i++){const uint8_t*p=b+relocs+(size_t)i*12;uint32_t site=w(p),addend=w(p+8);unsigned index=h(p+5);if((site&3)||site>n||4>n-site||index>=count||!s->symbols[index].imported||(p[4]!=4&&p[4]!=5&&p[4]!=6))return fail(s,site,1,GH_EVENT_FORMAT);
  uint32_t instruction=w(b+site),op=instruction>>26;if(p[4]==4){if(op!=3||(instruction&0x3ffffff)!=(addend>>2))return fail(s,site,1,GH_EVENT_FORMAT);}else if(p[4]==5){if(op!=15||(instruction&0xffff)!=(addend>>16))return fail(s,site,1,GH_EVENT_FORMAT);}else{if((op!=9&&op!=35&&op!=43&&op!=36&&op!=40&&op!=37&&op!=41&&op!=55)||(instruction&0xffff)!=(addend&0xffff))return fail(s,site,1,GH_EVENT_FORMAT);}
  for(uint32_t j=0;j<i;j++)if(s->relocations[j].site==site)return fail(s,site,1,GH_EVENT_FORMAT);
  s->relocations[i]=(gh_event_reloc_binding){.site=site,.addend=addend,.symbol=(uint16_t)index,.kind=p[4]};if(s->symbols[index].first_reloc_site==UINT32_MAX)s->symbols[index].first_reloc_site=site;}
 s->import_count=0;for(uint32_t i=1;i<count;i++){gh_event_symbol_binding*p=s->symbols+i;if(p->imported){uint32_t address;if(!main_resolve(elf,en,p->name,&address)){s->failed_symbol=(uint16_t)i;return fail(s,p->first_reloc_site==UINT32_MAX?0x394dd8:p->first_reloc_site,p->first_reloc_site!=UINT32_MAX,GH_EVENT_UNSUPPORTED);}p->original_address=address;s->import_count++;}else if(p->original_address>=n)return fail(s,p->original_address,1,GH_EVENT_FORMAT);p->identity_resolved=1;}
 for(uint32_t i=0;i<numrel;i++){gh_event_reloc_binding*p=s->relocations+i;uint64_t target=(uint64_t)s->symbols[p->symbol].original_address+p->addend;if(target>UINT32_MAX||!elf_at(elf,en,(uint32_t)target,1,1)||(p->kind==4&&(target&3)))return fail(s,p->site,1,GH_EVENT_BOUNDS);p->resolved_original_address=(uint32_t)target;}
 /* Actual __CTOR_LIST__ CA94 contains one constructor, reverse traversal
  * thereforecalls46B8 once. Never interpret or silentlyignore anotherbody. */
 if(n<0xcaa0||w(b+0xca94)!=1||w(b+0xca9c)){return fail(s,0xa890,1,GH_EVENT_UNSUPPORTED);}
 if(w(b+0xca98)!=0x46b8)return fail(s,w(b+0xca98),1,GH_EVENT_UNSUPPORTED);
 if(w(b+24)!=0xa890||!code(b,n,0xa890,traversal,44))return fail(s,0xa890,1,GH_EVENT_UNSUPPORTED);
 if(!code(b,n,0x4688,init,12)||!code(b,n,0x46b8,ctor,8))return fail(s,0x46b8,1,GH_EVENT_UNSUPPORTED);
 gh_game_event_status status=gh_event_components_bind(&s->components,m,elf,en);if(status)return fail(s,s->components.frontier?s->components.frontier:0x297680,0,status);
 for(unsigned i=0;i<15;i++){gh_event_component_view*v=s->components.components+i;if(v->original_offset>n||v->size>n-v->original_offset)return fail(s,v->original_offset,1,GH_EVENT_BOUNDS);memcpy(v->bytes,b+v->original_offset,v->size);for(uint32_t j=1;j<count;j++)if(!s->symbols[j].imported&&!strcmp(s->symbols[j].name,v->name))s->symbols[j].native_kind=GH_EVENT_BINDING_COMPONENT;}
 for(unsigned i=0;i<3;i++){/* Single actualstaticconstructor, VF0 isarchitecturalXYZ0/W1. */s->vectors_cf30_cf50[i][0]=s->vectors_cf30_cf50[i][1]=s->vectors_cf30_cf50[i][2]=0;s->vectors_cf30_cf50[i][3]=0x3f800000;}
 for(uint32_t i=1;i<count;i++)if(!s->symbols[i].imported){if(!strcmp(s->symbols[i].name,"setEventNo__9cEventEnvi")){s->symbols[i].native_kind=GH_EVENT_BINDING_SET_EVENT_NO;s->symbols[i].callable_native=1;}else if(s->symbols[i].original_address==0x46b8){s->symbols[i].native_kind=GH_EVENT_BINDING_STATIC_CTOR;s->symbols[i].callable_native=1;}}
 s->module=m;s->module_job=m->job;s->symbol_count=count;s->reloc_count=numrel;s->constructor_calls=1;s->constructors_done=1;s->bound=1;s->frontier=0;s->frontier_relative=0;return GH_EVENT_OK;
}
gh_game_event_status gh_game_event_link_symbol(const gh_event_link*s,const char*name,gh_event_symbol_binding*out){if(!s||!s->bound||!name||!out)return GH_EVENT_ARGUMENT;for(uint32_t i=1;i<s->symbol_count;i++)if(!strcmp(name,s->symbols[i].name)){*out=s->symbols[i];return GH_EVENT_OK;}return GH_EVENT_BOUNDS;}
gh_game_event_status gh_game_event_link_set_event_no_000054b8(gh_event_link*s,int32_t no){if(!s||!s->bound||!s->constructors_done)return GH_EVENT_ARGUMENT;memcpy(s->components.components[0].bytes,&no,4);s->components.event_no=no;return GH_EVENT_OK;}
gh_game_event_status gh_game_event_link_require_native(const gh_event_link*s,const char*name,uint32_t*f,int*relative){gh_event_symbol_binding binding;if(!f||!relative)return GH_EVENT_ARGUMENT;gh_game_event_status e=gh_game_event_link_symbol(s,name,&binding);if(e)return e;*f=binding.original_address;*relative=!binding.imported;return binding.callable_native?GH_EVENT_OK:GH_EVENT_UNSUPPORTED;}
