Thread pool implementation using pthreads API and POSIX message queues
==========
A function creates threads, according to the parameters it receives.
A message queue, initialised in a test program(main) is used to pass the job to the worker threads.
On shutdown, worker threads are joined, and the structures are freed, after all remaining jobs are done.

Acknowledgement to mbrossard. I read his code, and used some ideas from that in my implementation.
