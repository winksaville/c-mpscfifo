/**
 * Debug code for mpscfifo
 */
#include <stdio.h>

#include "mpscfifo_dbg.h"

/**
 * @see mpscfifo_dbg.h
 */
void printMsg(Msg_t* pMsg) {
  if (pMsg != NULL) {
    printf("pMsg=%p pNext=%p pRspq=%p pExtra=%p cmd=%d\n",
	pMsg, pMsg->pNext, pMsg->pRspq, pMsg->pExtra, pMsg->cmd);
  } else {
    printf("pMsg == NULL\n");
  }
}

/**
 * @see mpscfifo_dbg.h
 */
void printMpscFifo(MpscFifo_t* pQ) {
  if (pQ != NULL) {
    printf("pQ->pHead: "); printMsg(pQ->pHead);
    printf("pQ->pTail: "); printMsg(pQ->pTail);
  } else {
    printf("pQ == NULL");
  }
}

