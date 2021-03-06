/**
 * Test mpscfifo, returns 0 if no errors.
 */

#include "mpscfifo.h"
#include "mpscfifo_misc.h"

#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>

extern int multi_thread(const u_int32_t client_count, const u_int64_t loops);
extern int multi_thread_msg(const u_int32_t client_count, const u_int64_t loops, const u_int32_t msg_count);

_Atomic(int) ai = 0;
_Atomic(int) bi = 0;
int i = 0;
int j = 0;

int simple(void) {
  ai += 1;
  bi = 3;

  _Atomic(int) tmp;
  tmp = ai;
  ai = bi;
  bi = tmp;
  printf("ai=%d bi=%d tmp=%d\n", ai, bi, tmp);

  __atomic_store_n(&i, -1, __ATOMIC_RELEASE);
  __atomic_store_n(&j, -2, __ATOMIC_SEQ_CST); //RELEASE); //__ATOMIC_SEQ_CST);

  __atomic_fetch_add(&i, 2, __ATOMIC_SEQ_CST);
  __atomic_fetch_add(&j, 1, __ATOMIC_SEQ_CST);

  //printf("before exchange j=%d i=%d\n", j, i);
  __atomic_exchange(&i, &j, &j, __ATOMIC_SEQ_CST);
  printf("after  exchange j=%d i=%d\n", j, i);

  return 0;
}

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
  if (argc != 4) {
    printf("Usage:\n");
    printf(" %s client_count loops msg_count\n", argv[0]);
    return 1;
  }

  u_int32_t client_count = strtoul(argv[1], NULL, 10);
  u_int64_t loops = strtoull(argv[2], NULL, 10);
  u_int32_t msg_count = strtoul(argv[3], NULL, 10);
  int result = 0;

  printf("test client_count=%d loops=%ld\n", client_count, loops);

  result |= test_init_And_deinit_MpscFifo();
  result |= test_add_rmv();
  //result |= simple();
  //result |= multi_thread(client_count, loops);
  result |= multi_thread_msg(client_count, loops, msg_count);

  if (result == 0) {
    printf("Success\n");
  }

  return result;
}
