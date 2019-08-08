
/* This program tests to see if the shared buffer allows an item to
  be removed from an empty buffer and add an item to a full buffer. */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#define MAX_SIZE 5

/* Node struct. */
typedef struct Node {
  char *word;
  struct Node *next;
}Node;

/* Queue struct. */
typedef struct Queue {
  Node *front;
  int queue_size;
}Queue;

Queue *test;
pthread_cond_t cond1;
pthread_cond_t cond2;
pthread_mutex_t lock;

void * full_test(void *);
void * empty_test(void *);
Queue *create_queue();
Node *create_node(char *);
void push(Queue * , char *);
Node *pop(Queue *);

int main() {
  pthread_mutex_init(&lock, NULL);
  pthread_cond_init(&cond1, NULL);
  pthread_cond_init(&cond2, NULL);

  test = create_queue();

  pthread_t workers[20];
  for (int i = 0; i < 20; i++) {
    pthread_create(&workers[i], NULL, &empty_test, NULL);
  }

  pthread_t workers2[20];
  for (int i = 0; i < 20; i++) {
    pthread_create(&workers2[i], NULL, &full_test, NULL);
  }



  return 0;
}

Queue *create_queue() {
 Queue *temp = (Queue *) malloc(sizeof(Queue));
 if(temp == NULL) {
   printf("Unable to allocate memory for Queue.\n");
   exit(1);
 }
 temp->front = NULL;
 temp->queue_size = 0;
 return temp;
}

Node *create_node(char *word) {
  Node *temp = (Node *) malloc(sizeof(Node));
  if(temp == NULL) {
    printf("Unable to allocate memory for Node.\n");
    exit(1);
  }
  if(word == NULL) {
    temp->word = word;
  } else {
    temp->word = malloc(sizeof(char *) * strlen(word) + 1);
    if(temp->word == NULL) {
      printf("Unable to allocate memory for Node.\n");
      exit(1);
    }
    strcpy(temp->word, word);
  }
  temp->next = NULL;
  return temp;
}

void push(Queue *queue,  char *word) {
  if(queue->queue_size >= MAX_SIZE) {
    printf("This shouldn't happen\n");
    return;
  }
  /* Creates new Node. */
  Node *temp = create_node(word);
  /* Empty Queue. */
  if(queue->queue_size == 0) {
    queue->front = temp;
    /* Places new Node at the end. */
  } else {
      Node * head = queue->front;
      while(head->next != NULL) {
        head = head->next;
      }
      head->next = temp;
  }
  queue->queue_size++;
}

Node *pop(Queue *queue) {
  // If queue is empty, return NULL.
  if (queue->front == NULL) {
    queue->queue_size = 0;
    printf("This shouldn't happen\n");
    return NULL;
  }

  /* Replaces first Node and returns first Node. */
  Node *temp = queue->front;
  queue->front = queue->front->next;
  queue->queue_size--;
  free(queue->front);
  return temp;
}

void * empty_test(void * args) {
  pthread_mutex_lock(&lock);
  if(test->queue_size <= 0) {
    pthread_cond_wait(&cond1, &lock);
  }
  pop(test);
  pthread_mutex_unlock(&lock);
  pthread_cond_signal(&cond2);
  return NULL;
}
void * full_test(void * args) {

  pthread_mutex_lock(&lock);
  if(test->queue_size >= MAX_SIZE) {
    pthread_cond_wait(&cond2, &lock);
  }
  push(test, "One");

  pthread_mutex_unlock(&lock);
  pthread_cond_signal(&cond1);
  return NULL;
}
