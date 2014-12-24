#include <mqueue.h>

#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#define q_name "/queue"

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

struct threadpool_t *init_threadpool(int num_threads);

int submit_job(struct threadpool_t *pool, void(*function)(void *), void *argument);

int threadpool_shutdown(struct threadpool_t *pool);

int free_threadpool(struct threadpool_t *pool);


#endif
