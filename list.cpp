#include <stdlib.h>
#include <math.h>
#include "list.h"

//generate a random time
double rand_time(double n)
{
  double u = (double)rand()/(RAND_MAX+(double)1);
  double x = ((double)-1/n)*log((double)1-u);
  return x;
}

//Returns the first node of the queue
struct QueueNode * popNode(struct Queue* Q){
  if (Q->head == NULL){
    return NULL;
  }
	struct QueueNode* to_pop = Q->head;
  Q->head = Q->head->next; 
  
  to_pop->next = NULL;
  return to_pop;
}


void InsertNode(struct Queue* Q, struct QueueNode* newNode){
  
  //If queue is empty, do...
  if(Q->head == NULL)
  {
    Q->head = newNode;
    Q->tail = newNode;
    return;
  }

  //If newNode is before head, do...
  if(newNode->arrival_time < Q->head->arrival_time)
  {
    newNode->next = Q->head;
    Q->head = newNode;
    return;
  }
  
  //Else, insert newNode based on Arrival time
  struct QueueNode* temp;
  temp = Q->head;
  while(temp->next != NULL)
  { 
    if(newNode->arrival_time < temp->next->arrival_time)
    {
      newNode->next = temp->next;
      temp->next = newNode;
      return;
    }
    else 
      temp = temp->next;
  }
  temp->next = newNode;
  Q->tail = newNode;
}

void InitializeQueue(struct Queue* Q, double lambda, double mu, double mu_e, double mu_c, int p){
  double t = 0;

  //populate queue, and assign exponentially distributed values based on lamda, mu
  while(1)
  {
    struct QueueNode* newNode = (struct QueueNode*)malloc(sizeof(struct QueueNode));

    newNode->arrival_time = t + rand_time(lambda);
    newNode->service_time = rand_time(mu_e);
    newNode->treatment_service_time = rand_time(mu);
    newNode->cleaning_service_time = rand_time(mu_c);
    newNode->priority = p; 
    newNode->treatment_arrival_time = 0;
    newNode->cleaning_arrival_time = 0;
    newNode->departure_time = 0;
    newNode->treatment_departure_time = 0;
    newNode->cleaning_departure_time = 0;
    newNode->next = NULL;

    if(newNode->arrival_time < 15000)
    {
      InsertNode(Q, newNode);
      t = newNode->arrival_time;
    }
    else
    {
      free(newNode);
      break;
    }
  }
}

//Allocate memory at &Q
struct Queue * createQueue(struct Queue * Q){
  Q = (struct Queue*)malloc(sizeof(struct Queue));
  Q->head = NULL;
  Q->tail = NULL;
  return Q;
}

//Free all elements in Q
void FreeQueue(struct Queue * Q){
  struct QueueNode * current = Q->head;
  while (current != NULL){
    free(current);
    current = current->next;
  }
  free(Q);
}