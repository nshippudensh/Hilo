#ifndef THREADPOOL
#define THREADPOOL

#include <pthread.h>

#define THREADS 16
#define QUEUE_SIZE 100

typedef struct {
  void (*fn)(void* arg); // A pointer to the function that will ve executed
  void* arg;             // A pointer to the argument that will be passed to the function
}task_t;


typedef struct {
  pthread_t threads[THREADS];  // An array of threads that make up the thread pool
  pthread_mutex_t lock;           // A mutex used to synchronize access to the task queue
  pthread_cond_t notify;          // A condition variable to notify threads when a new task is available
  task_t task_queue[QUEUE_SIZE];  // An array representing the queue of task waiting to be executed
  int queued;                     // The maximum size of the task queue
  int queue_front;                // The index of the front of the task queue
  int queue_back;                 // The index of the rear of the task queue
  int stop;                       // A flag to indicate when the thread pool should stop processing tasks and terminate
}threadpool_t;

void threadpool_init(threadpool_t *);
void threadpool_destroy(threadpool_t *);
void threadpool_add_task(threadpool_t *, void (*function)(void*), void* arg);
void example_task(void *arg);
#endif
