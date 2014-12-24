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
  struct mq_attr attr;
  attr.mq_maxmsg = 10;
  attr.mq_flags = 0;
  attr.mq_msgsize = sizeof(struct threadpool_job);
  attr.mq_curmsgs = 0;

  struct threadpool_t *pool;
  mqd_t m;
  pool = init_threadpool(THREADS);

  mq_unlink(q_name);
 
  if((m = mq_open(q_name, O_RDWR | O_CREAT, 0666, &attr)) == -1) {
    printf("mq_open failed %s\n", strerror(errno));
    exit(0);
  }
  pool->mq = m;
  int i = 0;
  while(i < 15) {
    submit_job(pool, &dummy, NULL);
    i++;
  }
  
  threadpool_shutdown(pool);
  mq_close(m);
  mq_unlink(q_name);
  

  return 0;
}

