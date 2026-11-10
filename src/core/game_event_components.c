#include "game_event_components.h"
#include <string.h>
static uint32_t w(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint16_t h(const uint8_t*p){return (uint16_t)(p[0]|(unsigned)p[1]<<8);}
static const uint8_t*elf_at(const uint8_t*b,size_t n,uint32_t va,size_t bytes){
 if(!b||n<52||memcmp(b,"\177ELF\1\1\1",7)||h(b+18)!=8)return NULL;
 size_t ph=w(b+28);unsigned stride=h(b+42),count=h(b+44);if(stride<32||count>128||ph>n||(size_t)count*stride>n-ph)return NULL;
 const uint8_t*out=NULL;for(unsigned i=0;i<count;i++){const uint8_t*p=b+ph+(size_t)i*stride;uint32_t base=w(p+8),files=w(p+16),off=w(p+4);if(w(p)!=1||va<base||(uint64_t)va+bytes>(uint64_t)base+files)continue;if(off>n||files>n-off||out)return NULL;out=b+off+(va-base);}return out;
}
static int module_symbol(const uint8_t*b,size_t n,const char*name,uint32_t*offset){
 if(n<60||memcmp(b,"SNR2",4)||w(b+44)!=n)return 0;uint32_t table=w(b+12),count=w(b+16);if(table>n||count>(n-table)/12)return 0;
 unsigned found=0;size_t len=strlen(name)+1;for(uint32_t i=0;i<count;i++){const uint8_t*p=b+table+(size_t)i*12;uint32_t str=w(p);if(str>n||len>n-str||memcmp(b+str,name,len))continue;if(p[10]!=2)return 0;*offset=w(p+4);found++;}return found==1;
}
static int main_import(const uint8_t*b,size_t n,uint32_t site,uint8_t kind,const char*name){
 const uint8_t*header=elf_at(b,n,0x40a400,60);if(!header||memcmp(header,"SNR2",4))return 0;
 uint32_t rel=w(header+4),count=w(header+8),symbols=w(header+12),symbol_count=w(header+16);if(count>4096||symbol_count>4096)return 0;
 const uint8_t*records=elf_at(b,n,rel,(size_t)count*12),*syms=elf_at(b,n,symbols,(size_t)symbol_count*12);if(!records||!syms)return 0;
 unsigned found=0;for(uint32_t i=0;i<count;i++){const uint8_t*p=records+(size_t)i*12;if(w(p)!=site||p[4]!=kind)continue;unsigned index=h(p+5);if(index>=symbol_count||w(p+8))return 0;const uint8_t*s=syms+(size_t)index*12;const uint8_t*str=elf_at(b,n,w(s),strlen(name)+1);if(!str||memcmp(str,name,strlen(name)+1)||w(s+4)||s[10]!=1)return 0;found++;}return found==1;
}
gh_game_event_status gh_event_components_bind(gh_event_components*s,const gh_event_module*m,const void*elf,size_t bytes){
 static const char*const names[]={"EventEnv","EventCamera","EventActor","EventVertex","EventEffect","EventScrLight","EventObjLight","EventFilter","EventSe","EventBgm","EventScrLayer","EventVib","EventWait","EventFade","EventAdditionalLight"};
 static const uint32_t offsets[]={0xd200,0xd1c8,0xcf00,0xda08,0xd1e8,0xd910,0xd860,0xd268,0xd9f0,0xd1b0,0xd898,0xda20,0xda40,0xd248,0xcf90};
 static const uint16_t sizes[]={0x44,0x18,0x1c,0x18,0x18,0xdc,0x34,0x5e8,0x18,0x18,0x78,0x1c,0x18,0x1c,0x1c};
 static const uint32_t hi[]={0x297690,0x2976ac,0x2976c0,0x2976d4,0x2976e8,0x2976fc,0x297710,0x297724,0x297738,0x29774c,0x297760,0x297774,0x297788,0x29779c,0x2977b0};
 if(!s||!m||!m->loaded||!m->allocation.pointer||!elf)return GH_EVENT_ARGUMENT;
 if(s->bound)return s->module==m->allocation.pointer&&s->module_size==m->file_bytes?GH_EVENT_OK:GH_EVENT_ARGUMENT;
 const uint8_t*raw=m->allocation.pointer;uint32_t address;
 for(unsigned i=0;i<15;i++){if(!module_symbol(raw,m->file_bytes,names[i],&address)||address!=offsets[i]||!main_import(elf,bytes,hi[i],5,names[i])||!main_import(elf,bytes,hi[i]+(i?8:4),6,names[i])){s->frontier=hi[i];return GH_EVENT_FORMAT;}}
 if(!module_symbol(raw,m->file_bytes,"setEventNo__9cEventEnvi",&address)||address!=0x54b8||m->file_bytes<0x54c0||w(raw+0x54b8)!=0x03e00008||w(raw+0x54bc)!=0xac850000||!main_import(elf,bytes,0x2977c8,4,"setEventNo__9cEventEnvi")){s->frontier=0x2977c8;return GH_EVENT_FORMAT;}
 size_t cursor=0;for(unsigned i=0;i<15;i++){s->components[i]=(gh_event_component_view){s->storage+cursor,sizes[i],offsets[i],names[i]};cursor+=sizes[i];}
 s->module=raw;s->module_size=m->file_bytes;s->set_event_no_offset=address;s->bound=1;s->frontier=0;return GH_EVENT_OK;
}
gh_game_event_status gh_event_components_construct_00297680(gh_event_components*s,int32_t no){
 if(!s||!s->bound)return GH_EVENT_ARGUMENT;for(unsigned i=0;i<15;i++)memset(s->components[i].bytes,0,s->components[i].size);
 memcpy(s->components[0].bytes,&no,4);s->event_no=no;s->constructed=1;s->frontier=0;return GH_EVENT_OK;
}
gh_game_event_status gh_event_component_at(gh_event_components*s,unsigned i,gh_event_component_view*out){if(!s||!out||!s->bound)return GH_EVENT_ARGUMENT;if(i>=15)return GH_EVENT_BOUNDS;*out=s->components[i];return GH_EVENT_OK;}
