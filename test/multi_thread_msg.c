#define NDEBUG

#define _DEFAULT_SOURCE

#include <mpscfifo.h>
#include <msg.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <pthread.h>

#include <semaphore.h>

typedef _Atomic(uint64_t) Counter;
//static typedef uint64_t Counter;

typedef struct ClientParams {
  pthread_t thread;

  bool done;
  Msg_t* msg;
  uint64_t error_count;
  uint64_t count;
  sem_t sem_ready;
  sem_t sem_waiting;
  sem_t sem_work_complete;
} ClientParams;


#ifdef NDEBUG
#define DPF(format, ...) ((void)(0))
#else
#define DPF(format, ...)  printf(format, __VA_ARGS__)
#endif

static void* client(void* p) {
  DPF("client:+param=%p\n", p);
  ClientParams* cp = (ClientParams*)p;

  // Signal we're ready
  sem_post(&cp->sem_ready);

  // While we're not done wait for a signal to do work
  // do the work and signal work is complete.
  bool* ptr_done = &cp->done;
  while (__atomic_load_n(ptr_done, __ATOMIC_SEQ_CST /*ACQUIRE*/) == false) {
    sem_wait(&cp->sem_waiting);

    if (!cp->done) {
      cp->count += 1;
      Msg_t** ptr_msg = &cp->msg;
      Msg_t* msg = __atomic_exchange_n(ptr_msg, NULL, __ATOMIC_SEQ_CST); //ACQUIRE);
      if (msg == NULL) {
        cp->error_count += 1;
      } else {
        ret(msg);
        sem_post(&cp->sem_work_complete);
      }
    }
  }

  // Remove the last message and return it if there is one
  Msg_t** ptr_msg = &cp->msg;
  Msg_t* msg = __atomic_exchange_n(ptr_msg, NULL, __ATOMIC_SEQ_CST); //ACQUIRE);
  if (msg != NULL) {
    ret(msg);
    sem_post(&cp->sem_work_complete);
  }

  DPF("client:-param=%p\n", p);

  return NULL;
}

int multi_thread_msg(const uint32_t client_count, const uint64_t loops,
    const uint32_t msg_count) {
  bool error;
  ClientParams clients[client_count];
  MpscFifo_t fifo;
  uint32_t clients_created = 0;
  uint64_t msgs_count = 0;
  uint64_t no_msgs_count = 0;
  uint64_t not_ready_client_count = 0;

  printf("multi_thread_msg:+client_count=%d loops=%ld msg_count=%d\n",
      client_count, loops, msg_count);

  Msg_t* msgs = malloc(sizeof(Msg_t) * (msg_count + 1));
  printf("multi_thread_msg: msgs=%p\n", msgs);
  if (msgs == NULL) {
    printf("multi_thread_msg: Unable to allocate messages, aborting\n");
    error = 1;
    goto done;
  }

  // Add the remaining msgs to the fifo
  *((MpscFifo_t**)&msgs[0].pOwner) = &fifo;
  initMpscFifo(&fifo, &msgs[0]);
  for (uint32_t i = 1; i <= msg_count; i++) {
    DPF("multi_thread_msg: add %d msg=%p\n", i, &msgs[i]);
    // Cast away the constantness to initialize
    *((MpscFifo_t**)&msgs[i].pOwner) = &fifo;
    add(&fifo, &msgs[i]);
  }
  printf("multi_thread_msg: after creating pool fifo.count=%d\n", fifo.count);

  for (uint32_t i = 0; i < client_count; i++, clients_created++) {
    ClientParams* param = &clients[i];
    param->done = false;
    param->msg = NULL;
    param->error_count = 0;
    param->count = 0;

    sem_init(&param->sem_ready, 0, 0);
    sem_init(&param->sem_waiting, 0, 0);
    sem_init(&param->sem_work_complete, 0, 0);

    error = pthread_create(&param->thread, NULL, client, (void*)&clients[i]);
    if (error != 0) {
      printf("multi_thread_msg: aborting, clients[%u]=%p error=%d\n", i, (void*)param, error);
      goto done;
    }

    // Wait until it starts
    sem_wait(&param->sem_ready);
  }

  printf("multi_thread_msg: created %u clients\n", clients_created);
  for (uint32_t i = 0; i < loops; i++) {
    for (uint32_t c = 0; c < clients_created; c++) {
      ClientParams* client = &clients[c];
      Msg_t** ptr_msg = &client->msg;
      Msg_t* msg = __atomic_load_n(ptr_msg, __ATOMIC_SEQ_CST); //ACQUIRE);
      if (msg == NULL) {
        msg = rmv_raw(&fifo);
        //msg = rmv(&fifo);
        if (msg != NULL) {
          msgs_count += 1;
          __atomic_store_n(ptr_msg, msg, __ATOMIC_SEQ_CST); //RELEASE);
          sem_post(&client->sem_waiting);
        } else {
          no_msgs_count += 1;
          //*((uint8_t*)0) = 0; // Crash
          sched_yield();
        }
      } else {
        DPF("multi_thread_msg: client=%p msg=%p != NULL\n", client, msg);
        not_ready_client_count += 1;
        sched_yield();
      }
    }
  }

  error = 0;

done:
  printf("multi_thread_msg: done, joining %u clients\n", clients_created);
  for (uint32_t i = 0; i < clients_created; i++) {
    ClientParams* param = &clients[i];

    // Signal the client to stop
    bool* ptr_done = &param->done;
    __atomic_store_n(ptr_done, true, __ATOMIC_SEQ_CST); //RELEASE);
    sem_post(&param->sem_waiting);

    // Wait until the thread completes
    error = pthread_join(param->thread, NULL);
    if (error != 0) {
      printf("multi_thread_msg: joing failed, clients[%u]=%p error=%d\n",
          i, (void*)param, error);
    }

    sem_destroy(&param->sem_ready);
    sem_destroy(&param->sem_waiting);
    sem_destroy(&param->sem_work_complete);
    printf("multi_thread_msg: clients[%u]=%p stopped error_count=%ld\n",
        i, (void*)param, param->error_count);
  }

  // Remove all msgs
  printf("multi_thread_msg: fifo.count=%d\n", fifo.count);
  Msg_t* msg;
  uint32_t rmv_count = 0;
  while ((msg = rmv(&fifo)) != NULL) {
    rmv_count += 1;
    DPF("multi_thread_msg: remove msg=%p\n", msg);
  }
  printf("multi_thread_msg: fifo had %d msgs expected %d fifo.count=%d\n",
      rmv_count, msg_count, fifo.count);
  error |= rmv_count != msg_count;

  deinitMpscFifo(&fifo);
  if (msgs != NULL) {
    free(msgs);
  }

  uint64_t expected_value = loops * clients_created;
  uint64_t sum = msgs_count + no_msgs_count + not_ready_client_count;
  printf("multi_thread_msg: sum=%ld expected_value=%ld\n", sum, expected_value);
  printf("multi_thread_msg: msgs_count=%ld no_msgs_count=%ld not_ready_client_count=%ld\n",
      msgs_count, no_msgs_count, not_ready_client_count);

  error |= sum != expected_value;
  printf("multi_thread_msg:-error=%d\n\n", error);

  return error ? 1 : 0;
}
