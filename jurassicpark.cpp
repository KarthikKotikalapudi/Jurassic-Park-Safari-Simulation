#include <bits/stdc++.h>
#include <semaphore.h>

using namespace std;

//No. of cars, passengers, rides, parameter for exponential wait
int p, c, k;
double lambda_p, lambda_c;

vector<int> dur;

//varoables for allotting cars to passengers
int passengers_allotted = 0;
vector<bool>waiting_passengers;

//output file
ofstream outfile;

//semaphores for synchronizing passengers and cars
sem_t alloting_passengers,empty_cars;
vector<sem_t> passenger_riding, riding_completed;

//semaphore for writing in the output file
sem_t write_file;

//random engine with time-based seed
unsigned seed = chrono::system_clock::now().time_since_epoch().count();
default_random_engine generator(seed);

//function to generate time
string getSysTime(){
    time_t now = time(nullptr);
    tm *ptm = localtime(&now);
    char timestamp[32];
    strftime(timestamp, 32, "%H:%M:%S", ptm);
    return timestamp;
}

//function for passenger
void passenger(int id) {
    exponential_distribution<double> dist(lambda_p);

    //Entering museum
    string time = getSysTime();
    sem_wait(&write_file);       
    outfile<<"Passenger "<<id<<" enters the museum at "<<time<<endl;
    sem_post(&write_file);

    for(int i = 0; i < k; i++) {
        
        //simulating time in museum
        chrono::microseconds sleeptime((int)(dist(generator)*1000)); 
        this_thread::sleep_for(sleeptime);

        //ride request
        sem_wait(&write_file);  
        time = getSysTime();
        outfile<<"Passenger "<<id<<" makes a ride request at "<<time<<endl;
        sem_post(&write_file);
        waiting_passengers[id-1] = 1;

        //checking if cars are available
        sem_wait(&empty_cars);

        //starts riding by checking if riding started from car thread
        sem_wait(&passenger_riding[id-1]);
        time = getSysTime();
        sem_wait(&write_file);
        outfile<<"Passenger "<<id<<" starts riding at "<<time<<endl;
        sem_post(&write_file);

        //checks if riding completed from car thread and record in log
        sem_wait(&riding_completed[id-1]);
        time = getSysTime();
        sem_wait(&write_file);    
        outfile<<"Passenger "<<id<<" finished riding at "<<time<<endl;
        sem_post(&write_file);
    }
    //exiting museum
    sem_wait(&write_file);       
    time = getSysTime();
    outfile<<"Passenger "<<id<<" exits the museum at "<<time<<endl;
    sem_post(&write_file);
}

//function for car
void car(int id) {
    exponential_distribution<double> dist(lambda_c);

    auto start = chrono::high_resolution_clock::now();
    while(true){
        
        //mapping car for a waiting passenger
        sem_wait(&alloting_passengers);
        if(passengers_allotted >= p*k){
            sem_post(&alloting_passengers);
            break;
        }
        int i = 0;
        for(; i < p; i++){
            if(waiting_passengers[i] == 1){
                break;
            }
        }
        if(i == p){
            sem_post(&alloting_passengers);
            continue;
        }
        waiting_passengers[i] = 0;
        passengers_allotted++;
        sem_post(&alloting_passengers);

        //accepting the passenger's request
        sem_wait(&write_file);   
        outfile<<"Car "<<id<<" accepts Passenger "<<i+1<<"'s request"<<endl;
        sem_post(&write_file);
        sem_post(&passenger_riding[i]);

        //starts riding
        sem_wait(&write_file); 
        outfile<<"Car "<<id<<" is riding Passenger "<<i+1<<endl;
        sem_post(&write_file);

        //simulating riding time
        chrono::microseconds sleeptime((int)(dist(generator)*1000)); 
        this_thread::sleep_for(sleeptime);

        //finished ride
        sem_wait(&write_file);   
        outfile<<"Car "<<id<<" has finished Passenger "<<i+1<<"'s tour"<<endl;
        sem_post(&write_file);
        sem_post(&riding_completed[i]);

        //simulate time between 2 successive ride request accepted by the car
        chrono::microseconds sleeptime2((int)(dist(generator)*1000)); 
        this_thread::sleep_for(sleeptime2);

        //signals availability of cars
        sem_post(&empty_cars);
    }
    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
    
    dur[id-1] = (int)duration.count();
}


int main() {

    // reading input from file
    ifstream infile("inp-params.txt");
    infile>>p>>c>>lambda_p>>lambda_c>>k;
    infile.close();

    dur.resize(c);

    //initializing semaphores
    sem_init(&alloting_passengers, 0, 1);
    sem_init(&write_file, 0, 1);
    sem_init(&empty_cars, 0, c);
    
    //resizing the vectors and intializing
    riding_completed.resize(p);
    for(int i = 0; i < p; i++){
        sem_init(&riding_completed[i], 0, 0);
    }
    passenger_riding.resize(p);
    for(int i = 0; i < p; i++){
        sem_init(&passenger_riding[i], 0, 0);
    }
    waiting_passengers.resize(p);

    // opening output file
    outfile.open("output.txt");

    //creating threads
    thread p_threads[p],c_threads[c];
    for(int i = 0; i < p; i++) {
        p_threads[i] = thread(passenger, i+1);
    }
    for(int i = 0; i < c; i++) {
        c_threads[i] = thread(car, i+1);
    }

    //waiting for the threads to finish
    for(auto& thread : p_threads) {
        thread.join();
    }
    for(auto& thread : c_threads) {
        thread.join();
    }
    
    //destroying the semaphores
    sem_destroy(&alloting_passengers);
    sem_destroy(&write_file);
    for (int i = 0; i < p; i++) {
        sem_destroy(&riding_completed[i]);
    }
    for (int i = 0; i < p; i++) {
        sem_destroy(&passenger_riding[i]);
    }

    //closing the output file
    outfile.close();

    int sum = 0;
    for(int i = 0; i < c; i++){
        sum += dur[i];
    }
    cout<<(double)sum/c<<endl;
    return 0;
}
