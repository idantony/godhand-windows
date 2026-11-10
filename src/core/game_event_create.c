#include "game_event_create.h"
static gh_game_event_status fail(gh_event_create_state*s,uint32_t pc,gh_game_event_status e){s->frontier=pc;return e;}
static gh_game_event_status audio(gh_event_create_state*s,const gh_event_create_bindings*b,gh_event_audio_operation op,int32_t arg,int*ready){
 static const uint32_t calls[]={0x2d1900,0x2d19d8,0x2d1a30,0x2d1b70};if(!b->audio_channel1)return fail(s,calls[op],GH_EVENT_UNSUPPORTED);uint32_t pc=0;gh_game_event_status e=b->audio_channel1(b->context,op,b->event->fields.data_number,arg,ready,&pc);if(e)return fail(s,pc?pc:calls[op],e);if((op==GH_EVENT_AUDIO_READY_002D19D8||op==GH_EVENT_AUDIO_WAIT_002D1B70)&&*ready!=0&&*ready!=1)return fail(s,calls[op],GH_EVENT_ARGUMENT);return GH_EVENT_OK;
}
static gh_game_event_status leaf(gh_event_create_state*s,const gh_event_create_bindings*b,gh_event_create_leaf fn,uint32_t address){if(!fn)return fail(s,address,GH_EVENT_UNSUPPORTED);uint32_t pc=0;gh_game_event_status e=fn(b->context,&pc);return e?fail(s,pc?pc:address,e):GH_EVENT_OK;}
static void release_phase(gh_event_create_state*s,gh_game_event*e){e->fields.unknown07=0;e->fields.state=4;e->fields.phase=0;e->fields.unknown06=0;s->substep=0;s->finished=1;}
gh_game_event_status gh_game_event_create_00296958(gh_event_create_state*s,const gh_event_create_bindings*b,int*done){
 if(!s||!b||!done||!b->event||!b->event->initialized||!b->work||!b->module||!b->components||!b->resource||!b->dvd||!b->heap)return GH_EVENT_ARGUMENT;*done=0;if(s->event&&s->event!=b->event)return GH_EVENT_ARGUMENT;s->event=b->event;s->frontier=0;if(s->finished)return GH_EVENT_OK;gh_game_event*e=b->event;gh_game_event_status status;int ready=0;
 switch(e->fields.phase){case 0:goto first;case 1:goto module;case 2:goto resource;case 3:goto audio_ready;case 4:goto data_ready;case 5:return fail(s,0x297a68,GH_EVENT_UNSUPPORTED);case 6:return fail(s,0x296bd0,GH_EVENT_UNSUPPORTED);default:return GH_EVENT_OK;}
first:
 if(s->substep==0){if(!b->voices)return fail(s,0x2cbc58,GH_EVENT_UNSUPPORTED);while(s->bank<52){gh_voice_status v=gh_se_voice_stop_bank_002cbc58(b->voices,(int16_t)s->bank,0);if(v){s->detail=v;return fail(s,gh_se_voice_frontier(b->voices)?gh_se_voice_frontier(b->voices):0x2cbc58,GH_EVENT_CALLEE);}s->bank++;}s->substep=1;}
 if(s->substep==1){status=gh_game_event_module_begin_00297378(b->module,e,b->work,b->dvd,b->heap);if(status)return fail(s,b->module->frontier?b->module->frontier:0x297378,status);s->substep=2;}
 if(s->substep==2){if((b->on_loaded&&b->on_loaded->call)||e->fields.on_loaded_token){if(!b->on_loaded||!b->on_loaded->call)return fail(s,e->fields.on_loaded_token?e->fields.on_loaded_token:0x296958,GH_EVENT_UNSUPPORTED);uint32_t pc=0;status=b->on_loaded->call(b->on_loaded->context,&pc);if(status)return fail(s,pc?pc:0x296958,status);b->on_loaded->call=0;e->fields.on_loaded_token=0;}else{status=audio(s,b,GH_EVENT_AUDIO_PREPARE_002D1900,0,&ready);if(status)return status;}e->fields.phase++;s->substep=0;}
module:
 status=gh_game_event_module_poll_00297428(b->module,&ready);if(status)return fail(s,b->module->frontier?b->module->frontier:0x297428,status);if(!ready)return GH_EVENT_OK;if(!b->module->allocation.pointer){release_phase(s,e);return GH_EVENT_OK;}
 if(s->substep==0){if(!(e->fields.flags&0x80)){status=leaf(s,b,b->module_link_002be890,0x2be890);if(status)return status;e->fields.flags|=0x80;}s->substep=1;}
 if(s->substep==1){status=gh_event_components_bind(b->components,b->module,b->boot_elf,b->boot_elf_bytes);if(status)return fail(s,b->components->frontier?b->components->frontier:0x297680,status);status=gh_event_components_construct_00297680(b->components,e->fields.data_number);if(status)return fail(s,0x297680,status);e->fields.phase++;s->substep=0;}
resource:
 if(s->substep==0){status=gh_game_event_resource_begin_00297540(b->resource,e,b->dvd,b->heap);if(status)return fail(s,b->resource->frontier?b->resource->frontier:0x297540,status);e->fields.phase++;s->substep=0;}
audio_ready:
 if(e->fields.flags&0x100){if(s->substep==0){status=audio(s,b,GH_EVENT_AUDIO_READY_002D19D8,0,&ready);if(status)return status;if(!ready)return GH_EVENT_OK;s->substep=1;}if(s->substep==1){status=audio(s,b,GH_EVENT_AUDIO_START_002D1A30,-1,&ready);if(status)return status;}}
 e->fields.phase++;s->substep=0;
data_ready:
 status=gh_game_event_resource_poll_002975d0(b->resource,&ready);if(status)return fail(s,b->resource->frontier?b->resource->frontier:0x2975d0,status);if(!ready)return GH_EVENT_OK;if(!(e->fields.flags&0x100)){release_phase(s,e);return GH_EVENT_OK;}
 if(s->substep==0){status=audio(s,b,GH_EVENT_AUDIO_WAIT_002D1B70,-1,&ready);if(status)return status;if(!ready)return GH_EVENT_OK;s->substep=1;}
 if(s->substep==1){status=leaf(s,b,b->data_bind_002977e8,0x2977e8);if(status)return status;s->substep=2;}
 if(s->substep==2){status=leaf(s,b,b->data_prepare_002979f0,0x2979f0);if(status)return status;e->fields.phase++;s->substep=0;}
 if(e->fields.flags&0x10000000){release_phase(s,e);return GH_EVENT_OK;}return fail(s,0x297a68,GH_EVENT_UNSUPPORTED);
}
