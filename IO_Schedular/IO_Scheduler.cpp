//
//  main.cpp
//  Assignment_4
//
//  Created by Shreya Pandey on 12/10/19.
//  Copyright © 2019 Operating Systems. All rights reserved.
//

#include <iostream>
#include<queue>
#include <fstream>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iomanip>
#include<vector>
#include<queue>
#include<climits>
#include<stack>

using namespace std;

int no_process=0;
int tot_mov = 0;
string myfile;
ifstream infile;
bool verbose;
int head=0;
int dir = 1;
//bool AQ = false;
bool active_IO = false;

struct TableFormat {
    int width;
    char fill;
    TableFormat(): width(14), fill(' ') {}
    template<typename T>
    TableFormat& operator<<(const T& data) {
        std::cout << data << std::setw(width) << std::setfill(fill);
        return *this;
    }
    TableFormat& operator<<(std::ostream&(*out)(std::ostream&)) {
        std::cout << out;
        return *this;
    }
};

class process{
    
public:
    int start_time;
    int finish_time;
    int pid;
    int arrival_time;
    int track;
    
    process(int st,int ft,int p, int at, int t){
        start_time = st;
        finish_time = ft;
        pid = p;
        arrival_time =at;
        track = t;
    }
};

queue<process*> process_queue;
deque<process*> IO_req_final;

class IO_Scheduler
{
public:
    virtual void add_process(process *inp)=0;
    virtual process* get_IO_Process()=0;
    //virtual bool test_preempt( process *p,int curtime)=0;
    vector <process*> IO_QUEUE;
};

IO_Scheduler *Sche;

//----------------- FIFO -----------------------

class FIFO :public IO_Scheduler {
public:
    //process* p;
    
    process* get_IO_Process(){
        //gives the first event from event Q
        if(IO_QUEUE.empty()) return nullptr;
        process* p;
        p = IO_QUEUE.front();
        IO_QUEUE.erase (IO_QUEUE.begin());
        return p;
    }
    
    void add_process(process* inp)
    {
        IO_QUEUE.push_back(inp);
    }
    //bool test_preempt( process *p,int curtime){return false;}
    virtual ~FIFO(){};
};

//------------------ SSTF -----------------------

class SSTF :public IO_Scheduler {
public:
    //process* p;
    
    process* get_IO_Process(){
        int diff= 0;
        int min_diff = INT_MAX;
        int index = 0;
        //gives the first event from event Q
        if(IO_QUEUE.empty()) return nullptr;
        process* p;
        for(int i = 0; i< IO_QUEUE.size(); i++){
            diff = (head - IO_QUEUE[i]->track);
            if (diff < 0) diff = (-diff);
            if (min_diff > diff) {
                min_diff = diff;
                index = i;
            }
        }
        p = IO_QUEUE[index];
        IO_QUEUE.erase (IO_QUEUE.begin()+(index));
        return p;
    }
    
    void add_process(process* inp)
    {
        IO_QUEUE.push_back(inp);
    }
    //bool test_preempt( process *p,int curtime){return false;}
    virtual ~SSTF(){};
};

//------------------ LOOK -----------------------

class LOOK :public IO_Scheduler {
public:
    //process* p;
    process* get_IO_Process(){
        int diff= 0;
        int min_diff = INT_MAX;
        int index = 0;
        //gives the first event from event Q
        if(IO_QUEUE.empty()) return nullptr;
        process* p;
        while(!IO_QUEUE.empty()){
        if(dir==1){
        for(int i = 0; i< IO_QUEUE.size(); i++){
            if(head <= IO_QUEUE[i]->track){
                diff = (head - IO_QUEUE[i]->track);
                if (diff < 0) diff = (-diff);
                    if (min_diff > diff) {
                        min_diff = diff;
                        index = i;
                    }
            }
            else continue;
        }
        }
        
        if(min_diff == INT_MAX) dir = -1;
        
        if(dir==-1){
            for(int i = 0; i< IO_QUEUE.size(); i++){
                if(head >= IO_QUEUE[i]->track){
                diff = (head - IO_QUEUE[i]->track);
                if (diff < 0) diff = (-diff);
                if (min_diff > diff) {
                    min_diff = diff;
                    index = i;
                }
            }
            }
        }
            if (min_diff==INT_MAX) {
                dir = 1;
                continue;
            }
            p = IO_QUEUE[index];
            IO_QUEUE.erase (IO_QUEUE.begin()+(index));
            return p;
        }
    
        return nullptr;
    }
    void add_process(process* inp)
    {
        IO_QUEUE.push_back(inp);
    }
    //bool test_preempt( process *p,int curtime){return false;}
    virtual ~LOOK(){};
};

//------------------ CLOOK ----------------------

class CLOOK :public IO_Scheduler {
public:
    //process* p;
    process* get_IO_Process(){
        int h = head;
        int diff= 0;
        int min_diff = INT_MAX;
        int index = 0;
        //gives the first event from event Q
        if(IO_QUEUE.empty()) return nullptr;
        process* p;
        while(!IO_QUEUE.empty()){
        for(int i = 0; i< IO_QUEUE.size(); i++){
            if(h <= IO_QUEUE[i]->track){
                diff = (h - IO_QUEUE[i]->track);
                if (diff < 0) diff = (-diff);
                    if (min_diff > diff) {
                        min_diff = diff;
                        index = i;
                    }
            }
            else continue;
        }
            if(min_diff==INT_MAX){
                
                h = 0;
                continue;
            }
        p = IO_QUEUE[index];
        IO_QUEUE.erase (IO_QUEUE.begin()+(index));
        return p;
        }
    
        return nullptr;
    }
    
    void add_process(process* inp)
    {
        IO_QUEUE.push_back(inp);
    }
    //bool test_preempt( process *p,int curtime){return false;}
    virtual ~CLOOK(){};
};

//------------------ FLOOK ----------------------


class FLOOK :public IO_Scheduler {
public:
    vector <process*> QUEUE1;
    vector <process*> QUEUE2;
    vector <process*> *Arr[2];
    bool active_Q = true;
    bool flip = true;
    FLOOK(){
        Arr[0] = &QUEUE1 ;
        Arr[1] = &QUEUE2 ;  //pointer to the queue of pointers to processes
    }
    process* get_IO_Process(){
        int diff= 0;
        int min_diff = INT_MAX;
        int index = 0;
        
        process* p;
        while(!Arr[0]->empty()){
        if(dir == 1){
        for(int i = 0; i< Arr[0]->size(); i++){
            if(head <= (*Arr[0])[i]->track){
                diff = (head - (*Arr[0])[i]->track);
                if (diff < 0) diff = (-diff);
                    if (min_diff > diff) {
                        min_diff = diff;
                        index = i;
                    }
            }
            else continue;
        }
        }
        
        if(min_diff == INT_MAX) dir = -1;
        
        if(dir==-1){
            for(int i = 0; i< Arr[0]->size(); i++){
                if(head >= (*Arr[0])[i]->track){
                diff = (head - (*Arr[0])[i]->track);
                if (diff < 0) diff = (-diff);
                if (min_diff > diff) {
                    min_diff = diff;
                    index = i;
                }
            }
            }
        }
            if (min_diff==INT_MAX) {
                dir = 1;
                continue;
            }
            p = (*Arr[0])[index];
            (*Arr[0]).erase ((*Arr[0]).begin()+(index));
            if(Arr[0]->empty()) active_Q = false;
            return p;
        }
        
        if (flip){
            Arr[0] = &QUEUE2;
            Arr[1] = &QUEUE1;
            flip = false;
        }
        else{
            Arr[0] = &QUEUE1;
            Arr[1] = &QUEUE2;
            flip = true;
        }
        //---------------
        
        while(!Arr[0]->empty()){
        if(dir == 1){
        for(int i = 0; i< Arr[0]->size(); i++){
            if(head <= (*Arr[0])[i]->track){
                diff = (head - (*Arr[0])[i]->track);
                if (diff < 0) diff = (-diff);
                    if (min_diff > diff) {
                        min_diff = diff;
                        index = i;
                    }
            }
            else continue;
        }
        }
        
        if(min_diff == INT_MAX) dir = -1;
        
        if(dir==-1){
            for(int i = 0; i< Arr[0]->size(); i++){
                if(head >= (*Arr[0])[i]->track){
                diff = (head - (*Arr[0])[i]->track);
                if (diff < 0) diff = (-diff);
                if (min_diff > diff) {
                    min_diff = diff;
                    index = i;
                }
            }
            }
        }
            if (min_diff==INT_MAX) {
                dir = 1;
                continue;
            }
            p = (*Arr[0])[index];
            (*Arr[0]).erase ((*Arr[0]).begin()+(index));
            if(Arr[0]->empty()) active_Q = false;
            return p;
        }
      
        return nullptr;
    }
    
    void add_process(process* inp){
        if(active_Q){
            Arr[0]->push_back(inp);
        }
        if(!active_Q)
        {
         Arr[1]->push_back(inp);
        }
    }
    virtual ~FLOOK(){};
};

//------------------- get options -------------------------
void get_options(int argc, char* argv[]){
    
    string sch_algo = "";
    int c;
    opterr = 0;
    while ((c = getopt(argc, argv, "vs:")) != -1)
        switch (c)
    {
        case 'v':
            verbose = true;
            break;
        case 's':
            sch_algo = optarg;
            break;
        case '?':
            if (optopt == 's')
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf (stderr,
                         "Unknown option character `\\x%x'.\n",
                         optopt);
        default: abort ();
    }
        if (sch_algo[0]=='i')   Sche = new FIFO();
        else if (sch_algo[0]=='j')  Sche=new SSTF();
        else if (sch_algo[0] =='s') Sche=new LOOK();
        else if (sch_algo[0]=='c')  Sche=new CLOOK();
        else if (sch_algo[0]=='f')  Sche=new FLOOK();
        else{
            cout << "INVALID ARGUMENT" << endl;
        }
}


//-------------------------- SIMULATION -------------------------


class Simulation {
public:
    int mov = 0;
    int TIME = 0;
    int curr_proc = 0;
    int curr_track = 0;
    process* CRunP;
    
    process* get_process(){
           //gives the first event from event Q
           if(process_queue.empty()) return nullptr;
           process* p;
           p = process_queue.front();
           process_queue.pop();
           return p;
       }
    
    int get_next_process_time()
    {
        process* p;
        if(!process_queue.empty()){
            p = process_queue.front();
            return p->arrival_time;
        }
        return -1;
    }
    
    void put_finish_IO(process* inp){
        // Insert an event (insertion sort) in the event Q
        stack <process*> S;
        process t=*inp;
        
        if(IO_req_final.empty())
        {
            IO_req_final.push_front(inp);
        }
        else
        {
            while(t.pid >= (IO_req_final.front())->pid)
            {
                S.push(IO_req_final.front());
                IO_req_final.pop_front();
                if(IO_req_final.empty()){
                    break;
                }
            }
            IO_req_final.push_front(inp);
            while(!S.empty())
            {
                //cout << "I" << endl;
                IO_req_final.push_front(S.top());
                S.pop();
            }
        }
        
    }
    
    void main_exe(){
        if(verbose)  cout << "TRACE" << endl;
        
        while (curr_proc < no_process)
        {
            process* proc=nullptr;
            
            //compare current time with netprocess arrival time
            if(TIME == get_next_process_time() )
            {
                //call get process
                proc = get_process();
                // put it in IO_QUEUE
                if(verbose){
                        string process_id ="      ";
                        process_id = process_id + to_string(proc->pid);
                        process_id = process_id.substr(process_id.size() - 6);
                        cout << TIME << ":" << process_id << " add " << proc->track << endl;
                }
                Sche->add_process(proc);
            }
            //if no current active IO process then pick a process from IO
            if(active_IO){
                if(head == CRunP->track){
                    //88:     0 finish 87
                    if(verbose){
                            string process_id ="      ";
                            process_id = process_id + to_string(CRunP->pid);
                            process_id = process_id.substr(process_id.size() - 6);
                            cout << TIME << ":" << process_id << " finish " << TIME - CRunP->arrival_time << endl;
                    }
                    CRunP->finish_time = TIME;
                    put_finish_IO(CRunP);
                    //IO_req_final.push(CRunP);
                    curr_proc += 1;
                    curr_track = CRunP->track;
                    CRunP = nullptr;
                    active_IO = false;
                }
            else if(head < CRunP->track) head++;
            else head--;
           }
           if(!active_IO) {
                   CRunP = Sche->get_IO_Process();
                    if(CRunP != nullptr){
                    
                        if(verbose){
                                string process_id ="      ";
                                process_id = process_id + to_string(CRunP->pid);
                                process_id = process_id.substr(process_id.size() - 6);
                                cout << TIME << ":" << process_id << " issue " << CRunP->track << " " << curr_track << endl;
                        }
                    //calculating movement
                    mov = (CRunP->track - curr_track);
                    if(mov<0) tot_mov += (-mov);
                    else tot_mov += mov;
                    CRunP->start_time = TIME;
                    active_IO = true;
                    if(head < CRunP->track) head++;
                    else if(head < CRunP->track) head--;
                    else continue;
                }
           }
            TIME ++;
            }
}
};

//--------------------- PRINT SUMMARY -------------------------
void print_summary(){
    TableFormat out;
    out.width = 6 ;
    double turn_time=0;
    double wait_time = 0;
    double max_wait = 0;
    double total_time = 0;
    process *p=nullptr;
    while(!IO_req_final.empty()){
        p = IO_req_final.front();
        IO_req_final.pop_front();
        turn_time += (p->finish_time-p->arrival_time);
        wait_time += (p->start_time - p->arrival_time);
        if(total_time <= p->finish_time) total_time = p->finish_time;
        if(max_wait < (p->start_time - p->arrival_time) ) max_wait=(p->start_time - p->arrival_time);
        
        out.width = 6 ;
        string process_id ="      " + to_string(p->pid)+ ":" ;
        out << process_id.substr(process_id.size() - 6);
        out << p->arrival_time << p->start_time;
        out.width = 0 ;
        out << p->finish_time;
        out << endl;
    }
    cout << "SUM: " << total_time << " " << tot_mov <<std::fixed << setprecision(2)<< " " << turn_time/no_process << " " << wait_time/no_process << " " <<setprecision(0) << max_wait << endl;
}



int main(int argc,char * argv[]) {
    
    get_options(argc,argv);
    
    //Read the input file and create process Queue.
    myfile = argv[argc-1];
    infile.open(myfile);
    string line;

    // Read input file and create processes
    int at,trak;
    getline(infile,line);
    getline(infile,line);
    //Read file and update process queue
    while(infile >> at >> trak){
        //cout << at << trak << endl;
        //int stat_prio = myrandom(max_prio);
        process p(0,0,no_process,at,trak);
        process_queue.push(new process(p));
        no_process++;
    }

    Simulation Sim;
    
    Sim.main_exe();
    
    print_summary();
    
    return 0;
}
