//

#include<stdio.h>
#include<time.h>
#include<math.h>
#include<stdlib.h>
#include<unistd.h>
#include<assert.h>
#include<float.h>
#include<iostream>
#include<queue>

#include "list.h"

using namespace std;

// ------------Global variables------------------------------------------------------
//Provided inputs
int customers_capacity_B;
int treatment_servers_R;
int evaluation_servers_m1;
int janitor_servers_m2;
double lambda_h;
double lambda_m;
double lambda_l;
double mu_e ;
double mu_h;
double mu_m;
double mu_l;
double mu_c;
int random_seed;

//Array to store simulation results
static double simulated_stats[11];                  //Simulated statistics [n, r, r1, r2, r3, we, wt, wt1, wt2, wt2, rc]

//Variables
int evaluation_server_availability;                 //Number of nurses = m1
int treatment_server_availability;                  //Number of rooms = R
int cleaning_server_availability;                   //Number of janitors = m2
double time_prev_event = 0;                         //Time of previouse event
double current_time = 0;                            //Current time in the system
int current_customer_count = 0;                     //Current number of customers in the system

//Performance metrics
double cumulative_number = 0;            						//cumulative number of patients in system  
double cumulative_response_time[4] = {0, 0, 0, 0};  //cumulative response time(all, high_pri, med_pri, low_pri)
double cumulative_E_waiting_time = 0;               //cumulative waiting time in the E queue
double cumulative_P_waiting_time[4] = {0, 0, 0, 0}; //cumulative waiting time in the P queue(all, high_pri, med_pri, low_pri)
double cumulative_cleaning_time = 0;                //cumulative cleanup time for each room
int left_count = 0;                      						//Number of patients who leave the system because it's at full capacity when they arrive.  
int departure_count[4] = {0, 0, 0, 0};             	//Number of departures from the system(all, high_pri, med_pri, low_pri)
int departure_count_of_clean_rooms = 0;					    //Number of rooms cleaned
int departure_count_of_evaluation = 0;              //Number of patients left evaluation stage
int count_started_treatment[4] = {0,0,0,0};         //Number of patients started treatment(all, high_pri, med_pri, low_pri)

//Custom comparater for std::priority_queue
struct ComparePriority{
		bool operator()(const QueueNode& lhs, const QueueNode& rhs){
		    return lhs.priority > rhs.priority;
		}
};	
struct CompareDepartureTime{
		bool operator()(const QueueNode& lhs, const QueueNode& rhs){
		    return lhs.departure_time > rhs.departure_time;
		}
};	
struct CompareTreatDepTime{
		bool operator()(const QueueNode& lhs, const QueueNode& rhs){
		    return lhs.treatment_departure_time > rhs.treatment_departure_time;
		}
};
struct CompareCleanDepTime{
		bool operator()(const QueueNode& lhs, const QueueNode& rhs){
		    return lhs.cleaning_departure_time > rhs.cleaning_departure_time;
		}
};

// ------------Event routines------------------------------------------------------
//Arrival at the system
struct Queue * customer_Q;

//Evaluation Q
queue<QueueNode> evaluation_arrival_Q;
priority_queue<QueueNode, vector<QueueNode>, CompareDepartureTime> evaluation_departure_Q;

//Treatmnet PQ
priority_queue<QueueNode, vector<QueueNode>, ComparePriority> treatment_arrival_Q;
priority_queue<QueueNode, vector<QueueNode>, CompareTreatDepTime> treatment_departure_Q;

//Cleaning Q
queue<QueueNode> cleaning_arrival_Q;
priority_queue<QueueNode, vector<QueueNode>, CompareCleanDepTime> cleaning_departure_Q;




// ------------Event process functions------------------------------------------------------

//Process arrival at the system
//If system full, next customer leaves, do nothing
//If system not full, schedule arrival at evaluation
void Arrival_to_System(){

  //Pop customer out
	struct QueueNode * temp = popNode(customer_Q);
  if(temp == NULL){
    cout<<"No more customers arriving"<<endl;
    return;
  }
  struct QueueNode arrival_customer = *temp;
  free(temp);

  //Set current time & calculate cumulative number
  current_time = arrival_customer.arrival_time;
  cumulative_number = cumulative_number + current_customer_count * (current_time - time_prev_event);
  time_prev_event = current_time;

  //Schedule arrival if system not full, else customer leaves
  if(customers_capacity_B == 0)
  {
    left_count++;
    return;
  }else{
    evaluation_arrival_Q.push(arrival_customer);
    customers_capacity_B--;
    current_customer_count++;
  }
}

//Process arrival at the evaluation stage
//If all server busy, do nothing
//If at least 1 server idle, schedule departure at evaluation
void ProcessArrival_Evaluation() {
  
  // if no nurses available
  if (evaluation_server_availability == 0) {
    return;
  }
  
  // if nurses available --> evaluation_server_availability > 0)
  
  //Pop customer from Evaluation Arrival Queue
  struct QueueNode arrival_customer = evaluation_arrival_Q.front();
  evaluation_arrival_Q.pop();

  // statistics 
  if(arrival_customer.arrival_time > current_time){
    current_time = arrival_customer.arrival_time;
    cumulative_number = cumulative_number + current_customer_count * (current_time - time_prev_event);
    time_prev_event = current_time;
  }
  
  //Push customer to Evaluation Departure Queue
  arrival_customer.departure_time = current_time + arrival_customer.service_time;
	evaluation_departure_Q.push(arrival_customer);

  evaluation_server_availability--;

}


//Process departure at the evaluation stage
//Schedule arrival at treatment
void ProcessDeparture_Evaluation(){
	  
  //Pop customer from Evaluation Departure Queue
  struct QueueNode depart_customer = evaluation_departure_Q.top();
	evaluation_departure_Q.pop();
  
  //Push customer into treatment Arrival Queue
  depart_customer.treatment_arrival_time = depart_customer.departure_time;
  treatment_arrival_Q.push(depart_customer);
  
  //Process statistics  
  current_time = depart_customer.departure_time;
  cumulative_number = cumulative_number + current_customer_count * (current_time - time_prev_event);
  time_prev_event = current_time;
  cumulative_E_waiting_time = cumulative_E_waiting_time + current_time - (depart_customer.arrival_time + depart_customer.service_time);
  evaluation_server_availability++;
  departure_count_of_evaluation++;

}

//Process arrival at the treatment stage
//If all server busy, do nothing
//If at least 1 server idle, schedule departure at treatment
void ProcessArrival_Treatment(){

// if no rooms available
  if (treatment_server_availability == 0) {
    return;
  }
  
// if room available --> treatment_server_availability > 0)

  //Pop customer from Treatment Arrival Queue
  struct QueueNode arrival_customer = treatment_arrival_Q.top();
  treatment_arrival_Q.pop();

  // statistics 
  if(arrival_customer.treatment_arrival_time > current_time){
    current_time = arrival_customer.treatment_arrival_time;
    cumulative_number = cumulative_number + current_customer_count * (current_time - time_prev_event);
    time_prev_event = current_time;
  }  
  
  count_started_treatment[0]++;
  count_started_treatment[arrival_customer.priority]++;

  //Push customer into Treatment Departure Queue
  arrival_customer.treatment_departure_time = current_time + arrival_customer.treatment_service_time;
	treatment_departure_Q.push(arrival_customer);

  treatment_server_availability--;
}

//Process departure at the treatment stage
//Schedule arrival at cleaning
//Customer departs from the system
void ProcessDeparture_Treatment(){

  //pop customer from treatment departure queue
  struct QueueNode depart_customer = treatment_departure_Q.top();
	treatment_departure_Q.pop();
  
  //Push room into Cleaning Arrival Queue
  depart_customer.cleaning_arrival_time = depart_customer.treatment_departure_time;
  cleaning_arrival_Q.push(depart_customer);
  
  //Process statistics  
  current_time = depart_customer.treatment_departure_time;
  cumulative_number = cumulative_number + current_customer_count * (current_time - time_prev_event);
  time_prev_event = current_time;
  cumulative_response_time[0] = cumulative_response_time[0] + current_time - depart_customer.arrival_time;
  cumulative_response_time[depart_customer.priority] = cumulative_response_time[depart_customer.priority] + current_time - depart_customer.arrival_time;
  cumulative_P_waiting_time[0] = cumulative_P_waiting_time[0] + current_time - (depart_customer.treatment_arrival_time + depart_customer.treatment_service_time);
  cumulative_P_waiting_time[depart_customer.priority] = cumulative_P_waiting_time[depart_customer.priority] + current_time - (depart_customer.treatment_arrival_time + depart_customer.treatment_service_time);
  current_customer_count--;
  customers_capacity_B++;
  departure_count[0]++;
  departure_count[depart_customer.priority]++;

}

//Process arrival at the cleaning stage
//If all server busy, do nothing
//If at least 1 server idle, schedule departure at cleaning  
void ProcessArrival_Cleaning(){

// if no janitor available
  if (cleaning_server_availability == 0) {
    return;
  }
  
// if janitor available --> cleaning_server_availability > 0)

  //Pop room from Cleaning Arrival Queue
  struct QueueNode arrival_room = cleaning_arrival_Q.front();
  cleaning_arrival_Q.pop();

  // statistics 
  if(arrival_room.cleaning_arrival_time > current_time){
    current_time = arrival_room.cleaning_arrival_time;
    cumulative_number = cumulative_number + current_customer_count * (current_time - time_prev_event);
    time_prev_event = current_time;
  }
  
  //Push room into Cleaning Departure Queue
  arrival_room.cleaning_departure_time = current_time + arrival_room.cleaning_service_time;
	cleaning_departure_Q.push(arrival_room); 

}

//Process departure at the cleaning stage
//server at treatment stage (room), becomes available
void ProcessDeparture_Cleaning(){

	//Pop customer from Cleaning Departure Queue
  struct QueueNode depart_room = cleaning_departure_Q.top();
	cleaning_departure_Q.pop();
  
  //Process statistics  
  current_time = depart_room.cleaning_departure_time;
  cumulative_number = cumulative_number + current_customer_count * (current_time - time_prev_event);
  time_prev_event = current_time;
  cumulative_cleaning_time = cumulative_cleaning_time + current_time - depart_room.cleaning_arrival_time; 
  cleaning_server_availability++;
  treatment_server_availability++;
  departure_count_of_clean_rooms++;
}

void PrintStatistics(){

    // Store simulated statistics [0, 1, 2 , 3,  4,  5,  6,  7,   8,   9,   10]
    // Store simulated statistics [n, r, r1, r2, r3, we, wt, wt1, wt2, wt2, rc]
    simulated_stats[0] = cumulative_number / (current_time - 60);
    simulated_stats[1] = cumulative_response_time[0] / departure_count[0];
    simulated_stats[2] = cumulative_response_time[1] / departure_count[1];
    simulated_stats[3] = cumulative_response_time[2] / departure_count[2];
    simulated_stats[4] = cumulative_response_time[3] / departure_count[3];
    simulated_stats[5] = cumulative_E_waiting_time / departure_count_of_evaluation;
    simulated_stats[6] = cumulative_P_waiting_time[0] / count_started_treatment[0];
    simulated_stats[7] = cumulative_P_waiting_time[1] / count_started_treatment[1];
    simulated_stats[8] = cumulative_P_waiting_time[2] / count_started_treatment[2];
    simulated_stats[9] = cumulative_P_waiting_time[3] / count_started_treatment[3];
    simulated_stats[10] = cumulative_cleaning_time / departure_count_of_clean_rooms;

    cout<<"Total number of departures:                      "<<departure_count[0]<<endl;
    cout<<"Average number of all patients in system:        "<<simulated_stats[0]<<endl;
    cout<<"Average response time for all patients:          "<<simulated_stats[1]<<endl;
    cout<<"Average response time for priority(high):        "<<simulated_stats[2]<<endl;
    cout<<"Average response time for priority(med):         "<<simulated_stats[3]<<endl;
    cout<<"Average response time for priority(low):         "<<simulated_stats[4]<<endl;
    cout<<"Average wait time in E queue:                    "<<simulated_stats[5]<<endl;
    cout<<"Average wait time in P queue for all patients:   "<<simulated_stats[6]<<endl;
    cout<<"Average wait time in P queue for priority(high): "<<simulated_stats[7]<<endl;
    cout<<"Average wait time in P queue for priority(med):  "<<simulated_stats[8]<<endl;
    cout<<"Average wait time in P queue for priority(low):  "<<simulated_stats[9]<<endl;
    cout<<"Average cleanup time for each room:              "<<simulated_stats[10]<<endl;
    cout<<"Number of patients who left the system:          "<<left_count<<endl;

}

//Get the earliest event among all event routine
int getMinHead(){

		int cond = 0;
    double min = 10000000000;

    if(customer_Q->head != NULL){
      cond = 1;
      min = customer_Q->head->arrival_time;  
    }

    if(evaluation_arrival_Q.empty() == 0 && evaluation_server_availability != 0 && evaluation_arrival_Q.front().arrival_time < min){
      cond = 2;
      min = evaluation_arrival_Q.front().arrival_time;
    }

    if(evaluation_departure_Q.empty() == 0 && evaluation_departure_Q.top().departure_time < min){
      cond = 3;
      min = evaluation_departure_Q.top().departure_time;
    }

    if(treatment_arrival_Q.empty() == 0 && treatment_arrival_Q.top().treatment_arrival_time < min && treatment_server_availability != 0){
      cond = 4;
      min = treatment_arrival_Q.top().treatment_arrival_time;
    }

    if(treatment_departure_Q.empty() == 0 && treatment_departure_Q.top().treatment_departure_time < min){
      cond = 5;
      min = treatment_departure_Q.top().treatment_departure_time;
    }

    if(cleaning_arrival_Q.empty() == 0 && cleaning_arrival_Q.front().cleaning_arrival_time < min && cleaning_server_availability != 0){
      cond = 6;
      min = cleaning_arrival_Q.front().cleaning_arrival_time;
    }

    if(cleaning_departure_Q.empty() == 0 && cleaning_departure_Q.top().cleaning_departure_time < min){
      cond = 7;
    }
  
  
  return cond;
}

//Reset performance_metrics
void reset_performance_metrics(){
  cumulative_number = 0;
  cumulative_E_waiting_time = 0;
  cumulative_cleaning_time = 0;
  left_count = 0;
  departure_count_of_clean_rooms = 0;					
  departure_count_of_evaluation = 0;

  for(int i = 0; i<4; i++){
    cumulative_response_time[i] = 0;
    cumulative_P_waiting_time[i] = 0;
    departure_count[i] = 0;
    count_started_treatment[i] = 0;
  }
}

//Main simulation loop
void Simulation(){

  int cond;

  int start_measuring = 0;
  double print_time = 120;

  while(current_time <= 1500)
  { 
    
    if(current_time>=60 && start_measuring==0){
      start_measuring = 1;
      reset_performance_metrics();
    }

    if(print_time <= current_time){
      cout<<endl<<"-------------------------------------------------"<<endl;
      cout<<"Simulated results at time "<<print_time-60<<" minutes: "<<endl;
      cout<<"-------------------------------------------------"<<endl;
      PrintStatistics();
      print_time = print_time + 60;

    }

    cond = getMinHead();

		if(cond == 1)
    {
      Arrival_to_System();
    }
    else if (cond == 2)
    {
      ProcessArrival_Evaluation();
    }
    else if(cond == 3)
    {
      ProcessDeparture_Evaluation();
    }
    else if(cond == 4)
    {
      ProcessArrival_Treatment();
    }
    else if(cond == 5)
    {
      ProcessDeparture_Treatment();
    }
    else if(cond == 6)
    {
      ProcessArrival_Cleaning();
    }
    else if(cond == 7)
    {
      ProcessDeparture_Cleaning();
    }
    else
    {
      cout<<"No more customers"<<endl;
      return;
    }
    
  }
  cout<<endl<<"-------------------------------------------------"<<endl;
  cout<<"Simulated results at time "<<print_time-60<<" minutes: "<<endl;
  cout<<"-------------------------------------------------"<<endl;
  PrintStatistics();
}

// Program's main function
int main(int argc, char* argv[]){

	// input arguments lambda_h, lambda_m, lambda_l, mu_e, mu_h, mu_m, mu_l, mu_c, B, R, m1, m2, S
	if(argc >= 14){

    //Asserting imput is possitive
    for (int i = 1; i<14; i++){
      assert(atof(argv[i]) >= 0);
    }

    //Convert input to variables
    lambda_h = atof(argv[1]);
    lambda_m = atof(argv[2]);
    lambda_l = atof(argv[3]);
    mu_e = atof(argv[4]);
    mu_h = atof(argv[5]);
    mu_m = atof(argv[6]);
    mu_l = atof(argv[7]);
    mu_c = atof(argv[8]);
    customers_capacity_B = atoi(argv[9]);
    treatment_servers_R = atoi(argv[10]);
    evaluation_servers_m1 = atoi(argv[11]);
    janitor_servers_m2 = atoi(argv[12]);
		random_seed = atoi(argv[13]);
    srand(random_seed);

    evaluation_server_availability = evaluation_servers_m1;
    treatment_server_availability = treatment_servers_R;
	  cleaning_server_availability = janitor_servers_m2;
    
    //Print description  
		cout<<"Discrete Event Simulation of a Major Hospital's Emergency Department"<<endl;
    cout<<"(lambda_h, lambda_m, lambda_l, mu_e, mu_h, mu_m, mu_l, mu_c, B, R, m1, m2, S) equal to: "<<endl;
    cout<<"("<<lambda_h<<", "<<lambda_m<<", "<<lambda_l<<", "<<mu_e<<", "<<mu_h<<", "<<mu_m<<", "<<mu_l<<", "<<mu_c;
    cout<<", "<<customers_capacity_B<<", "<<treatment_servers_R<<", "<<evaluation_servers_m1<<", "<<janitor_servers_m2<<", "<<random_seed<<")"<<endl;

    //Initialize Queue
		customer_Q = createQueue(customer_Q);
    InitializeQueue(customer_Q, lambda_h, mu_h, mu_e, mu_c, 1);    //insert the high-priority patients into the evalution queue
    InitializeQueue(customer_Q, lambda_m, mu_m, mu_e, mu_c, 2);    //insert the medium-priority patients into the evalution queue
    InitializeQueue(customer_Q, lambda_l, mu_l, mu_e, mu_c, 3);    //insert the low-priority patients into the evalution queue
    
    //Start Simulation
    cout<<"Start simulation..."<<endl;
    Simulation();

    //Free allocated memory
    FreeQueue(customer_Q);
    
	}else{
	  cout<<"Insufficient number of arguments provided! "<<endl<<"Please provide these arguments:"<<endl;
    cout<<"(lambda_h, lambda_m, lambda_l, mu_e, mu_h, mu_m, mu_l, mu_c, B, R, m1, m2, S)"<<endl;
  }
   
	return 0;
}
