#ifndef GH_GAME_MODEL_PACKETS_H
#define GH_GAME_MODEL_PACKETS_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Original4664A0+40,1280records. These fixedwidthpacketwords are scalar data,
 * never guest code/hostpointers. Native renderer consumption is separate. */
typedef struct {uint8_t unknown00[8];uint32_t vif_direct08,unknown0c;
 uint64_t gif_ad10,gif_regs18,unknown20,gs_bitbltbuf28,bitbltbuf30,
 gs_trxpos38,unknown40,gs_trxreg48,trxreg50,gs_trxdir58,unknown60,trxdir68,
 unknown70;uint32_t field78,unknown7c;uint64_t dma_end80;
 uint32_t field88,vif_direct8c;uint64_t gif_ad90,gif_regs98,mask_a0,gs_texflush_a8;
 uint8_t unknownb0[16];} gh_model_packet32;
typedef struct {uint32_t packet_count3c;gh_model_packet32 packets[1280];} gh_game_model_packets;
/* Actual1509A0 reset:640descendingpairs, preservesallunspecifiedpacketbytes.
 * Doesnotchangeexistingstage texturecursor/VRAM orclaimrendercompletion. */
void gh_game_model_packets_reset_001509a0(gh_game_model_packets *);
#ifdef __cplusplus
}
#endif
#endif
