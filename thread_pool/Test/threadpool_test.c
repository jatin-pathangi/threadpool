#include <stdio.h>
#include <stdlib.h>
#include "thread_pool.h"

#define THREADS 15
#define q_name "/queue"

void dummy(void *arg) 
{ 
  printf("Executing\n");
}

int main(void)
{
  void *pool;
  pool = init_threadpool(THREADS);

  int i = 0;
  while(i < 15) {
    submit_job(pool, &dummy, NULL);
    i++;
  }
  
  threadpool_shutdown(pool);
  
  return 0;
}



