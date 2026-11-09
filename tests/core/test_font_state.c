#include "../../src/core/font_state.h"
#include "../../src/platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(int argc,char **argv){gh_disc *d;size_t i;gh_disc_entry e;unsigned char *data;gh_bundle bundle;gh_font_state state={0};gh_font_texture_record tex;const gh_font_slot *s;
 C(argc==2);C(gh_disc_open(argv[1],&d)==GH_DISC_OK);C(gh_disc_find(d,"etc/core_usa.bin",&i)==GH_DISC_OK);C(gh_disc_entry_at(d,i,&e)==GH_DISC_OK);
 data=malloc((size_t)e.size);C(data);C(gh_disc_read(d,i,0,data,(size_t)e.size)==GH_DISC_OK);C(gh_bundle_open(data,(size_t)e.size,&bundle)==GH_BUNDLE_OK);
 C(gh_font_bind_language(&state,&bundle)==GH_FONT_OK);s=&state.slots[0];C(s->bound&&s->width==512&&s->height==256&&s->cell_width==32&&s->cell_height==32);
 C(!state.slots[1].bound&&!state.slots[4].bound);C(s->table_count[0]==1&&s->table_count[1]==1);
 C(gh_font_texture_at(s,0,0,&tex)==GH_FONT_OK&&tex.record_offset==48&&tex.data_offset==128);
 C(gh_font_texture_at(s,1,0,&tex)==GH_FONT_OK&&tex.record_offset==65792&&tex.data_offset==65920);
 C(gh_font_texture_at(s,1,1,&tex)==GH_FONT_BOUNDS);C(gh_font_bind_language(&state,&bundle)==GH_FONT_OK);
 C(state.slots[0].mst[0]==1);
 {gh_font_glyph glyph;C(gh_font_glyph_at(&state,1,&glyph)==GH_FONT_OK);
 C(glyph.packed==0x69c10301U&&glyph.low_byte==1&&glyph.left==3&&glyph.right==28&&glyph.width==26);
 C(gh_font_glyph_at(&state,135,&glyph)==GH_FONT_BOUNDS);}
 state.draw_word_2a08c=123;C(gh_font_initialize(&state,&bundle,1)==GH_FONT_OK&&state.draw_word_2a08c==0);
 state.draw_word_2a08c=42;C(gh_font_initialize(&state,&bundle,-1)==GH_FONT_OK&&state.draw_word_2a08c==0);
 free(data);gh_disc_close(d);puts("PASS full font bind selector/reset, original packed glyph metrics and MST relocations");return 0;}
