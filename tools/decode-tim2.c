/* Restricted TIM2 asset QA export. Not a general TIM2 converter or renderer. */
#include "../src/assets/tim2.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
int main(int argc,char **argv){
 FILE *f;unsigned char *input=NULL,*rgba=NULL;long length;size_t bytes;
 gh_tim2_picture pic;gh_tim2_error error;int result=1;
 if(argc!=3){fprintf(stderr,"usage: decode-tim2 input.tm2 new-output.rgba\n");return 2;}
#ifdef _WIN32
 {char a[32768],b[32768];DWORD na=GetFullPathNameA(argv[1],sizeof(a),a,NULL),nb=GetFullPathNameA(argv[2],sizeof(b),b,NULL);
 if(!na||!nb||na>=sizeof(a)||nb>=sizeof(b)||!lstrcmpiA(a,b)){fprintf(stderr,"invalid or identical input/output path\n");return 2;}}
#else
 if(!strcmp(argv[1],argv[2])){fprintf(stderr,"identical input/output path\n");return 2;}
#endif
 f=fopen(argv[1],"rb");if(!f){perror("input");return 1;}
 if(fseek(f,0,SEEK_END)||(length=ftell(f))<0||length>16*1024*1024||fseek(f,0,SEEK_SET)){fprintf(stderr,"input bounds\n");fclose(f);return 1;}
 input=malloc((size_t)length? (size_t)length:1);
 if(!input||fread(input,1,(size_t)length,f)!=(size_t)length){fprintf(stderr,"input read failed\n");fclose(f);goto done;}
 fclose(f);error=gh_tim2_first(input,(size_t)length,&pic);
 if(error){fprintf(stderr,"TIM2 parse rejected: %d\n",(int)error);goto done;}
 bytes=(size_t)pic.width*pic.height;
 if(bytes>SIZE_MAX/4||bytes>64*1024*1024/4){fprintf(stderr,"output bounds\n");goto done;}
 bytes*=4;rgba=malloc(bytes);if(!rgba){fprintf(stderr,"allocation failed\n");goto done;}
 error=gh_tim2_rgba4(&pic,rgba,bytes);
 if(error){fprintf(stderr,"restricted RGBA4 conversion rejected: %d\n",(int)error);goto done;}
 /* Exclusive creation also rejects hardlink aliases and existing products.
  * Do not touch destination until a full validated RGBA buffer exists. */
 f=fopen(argv[2],"wbx");if(!f){perror("new output (must not exist)");goto done;}
 {int failed=fwrite(rgba,1,bytes,f)!=bytes;if(fclose(f))failed=1;
 if(failed){remove(argv[2]);fprintf(stderr,"output write failed; partial removed\n");goto done;}}
 printf("OK subset=TIM2-indexed4 width=%u height=%u bytes=%zu format=RGBA8\n",(unsigned)pic.width,(unsigned)pic.height,bytes);result=0;
done:free(input);free(rgba);return result;
}
