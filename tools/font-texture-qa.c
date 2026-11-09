/* Original-font glyph asset QA, no replacement game UI. */
#include "../src/assets/font_texture.h"
#include "../src/platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char **argv){
 gh_disc *disc=NULL;gh_disc_entry entry;size_t index;unsigned char *language=NULL,*pixels=NULL;gh_bundle pack;gh_font_state state={0};unsigned width=0,height=0;unsigned long code;char *end;FILE *out;int result=1;
 if(argc!=4){fprintf(stderr,"usage: font-texture-qa iso glyph-code new-output.rgba\n");return 2;}
 code=strtoul(argv[2],&end,0);if(!*argv[2]||*end||code>65535)return 2;
 if(gh_disc_open(argv[1],&disc)||gh_disc_find(disc,"etc/core_usa.bin",&index)||gh_disc_entry_at(disc,index,&entry))goto done;
 if(entry.size>2*1024*1024)goto done;
 language=malloc((size_t)entry.size);pixels=malloc(512*256*4);if(!language||!pixels)goto done;
 if(gh_disc_read(disc,index,0,language,(size_t)entry.size)||gh_bundle_open(language,(size_t)entry.size,&pack)||gh_font_initialize(&state,&pack,1))goto done;
 if(gh_font_glyph_rgba(&state,(uint16_t)code,pixels,512*256*4,&width,&height))goto done;
 out=fopen(argv[3],"wbx");if(!out)goto done;
 {size_t bytes=(size_t)width*height*4;int failed=fwrite(pixels,1,bytes,out)!=bytes;if(fclose(out))failed=1;if(failed){remove(argv[3]);goto done;}}
 printf("OK original-font glyph=%lu width=%u height=%u parity=%lu RGBA8\n",code,width,height,code&1);result=0;
done:
 if(result)fprintf(stderr,"font QA failed or unsupported; no overwrite permitted\n");
 free(language);free(pixels);gh_disc_close(disc);return result;
}
