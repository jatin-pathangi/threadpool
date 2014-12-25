#include <mqueue.h>

#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#define QUEUE_NAME_LENGTH 10

struct threadpool_t { 
  pthread_mutex_t lock;
  pthread_cond_t cond;
  pthread_t *threads;
  mqd_t mq;
  int pending_count;   //Count of the number of pending jobs in the message queue
  int thread_count;
  int shutdown;  
};

/*Structure for the job given to the threads*/
struct threadpool_job {     
  void(*func)(void *);    
  void *arg;
};

void get_random_qname(char *qname, int len);

void *init_threadpool(int num_threads);

int submit_job(void *pool, void(*function)(void *), void *argument);

int free_threadpool(struct threadpool_t *pool);

int threadpool_shutdown(void *pool);

#endif
