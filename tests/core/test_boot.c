#include "boot.h"
#include <stdio.h>
#include <string.h>
static int failures;
#define REQUIRE(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); ++failures; } } while(0)
typedef struct fixture { unsigned count; int pal; int fail_at; } fixture;
static int platform_adapter(void *ctx, gh_boot_state *s, const gh_boot_request *r)
{
    fixture *f = ctx;
    static const uint32_t ntsc_calls[] = {
        0x30d258,0x30ecf8,0x15b0a0,0x31ceb8,0x2bdff8,
        0x30cb30,0x395f68,0x30d1f8,0x1ff858,0x2be6e8
    };
    if (!f->pal) {
        REQUIRE(f->count < sizeof(ntsc_calls)/sizeof(ntsc_calls[0]));
        if (f->count < sizeof(ntsc_calls)/sizeof(ntsc_calls[0]))
            REQUIRE(r->function_address == ntsc_calls[f->count]);
    }
    ++f->count;
    if ((int)r->operation == f->fail_at) return -73;
    if (r->operation == GH_BOOT_CONFIG_30ECF8) {
        REQUIRE(r->argument_count == 1 && r->arguments[0] == 0);
        REQUIRE(s->work.phase569 == 0xa5); /* clear occurs after four calls */
    }
    if (r->operation == GH_BOOT_OBJECT_INIT_2BDFF8) {
        REQUIRE(r->argument_count == 1 && r->arguments[0] == 0x5e8140);
        REQUIRE(s->work.language == 1 && s->work.room == 0x20);
        REQUIRE(s->video_parameter == 0x40);
        REQUIRE(s->work.pad000[0] == 0 && s->work.pad5b2[0] == 0);
    }
    if (r->operation == GH_BOOT_VIDEO_SETUP_30CB30 && f->pal)
        s->work.mode = 1; /* a real callee can mutate the original globals */
    if (r->operation == GH_BOOT_GS_RESET) {
        REQUIRE(f->pal && f->count == 7);
        REQUIRE(r->argument_count == 4);
        REQUIRE(r->arguments[0] == 0 && r->arguments[1] == 1);
        REQUIRE(r->arguments[2] == 3 && r->arguments[3] == 0);
    }
    if (r->operation == GH_BOOT_GS_SYNC_V)
        REQUIRE(f->pal && f->count == 8 && r->arguments[0] == 0);
    if (r->operation == GH_BOOT_REGISTER_EXIT)
        REQUIRE(r->argument_count == 1 && r->arguments[0] == 0x30b148);
    if (r->operation == GH_BOOT_DATA_INIT_1FF858) {
        REQUIRE(r->argument_count == 2 && r->arguments[0] == 0x580d40);
        REQUIRE(r->arguments[1] == 0x4504d8 && s->work.phase569 == 0);
    }
    if (r->operation == GH_BOOT_OBJECT_START_2BE6E8) {
        REQUIRE(s->work.phase569 == 2 && s->word752d3c == 0);
        REQUIRE(r->arguments[0] == 0x5e8640);
    }
    return 0;
}
int main(void)
{
    gh_boot_state s, before;
    gh_boot_platform p;
    gh_boot_result r;
    fixture f = {0,0,-1};
    unsigned i;
    memset(&s,0xa5,sizeof(s)); before=s;
    memset(&p,0,sizeof(p)); p.context=&f;
    r=gh_boot_0030b298(&s,&p);
    REQUIRE(r.status == GH_BOOT_MISSING_CALLBACK && f.count == 0);
    REQUIRE(memcmp(&s,&before,sizeof(s)) == 0);
    REQUIRE(gh_boot_0030b298(0,&p).status == GH_BOOT_INVALID_ARGUMENT);
    for(i=0;i<GH_BOOT_OPERATION_COUNT;++i) p.calls[i]=platform_adapter;
    r=gh_boot_0030b298(&s,&p);
    REQUIRE(r.status == GH_BOOT_OK && f.count == 10);
    REQUIRE(sizeof(s.work)==0x620 && offsetof(gh_boot_work,room)==0x5b0);
    /* Whole clear extent check, allowing only the three default nonzero fields. */
    {
        gh_boot_work expected = {0};
        expected.language=1; expected.room=0x20; expected.phase569=2;
        REQUIRE(memcmp(&s.work,&expected,sizeof(expected))==0);
    }
    memset(&s,0xa5,sizeof(s)); f.count=0; f.pal=1;
    r=gh_boot_0030b298(&s,&p);
    REQUIRE(r.status==GH_BOOT_OK && f.count==12 && s.work.mode==1);
    /* Platform failure has a visible boundary, never a fabricated success. */
    memset(&s,0xa5,sizeof(s)); f.count=0; f.pal=0;
    f.fail_at=GH_BOOT_CONFIG_30ECF8;
    r=gh_boot_0030b298(&s,&p);
    REQUIRE(r.status==GH_BOOT_PLATFORM_FAILED && r.platform_error==-73);
    REQUIRE(r.operation==GH_BOOT_CONFIG_30ECF8 && f.count==2);
    REQUIRE(s.work.phase569==0xa5); /* failed before original clear */
    p.calls[GH_BOOT_GS_RESET]=0; f.count=0; f.fail_at=-1;
    r=gh_boot_0030b298(&s,&p);
    REQUIRE(r.status==GH_BOOT_MISSING_CALLBACK && r.operation==GH_BOOT_GS_RESET);
    REQUIRE(f.count==0); /* even conditional callbacks must be provided */
    if (failures) return 1;
    puts("Boot control-flow/layout tests passed; test adapter is not a PS2 platform");
    return 0;
}
