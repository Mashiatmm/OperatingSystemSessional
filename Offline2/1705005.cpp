#include<cstdio>
#include<pthread.h>
#include<semaphore.h>
#include<queue>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <random>
#include <chrono>
#define LEFT 0
#define RIGHT 1
#define EMPTY 2

using namespace std;

int m, n, p, w, x, y, z;
sem_t board_pass;
sem_t* belts;
sem_t special_kiosk;
sem_t boarding_line;

pthread_mutex_t mtx_print;
pthread_mutex_t mtx_vip;
pthread_mutex_t mtx_vipr;
pthread_cond_t cond_left;
pthread_cond_t cond_right;
int count_vip = 0;
int count_miss_pass = 0;
int dir = EMPTY;
bool priority = false;

queue<int> kiosk_q;
ofstream output_file;
// do something
auto t_start = chrono::high_resolution_clock::now();

class passenger{

public:
    int id;
    bool VIP;
    bool lost;

    passenger(int id, bool VIP){
        this->id = id;
        this->VIP = VIP;
        this->lost = 0;
    }

    bool get_lost(){
        return lost;
    }

    string getname(){
        string name = to_string(id);
        if(VIP == true){
            name += "(VIP)";
        }
        return name;
    }

};

int id = 0;

void lost_boarding_pass(passenger* curr_psngr);

void take_input(){
    string filename("input.txt");

    ifstream input_file(filename);
    if (!input_file.is_open()) {
        cout << "Could not open the file - '"<< filename << "'" << endl;
        return;
    }

    input_file >> m;
    input_file >> n;
    input_file >> p;
    input_file >> w;
    input_file >> x;
    input_file >> y;
    input_file >> z;


    // cout << m<< " " << n << " " << p << " " << w << " " << x << " " << y << " " << z << endl;
    input_file.close();
}

void print(string msg){
    pthread_mutex_lock(&mtx_print);
    auto t_end = std::chrono::high_resolution_clock::now();
    int elapsed_time = round( ( std::chrono::duration<double, std::milli>(t_end-t_start).count() ) / 1000 );
    output_file<< msg << elapsed_time << endl;
    pthread_mutex_unlock(&mtx_print);
}

void kiosk_avail(passenger* curr_psngr)
{
	int value;
    sem_wait(&board_pass);
    pthread_mutex_lock(&mtx_print);
    value = kiosk_q.front();
    kiosk_q.pop();
    auto t_end = std::chrono::high_resolution_clock::now();
    int elapsed_time = round( ( std::chrono::duration<double, std::milli>(t_end-t_start).count() ) / 1000 );
    output_file<< "Passenger " << curr_psngr->getname() << " has started waiting in kiosk " << to_string(value) <<
            " at time " << elapsed_time << endl;
    kiosk_q.push(value);
    pthread_mutex_unlock(&mtx_print);
    sleep(w);
    sem_post(&board_pass);
    print("Passenger " + curr_psngr->getname() + " has got his boarding pass at time ");

	return;

}

void security_check(passenger* curr_psngr)
{
    int belt_no = rand() % n;
    print("Passenger " + curr_psngr->getname() + " has started waiting for security check in belt " + to_string(belt_no) + " from time " );
    sem_wait(&belts[belt_no]);
    print("Passenger " + curr_psngr->getname() + " has started the security check at time " );
    sleep(x);
    sem_post(&belts[belt_no]);
    print("Passenger " + curr_psngr->getname() + " has crossed the security check at time " );
	return;
}

void VIP_left_to_right(passenger* curr_psngr){
    priority = true;
    pthread_mutex_lock(&mtx_vip);
    count_vip += 1;
    if(dir == RIGHT) pthread_cond_wait(&cond_left, &mtx_vip);
    dir = LEFT;
    pthread_mutex_unlock(&mtx_vip);
    print("Passenger " + curr_psngr->getname() + " has started using the VIP channel at time " );
    sleep(z);
    print("Passenger " + curr_psngr->getname() + " has crossed the VIP channel at time " );
    pthread_mutex_lock(&mtx_vip);
    count_vip -= 1;
    if(count_vip == 0){
        dir = EMPTY;
        priority = false;
        pthread_cond_broadcast(&cond_right);
    }
    pthread_mutex_unlock(&mtx_vip);
}

void VIP_right_to_left(passenger* curr_psngr){
    pthread_mutex_lock(&mtx_vipr);
    if(dir == LEFT || priority == true){
        pthread_cond_wait(&cond_right, &mtx_vipr);
    }
    dir = RIGHT;
    count_miss_pass += 1;
    pthread_mutex_unlock(&mtx_vipr);
    print("Passenger " + curr_psngr->getname() + " has started using the VIP channel to get boarding pass at time ");
    sleep(z);
    print("Passenger " + curr_psngr->getname() + " has crossed the VIP channel to get boarding pass at time " );
    pthread_mutex_lock(&mtx_vipr);
    count_miss_pass -= 1;
    if( count_miss_pass == 0 ){
        dir = EMPTY;
        pthread_cond_broadcast(&cond_left);
    }
    pthread_mutex_unlock(&mtx_vipr);
}

void board_on_plane(passenger* curr_psngr){
    print("Passenger " + curr_psngr->getname() + " has started waiting to be boarded at time ");
    sem_wait(&boarding_line);
    if(curr_psngr->get_lost() == 1){
        curr_psngr->lost = 0;
    }
    else{
        curr_psngr->lost = rand()%2;
        if(curr_psngr->get_lost() == 1){
            sem_post(&boarding_line);
            print("Passenger " + curr_psngr->getname() + " has lost his boarding pass at time ");
            lost_boarding_pass(curr_psngr);
            return;
        }
    }
    print("Passenger " + curr_psngr->getname() + " has started boarding the plane at time ");
    sleep(y);
    print("Passenger " + curr_psngr->getname() + " has boarded the plane at time ");
    sem_post(&boarding_line);
}

void lost_boarding_pass(passenger* curr_psngr){
    VIP_right_to_left(curr_psngr);
    sem_wait(&special_kiosk);
    print("Passenger " + curr_psngr->getname() + " has started waiting for his lost boarding pass in special kiosk\
            at time " );
    sleep(w);
    sem_post(&special_kiosk);
    print("Passenger " + curr_psngr->getname() + " has got his lost boarding pass at time ");
    VIP_left_to_right(curr_psngr);
    board_on_plane(curr_psngr);
}

void * passenger_tasks(void* arg)
{
	// printf("%s\n",(char*)arg);
	passenger* curr_psngr = (passenger*) arg;
	kiosk_avail(curr_psngr);
	if(curr_psngr->VIP == true){
        VIP_left_to_right(curr_psngr);
	}
	else{
        security_check(curr_psngr);
	}
	board_on_plane(curr_psngr);

	return 0;

}



int main(void)
{
    // for milliseconds, use using ms = std::chrono::duration<double, std::milli>;

    take_input();
    cout << "Number of kiosk : " << m << endl;
    cout << "Number of belts for security check : " << n << endl;
    cout << "Number of passengers belt can serve : " << p <<endl;
    cout << "Self check-in at kiosk : " << w << endl;
    cout << "Security check : " << x << endl;
    cout << "Boarding at the gate : " << y << endl;
    cout << "Walking on VIP channel in either direction : " << z << endl;

    output_file.open("output.txt");
    //init_semaphore();
    pthread_cond_init(&cond_left, 0);
    pthread_cond_init(&cond_right, 0);
    pthread_mutex_init(&mtx_vip, 0);
    pthread_mutex_init(&mtx_vipr, 0);
    pthread_mutex_init(&mtx_print, 0);
    sem_init(&board_pass,0, m);
    sem_init(&special_kiosk,0, 1);
    sem_init(&boarding_line,0, 1);
    belts = new sem_t[n];
    for(int i = 0 ; i < n ; i++){
        sem_init(&belts[i],0, p);
    }

    const int nrolls = 10000; // number of experiments
    const int nstars = 100;   // maximum number of stars to distribute

    default_random_engine generator;
    poisson_distribution<int> distribution(5);

    int pois[10]={};

    for (int i=0; i<nrolls; ++i) {
        int number = distribution(generator);
        // cout << number << endl;
        if (number<10) ++pois[number];
    }

    // randomness for VIP status
    srand(time(0));

    for(int i = 0 ; i < m ; i++ ){
        kiosk_q.push(i);
    }

    // cout << "poisson_distribution (mean=5):" << endl;

    for (int i=0; i<10; ++i){

        // cout << i << ": " << string( pois[i]*nstars/nrolls,'*') << endl;
        // sleep(pois[i]*nstars/nrolls);
        int arrived = pois[i]*nstars/nrolls;
        for ( int j = 0 ; j < arrived ; j++ ){
            passenger* psngr = new passenger(id, rand() % 2);
            print("Passenger " + psngr->getname() + " arrived at the airport at time ");
            id++;
            pthread_t thread1;
            pthread_create(&thread1, NULL, passenger_tasks, psngr);
        }
        sleep(2);

    }


    while(1);

	return 0;
}
