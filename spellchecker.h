#ifndef SPELLCHECKER_H
#define SPELLCHECKER_H

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

/* Node struct. */
typedef struct Node {
  struct sockaddr_in client;
  int client_socket;
  char *word;
  struct Node *next;
}Node;

/* Queue struct. */
typedef struct Queue {
  Node *front;
  int queue_size;
}Queue;

/* Queue functions. */
Queue *create_queue();
void del_queue(Queue *);
Node *create_node(struct sockaddr_in, char *, int);
void push(Queue *, struct sockaddr_in , char *, int);
Node *pop(Queue *);

/* Function declarations for spellchecker. */
char **open_dictionary(char *);
int open_listenfd(int);
void *worker_thread(void *);
void *log_thread(void *);

#endif
