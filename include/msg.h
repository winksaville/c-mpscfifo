/**
 * Declare a Msg_t
 */

#ifndef COM_SAVILLE_MSG_H
#define COM_SAVILLE_MSG_H

#include <stdint.h>

typedef struct _msg_t {
  struct _msg_t *pNext; // Next message
  void *pRspq;          // Response queue, null if none
  void *pExtra;         // Extra information, null if none
  int32_t cmd;          // Command to perform
  int32_t arg;          // argument
} Msg_t;

#endif
