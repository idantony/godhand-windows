/* Read-only developer inspection; output is private asset evidence. */
#include "../src/platform/disc.h"
#include "../src/assets/core_bundle.h"
#include "../src/assets/skinned_mesh.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char**argv){
 if(argc!=3)return 1;gh_disc*d=NULL;if(gh_disc_open(argv[1],&d))return 2;
 size_t index;gh_disc_entry e;if(gh_disc_find(d,argv[2],&index)||gh_disc_entry_at(d,index,&e)||e.size>SIZE_MAX)return 3;
 void*bytes=malloc((size_t)e.size);if(!bytes||gh_disc_read(d,index,0,bytes,(size_t)e.size))return 4;
 gh_bundle b;gh_bundle_entry md;gh_model_view m;if(gh_bundle_open(bytes,(size_t)e.size,&b)||gh_bundle_find(&b,"MD",0,&md)||gh_model_open_scr(md.bytes,md.size,&m))return 5;
 unsigned total=0,misaligned=0,maxindex=0,mins=999,maxs=0;
 for(size_t mesh=0;mesh<m.mesh_count;mesh++){
  size_t streams;if(gh_skinned_mesh_stream_count(&m,mesh,&streams))return 6;
  for(size_t stream=0;stream<streams;stream++){
   gh_skinned_mesh_stream s;if(gh_skinned_mesh_stream_at(&m,mesh,stream,&s))return 7;
   for(size_t vertex=0;vertex<s.vertices;vertex++){
    gh_skinned_mesh_vertex v;if(gh_skinned_mesh_vertex_at(&s,vertex,&v))return 8;
    unsigned sum=v.weights[0]+v.weights[1]+v.weights[2];if(sum<mins)mins=sum;if(sum>maxs)maxs=sum;
    for(unsigned j=1;j<4;j++){if(v.bone_indices[j]&3)misaligned++;if(v.bone_indices[j]>maxindex)maxindex=v.bone_indices[j];}
    if(total<4)printf("vertex%u position %.9g %.9g %.9g normal %u %u %u bones %u %u %u %u weights %u %u %u %u control%08X\n",total,
     v.position[0],v.position[1],v.position[2],v.normal[0],v.normal[1],v.normal[2],v.bone_indices[0],v.bone_indices[1],v.bone_indices[2],v.bone_indices[3],v.weights[0],v.weights[1],v.weights[2],v.weights[3],v.raw_control);
    total++;
   }
  }
 }
 printf("total%u palette-index-misaligned%u max-qword-address%u weight-sum-min%u max%u\n",total,misaligned,maxindex,mins,maxs);
 free(bytes);gh_disc_close(d);return 0;
}
