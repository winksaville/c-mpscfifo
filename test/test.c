/**
 * Test mpscfifo, returns 0 if no errors.
 */

#include "mpscfifo.h"
#include "mpscfifo_misc.h"

#include <stdbool.h>

/**
 * Test we can initialize and deinitialize MpscFifo_t,
 *
 * return !0 if an error.
 */
static int test_init_And_deinit_MpscFifo() {
  bool error = false;
  Msg_t stub;
  MpscFifo_t q;

  stub.cmd = -1;

  MpscFifo_t *pQ = initMpscFifo(&q, &stub);
  TEST(pQ != NULL, "expecting q");
  TEST(pQ->pHead == &stub, "pHead not initialized");
  TEST(pQ->pTail == &stub, "pTail not initialized");
  TEST(pQ->pHead->pNext == NULL, "pStub->pNext not initialized");

  Msg_t *pStub = deinitMpscFifo(&q);
  TEST(pStub == &stub, "pStub not retuned");
  TEST(pQ->pHead == NULL, "pHead not deinitialized");
  TEST(pQ->pTail == NULL, "pTail not deinitialized");

  return error ? 1 : 0;
}

/**
 * Test we can add and remove from Q.
 *
 * return !0 if an error.
 */
static int test_add_rmv() {
  bool error = false;
  Msg_t stub;
  Msg_t msg;
  MpscFifo_t q;
  Msg_t *pResult;

  stub.cmd = -1;

  MpscFifo_t *pQ = initMpscFifo(&q, &stub);
  pResult = rmv(pQ);
  TEST(pResult == NULL, "expecting rmv from empty queue to return NULL");

  msg.cmd = 1;
  add(pQ, &msg);
  TEST(pQ->pHead == &msg, "pHead should point at msg");
  TEST(pQ->pTail->pNext == &msg, "pTail->pNext should point at msg");

  pResult = rmv(pQ);
  TEST(pResult != NULL, "expecting Q is not empty");
  TEST(pResult != &msg, "expecting return msg to not have original address");
  TEST(pResult->cmd == 1, "expecting msg.cmd == 1");

  pResult = rmv(pQ);
  TEST(pResult == NULL, "expecting Q is empty");

  pResult = deinitMpscFifo(&q);
  TEST(pResult == &msg, "expecting last stub to be address of msg");

  return error ? 1 : 0;
}

int main(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);

  int result = 0;

  result |= test_init_And_deinit_MpscFifo();
  result |= test_add_rmv();

  if (result == 0) {
    printf("Success\n");
  }

  return result;
}
