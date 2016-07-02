#define _DEFAULT_SOURCE

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
  u_int64_t loops;
  u_int64_t initial;
  Counter* pU64;
  u_int64_t final;

  bool done;
  u_int8_t* mem;
  u_int64_t error_count;
  u_int64_t count;
  sem_t sem_ready;
  sem_t sem_waiting;
  sem_t sem_work_complete;
  sem_t sem_done;
} ClientParams;

static void* client(void* p) {
  ClientParams* params = (ClientParams*)p;
  //printf("client:+%p\n", p);
  params->initial = *params->pU64;
  for (u_int64_t i = 0; i < params->loops; i++) {
     *params->pU64 += 1;
  }
  params->final = *params->pU64;
  //printf("client:-%p\n", p);
  return NULL;
}

static Counter gU64;

int multi_thread_msg(const u_int32_t client_count, const u_int64_t loops) {
  int error;
  ClientParams clients[client_count];

  gU64 = 0;
  u_int32_t clients_created = 0;
  for (u_int32_t i = 0; i < client_count; i++, clients_created++) {
    ClientParams* param = &clients[i];
    param->loops = loops;
    param->pU64 = &gU64;
    param->done = false;
    param->mem = NULL;
    param->error_count = 0;
    param->count = 0;
    sem_init(&param->sem_ready, 0, 0);
    sem_init(&param->sem_waiting, 0, 0);
    sem_init(&param->sem_work_complete, 0, 0);
    sem_init(&param->sem_done, 0, 0);

    error = pthread_create(&param->thread, NULL, client, (void*)&clients[i]);
    if (error != 0) {
      printf("multi_thread_msg: aborting, clients[%u]=%p error=%d\n", i, (void*)param, error);
      goto done;
    }
  }

  printf("multi_thread_msg: success, created %u clients\n", clients_created);

done:
  printf("multi_thread_msg: done, joining %u clients\n", clients_created);
  for (u_int32_t i = 0; i < clients_created; i++) {
    ClientParams* param = &clients[i];
    error = pthread_join(param->thread, NULL);
    if (error != 0) {
      printf("multi_thread_msg: joing failed, clients[%u]=%p error=%d\n", i, (void*)param, error);
    }
    sem_destroy(&param->sem_ready);
    sem_destroy(&param->sem_waiting);
    sem_destroy(&param->sem_work_complete);
    sem_destroy(&param->sem_done);
    printf("multi_thread_msg: clients[%u]=%p loops=%lu initial=%ld final=%lu\n",
        i, (void*)param, param->loops, param->initial, param->final);
  }
  u_int64_t expected_value = loops * clients_created;
  printf("multi_thread_msg: %u clients gU64=%lu %s expected_value=%lu\n",
      clients_created, gU64, gU64 == expected_value ? "==" : "!=", expected_value);

  return gU64 == expected_value ? 0 : 1;
}
