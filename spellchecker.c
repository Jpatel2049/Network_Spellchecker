#include "spellchecker.h"

#define DEFAULT_PORT 8088
#define DEFAULT_DICTIONARY "words.txt"
#define DICTIONARY_LENGTH 99171
#define DEFAULT_LOG_FILE "log.txt"
#define NUM_WORKERS 5
#define EXIT_NUM -1
#define MAX_SIZE 1000
#define BUF_SIZE 256

/* To hold the dictionary words. */
char ** dictionary_words;

/* The locks and condition variables. */
pthread_mutex_t job_queue_lock;
pthread_mutex_t log_queue_lock;
pthread_mutex_t log_lock;
pthread_cond_t cond1;
pthread_cond_t cond2;
pthread_cond_t cond3;
pthread_cond_t cond4;

/* The queues. */
Queue *job_queue;
Queue *log_queue;

int main(int argc, char **argv) {
  int port;
  char *dictionary;

  /* Selects ports and dictionary depending on args. */
  if(argc == 1) {
    port = DEFAULT_PORT;
    dictionary = DEFAULT_DICTIONARY;
  } else if(argc == 2) {
      port = atoi(argv[1]);
      dictionary = DEFAULT_DICTIONARY;
  } else {
      port = atoi(argv[1]);
      dictionary = argv[2];
  }

  /* Checks if port is in the proper range. */
  if (port < 1024 || port > 65535) {
    printf("Please enter a port number between 1024 and 65535.\n");
    exit(1);
  }

  /* Gets the dictionary words. */
  dictionary_words = open_dictionary(dictionary);

  /* Creates the job and log queues. */
  job_queue = create_queue();
  log_queue = create_queue();

  /* Intialize condition variables and locks. */
  pthread_mutex_init(&job_queue_lock, NULL);
  pthread_mutex_init(&log_queue_lock, NULL);
  pthread_mutex_init(&log_lock, NULL);
  pthread_cond_init(&cond1, NULL);
  pthread_cond_init(&cond2, NULL);
  pthread_cond_init(&cond3, NULL);
  pthread_cond_init(&cond4, NULL);

  /* Creats NUM_WORKERS threads. */
  pthread_t workers[NUM_WORKERS];
  for (int i = 0; i < NUM_WORKERS; i++) {
    pthread_create(&workers[i], NULL, &worker_thread, NULL);
  }

  /* Creates the log thread. */
  pthread_t log;
  pthread_create(&log, NULL, &log_thread, NULL);

  /* Sets up the socket. */
  struct sockaddr_in client;
  socklen_t client_size = sizeof(struct sockaddr_in);
  int connection_socket = open_listenfd(port);

  char *success = "Connected to server. Type -1 to exit.\n";
  char *full = "Job buffer is full.\n";

  while(1) {
    /* Accepts the incoming connection. */
    int client_socket = accept(connection_socket, (struct sockaddr*)&client, &client_size);
    if (client_socket == -1) {
      printf("Could not connect to socket %d.\n", client_socket);
      continue;
    }

    /* Locks the job queue to check if full. */
    pthread_mutex_lock(&job_queue_lock);
    if(job_queue->queue_size >= MAX_SIZE) {
      send(client_socket, full, strlen(full), 0);
      pthread_cond_wait(&cond2, &job_queue_lock);
    }

    printf("Connected to a new client! Client ID: %d\n", client_socket);
    send(client_socket, success, strlen(success), 0);

    /* Adds the new job. */
    push(job_queue, client, NULL, client_socket);
    /* Releases the lock. */
    pthread_mutex_unlock(&job_queue_lock);
    /* Sends signal. */
    pthread_cond_signal(&cond1);
  }
  return 0;
}

/* Returns a char** to all of the words in the dictionary file */
char ** open_dictionary(char *filename) {
  FILE *fd;
  char ** output = malloc(DICTIONARY_LENGTH * sizeof(char *) + 1);
  if(output == NULL) {
    printf("Unable to allocate memory for the dictionary.\n");
    exit(1);
  }
  char line [BUF_SIZE];
  int index = 0;

  /* Opens the dictionary file. */
  fd = fopen(filename, "r");
  if(fd == NULL) {
    printf("Unable to open dictionary file. \n");
    exit(1);
  }

  /* Copies the file line by line. */
  while((fgets(line, BUF_SIZE, fd)) != NULL) {
    output[index] = (char *) malloc(strlen(line) * sizeof(char *) + 1);
    if(output[index] == NULL) {
      printf("Unable to allocate memory for the dictionary.\n");
      exit(1);
    }
    int temp = strlen(line) - 2;
    line[temp] = '\0';
    strcpy(output[index], line);
    index++;
  }
  fclose(fd);
  return output;
}

/* Taken from the slides given in lab 
  Copied from the Computer Systems textbook.
  This function creates a socket descriptor, and binds
  the socket descriptor the specified port.
  bind() associates the socket descriptor created
  with socket() to the port we want the server to listen on.
  Once the descriptor is bound, the listen() call
  will prepare the socket so that we can call accept() on it
  and get a connection to a user. */

int open_listenfd(int port) {
	int listenfd, optval = 1;
	struct sockaddr_in serveraddr;

	/* Create a socket descriptor */
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		return -1;
	}

	 /* Eliminates "Address already in use" error from bind */
	 if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int)) < 0){
	 	return -1;
	 }

	 //Reset the serveraddr struct, setting all of it's bytes to zero.
	 //Some properties are then set for the struct, you don't
	 //need to worry about these.
	 //bind() is then called, associating the port number with the
	 //socket descriptor.
	 bzero((char *) &serveraddr, sizeof(serveraddr));
	 serveraddr.sin_family = AF_INET;
	 serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	 serveraddr.sin_port = htons((unsigned short)port);
	 if (bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0){
	 	return -1;
	 }

	 //Prepare the socket to allow accept() calls. The value 20 is
	 //the backlog, this is the maximum number of connections that will be placed
	 //on queue until accept() is called again.
	 if (listen(listenfd, 20) < 0) {
	 	return -1;
	 }
	 return listenfd;
}

/* Creates the queue. */
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

/* Deletes the queue. */
void del_queue(Queue *queue) {
  free(queue->front);
  free(queue);
}

/* Creates a Node. */
Node *create_node(struct sockaddr_in client, char *word, int socket) {
  Node *temp = (Node *) malloc(sizeof(Node));
  if(temp == NULL) {
    printf("Unable to allocate memory for Node.\n");
    exit(1);
  }
  temp->client = client;
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
  temp->client_socket = socket;
  return temp;
}

/* Pushes a Node sturct onto the Queue. */
void push(Queue *queue, struct sockaddr_in client, char *word, int socket) {
  /* Creates new Node. */
  Node *temp = create_node(client, word, socket);
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

/* Pops the first Node struct off of the Queue. */
Node *pop(Queue *queue) {
  // If queue is empty, return NULL.
  if (queue->front == NULL) {
    queue->queue_size = 0;
    return NULL;
  }

  /* Replaces first Node and returns first Node. */
  Node *temp = queue->front;
  queue->front = queue->front->next;
  queue->queue_size--;
  free(queue->front);
  return temp;
}


/* NUM_WORKERS check to see if words are correct or incorrect. */
void *worker_thread(void *args) {
  char *prompt_msg = "Word to spell check >> ";
  char *close_msg = "Connection with the server is closed.\n";
  char *error_msg = "Unable to display message.\n";

  while (1) {
    /* Lock the job queue. */
    pthread_mutex_lock(&job_queue_lock);
    if (job_queue->queue_size <= 0) {
      pthread_cond_wait(&cond1, &job_queue_lock);
    }

    /* Pop the first job off the queue. */
    Node *job = pop(job_queue);
    /* Release the lock. */
    pthread_mutex_unlock(&job_queue_lock);
    /* Sends the signal. */
    pthread_cond_signal(&cond2);

    /* Get the client socket. */
    int client_socket = job->client_socket;

    /* Use  recv() and send() to talk with client. */
    while (1) {
      char recv_buffer[BUF_SIZE] = "";
      /* Send the prompt to clinet. */
      send(client_socket, prompt_msg, strlen(prompt_msg), 0);
      int bytes_returned = recv(client_socket, recv_buffer, BUF_SIZE, 0);

      /* Check if there was an error. */
      if (bytes_returned <= -1) {
        send(client_socket, error_msg, strlen(error_msg), 0);
        continue;
        /* If the client wishes to quit. */
      } else if (atoi(&recv_buffer[0]) == EXIT_NUM) {
        send(client_socket, close_msg, strlen(close_msg), 0);
        close(client_socket);
        break;
        /* Checks if word is in the dictionary. */
      } else {
        recv_buffer[strlen(recv_buffer) - 1] = '\0';
        recv_buffer[bytes_returned - 2] = '\0';

        char *result = " INCORRECT\n";
        for (int i = 0; i < DICTIONARY_LENGTH; i++) {
          if (strcmp(recv_buffer, dictionary_words[i]) == 0) {
            result = " CORRECT\n";
            break;
          }
        }

        /* Combines the word and the result. */
        strcat(recv_buffer, result);
        printf("%s", recv_buffer);

        /* Sends word and result to client. */
        send(client_socket, recv_buffer, strlen(recv_buffer), 0);

        struct sockaddr_in client = job->client;

        /* Locks the log buffer. */
        pthread_mutex_lock(&log_queue_lock);

        if(log_queue->queue_size >= MAX_SIZE) {
          pthread_cond_wait(&cond4, &log_queue_lock);

        }

        /* Adds the word and result to log buffer. */
        push(log_queue, client, recv_buffer, client_socket);
        /* Releases the lock. */
        pthread_mutex_unlock(&log_queue_lock);
        /* Sends the signal. */
        pthread_cond_signal(&cond3);

      }
    }
  }
}

/* Writes the result to a log file. */
void *log_thread(void *args) {
  while(1) {

    /* Locks the log queue. */
    pthread_mutex_lock(&log_queue_lock);
    if (log_queue->queue_size <= 0) {
      /* Waits if empty. */
      pthread_cond_wait(&cond3, &log_queue_lock);
    }

    /* Gets the word. */
    Node *node = pop(log_queue);
    char *word = node->word;

    /* Releases the lock. */
    pthread_mutex_unlock(&log_queue_lock);
    /* Sends the signal. */
    pthread_cond_signal(&cond4);

    /* If empty do nothing. */
    if (word == NULL) {
      continue;
    }

    /* Lock the log file. */
    pthread_mutex_lock(&log_lock);

    /* Write results to log file. */
    FILE *log_file = fopen(DEFAULT_LOG_FILE, "a");
    fprintf(log_file, "%s", word);
    fclose(log_file);

    /* Releases the lock. */
    pthread_mutex_unlock(&log_lock);
  }
}
