#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <pthread.h>
#include "thread_pool.h"
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#define THREADS 32
#define q_name "/queue"

int done = 0;
int tasks = 0;
pthread_mutex_t mut;

void dummy(void *arg) 
{ 
  pthread_mutex_lock(&mut);
  done++;
  pthread_mutex_unlock(&mut);
}

int main(void)
{
  struct mq_attr attr;
  attr.mq_maxmsg = 10;
  attr.mq_flags = 0;
  attr.mq_msgsize = sizeof(struct threadpool_job *);
  attr.mq_curmsgs = 0;

  struct threadpool_t *pool;
  mqd_t m;
  pthread_mutex_init(&mut, NULL);

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
    pthread_mutex_lock(&mut);
    tasks++;
    printf("Done %d tasks\n", done);
    pthread_mutex_unlock(&mut);
    i++;
  }

  printf("Added %d tasks\n", tasks);
  
  free_threadpool(pool);
  mq_close(m);
  mq_unlink(q_name);
  

  printf("Did %d tasks\n", done);

  return 0;
}

