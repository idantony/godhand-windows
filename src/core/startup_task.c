/* Recovered actual patched ELF 0030D358..0030D5C4; evidence in
 * analysis/startup-task-evidence.json. No upstream C implementation copied. */
#include "startup_task.h"
static int fail(gh_startup_task *s, unsigned op, int error) {
    s->failed_operation=op;s->error=error;return error;
}
#define DO(op,expr) do { int e=(expr); if(e) return fail(s,op,e); } while(0)
int gh_startup_task_0030d358(gh_startup_task *s,const gh_startup_platform *p) {
    static const char *const languages[]={"etc/core_jpn.bin","etc/core_usa.bin",
        "etc/core_eng.bin","etc/core_fra.bin","etc/core_ger.bin",
        "etc/core_ita.bin","etc/core_spa.bin"};
    int32_t job;
    void *id_data;
    if(!s || !s->work || !p) return -1;
    if(!p->sound_startup || !p->read_alloc || !p->wait_frame || !p->bind_fonts ||
       !p->load_menu_tables || !p->bind_id_global || !p->find_id_data ||
       !p->init_id_system || !p->register_task) return fail(s,GH_START_OP_COUNT,-1);
    s->error=0;s->failed_operation=GH_START_OP_COUNT;
    s->work->flags |= 0x44000u;
    DO(GH_START_SOUND,p->sound_startup(p->context));
    DO(GH_START_READ,p->read_alloc(p->context,"etc/core.bin",&s->core_data,&job));
    DO(GH_START_WAIT,p->wait_frame(p->context,job));
    /* Unsigned range check matches sltiu: negative languages use default too.
     * Default retains first job and still waits again, as original s4 does. */
    if((uint32_t)s->work->language<7u)
        DO(GH_START_READ,p->read_alloc(p->context,languages[s->work->language],
            &s->language_data,&job));
    DO(GH_START_WAIT,p->wait_frame(p->context,job));
    DO(GH_START_FONT,p->bind_fonts(p->context,s));
    DO(GH_START_MENU,p->load_menu_tables(p->context,s));
    DO(GH_START_BIND_ID,p->bind_id_global(p->context,s));
    DO(GH_START_FIND_ID,p->find_id_data(p->context,s->language_data,"IDD",0,&id_data));
    DO(GH_START_INIT_ID,p->init_id_system(p->context,id_data));
    s->work->flags |= 0x80000u;
    if(s->work->flags598 & 0x80000000u) {
        DO(GH_START_REGISTER,p->register_task(p->context,0x2a5998,0));
        DO(GH_START_REGISTER,p->register_task(p->context,0x30d5c8,2));
    } else {
        DO(GH_START_REGISTER,p->register_task(p->context,0x2ae8a8,2));
    }
    return 0;
}
