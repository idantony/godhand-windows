#include "game_system_menu.h"
#include <string.h>
#include <fenv.h>
static uint32_t w(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static unsigned h(const uint8_t*p){return (unsigned)p[0]|(unsigned)p[1]<<8;}
static void put(uint8_t*p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static float f(const uint8_t*p){uint32_t n=w(p);float v;memcpy(&v,&n,4);return v;}
static void pf(uint8_t*p,float v){uint32_t n;memcpy(&n,&v,4);put(p,n);}
static float add(float a,float b){volatile float r=a+b;return r;}
static float mul(float a,float b){volatile float r=a*b;return r;}
static gh_system_menu_status fail(gh_game_system_menu*s,uint32_t pc,gh_system_menu_status e){s->frontier=pc;s->error=e;return e;}
gh_system_menu_status gh_system_menu_from_disc(gh_game_system_menu*s,gh_disc*d){
 if(!s||!d)return GH_SYSTEM_MENU_ARGUMENT;if(s->initialized)return GH_SYSTEM_MENU_BUSY;
 uint8_t eh[52],ph[32],initial;uint64_t total=gh_disc_boot_elf_size(d);if(total<52||gh_disc_read_boot_elf(d,0,eh,52)||memcmp(eh,"\177ELF\1\1\1",7)||h(eh+18)!=8||h(eh+42)!=32)return GH_SYSTEM_MENU_BOUNDS;
 unsigned count=h(eh+44);uint64_t at=w(eh+28);if(count>128||at>total||count*32u>total-at)return GH_SYSTEM_MENU_BOUNDS;
 for(unsigned i=0;i<count;i++){if(gh_disc_read_boot_elf(d,at+32*i,ph,32))return GH_SYSTEM_MENU_BOUNDS;uint64_t base=w(ph+8),fs=w(ph+16),ms=w(ph+20),off=w(ph+4),a=0x3c2554;
  if(w(ph)!=1||a<base||a>=base+ms)continue;if(fs>ms||off>total||fs>total-off)return GH_SYSTEM_MENU_BOUNDS;
  initial=0;if(a-base<fs&&gh_disc_read_boot_elf(d,off+a-base,&initial,1))return GH_SYSTEM_MENU_BOUNDS;
  memset(s,0,sizeof(*s));s->active_3c2554=initial;s->task_slot=-1;s->initialized=1;return GH_SYSTEM_MENU_OK;
 }return GH_SYSTEM_MENU_BOUNDS;
}
static gh_system_menu_status entry(gh_game_system_menu*s,int id,gh_id_entry**e){int fallback=0;if(gh_id_base_get_work_002aa410(s->b.runtime,s->widget.base,id,e,&fallback)||fallback)return fail(s,0x2aa410,GH_SYSTEM_MENU_ID);return GH_SYSTEM_MENU_OK;}
static uint32_t random_next(void*p){uint32_t n=0;(void)gh_game_mt_next_0030f348(p,&n);return n;}
gh_system_menu_status gh_system_menu_initialize_0013ed28(gh_game_system_menu*s){
 if(!s||!s->initialized||!s->b.runtime||!s->b.random)return GH_SYSTEM_MENU_ARGUMENT;
 if(gh_game_cockpit_get_system_menu(s->b.cockpit,&s->widget)||s->widget.size<0x30)return fail(s,0x13ed28,GH_SYSTEM_MENU_ARGUMENT);
 gh_id_entry*e[4];for(int i=0;i<4;i++){gh_system_menu_status r=entry(s,i,&e[i]);if(r)return r;if(gh_id_entry_restore_source_002aa2d0(s->widget.base,e[i]))return fail(s,0x2aa2d0,GH_SYSTEM_MENU_ID);}
 gh_id_error ie=gh_id_base_restart_anim(s->widget.base,random_next,s->b.random);if(ie)return fail(s,s->widget.base->first_unported?s->widget.base->first_unported:0x2abbe8,GH_SYSTEM_MENU_ID);s->widget.base->playing=1;
 uint8_t*p=s->widget.scalars90;for(unsigned i=0;i<2;i++){pf(p+16*i,2);pf(p+16*i+4,1);pf(p+16*i+8,e[i]->f70[1]);pf(p+16*i+12,e[i]->f70[3]);}
 pf(p+0x24,e[2]->pos_x);pf(p+0x28,256);p[0x2c]=0;put(p+0x20,0);s->frontier=0;return GH_SYSTEM_MENU_OK;
}
gh_system_menu_status gh_system_menu_move_0013ee58(gh_game_system_menu*s){
 if(!s||!s->widget.base||!s->widget.scalars90||s->widget.size<0x30)return GH_SYSTEM_MENU_ARGUMENT;
 if(!s->b.binary32_reference)return fail(s,0x13ee58,GH_SYSTEM_MENU_UNSUPPORTED);
 gh_id_entry*e[4];for(int i=0;i<4;i++){gh_system_menu_status r=entry(s,i,&e[i]);if(r)return r;}
 fenv_t saved;if(feholdexcept(&saved))return fail(s,0x13ee58,GH_SYSTEM_MENU_UNSUPPORTED);if(fesetround(FE_TONEAREST)){(void)fesetenv(&saved);return fail(s,0x13ee58,GH_SYSTEM_MENU_UNSUPPORTED);}
 uint8_t*p=s->widget.scalars90;for(unsigned i=0;i<2;i++){uint8_t*q=p+16*i;pf(q+4,add(f(q+4),.03f));pf(q,add(f(q),.03f));if(f(q+4)>=2){pf(q+4,0);put(p+0x28,0);put(p+0x20,0);}if(f(q)>=3)pf(q,1);e[i]->f70[0]=f(q);e[i]->f70[2]=f(q+4);e[i]->f70[1]=f(q+8);e[i]->f70[3]=f(q+12);}
 float v=f(p+4);if(v<1)pf(p+0x28,mul(v,256));else if(v>1)pf(p+0x20,mul(add(v,-1),256));e[2]->pos_x=add(f(p+0x24),f(p+0x20));e[2]->f68=f(p+0x28);
 unsigned message=h(p+0x2e);if(message==65535){e[3]->flags|=0x8000000;e[3]->message=65535;}else{e[3]->flags&=~0x8000000u;e[3]->message=(uint16_t)message;}
 int restore=fesetenv(&saved);if(restore)return fail(s,0x13ee58,GH_SYSTEM_MENU_UNSUPPORTED);
 gh_id_frontier frontier={0};gh_id_error ie=gh_id_jobs_move_reference(s->widget.base,s->b.frame.post,s->b.frame.post_context,&s->b.animation_profile,&frontier);if(ie)return fail(s,frontier.address?frontier.address:0x2aaa40,ie==GH_ID_UNSUPPORTED?GH_SYSTEM_MENU_UNSUPPORTED:GH_SYSTEM_MENU_ID);s->frontier=0;return GH_SYSTEM_MENU_OK;
}
gh_system_menu_status gh_system_menu_trans_0013f070(gh_game_system_menu*s){
 if(!s||!s->widget.base)return GH_SYSTEM_MENU_ARGUMENT;if(!s->b.frame.trans)return fail(s,0x2aac28,GH_SYSTEM_MENU_UNSUPPORTED);uint32_t pc=0;gh_id_error e=s->b.frame.trans(s->b.frame.context,s->widget.base,&pc);if(e)return fail(s,pc?pc:0x2aac28,e==GH_ID_UNSUPPORTED?GH_SYSTEM_MENU_UNSUPPORTED:GH_SYSTEM_MENU_ID);s->frontier=0;return GH_SYSTEM_MENU_OK;
}
static void park(gh_game_system_menu*s){while(s->error){if(gh_task_threads_sleep(s->b.global_threads,1)!=GH_THREAD_OK){s->error=GH_SYSTEM_MENU_THREAD;s->frontier=0x2d5aa0;return;}}}
static void task(void*p,int32_t arg){gh_game_system_menu*s=p;(void)arg;
 if(gh_system_menu_initialize_0013ed28(s)){park(s);return;}
 for(;;){if(gh_system_menu_move_0013ee58(s)||gh_system_menu_trans_0013f070(s)){park(s);return;}s->iterations++;
  if(*s->b.enabled_3c2555&&s->active_3c2554){s->active_3c2554=0;*s->b.enabled_3c2555=0;s->task_live=0;s->task_slot=-1;return;}
  if(gh_task_threads_sleep(s->b.global_threads,1)!=GH_THREAD_OK){fail(s,0x2d5aa0,GH_SYSTEM_MENU_THREAD);s->task_live=0;return;}
 }
}
gh_system_menu_status gh_system_menu_start_002b65b0(gh_game_system_menu*s,const gh_system_menu_bindings*b){
 if(!s||!s->initialized||!b||!b->cockpit||!b->runtime||!b->random||!b->global_threads||!b->enabled_3c2555)return GH_SYSTEM_MENU_ARGUMENT;
 if(s->error)return s->error;if(s->active_3c2554)return GH_SYSTEM_MENU_OK;if(s->task_live)return GH_SYSTEM_MENU_BUSY;
 s->b=*b;s->active_3c2554=1;int32_t slot=-1;gh_thread_status e=gh_task_threads_start(b->global_threads,-1,task,s,-1,&slot);if(e)return fail(s,0x2d5470,GH_SYSTEM_MENU_THREAD);if(slot<0)return fail(s,0x2d5470,GH_SYSTEM_MENU_FULL);
 s->task_slot=slot;s->task_live=1;s->frontier=0;return GH_SYSTEM_MENU_OK;
}
gh_system_menu_status gh_system_menu_cancel(gh_game_system_menu*s){
 if(!s||!s->initialized)return GH_SYSTEM_MENU_ARGUMENT;if(s->task_live&&s->task_slot>=0&&gh_task_threads_cancel(s->b.global_threads,s->task_slot)!=GH_THREAD_OK)return GH_SYSTEM_MENU_THREAD;s->task_live=0;s->task_slot=-1;return GH_SYSTEM_MENU_OK;
}
