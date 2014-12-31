#ifndef _THREADPOOL_H
#define _THREADPOOL_H
/*3 types of shutdown, 1 for finishing only running jobs and not do the jobs still in the queue
  2 for finishing all currently pending jobs in the queue
  3 for shutting down immediately aborting running functions.*/
#define IMM_SHUT 1
#define GRACEFUL_SHUT 2
#define ABORT_SHUT 3

/*Structure to store attributes of the threadpool*/
struct threadpool_t;

/*Structure to store the atrributes of a job given to the threadpool*/
struct threadpool_job;

void *init_threadpool(int num_threads);

int submit_job(void *pool, void(*function)(void *), void *argument);


int threadpool_shutdown(void *pool, int flag);

#endif

