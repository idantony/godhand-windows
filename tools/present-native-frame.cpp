/* Static frame QA only. Engine integration calls the same reusable API with
 * real sequential MC/movie frames. Hidden default; --show explicitlyoptsin. */
#include "../src/platform/window_host.h"
#include "../src/platform/disc.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
int main(int argc,char**argv){
 if(argc!=6&&argc!=7){std::fprintf(stderr,"usage: present-native-frame ISO raw.rgba width height DARnum:DARden [--hidden|--show]\n");return 2;}unsigned width=(unsigned)std::strtoul(argv[3],nullptr,10),height=(unsigned)std::strtoul(argv[4],nullptr,10),an=0,ad=0;bool show=false;if(argc==7){if(!std::strcmp(argv[6],"--show"))show=true;else if(std::strcmp(argv[6],"--hidden"))return 2;}if(!width||!height||width>4096||height>4096||std::sscanf(argv[5],"%u:%u",&an,&ad)!=2||!an||!ad)return 2;
 gh_disc*d=nullptr;if(gh_disc_open(argv[1],&d)!=GH_DISC_OK)return 2;std::vector<unsigned char>elf((size_t)gh_disc_boot_elf_size(d));if(gh_disc_read_boot_elf(d,0,elf.data(),elf.size())!=GH_DISC_OK){gh_disc_close(d);return 2;}gh_disc_close(d);std::vector<unsigned char>rgba((size_t)width*height*4);FILE*f=std::fopen(argv[2],"rb");if(!f)return 2;bool read=std::fread(rgba.data(),1,rgba.size(),f)==rgba.size()&&std::fgetc(f)==EOF;std::fclose(f);if(!read)return 2;
 gh_window_config config={};config.width=640;config.height=480;config.show=show;config.title="God Hand original asset frame QA";config.xinput_index=-1;gh_native_input_defaults(&config.input);if(!gh_native_input_map_from_elf(elf.data(),elf.size(),0,&config.map))return 2;gh_window_host*w=nullptr;gh_window_error error=gh_window_create(&config,&w);if(error)return 1;gh_frame_clock clock;if(!gh_frame_clock_init(&clock,30000,1001)){gh_window_destroy(w);return 1;}unsigned frames=0;for(;frames<30;frames++){gh_native_input_snapshot input;error=gh_window_poll(w,&input);if(error)break;error=gh_window_present(w,rgba.data(),rgba.size(),width,height,an,ad);if(error)break;error=gh_window_wait_frame(w,&clock);if(error)break;}gh_window_destroy(w);std::printf("{\"error\":%u,\"hidden\":%s,\"presentations\":%u,\"source\":\"%ux%u\",\"display_aspect\":\"%u:%u\",\"scope\":\"static originalframe presentationQA only, not engineexecution\"}\n",(unsigned)error,show?"false":"true",frames,width,height,an,ad);return error?1:0;
}
