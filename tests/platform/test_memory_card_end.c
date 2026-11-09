#include "../../src/platform/memory_card.h"
#include <stdio.h>
#include <stdlib.h>
#define C(x)do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(void){gh_memory_card*card;gh_mc_result r;int done;C(gh_memory_card_end(NULL)==GH_MC_ARGUMENT);C(gh_memory_card_open("G:\\middle_thing\\godhand_windows\\build\\mc-exit\\absent-profile-end",&card)==GH_MC_OK);C(gh_memory_card_request_info(card)==GH_MC_OK);C(gh_memory_card_end(card)==GH_MC_BUSY);C(gh_memory_card_poll(card,&done,&r)==GH_MC_OK&&done&&r.result==-1);C(gh_memory_card_end(card)==GH_MC_OK);C(gh_memory_card_request_info(card)==GH_MC_ENDED);C(gh_memory_card_request_icon(card,0)==GH_MC_ENDED);done=7;C(gh_memory_card_poll(card,&done,&r)==GH_MC_ENDED&&!done);C(gh_memory_card_end(card)==GH_MC_OK);gh_memory_card_close(card);puts("PASS nativecard End: pendingquery remainsBUSY and reallycompletes; idleendpoint ends, subsequentrequest/poll rejected; idempotentend/callerclose");return 0;}
