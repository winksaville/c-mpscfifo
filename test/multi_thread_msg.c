#define NDEBUG

#define _DEFAULT_SOURCE

#include <mpscfifo.h>
#include <msg.h>

#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <pthread.h>

#include <semaphore.h>

typedef _Atomic(u_int64_t) Counter;
//static typedef u_int64_t Counter;

typedef struct ClientParams {
  pthread_t thread;
  MpscFifo_t* fifo;

  bool done;
  Msg_t* msg;
  u_int64_t error_count;
  u_int64_t count;
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
  while (__atomic_load_n(ptr_done, __ATOMIC_ACQUIRE) == false) {
    sem_wait(&cp->sem_waiting);

    if (!cp->done) {
      cp->count += 1;
      Msg_t** ptr_msg = &cp->msg;
      Msg_t* msg = __atomic_exchange_n(ptr_msg, NULL, __ATOMIC_ACQUIRE);
      if (msg == NULL) {
        cp->error_count += 1;
      } else {
        add(cp->fifo, msg);
        sem_post(&cp->sem_work_complete);
      }
    }
  }

  DPF("client:-param=%p\n", p);

  return NULL;
}

int multi_thread_msg(const u_int32_t client_count, const u_int64_t loops) {
  printf("multi_thread_msg:+client_count=%d loops=%ld\n", client_count, loops);

  int error;
  ClientParams clients[client_count];
  MpscFifo_t fifo;
  u_int32_t clients_created = 0;
  u_int64_t msgs_count = 0;
  u_int64_t no_msgs_count = 0;

  // Allocate the messages plus a stub
  Msg_t* msgs = malloc(sizeof(Msg_t) * (client_count + 1));
  if (msgs == NULL) {
    printf("multi_thread_msg: Unable to allocate messages, aborting\n");
    error = 1;
    goto done;
  }

  // Add the remaining msgs to the fifo
  initMpscFifo(&fifo, &msgs[0]);
  for (u_int32_t i = 1; i <= client_count; i++) {
    DPF("multi_thread_msg: add msg=%p\n", &msgs[i]);
    add(&fifo, &msgs[i]);
  }

  for (u_int32_t i = 0; i < client_count; i++, clients_created++) {
    ClientParams* param = &clients[i];
    param->done = false;
    param->msg = NULL;
    param->error_count = 0;
    param->count = 0;
    param->fifo = &fifo;

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

  printf("multi_thread_msg: success, created %u clients\n", clients_created);
  for (u_int32_t i = 0; i < loops; i++) {
    for (u_int32_t c = 0; c < clients_created; c++) {
      ClientParams* client = &clients[c];
       Msg_t** ptr_msg = &client->msg;
      if (__atomic_load_n(ptr_msg, __ATOMIC_ACQUIRE) == NULL) {
        Msg_t* msg = rmv(&fifo);
        if (msg != NULL) {
          msgs_count += 1;
          __atomic_store_n(ptr_msg, msg, __ATOMIC_RELEASE);
        } else {
          no_msgs_count += 1;
          sched_yield();
        }
      }
    }
    sched_yield();
  }

  error = 0;

done:
  printf("multi_thread_msg: done, joining %u clients\n", clients_created);
  for (u_int32_t i = 0; i < clients_created; i++) {
    ClientParams* param = &clients[i];

    // Signal the client to stop
    bool* ptr_done = &param->done;
    __atomic_store_n(ptr_done, true, __ATOMIC_RELEASE);
    sem_post(&param->sem_waiting);

    // Wait until the thread completes
    error = pthread_join(param->thread, NULL);
    if (error != 0) {
      printf("multi_thread_msg: joing failed, clients[%u]=%p error=%d\n", i, (void*)param, error);
    }

    sem_destroy(&param->sem_ready);
    sem_destroy(&param->sem_waiting);
    sem_destroy(&param->sem_work_complete);
    printf("multi_thread_msg: clients[%u]=%p stopped\n", i, (void*)param);
  }

  // Remove all msgs
  Msg_t* msg;
  while ((msg = rmv(&fifo)) != NULL) {
    DPF("multi_thread_msg: remove msg=%p\n", msg);
  }

  deinitMpscFifo(&fifo);
  if (msgs != NULL) {
    free(msgs);
  }

  printf("multi_thread_msg: msgs_count=%ld no_msgs_count=%ld\n", msgs_count, no_msgs_count);

  printf("multi_thread_msg:-error=%d\n\n", error);

  return error;
}
