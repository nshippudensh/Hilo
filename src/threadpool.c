#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "threadpool.h"

void* thread_function(void* threadpool) {
  threadpool_t *pool = (threadpool_t*)threadpool;

  // El loop se ejecuta hasta que la thread pool se detiene
  while(1) {
    /* La funcion bloquea el mutex para asegurar que ningun otro thread pueda acceder a la
    cola de tareas mientras que el thread actual esta trabajando en ella.
    Esto evita las condiciones de carrera (race conditions) y garantiza un acceso seguro
    para los subprocesos a los recursos compartidos.*/
      
    pthread_mutex_lock(&(pool->lock));

    /* Dentro del bucle, el thread comprueba si la cola de tareas es 0 Y si
    la pool no ha sido detenida */
    while(pool->queued == 0 && !pool->stop)
    /* Pone al thread en espera hasta que se agregue una nueva tarea a la cola,
    momento en el que se activa la variable de condicion pool->notify y el thread
    se despierta*/
      pthread_cond_wait(&(pool->notify), &(pool->lock));

    /* Al despertarse el thread, comprueba si la pool debe detenerse
    Si la pool se esta deteniendo, el thread libera el bloque mutex y sale mediante
    pthread_exit(NULL), lo que supone la terminacion efectiva del hilo*/
    if(pool->stop) {
      pthread_mutex_unlock(&(pool->lock));
      pthread_exit(NULL);
    }

    
    /* Si la pool de threads no se detiene, el thread recupera la siguiente tarea de la cabecera
    de la queue.
    El indice queue_front se incrementa de forma circular, lo que gararntiza que la cola funcione
    como un bufer circrular.
    El tamano de la cola se decrementa para reflejar que se ha eliminado una tarea*/
    task_t task = pool->task_queue[pool->queue_front];
    pool->queue_front = (pool->queue_front + 1) % QUEUE_SIZE;
    pool->queued--;

    /* Tras recuperar la tarea, el thread libera el bloqueo mutex, lo que permite que otros
    threads accedan a la cola*/
    pthread_mutex_unlock(&(pool->lock));

    /* Finalmente, el thread ejecuta la tarea recuperada llamando al pointer almacenado en task_t*/
    (*(task.fn))(task.arg);
  }
  return NULL;
}

void threadpool_init(threadpool_t *pool) {
  pool->queued = 0;
  pool->queue_front = 0;
  pool->queue_back = 0;
  pool->stop = 0;

  pthread_mutex_init(&(pool->lock), NULL);
  pthread_cond_init(&(pool->notify), NULL);

  for(int i = 0; i < THREADS; i++)
    pthread_create(&(pool->threads[i]), NULL, thread_function, pool);
    
}

void threadpool_destroy(threadpool_t *pool) {
  pthread_mutex_lock(&(pool->lock));
  pool->stop = 1;
  pthread_cond_broadcast(&(pool->notify));
  pthread_mutex_unlock(&(pool->lock));

  for(int i = 0; i < THREADS; i++ )
    pthread_join(pool->threads[i], NULL);

  pthread_mutex_destroy(&(pool->lock));
  pthread_cond_destroy(&(pool->notify));
}

void threadpool_add_task(threadpool_t *pool, void(*function)(void*), void *arg) {
  pthread_mutex_lock(&(pool->lock));

  int next_rear = (pool->queue_back + 1) % QUEUE_SIZE;

  if(pool->queued < QUEUE_SIZE) {
    pool->task_queue[pool->queue_back].fn = function;
    pool->task_queue[pool->queue_back].arg = arg;
    pool->queue_back = next_rear;
    pool->queued++;
    pthread_cond_signal(&(pool->notify));
  } else {
    printf("Task queue is full! Cannot add more tasks.\n");
  }

  pthread_mutex_unlock(&(pool->lock));
}

void example_task(void *arg) {
  int *num = (int*)arg;
  printf("Processing task %d\n", *num);
  sleep(1);
  free(arg);
}

