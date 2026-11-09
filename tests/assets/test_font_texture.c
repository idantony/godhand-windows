#include "../../src/assets/font_texture.h"
#include "../../src/platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(int argc,char **argv){gh_disc *d;gh_disc_entry e;size_t i;unsigned char *b;gh_bundle pack;gh_font_state font={0};gh_font_transfer image,clut;
 C(argc==2||argc==3);C(gh_disc_open(argv[1],&d)==GH_DISC_OK);C(gh_disc_find(d,"etc/core_usa.bin",&i)==GH_DISC_OK);C(gh_disc_entry_at(d,i,&e)==GH_DISC_OK);b=malloc((size_t)e.size);C(b);C(gh_disc_read(d,i,0,b,(size_t)e.size)==GH_DISC_OK);C(gh_bundle_open(b,(size_t)e.size,&pack)==GH_BUNDLE_OK);C(gh_font_initialize(&font,&pack,1)==GH_FONT_OK);
 C(gh_font_transfer_at(&font.slots[0],0,0,&image)==GH_FONT_OK);C(image.psm==0&&image.width==64&&image.height==256&&image.pixel_bytes==65536);
 C(gh_font_transfer_at(&font.slots[0],1,0,&clut)==GH_FONT_OK);C(clut.psm==0&&clut.width==16&&clut.height==16&&clut.pixel_bytes==1024);
 C(image.pixels==font.slots[0].mst+224&&clut.pixels==font.slots[0].mst+66016);
 C(gh_font_transfer_at(&font.slots[0],0,1,&image)==GH_FONT_BOUNDS);
 {unsigned char *rgba=malloc(512U*256U*4U);FILE *out;C(rgba);C(gh_font_atlas_rgba(&font.slots[0],rgba,512U*256U*4U)==GH_FONT_OK);
 if(argc==3){out=fopen(argv[2],"wbx");C(out);C(fwrite(rgba,1,512U*256U*4U,out)==512U*256U*4U);C(fclose(out)==0);}free(rgba);}
 {unsigned char pixels[32*32*4];unsigned w,h;C(gh_font_glyph_rgba(&font,1,pixels,sizeof(pixels),&w,&h)==GH_FONT_OK);C(w==26&&h==32);
 C(gh_font_glyph_rgba(&font,0,pixels,sizeof(pixels),&w,&h)==GH_FONT_UNSUPPORTED);}
 free(b);gh_disc_close(d);puts("PASS actual font transfers, atlas GS layout and paired-code glyph primitive");return 0;}
