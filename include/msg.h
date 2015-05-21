/**
 * Message and queue header file
 */
#include <stdint.h>

typedef struct _msg_t {
  struct _msg_t *pNext;  // Next message
  uint32_t cmd;          // Command to perform
  void *pRspq;           // Response queue, null if none
  void *pExtra;          // Extra information, null if none
} Msg_t;
