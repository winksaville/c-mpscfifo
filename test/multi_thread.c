#define _DEFAULT_SOURCE

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <pthread.h>

//typedef _Atomic(u_int64_t) Counter;
typedef u_int64_t Counter;

typedef struct ClientParams {
  pthread_t thread;
  u_int64_t loops;
  u_int64_t initial;
  Counter* pU64;
  u_int64_t final;
} ClientParams;

void* client(void* p) {
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

Counter gU64;

int multi_thread(const u_int32_t client_count, const u_int64_t loops) {
  int error;
  ClientParams clients[client_count];

  gU64 = 0;
  u_int32_t clients_created = 0;
  for (u_int32_t i = 0; i < client_count; i++, clients_created++) {
    clients[i].loops = loops;
    clients[i].pU64 = &gU64;
    error = pthread_create(&clients[i].thread, NULL, client, (void*)&clients[i]);
    if (error != 0) {
      printf("multi_thread: aborting, clients[%u]=%p error=%d\n", i, (void*)&clients[i], error);
      goto done;
    }
  }

  printf("multi_thread: success, created %u clients\n", clients_created);

done:
  printf("multi_thread: done, joining %u clients\n", clients_created);
  for (u_int32_t i = 0; i < clients_created; i++) {
    error = pthread_join(clients[i].thread, NULL);
    if (error != 0) {
      printf("multi_thread: joing failed, clients[%u]=%p error=%d\n", i, (void*)&clients[i], error);
    }
    printf("multi_thread: clients[%u]=%p loops=%lu initial=%ld final=%lu\n",
        i, (void*)&clients[i], clients[i].loops, clients[i].initial, clients[i].final);
  }
  u_int64_t expected_value = loops * clients_created;
  printf("multi_thread: %u clients gU64=%lu %s expected_value=%lu\n",
      clients_created, gU64, gU64 == expected_value ? "==" : "!=", expected_value);

  return gU64 == expected_value ? 0 : 1;
}
