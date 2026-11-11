#include "game_player_node_cache.h"
#include <string.h>
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static unsigned u16(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static uint64_t u64(const uint8_t*p){uint64_t v=0;for(unsigned i=0;i<8;i++)v|=(uint64_t)p[i]<<(i*8);return v;}
static void ww(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
gh_player_nodes_status gh_actor_mesh_name_key_0014b730(const uint8_t*name,size_t bytes,uint64_t*out){if(!name||!out)return GH_PLAYER_NODES_ARGUMENT;uint64_t key=0;for(unsigned i=0;i<8;i++){if(i>=bytes)return GH_PLAYER_NODES_BOUNDS;unsigned c=name[i];if(!c){*out=key;return GH_PLAYER_NODES_OK;}int64_t signed_byte=c<128?(int64_t)c:(int64_t)c-256;key|=(uint64_t)signed_byte<<(i*8);}*out=key;return GH_PLAYER_NODES_OK;}
gh_player_nodes_status gh_actor_model_find_mesh_0014b730(gh_actor_model*m,const uint8_t*name,size_t bytes,gh_player_mesh_reference*out,int*original){
 if(!m||!m->bound||!m->root.bytes||!out||!original)return GH_PLAYER_NODES_ARGUMENT;*original=0;*out=(gh_player_mesh_reference){0};uint64_t key;gh_player_nodes_status e=gh_actor_mesh_name_key_0014b730(name,bytes,&key);if(e)return e;if(!m->nodes)return GH_PLAYER_NODES_OK;
 /*14B640 appendsinScriptorder. Followexistingtypednode sequence; no treecopy. */
 for(unsigned i=0;i<m->script.mesh_count;i++){const uint8_t*node;size_t mesh_index;gh_actor_model_status status=gh_actor_model_node_at(m,i,&node,&mesh_index);if(status==GH_ACTOR_MODEL_BOUNDS)return GH_PLAYER_NODES_OK;if(status)return GH_PLAYER_NODES_UNSUPPORTED;gh_model_mesh mesh;if(gh_model_mesh_at(&m->script,mesh_index,&mesh))return GH_PLAYER_NODES_BOUNDS;if(u64((const uint8_t*)mesh.name)==key){*out=(gh_player_mesh_reference){m,node,mesh_index};*original=1;return GH_PLAYER_NODES_OK;}}
 return GH_PLAYER_NODES_OK;
}
gh_player_nodes_status gh_actor_model_set_mesh_visible_0014a338(gh_actor_model *model,
    const uint8_t *name, size_t name_bytes, int visible, int *original_found) {
    gh_player_mesh_reference mesh;
    int found;
    if (!original_found) return GH_PLAYER_NODES_ARGUMENT;
    *original_found = 0;
    gh_player_nodes_status status = gh_actor_model_find_mesh_0014b730(model, name, name_bytes, &mesh, &found);
    if (status || !found) return status;
    if (gh_actor_model_node_mask_flags(model, (unsigned)mesh.mesh_index, 0x380, ~1u, !visible))
        return GH_PLAYER_NODES_UNSUPPORTED;
    *original_found = 1;
    return GH_PLAYER_NODES_OK;
}

static int mapped(const uint8_t*elf,size_t bytes,uint32_t table,unsigned stride,unsigned count,uint32_t va,uint8_t*out,size_t n){for(unsigned i=0;i<count;i++){const uint8_t*p=elf+table+(size_t)i*stride;uint32_t off=u32(p+4),base=u32(p+8),files=u32(p+16);if(u32(p)!=1||va<base||(uint64_t)va+n>(uint64_t)base+files)continue;if(off>bytes||files>bytes-off)return 0;memcpy(out,elf+off+(va-base),n);return 1;}return 0;}
gh_player_nodes_status gh_player_node_names_from_elf(gh_player_node_names*s,const void*raw,size_t bytes){if(!s||!raw)return GH_PLAYER_NODES_ARGUMENT;const uint8_t*elf=raw;if(bytes<52)return GH_PLAYER_NODES_BOUNDS;if(memcmp(elf,"\177ELF\1\1\1",7)||u16(elf+18)!=8)return GH_PLAYER_NODES_UNSUPPORTED;uint32_t table=u32(elf+28);unsigned stride=u16(elf+42),count=u16(elf+44);if(stride<32||count>128||table>bytes||count>(bytes-table)/stride)return GH_PLAYER_NODES_BOUNDS;
 uint8_t instruction[4];if(!mapped(elf,bytes,table,stride,count,0x14b730,instruction,4))return GH_PLAYER_NODES_BOUNDS;if(u32(instruction)!=0x80a20000)return GH_PLAYER_NODES_UNSUPPORTED;gh_player_node_names n={0};for(unsigned i=0;i<15;i++)if(!mapped(elf,bytes,table,stride,count,0x41b828+i*8,n.names[i],8))return GH_PLAYER_NODES_BOUNDS;n.initialized=1;*s=n;return GH_PLAYER_NODES_OK;}
static gh_player_nodes_status fail(gh_game_player_node_cache*s,uint32_t pc,gh_player_nodes_status e){s->frontier=pc;return e;}
static gh_player_nodes_status lookup(gh_game_player_node_cache*s,unsigned name,gh_player_mesh_reference*out){int present;gh_player_nodes_status e=gh_actor_model_find_mesh_0014b730(s->model,s->names->names[name],8,out,&present);return e?fail(s,0x14b730,e):GH_PLAYER_NODES_OK;}
static gh_player_nodes_status flag(gh_game_player_node_cache*s,const gh_player_mesh_reference*v,uint16_t offset,uint32_t bits){if(!v->bytes)return GH_PLAYER_NODES_OK;if(v->owner!=s->model)return fail(s,0x107f58,GH_PLAYER_NODES_ARGUMENT);gh_actor_model_status e=gh_actor_model_node_or_flags(v->owner,(unsigned)v->mesh_index,offset,bits);return e?fail(s,0x107f58,GH_PLAYER_NODES_UNSUPPORTED):GH_PLAYER_NODES_OK;}
gh_player_nodes_status gh_game_player_node_cache_initialize_00107f58(gh_game_player_node_cache*s,gh_game_title_player*p,gh_actor_model*m,const gh_player_node_names*names,gh_core_save_view save){
 if(!s||!p||!p->view.bytes||p->view.size<GH_TITLE_PLAYER_BYTES||!m||m->root.bytes!=p->view.bytes||!names||!names->initialized||!save.bytes||save.size<GH_CORE_SAVE_BYTES)return GH_PLAYER_NODES_ARGUMENT;uint8_t costume=gh_core_save_costume(save);if(s->bound&&(s->player!=p||s->model!=m||s->names!=names||s->costume!=costume))return GH_PLAYER_NODES_ARGUMENT;if(s->ready)return GH_PLAYER_NODES_OK;if(!s->bound){s->player=p;s->model=m;s->names=names;s->costume=costume;s->bound=1;}
 const uint16_t offsets[]={0x1550,0x1574,0x1554,0x1558,0x155c,0x1560,0x1564,0x1568,0x156c,0x1570};ww(p->view.bytes+0x1550,0);for(unsigned i=0;i<10;i++){gh_player_nodes_status e=lookup(s,i,&s->nodes[i]);if(e)return e;ww(p->view.bytes+offsets[i],0);if(i==0&&(costume==1||costume==3||costume==5||costume==7)){e=flag(s,&s->nodes[0],0x380,1);if(e)return e;s->nodes[0]=(gh_player_mesh_reference){0};}}
 gh_player_mesh_reference extra;gh_player_nodes_status e;if(costume==1||costume==3){s->ready=1;s->frontier=0;return GH_PLAYER_NODES_OK;}
 if(costume==5||costume==7){e=lookup(s,13,&extra);if(e)return e;e=flag(s,&extra,0x380,1);if(e)return e;e=lookup(s,14,&extra);if(e)return e;e=flag(s,&extra,0x380,1);if(e)return e;}
 else{unsigned name=costume==2?11:costume==4||costume==6?12:10;e=lookup(s,name,&extra);if(e)return e;e=flag(s,&extra,0x410,0x40000000);if(e)return e;for(unsigned i=6;i<10;i++){e=flag(s,&s->nodes[i],0x410,0x40000000);if(e)return e;}if(costume==4||costume==6){e=lookup(s,13,&extra);if(e)return e;e=flag(s,&extra,0x380,1);if(e)return e;}}
 s->ready=1;s->frontier=0;return GH_PLAYER_NODES_OK;
}
