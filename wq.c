#include <stdlib.h>
#include "wq.h"
#include "utlist.h"
#include <stdio.h>

/* Initializes a work queue WQ. */
void wq_init(wq_t *wq) {

  /* TODO: Make me thread-safe! */

  wq->size = 0;
  wq->head = NULL;
  printf("Init mutex");
  pthread_mutex_init(&(wq->wqmx),NULL);
  pthread_cond_init(&(wq->notempty),NULL);
  pthread_mutex_init(&(wq->condlock),NULL);
}

/* Remove an item from the WQ. This function should block until there
 * is at least one item on the queue. */
int wq_pop(wq_t *wq) {

  /* TODO: Make me blocking and thread-safe! */
  printf("Pop attempted \n");
  //pthread_mutex_lock(&(wq->empty)); //Wait until queue is not empty
  //pthread_mutex_lock(&(wq->condlock));
  //pthread_cond_wait(&(wq->notempty),&(wq->condlock));
  pthread_mutex_lock(&(wq->wqmx));
  while (wq->size <= 0) { //Should not be put outside as size in wq might change if we check without having lock and we're screwed
    pthread_mutex_unlock(&(wq->wqmx));
    pthread_mutex_lock(&(wq->condlock));
    pthread_cond_wait(&(wq->notempty),&(wq->condlock)); //wait for signal
    pthread_mutex_unlock(&(wq->condlock));
    pthread_mutex_lock(&(wq->wqmx));
  }
  printf("Got lock and rights \n");
  printf("wq->size: %d \n",wq->size);
  wq_item_t *wq_item = wq->head;
  int client_socket_fd = wq->head->client_socket_fd;
  wq->size--;
  DL_DELETE(wq->head, wq->head);
  //pthread_cond_broadcast(&(wq->notempty))
  pthread_mutex_unlock(&(wq->wqmx));
  //pthread_mutex_unlock(&(wq->
  free(wq_item);
  printf("Pop succeeded \n");
  return client_socket_fd;
}

/* Add ITEM to WQ. */
void wq_push(wq_t *wq, int client_socket_fd) {
  printf("Push attempted. \n");
  /* TODO: Make me thread-safe! */
  pthread_mutex_lock(&(wq->wqmx));
  wq_item_t *wq_item = calloc(1, sizeof(wq_item_t));
  wq_item->client_socket_fd = client_socket_fd;
  DL_APPEND(wq->head, wq_item);
  wq->size++;
  pthread_cond_broadcast(&(wq->notempty)); 
  pthread_mutex_unlock(&(wq->wqmx));
  printf("Push succeeded. \n");
}
