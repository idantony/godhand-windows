/* Actual cSnd controller fields and initialization provenance, not a cold-zero
 * surrogate. Evidence analysis/sound-control-semantics.json. */
#include "sound_control.h"
#include <string.h>
static uint32_t word(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static unsigned half(const uint8_t *p){return (unsigned)p[0]|(unsigned)p[1]<<8;}
static gh_voice_status initial_word(gh_disc *d,uint32_t address,uint32_t *value)
{
 uint8_t header[52],ph[32],bytes[4]={0};uint64_t file_size=gh_disc_boot_elf_size(d);unsigned count;uint32_t phoff;
 if(file_size<52 || gh_disc_read_boot_elf(d,0,header,52)!=GH_DISC_OK)return GH_VOICE_BOUNDS;
 if(memcmp(header,"\177ELF\1\1\1",7) || half(header+18)!=8 || half(header+42)!=32)return GH_VOICE_UNSUPPORTED;
 count=half(header+44);phoff=word(header+28);if(count>128 || (uint64_t)phoff+(uint64_t)count*32>file_size)return GH_VOICE_BOUNDS;
 for(unsigned i=0;i<count;++i){
  if(gh_disc_read_boot_elf(d,(uint64_t)phoff+i*32,ph,32)!=GH_DISC_OK)return GH_VOICE_BOUNDS;
  uint32_t base=word(ph+8),files=word(ph+16),memory=word(ph+20),off=word(ph+4);
  if(word(ph)!=1 || address<base || (uint64_t)address+4>(uint64_t)base+memory)continue;
  uint32_t delta=address-base;if(files>memory || (uint64_t)off+files>file_size)return GH_VOICE_BOUNDS;
  for(unsigned j=0;j<4;++j)if((uint64_t)delta+j<files && gh_disc_read_boot_elf(d,(uint64_t)off+delta+j,bytes+j,1)!=GH_DISC_OK)return GH_VOICE_BOUNDS;
  *value=word(bytes);return GH_VOICE_OK;
 }
 return GH_VOICE_BOUNDS;
}
gh_voice_status gh_sound_control_from_startup(gh_sound_control *s,gh_disc *d,const gh_audio_registry *registry)
{
 gh_sound_control candidate={0};uint32_t bgm;gh_voice_status error;
 if(!s || !d || !registry)return GH_VOICE_ARGUMENT;if(s->initialized)return GH_VOICE_UNSUPPORTED;
 if(!gh_audio_startup_ready(registry,0,0,0))return GH_VOICE_UNSUPPORTED;
 error=initial_word(d,0x5feeac,&candidate.flags_ac);if(error)return error;
 error=initial_word(d,0x5feeb0,&candidate.flags_b0);if(error)return error;
 error=initial_word(d,0x5fef3c,&bgm);if(error)return error;
 memcpy(&candidate.bgm_state_13c,&bgm,4);candidate.initialized=1;*s=candidate;return GH_VOICE_OK;
}
gh_voice_status gh_sound_control_prepare_scene_002ca8c8(gh_sound_control *s)
{
 if(!s || !s->initialized)return GH_VOICE_ARGUMENT;
 s->bgm_state_13c=0;s->flags_b0&=UINT32_C(0xffbfffff);s->flags_ac&=UINT32_C(0xffbfffff);return GH_VOICE_OK;
}
gh_voice_status gh_sound_control_flush_se_002ca5d8(gh_sound_control *s,gh_se_voice_engine *engine)
{
 if(!s || !s->initialized || !engine)return GH_VOICE_ARGUMENT;
 s->flags_b0&=~UINT32_C(0x04000000);s->flags_ac&=~UINT32_C(0x04000000);
 return gh_se_voice_update(engine);
}
