#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "thread_pool.h"
#include <errno.h>
#include <string.h>

void *thr_fn(void *arg);


struct threadpool_t *init_threadpool(int num_threads) 
{
  
  struct threadpool_t *pool;
  int i;

  if((pool = (struct threadpool_t *)malloc(sizeof(struct threadpool_t))) == NULL) {
    printf("Allocating threadpool failed %s\n", strerror(errno));
    exit(0);
  }

  pool->thread_count = 0;
  pool->pending_count = 0;

  /*Initialize mutex and condition variables*/

  if((pthread_mutex_init(&pool->lock, NULL)) != 0) {
    printf("Lock init falied %s\n", strerror(errno));
    exit(0);
  }

  if(pthread_cond_init(&pool->cond, NULL) != 0) {
    printf("Cond init failed %s\n", strerror(errno));
    exit(0);
  }

  /*Allocate threads*/
  if((pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * num_threads)) == NULL) {
      printf("Allocating threads failed %s\n", strerror(errno));
      exit(0);
    }

  /*Start Worker threads*/

  for(i = 0; i < num_threads; i++) { 
    if(pthread_create(&pool->threads[i], NULL, thr_fn, (void *)pool) != 0) {
      printf("Creating threads failed %s\n", strerror(errno));
      exit(0);
      }
    pool->thread_count++;
    }

    return pool;
    
}


int submit_job(struct threadpool_t *pool, void(*function)(void *), void *argument) 
{
  struct threadpool_job j;
  j.func = function;
  j.arg = argument;
  
  struct threadpool_job *jsend = &j;

  if(pool == NULL || function == NULL) {
    printf("Invalid arguments to submit_job\n");
    return -1;
  }

  if(pthread_mutex_lock(&pool->lock) != 0) {
    printf("Lock failed %s\n", strerror(errno));
    exit(0);
  }
  
  /*Add job to message queue, sending address of pointer*/
  if(mq_send(pool->mq, (const char *)&jsend, sizeof(jsend), 0) == -1) {
    printf("mq_send failed %s\n", strerror(errno));
    exit(0);
  }

  /*One more pending job to be received*/
  pool->pending_count++;
   
  /*Signal (or broadcast) sleeping threads waiting for pending jobs to receive*/
  if(pthread_cond_signal(&pool->cond) != 0) {
    printf("pthread_cond_broadcast failed %s\n", strerror(errno));
    exit(0);
  }
  
  if(pthread_mutex_unlock(&pool->lock) != 0) {
    printf("Unlock failed %s\n", strerror(errno));
    exit(0);
  }

  return 0;
}

int free_threadpool(struct threadpool_t *pool) 
{
  if(pool == NULL)
    return -1;

  if(pool->threads) { 

    free(pool->threads);
    pthread_mutex_lock(&pool->lock);
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->cond);
  }
  
  free(pool);
  return 0;

}

/*Thread function which receives from the message queue and executes the function*/

void *thr_fn(void *arg) 
{
  struct threadpool_t *pool = (struct threadpool_t *)arg;
  struct threadpool_job *j;
  
  while(1) {
    if(pthread_mutex_lock(&pool->lock) != 0) {
      printf("Lock failed %s\n", strerror(errno));
      exit(0);
    }
    /*Wait on condition variable and check for spurious wakeups*/
    while(pool->pending_count == 0) 
      pthread_cond_wait(&pool->cond, &pool->lock);

    /*Pull job from the message queue*/

    if(mq_receive(pool->mq, (char *)&j, sizeof(j), NULL) == -1) {
      printf("mq_receive failed %s\n", strerror(errno));
      exit(0);
    }
    
    /*Since job is fetched, one less job is pending*/

    pool->pending_count--;
 
    pthread_mutex_unlock(&pool->lock);

    /*Execute the function present in the threadpool_job structure*/

    (*(j->func))(j->arg);

  };

  pthread_exit(NULL);
  return NULL;
}
