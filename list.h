#include <stdlib.h>
#include <math.h>

// -----------Data structure--------------------------------------------------------

//Date structure for customer
//Need to finish
// Definition of a Queue Node including arrival and service time
struct QueueNode {
    double arrival_time;  // customer arrival time, measured from time t=0, inter-arrival times exponential
  	double treatment_arrival_time;
  	double cleaning_arrival_time;
    double service_time;  // customer service time (exponential) 
  	double treatment_service_time;
  	double cleaning_service_time;
  	double departure_time;
    double treatment_departure_time;
	double cleaning_departure_time;  
    int priority;
    struct QueueNode *next;  // next element in line; NULL if this is the last element
};

struct Queue {
    struct QueueNode* head = NULL;    // Point to the first node of the element queue
    struct QueueNode* tail = NULL;    // Point to the tail node of the element queue
};


//generate a random time
double rand_time(double n);

struct QueueNode * popNode(struct Queue* Q);

void InsertNode(struct Queue* Q, struct QueueNode* newNode);

void InitializeQueue(struct Queue* Q, double lambda, double mu, double mu_e, double mu_c, int p);

struct Queue * createQueue(struct Queue * Q);

void FreeQueue(struct Queue * Q);