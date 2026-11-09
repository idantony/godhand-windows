#include "../../src/assets/model.h"
#include <stdio.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(int argc,char **argv){unsigned char b[1025],bad[1024];FILE *f;gh_model_view v;gh_model_mesh m;
 C(argc==2);f=fopen(argv[1],"rb");C(f);C(fread(b,1,sizeof(b),f)==1024);fclose(f);
 C(gh_model_open_scr(b,1024,&v)==GH_MODEL_OK&&v.mesh_count==1);
 C(gh_model_mesh_at(&v,0,&m)==GH_MODEL_OK);C(m.descriptor_offset==0x370&&m.data_offset==0x20);
 C(!strcmp(m.name,"_00_glas")&&m.data_num==2);C(gh_model_mesh_at(&v,1,&m)==GH_MODEL_BOUNDS);
 memcpy(bad,b,1024);memset(bad+16,255,4);C(gh_model_open_scr(bad,1024,&v)==GH_MODEL_BOUNDS);
 C(gh_model_open_scr(b,900,&v)==GH_MODEL_BOUNDS);b[0]='X';C(gh_model_open_scr(b,1024,&v)==GH_MODEL_UNSUPPORTED);
 puts("PASS original om0e SCR: mesh descriptor/name/signed data relocation and bounds");return 0;}
