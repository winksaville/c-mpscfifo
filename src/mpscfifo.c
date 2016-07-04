/**
 * A MpscFifo is a wait free/thread safe multi-producer
 * single consumer first in first out queue. This algorithm
 * is from Dimitry Vyukov's non intrusive MPSC code here:
 *   http://www.1024cores.net/home/lock-free-algorithms/queues/non-intrusive-mpsc-node-based-queue
 *
 * The fifo has a head and tail, the elements are added
 * to the head of the queue and removed from the tail.
 * To allow for a wait free algorithm a stub element is used
 * so that a single atomic instruction can be used to add and
 * remove an element. Therefore, when you create a queue you
 * must pass in an areana which is used to manage the stub.
 *
 * A consequence of this algorithm is that when you add an
 * element to the queue a different element is returned when
 * you remove it from the queue. Of course the contents are
 * the same but the returned pointer will be different.
 */
#include <sys/types.h>
#include <pthread.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "mpscfifo_misc.h"
#include "mpscfifo.h"

/**
 * @see mpscfifo.h
 */
MpscFifo_t *initMpscFifo(MpscFifo_t *pQ, Msg_t *pStub) {
  pStub->pNext = NULL;
  pQ->pHead = pStub;
  pQ->pTail = pStub;
  pQ->count = 0;
  return pQ;
}

/**
 * @see mpscfifo.h
 */
Msg_t *deinitMpscFifo(MpscFifo_t *pQ) {
  // Assert that the Q empty
  assert(pQ->pTail->pNext == NULL);
  assert(pQ->pTail == pQ->pHead);

  // Get the stub and null head and tail
  Msg_t *pStub = pQ->pHead;
  pQ->pHead = NULL;
  pQ->pTail = NULL;
  return pStub;
}

/**
 * @see mpscifo.h
 */
void add(MpscFifo_t *pQ, Msg_t *pMsg) {
//  assert(pQ != NULL);
  if (pMsg != NULL) {
    // Be sure pMsg->pNext == NULL
    pMsg->pNext = NULL;

    // Using Builtin Clang doesn't seem to support stdatomic.h
    Msg_t** ptr_pHead = &pQ->pHead;
    Msg_t *pPrevHead = __atomic_exchange_n(ptr_pHead, pMsg, __ATOMIC_SEQ_CST); //ACQ_REL);
    Msg_t** ptr_pNext = &pPrevHead->pNext;
    __atomic_store_n(ptr_pNext, pMsg, __ATOMIC_SEQ_CST); //RELEASE);
    int32_t* ptr_count = &pQ->count;
    __atomic_fetch_add(ptr_count, 1, __ATOMIC_SEQ_CST);
    // TODO: Support "blocking" which means use condition variable
  }
}

/**
 * @see mpscifo.h
 */
Msg_t *rmv(MpscFifo_t *pQ) {
//  assert(pQ != NULL);
  Msg_t *pResult = pQ->pTail;
  Msg_t** ptr_next = &pResult->pNext;
  Msg_t *pNext = __atomic_load_n(ptr_next, __ATOMIC_SEQ_CST); //ACQUIRE);
  if (pNext != NULL) {
    // TODO: Support "blocking" which means use condition variable
    int32_t* ptr_count = &pQ->count;
    __atomic_fetch_sub(ptr_count, 1, __ATOMIC_SEQ_CST);
    pQ->pTail = pNext;
    pResult->pNext = NULL;
    pResult->pRspq = pNext->pRspq;
    pResult->pExtra = pNext->pExtra;
    pResult->cmd = pNext->cmd;
    pResult->arg = pNext->arg;
  } else {
    pResult = NULL;
  }
  return pResult;
}

#if 0
/**
 * @see mpscifo.h
 */
Msg_t *rmv_raw(MpscFifo_t *pQ) {
  Msg_t *pResult = pQ->pTail;
  Msg_t *pNext = __atomic_load_n(&pResult->pNext, __ATOMIC_SEQ_CST); //ACQUIRE);
  if (pNext != NULL) {
    __atomic_fetch_sub(&pQ->count, 1, __ATOMIC_SEQ_CST);
    __atomic_store_n(&pQ->pTail, pNext, __ATOMIC_SEQ_CST); //RELEASE
  } else {
    pResult = NULL;
  }
  return pResult;
}
#else
/**
 * @see mpscifo.h
 */
Msg_t *rmv_raw(MpscFifo_t *pQ) {
//  assert(pQ != NULL);
  int32_t* ptr_count = &pQ->count;
  int32_t initial_count = __atomic_load_n(ptr_count, __ATOMIC_SEQ_CST);
  Msg_t *pNext_retry;
  Msg_t *pResult = pQ->pTail;
  Msg_t** ptr_next = &pResult->pNext;
  Msg_t *pNext = __atomic_load_n(ptr_next, __ATOMIC_SEQ_CST); //ACQUIRE);
  if (pNext != NULL) {
#if 1
    Msg_t** ptr_tail = &pQ->pTail;
    __atomic_store_n(ptr_tail, pNext, __ATOMIC_SEQ_CST); //RELEASE
#elif 0
    __atomic_store_n(&pQ->pTail, pNext, __ATOMIC_SEQ_CST); //RELEASE
#else
    pQ->pTail = pNext;
#endif
    int32_t* ptr_count = &pQ->count;
    __atomic_fetch_sub(ptr_count, 1, __ATOMIC_SEQ_CST);
  } else {
    pNext_retry = __atomic_load_n(ptr_next, __ATOMIC_SEQ_CST);
    printf("rmv_raw 1 initial_count=%d pNext=%p pNext_retry=%p pQ->count=%d\n",
        initial_count, pNext, pNext_retry, pQ->count);
    sched_yield();
    pNext_retry = __atomic_load_n(ptr_next, __ATOMIC_SEQ_CST);
    printf("rmv_raw 2 initial_count=%d pNext=%p pNext_retry=%p pQ->count=%d\n",
        initial_count, pNext, pNext_retry, pQ->count);
    //*((uint8_t*)0) = 0; // Crash
    pResult = NULL;
  }
  return pResult;
}
#endif

/**
 * @see mpscifo.h
 */
void ret(Msg_t *pMsg) {
  //assert(pMsg->pOwner != NULL);
  add(pMsg->pOwner, pMsg);
}
