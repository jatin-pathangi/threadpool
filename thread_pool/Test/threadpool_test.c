#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <pthread.h>
#include "thread_pool.h"
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#define THREADS 15
#define q_name "/queue"

void dummy(void *arg) 
{ 
  printf("Executing\n");
}

int main(void)
{
  struct threadpool_t *pool;
  pool = init_threadpool(THREADS);

  int i = 0;
  while(i < 15) {
    submit_job(pool, &dummy, NULL);
    i++;
  }
  
  threadpool_shutdown(pool);
  
  return 0;
}

