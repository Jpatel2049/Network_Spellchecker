
/* Tests to see if the right amount of threads are made and prints a line. */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#define NUM_THREADS 10

void* test_func(void *);

int main() {
  pthread_t workers [NUM_THREADS];
  for(int i = 0; i < NUM_THREADS; i++) {
    pthread_create(&workers[i], NULL, &test_func, NULL);
  }
  return 0;
}

void * test_func(void * args) {
  printf("I am thread #%d.\n", pthread_self());
  return NULL;
}
