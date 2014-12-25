#include <mqueue.h>
#include <stdlib.h>
#include <pthread.h>
#include "thread_pool.h"

void *thr_fn(void *arg);

char q_name[QUEUE_NAME_LENGTH];

void get_random_qname(char *qname, int len) 
{
  const char alph [] = "ABCDEFGHIJKLMONPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz";
  qname[0] = '/';
  int i;
  for(i = 1; i < len; i++) {
    qname[i] = alph[rand() % (sizeof(alph) -1)];
  }

  qname[len] = 0;
}
/*Function to allocate data structures and create worker threads*/
void *init_threadpool(int num_threads) 
{
  /*Message queue attributes*/
  struct mq_attr attr;
  attr.mq_maxmsg = 10;
  attr.mq_flags = 0;
  attr.mq_msgsize = sizeof(struct threadpool_job);
  attr.mq_curmsgs = 0;
  get_random_qname(q_name, QUEUE_NAME_LENGTH);
  

  struct threadpool_t *pool;
  int i;

  if((pool = (struct threadpool_t *)malloc(sizeof(struct threadpool_t))) == NULL) {
    return NULL;
  }

  pool->thread_count = 0;
  pool->pending_count = 0;
  pool->shutdown = 0;
  mqd_t m;
  /*Create a message queue for storing pending job requests*/
  mq_unlink(q_name);
 
  if(( m = mq_open(q_name, O_RDWR | O_CREAT, 0666, &attr)) == -1) {

    return NULL;
  }

  pool->mq = m;

  /*Initialize mutex and condition variables*/
  if((pthread_mutex_init(&pool->lock, NULL)) != 0) {
    return NULL;
  }

  if(pthread_cond_init(&pool->cond, NULL) != 0) {
    return NULL;
  }

  /*Allocate threads*/
  if((pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * num_threads)) == NULL) {
    return NULL;
    }

  /*Start Worker threads*/

  for(i = 0; i < num_threads; i++) { 
    if(pthread_create(&pool->threads[i], NULL, thr_fn, (void *)pool) != 0) {
      return NULL;
      }
    pool->thread_count++;
    }

  /*Returns opaque pointer*/
  return (void *)pool;
    
}

/*Function adds a job to the message queue, returns 0 on success and -1 on failure amd 1 if pool is shutting down*/
int submit_job(void *p, void(*function)(void *), void *argument) 
{
  struct threadpool_t *pool = (struct threadpool_t *)p;
  struct threadpool_job j;
  j.func = function;
  j.arg = argument;
  
  if(pool == NULL || function == NULL) {
    return -1;
  }

  /*Check if shutdown has started*/
   if(pthread_mutex_lock(&pool->lock) != 0) {
     return -1;
  }

   if(pool->shutdown) {
     return 1;
   }

   if(pthread_mutex_unlock(&pool->lock) != 0) {
     return -1;
  }

  /*Add job to message queue, sending address of pointer*/
  if(mq_send(pool->mq, (const char *)&j, sizeof(j), 0) == -1) {
    return -1;
  }


  if(pthread_mutex_lock(&pool->lock) != 0) {
    return -1 ;
  }
    
  /*One more pending job to be received*/
  pool->pending_count++;

  /*Signal (or broadcast) sleeping threads waiting for pending jobs to receive*/
  if(pthread_cond_signal(&pool->cond) != 0) {
    return -1;
  }
  
  if(pthread_mutex_unlock(&pool->lock) != 0) {
    return -1;
  }

  return 0;
}

/*Used by threadpool_shutdown to free data structures after joining worker threads*/
int free_threadpool(struct threadpool_t *pool) 
{
  if(pool == NULL)
    return -1;
 
  /*Free only if pool was allocated*/
  if(pool->threads) { 

    free(pool->threads);
    pthread_mutex_lock(&pool->lock);
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->cond);
    mq_close(pool->mq);
    mq_unlink(q_name);
  }
  
  free(pool);
  return 0;

}

/*Function sets shutdown variable and joins all worker threads, and then calls free_threadpool() to free the data structures. Returns 0 on success and -1 on failure*/ 
int threadpool_shutdown(void *p)
{ 
  struct threadpool_t *pool = (struct threadpool_t *)p; 
  int i;

  if(pthread_mutex_lock(&pool->lock) != 0) {
    return -1;
  }

  /*If already shutting down*/
  if(pool->shutdown) {
    return 1;
  }

  /*Set shutdown variable*/
  pool->shutdown = 1;

  /*Wake up all sleeping threads*/
  if(pthread_cond_broadcast(&pool->cond) != 0) {
    return -1;
  }
 
  pthread_mutex_unlock(&pool->lock);
  
  /*Join all running threads*/
  for(i = 0; i < pool->thread_count; i++) {
    if(pthread_join(pool->threads[i], NULL) != 0) {
      return -1;
    }
  }
 
 /*Free the threadpool*/
  free_threadpool(pool);

  return 0;
}

/*Thread worker function which receives from the message queue and executes the job function*/
void *thr_fn(void *arg) 
{
  struct threadpool_t *pool = (struct threadpool_t *)arg;
  struct threadpool_job j;
  
  while(1) {
    if(pthread_mutex_lock(&pool->lock) != 0) {
      pthread_exit(NULL);
    }
    /*Wait on condition variable and check for spurious wakeups*/
    while(pool->pending_count == 0 && pool->shutdown == 0) 
      pthread_cond_wait(&pool->cond, &pool->lock);

    /*If thread was woken up by the shutdown condition*/
    if(pool->shutdown == 1 && pool->pending_count == 0) 
      break;

    /*Pull job from the message queue*/

    if(mq_receive(pool->mq, (char *)&j, sizeof(j), NULL) == -1) {
      pthread_mutex_unlock(&pool->lock);
      pthread_exit(NULL);
    }
    
    /*Since job is fetched, one less job is pending*/

    pool->pending_count--;
 
    pthread_mutex_unlock(&pool->lock);

    /*Execute the function present in the threadpool_job structure*/

    (*(j.func))(j.arg);

  };

  pthread_mutex_unlock(&pool->lock);

  pthread_exit(NULL);
 

}
