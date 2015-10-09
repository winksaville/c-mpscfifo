/**
 * Debug code for mpscfifo
 */

#ifndef COM_SAVILLE_MPSCFIFO_DBG_H
#define COM_SAVILLE_MPSCFIFO_DBG_H

#include "mpscfifo.h"

/**
 * Print a Msg_t
 */
void printMsg(Msg_t *pMsg);

/**
 * Print a MpscFifo_t
 */
void printMpscFifo(MpscFifo_t *pQ);

#endif
