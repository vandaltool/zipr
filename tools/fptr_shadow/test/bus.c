#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>


//define constants for bus door
#define OPEN 1
#define CLOSED 0

pthread_cond_t bus_full, trip_finished, bus_empty;
pthread_mutex_t bus_lock;


int cap;     // bus capacity
int num;     // number of passengers on the bus
int door;    // whether the bus door is open or closed.
             // when open passengers can get on and get off
             // when closed they cannot
int trips;   // number of trips left to take



//****************************************************
// monitor

void take_trip() {
  pthread_mutex_lock(&bus_lock);            

  printf("Bus door is opening..\n");
  door = OPEN;

  // wait until the bus is full
  pthread_cond_wait(&bus_full, &bus_lock);  
  printf("Bus drives around Charlottesville.\n");

  //inform all passengers that the trip is over
  pthread_cond_broadcast(&trip_finished);

  //wait for them all to get off
  pthread_cond_wait(&bus_empty, &bus_lock);

  //if we're not done yet, open the door and we'll do it again
/* if (trips)
    door = OPEN; */
  pthread_mutex_unlock(&bus_lock);
}

void add_passenger(int p_nbr) {
  pthread_mutex_lock(&bus_lock);

  // if the door aint open, ya can't get in
  if ( door == OPEN ) {

    // if ya got on, then there's one more person on the bus
    num++;
    printf("1: Passenger %d gets in\n", p_nbr);

    // if the bus is full, shut the do' and let'er go
    if ( num == cap ) {
      door = CLOSED;
      printf("1: Bus door is closing...\n");
      pthread_cond_signal(&bus_full);
    }

    // looks like the trip's over
    pthread_cond_wait(&trip_finished, &bus_lock);

    //get off the bus
    num--;
    printf("1: Passenger %d gets off\n", p_nbr);

    // bus is empty, let's go home or do it again
    if ( num == 0 ) {
      pthread_cond_signal(&bus_empty);
    }
    
  }
  pthread_mutex_unlock(&bus_lock);
}

/* clone the add_passenger function */
void add_passenger2(int p_nbr) {
  pthread_mutex_lock(&bus_lock);

  // if the door aint open, ya can't get in
  if ( door == OPEN ) {

    // if ya got on, then there's one more person on the bus
    num++;
    printf("2: Passenger %d gets in\n", p_nbr);

    // if the bus is full, shut the do' and let'er go
    if ( num == cap ) {
      door = CLOSED;
      printf("2: Bus door is closing...\n");
      pthread_cond_signal(&bus_full);
    }

    // looks like the trip's over
    pthread_cond_wait(&trip_finished, &bus_lock);

    //get off the bus
    num--;
    printf("2: Passenger %d gets off\n", p_nbr);

    // bus is empty, let's go home or do it again
    if ( num == 0 ) {
      pthread_cond_signal(&bus_empty);
    }
    
  }
  pthread_mutex_unlock(&bus_lock);
}

/* clone the add_passenger function */
void add_passenger3(int p_nbr) {
  pthread_mutex_lock(&bus_lock);

  // if the door aint open, ya can't get in
  if ( door == OPEN ) {

    // if ya got on, then there's one more person on the bus
    num++;
    printf("3: Passenger %d gets in\n", p_nbr);

    // if the bus is full, shut the do' and let'er go
    if ( num == cap ) {
      door = CLOSED;
      printf("3: Bus door is closing...\n");
      pthread_cond_signal(&bus_full);
    }

    // looks like the trip's over
    pthread_cond_wait(&trip_finished, &bus_lock);

    //get off the bus
    num--;
    printf("3: Passenger %d gets off\n", p_nbr);

    // bus is empty, let's go home or do it again
    if ( num == 0 ) {
      pthread_cond_signal(&bus_empty);
    }
    
  }
  pthread_mutex_unlock(&bus_lock);
}

struct fptrs {
	char buf[8];
	void (*fptr)(int);
};

//****************************************************

void passenger(int nbr) { // nbr is the guy's id number
  struct fptrs f;

  printf("Passenger %d is born!\n", nbr);
  while ( 1 ) { // apparently riding the tour bus is all these losers do
    
    // get on, ride, get off
	switch(nbr % 3)
	{
		case 0:
  			f.fptr = &add_passenger; 
			break;
		case 1:
  			f.fptr = &add_passenger2; 
			break;
		case 2:
  			f.fptr = &add_passenger3; 
			break;
	}

	(*f.fptr)(nbr);

    // wander around or take a nap, but you'll be back. 'cause the c-ville
    // tour bus is more addictive than smokin' crack
    sleep( (5.0*rand()/(RAND_MAX+1.0)) + 1);
  }
}

void bus(int nbr) {
  printf("Bus %d is born!\n", nbr);
  while ( trips > 0  ) {
    trips--;
    take_trip();
  }
  // bus driver has poor taste
  printf("Bus driver goes home, gets drunk, and watches 'American Idol'.\n");
}

int main(int argc, char* argv[]) {
  pthread_t bus_thread;
  pthread_t * passenger_threads;

  int ind;        // short for index
  int c;          // short for character

  int n = 10;     // number of tour-bus addicts walking around, default: 10
  trips = 5;      // default to 5
  cap = 6;        // default to 6


  while ( (c = getopt(argc, argv, "n:c:t:s:")) != -1) {
    switch (c) {
    case 'n':
      n = atoi(optarg);     // store number of tour-bus addicts
      break;
    case 'c':
      cap = atoi(optarg);   // store bus size
      break;
    case 't':
      trips = atoi(optarg); // store number of trips before the busdriver gets drunk
      break;
    case 's': 		    // sleep for a while so a debugger can be attached.
    {
	int sleeptime=atoi(optarg);
	if(sleeptime<0)
	{
      		fprintf(stderr, "Incorrect arguments.\n"); 
    		exit(2);
	}
	sleep(sleeptime);
	break;
    }
    default:
      fprintf(stderr, "Incorrect arguments.\n"); 
      exit(2);
    }
  }

  // got to init
  pthread_mutex_init(&bus_lock, NULL);
  pthread_cond_init(&bus_full, NULL);
  pthread_cond_init(&trip_finished, NULL);
  pthread_cond_init(&bus_empty, NULL);
  door = CLOSED;

  // get some memory to store all them passenger threads
  passenger_threads = malloc(n*sizeof(pthread_t));

  //create the bus thread
  pthread_create(&bus_thread, NULL, (void *)bus, (void *)0);

  // create the passenger threads
  for ( ind = 0 ; ind < n ; ind++ ) {
    pthread_create(&(passenger_threads[ind]), NULL, (void *)passenger, (void *)ind);
  }

  printf("Main thread completed spawning threads\n");

  // crank her up and start rolling
  pthread_join(bus_thread, 0);

  // erase any memory of the passengers
  free(passenger_threads);
	
  return 0;
}
