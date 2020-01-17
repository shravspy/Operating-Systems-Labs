//
//  main.cpp
//  Assignment_2_Version1
//
//  Created by Shreya Pandey on 12/1/19.
//  Copyright © 2019 Operating Systems. All rights reserved.
//

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sstream>
#include <list>
#include <cstring>
#include <deque>
#include <queue>
#include <iostream>
#include <fstream>
#include<map>
#include<stack>
#include <iomanip>


using namespace std;

int max_prio = 4;  //default
int quantum = 10000; // default
bool verbose = false;
int run_status = 0;
string name="";
int I_O_time = 0;
int C = -1;
int F = -1;


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


typedef enum { CREATED,READY, RUNNING , BLOCK, PREEMPTION,DONE} process_state;


int * Random = NULL;
int size;

// Initialize Random File
void Initialize_random(char * Fl){
    
    ifstream rfile(Fl);
    string line;
    getline(rfile,line);
    size = atoi(line.c_str());
    Random = new int[size];  //pointer to array of numbers
    
    for(int i = 0; i< size; i++)
    {
        int num;
        getline(rfile,line);
        num = atoi(line.c_str());
        Random[i]=num;
    }
    rfile.close();
    
}

// Random Number Generator
int ofs = 0;

int myrandom(int burst){
    if(ofs == size) ofs = 0; //offset wrap around
    int val = 1+ (Random[ofs] % burst);
    ofs++;
    return val;
}


//Process class
class process{
    //Arrival time , TOTAL CPU time , CPU Burst , IO Burst
public:
    int pid ;           // pid number
    int at ;            //arrival time
    int tc;             // total cpu time
    int ini_cb ;        // cpu burst
    int ini_IO;         // IO burst
    int static_prio;
    int dynamic_prio;
    int state_tc ;      // current time of previous state
    int ini_tc;
    int rem_cpu_b;
    
    process(int id, int a, int t,int icb,int iob,int s_prio,int d_prio, int s_tc,int ini_t,int cpu_b)
    {
        pid = id;
        at = a;
        tc = t;
        ini_cb = icb;
        ini_IO = iob;
        static_prio = s_prio;
        dynamic_prio = d_prio;
        state_tc = s_tc;
        ini_tc = ini_t;
        rem_cpu_b = cpu_b;
    }
    
};
class process_sum {
public:
    int pid;
    int finishing_time ;
    int turnaround_time;
    int IO_time;
    int CPU_wait_time;
    //bool io_flag;
    
    process_sum(){
        finishing_time = turnaround_time = IO_time = CPU_wait_time = 0;
        //io_flag = false;
    }
};

vector<process_sum> proc_sum;

process* CRunP = NULL;

class event{
public:
    int transition_time,time_initial_state;  //dynamic_prio,static_prio;
    process* procp;
    process_state from_state,to_state;
    int rem_time;
    //event(){};
    event(int ttime,process* p,int ts,process_state from_s, process_state to_s, int rem)
    {
        transition_time = ttime;
        procp = p;
        time_initial_state = ts;
        //time_created=tc;
        from_state = from_s ;
        to_state = to_s;
        rem_time = rem;
    }
    
    
};

// created double sided queue of process pointers
deque <process*> proces;

// eve is event pointer queue
deque <event*> eve;

class scheduler
{
public:
    virtual void add_process(process *inp)=0;
    virtual process* get_next_process()=0;
    virtual bool test_preempt( process *p,int curtime)=0;
    deque <process*> RUN_QUEUE;
    virtual ~scheduler(){};
};

scheduler *sche;
bool preempt = false;

//----------------- FCFS -----------------------

class FCFS :public scheduler {
public:
    process* p;
    process* get_next_process()
    {
        
        if (!RUN_QUEUE.empty())
        {
            p = RUN_QUEUE.front();
            //cout << p << endl;
            //cout << p->pid << endl;
            RUN_QUEUE.pop_front();
            return p;
        }
        return NULL;
    }
    
    void add_process(process* inp)
    {
        RUN_QUEUE.push_back(inp);
        //p = RUN_QUEUE.front();
    }
    bool test_preempt( process *p,int curtime){return false;}
    virtual ~FCFS(){};
};

// --------------- LCFS ------------------------

class LCFS :public scheduler {
public:
    process* p;
    process* get_next_process()
    {
        
        if (!RUN_QUEUE.empty())
        {
            p = RUN_QUEUE.back();
            RUN_QUEUE.pop_back();
            return p;
        }
        return NULL;
    }
    
    void add_process(process* inp)
    {
        RUN_QUEUE.push_back(inp);
    }
    bool test_preempt( process *p,int curtime){return false;}
    virtual ~LCFS(){};
};

//--------------- SRTF ------------------------

class SRTF :public scheduler {
public:
    process* p;
    process* get_next_process()
    {
        
        if (!RUN_QUEUE.empty())
        {
            p = RUN_QUEUE.front();
            RUN_QUEUE.pop_front();
            return p;
        }
        return NULL;
    }
    
    void add_process(process* inp)
    {
            // Insert an process (insertion sort) in the RUN Q
            stack <process*> S;
            if(RUN_QUEUE.empty())
            {
                RUN_QUEUE.push_front(inp);
            }
            else
            {
                while(inp->tc >= (RUN_QUEUE.front())->tc)
                {
                    S.push(RUN_QUEUE.front());
                    RUN_QUEUE.pop_front();
                    if(RUN_QUEUE.empty()){
                        break;
                    }
                }
                RUN_QUEUE.push_front(inp);
                while(!S.empty())
                {
                    RUN_QUEUE.push_front(S.top());
                    S.pop();
                }
            }
    }
    
    bool test_preempt( process *p,int curtime){return false;}
    virtual ~SRTF(){};
};

//--------------------- Round Robin ------------------------

class RR :public scheduler {
public:
    process* p;
    process* get_next_process()
    {
        
        if (!RUN_QUEUE.empty())
        {
            p = RUN_QUEUE.front();
            RUN_QUEUE.pop_front();
            return p;
        }
        return NULL;
    }
    
    void add_process(process* inp)
    {
        RUN_QUEUE.push_back(inp);
    }
    bool test_preempt( process *p,int curtime){ return false;}
    virtual ~RR(){};
};

//------------------ Priority Scheduler -----------------------

class PRIO :public scheduler {
public:
    
    queue <process*> *activeQ;
    queue <process*> *expiredQ; //pointer to the queue of pointers to processes
    process* p;
    PRIO(){
         activeQ = new queue<process*>[max_prio];
         expiredQ = new queue<process*>[max_prio];  //pointer to the queue of pointers to processes
        
        
    }
    process* get_next_process()
    {
        
        for(int i = max_prio-1; i > -1 ; i--)
        {
            queue <process*> *a = &activeQ[i];
            if(a->size() == 0) continue;
            process *p = a->front();
            //add in expired queue
            a->pop();
            p->dynamic_prio = p->dynamic_prio-1;
            return p;
        }
        //swap
        queue<process*> *swap = activeQ;
        activeQ = expiredQ;
        expiredQ = swap;
        
        for(int i = max_prio-1; i > -1 ; i--)
        {
            queue <process*> *a = &activeQ[i];
            if(a->size() == 0) continue;
            process *p = a->front();
            //add in expired queue
            a->pop();
            p->dynamic_prio = p->dynamic_prio-1;
            return p;
        }
        return NULL;
    }
    
    void add_process(process* inp)
    {
        if(inp->dynamic_prio == -1){
            inp->dynamic_prio = inp->static_prio - 1;
            int ind = inp->dynamic_prio;
            if(&expiredQ[ind] == nullptr || expiredQ[ind].size()==0){
                queue <process*> a;
                a.push(inp);
                expiredQ[ind] = a;
            }
            else{
                queue <process*> *a = &expiredQ[ind];
                a->push(inp);
            }
            
        }
        else{
        int ind = inp->dynamic_prio;
        if(&activeQ[ind] == nullptr || activeQ[ind].size()==0){
            queue <process*> a;
            a.push(inp);
            activeQ[ind] = a;
        }
        else{
            queue <process*> *a = &activeQ[ind];
            a->push(inp);
        }
        }
    }
    
    bool test_preempt( process *p,int curtime){ return false;}
    virtual ~PRIO(){};
};

//-------------------- PREemptive PRIO -------------------------
class PREPRIO :public scheduler {
public:
    
    queue <process*> *activeQ;
    queue <process*> *expiredQ; //pointer to the queue of pointers to processes
    process* p;
    PREPRIO(){
        activeQ = new queue<process*>[max_prio];
        expiredQ = new queue<process*>[max_prio];  //pointer to the queue of pointers to processes
    }
    process* get_next_process()
    {
        
        for(int i = max_prio-1; i > -1 ; i--)
        {
            queue <process*> *a = &activeQ[i];
            if(a->size() == 0) continue;
            process *p = a->front();
            a->pop();
            p->dynamic_prio = p->dynamic_prio-1;
            return p;
        }

        queue<process*> *swap = activeQ;
        activeQ = expiredQ;
        expiredQ = swap;
        
        for(int i = max_prio-1; i > -1 ; i--)
        {
            queue <process*> *a = &activeQ[i];
            if(a->size() == 0) continue;
            process *p = a->front();
            a->pop();
            p->dynamic_prio = p->dynamic_prio-1;
            return p;
        }
        return NULL;
    }
    
    void add_process(process* inp)
    {
        
        if(inp->dynamic_prio == -1){
            inp->dynamic_prio = inp->static_prio - 1;
            int ind = inp->dynamic_prio;
            if(&expiredQ[ind] == nullptr || expiredQ[ind].size()==0){
                queue <process*> a;
                a.push(inp);
                expiredQ[ind] = a;
            }
            else{
                queue <process*> *a = &expiredQ[ind];
                a->push(inp);
            }
            
        }
        else{
            int ind = inp->dynamic_prio;
            if(&activeQ[ind] == nullptr || activeQ[ind].size()==0){
                queue <process*> a;
                a.push(inp);
                activeQ[ind] = a;
            }
            else{
                queue <process*> *a = &activeQ[ind];
                a->push(inp);
            }
        }
    }
    
    bool test_preempt( process *p,int curtime){
        if(CRunP == nullptr) return false;
        else if(run_status == curtime) return false;
        else{
            if(p->dynamic_prio > CRunP->dynamic_prio+1){
                return true;
            }
            
        }
        
        
        return false;
    }
    virtual ~PREPRIO(){};
};


//------------------- get options -------------------------
void get_options(int argc, char **argv){
    
    string sch_algo = "";
    int c;
    opterr = 0;
    //bool preempt = false;
    
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
        default:
            abort ();
    }
    int i =0;
    string quant=" ";
    if (sch_algo[i]=='F'){
        sche = new FCFS();
        name = "FCFS";
        
    }
    else if (sch_algo[i]=='L'){
        sche=new LCFS();
        name = "LCFS";
    }
    else if (sch_algo[i] =='S'){
        sche=new SRTF();
        name = "SRTF";
        
    }
        else if (sch_algo[i]=='R')
        {
            quantum = stoi(sch_algo.substr(i+1));
            sche=new RR();
            name = "RR "+to_string(quantum);
        }
        else if (sch_algo[i]=='P')
        {
            string input = sch_algo.substr(i+1);
            //cout << input << endl;
            //split input into quantum and priority
            if(input.length() >= 3){
                int j = 0;
                for(j = 0; j<input.length(); j++){
                    if(input[j] != ':') quant = quant + input[j];
                    else break;
                }
                quantum =stoi(quant);
                max_prio = stoi(input.substr(j+1));
            }
            else{
                quantum = stoi(input);
            }
            sche=new PRIO();
            name = "PRIO " + to_string(quantum);
        }
        else if (sch_algo[i]=='E'){
              string input = sch_algo.substr(i+1);
              //split input into quantum and priority
            if(input.length() >= 3){
                int j = 0;
                for(j = 0; j<input.length(); j++){
                    if(input[j] != ':') quant = quant + input[j];
                    else break;
                }
                quantum =stoi(quant);
                max_prio = stoi(input.substr(j+1));
            }
            else quantum = stoi(input);
            sche=new PREPRIO();
            name = "PREPRIO " + to_string(quantum);
            preempt = true;
        }
    
        else{
            cout << "INVALID ARGUMENT" << endl;
        }
}


//------------------PRINT_SUMMARY --------------------------

queue <process*> final_proc;

void print_summary(){
    TableFormat out;
    out.width = 5 ;
    double final_finish =0;
    double CPU_utilization;
    double IO_utilization;
    double avg_turn_time;
    double avg_cpu_wait;
    double throughput;
    double sum_arri=0;
    double sum_it = 0;
    double sum_tt = 0;
    double sum_cpu = 0;
    double no_proc = 0;
    double sum_ini_cpu = 0;
    
    cout<< name << endl;
    //cout << I_O_time + (F-C) << endl;
    for(int i = 0; i<proc_sum.size(); i++){
        process* ps = final_proc.front();
        final_proc.pop();
        sum_tt +=proc_sum[i].turnaround_time;
        sum_cpu += proc_sum[i].CPU_wait_time;
        sum_arri += ps->at;
        sum_it += proc_sum[i].IO_time;
        no_proc += 1;
        if(final_finish < proc_sum[i].finishing_time) final_finish = proc_sum[i].finishing_time;
        sum_ini_cpu += ps->ini_tc;
        out.width = 5 ;
        string process_id ="0000" + to_string(i);
        out << process_id.substr(process_id.size() - 4) +":";
        out << ps->at << ps->ini_tc << ps->ini_cb << ps->ini_IO<< ps->static_prio <<"|" << proc_sum[i].finishing_time<<proc_sum[i].turnaround_time << proc_sum[i].IO_time;
        out.width = 0 ;
        out << proc_sum[i].CPU_wait_time;
        out << endl;
    }

    CPU_utilization = (sum_ini_cpu/final_finish)*100;
    IO_utilization = ((I_O_time + (F-C)) / final_finish) * 100;
    avg_turn_time = sum_tt/no_proc;
    avg_cpu_wait = sum_cpu/no_proc;
    throughput = (no_proc*100)/final_finish;
    
    
    cout << "SUM: " << final_finish << " "<<std::fixed << setprecision(2) << CPU_utilization << " " << IO_utilization << " "<<avg_turn_time << " " << avg_cpu_wait <<" "<< setprecision(3) <<throughput <<  endl;
    
}

//------------------------------SIMULATION -----------------------



class DESimulation {
public:
    int time = 0;
    //bool symbol = false;
    event* get_event(){
        //gives the first event from event Q
        if(eve.empty()) return nullptr;
        event* evt;
        evt = eve.front();
        eve.pop_front();
        return evt;
    }
    
    void put_event(event* inp){
        // Insert an event (insertion sort) in the event Q
        stack <event*> S;
        //deque <event*> Q;
        event t=*inp;
        
        if(eve.empty())
        {
            eve.push_front(inp);
        }
        else
        {
            while(t.transition_time >= (eve.front())->transition_time)
            {
                S.push(eve.front());
                eve.pop_front();
                if(eve.empty()){
                    break;
                }
            }
            eve.push_front(inp);
            while(!S.empty())
            {
                //cout << "I" << endl;
                eve.push_front(S.top());
                S.pop();
            }
        }
        
    }
    
    void rm_event(int crrtime, int pre){
        stack <event*> S;
        while(true)
            {
                int temp = 0;
               
                if((pre + CRunP->state_tc) < (CRunP->state_tc + quantum)) temp =(pre + CRunP->state_tc);
                else temp = (CRunP->state_tc + quantum);
                if((CRunP->pid == (eve.front()->procp)->pid) && eve.front()->transition_time == temp){
                    //cout << "PRINT" << endl;
                    eve.pop_front();
                    break;
                }
                else {
                        S.push(eve.front());
                        eve.pop_front();
                        if(eve.empty()){
                        break;
                    }
                }
            }
        
        while(!S.empty())
            {
                eve.push_front(S.top());
                S.pop();
            }
    }
    
    int get_next_event_time()
    {
        event* evt;
        if(!eve.empty()){
            evt = eve.front();
            return evt->transition_time;
        }
        return -1;
    }
    
    void main_exe()
    {
        event* evt;
        bool CALL_SCHEDULER = false;
        int Q;
        int pre_empt_cpu = 0 ;
        //int C1,F1 = 0;
        
        while ((evt = get_event()))
        {
            process* proc = evt->procp;
            int CURRENT_TIME = evt->transition_time;
            int timeInPrevState = CURRENT_TIME - proc->state_tc;
            
            
            switch(evt->to_state)
            {
                case READY:{
                    //must come from BLOCK or from PREEMPTION or from CREATED
                    // must add to RUN QUEUE
                    if(verbose){
                    if(evt->from_state == CREATED){
                    cout << CURRENT_TIME << " " << proc->pid << " " << timeInPrevState << ": " << "CREATED" << " -> " << "READY" << endl;
                    }
                    else if(evt->from_state == BLOCK){
                        cout << CURRENT_TIME << " " << proc->pid << " " << timeInPrevState << ": " << "BLOCK" << " -> " << "READY" << endl;
                    }
                    }
                    //Case Preemption
                    if(preempt)
                    {
                        if(sche->test_preempt(proc,CURRENT_TIME))
                        {
                            CRunP->tc =  time - (CURRENT_TIME - CRunP->state_tc);
                            CRunP->rem_cpu_b =  pre_empt_cpu - (CURRENT_TIME - CRunP->state_tc) ;
                            //create event for RUNNING TO PREEMPTION
                            process_state from_state = RUNNING;
                            process_state to_state = PREEMPTION;
                            event e(CURRENT_TIME,CRunP,
                                    CRunP->state_tc,from_state,to_state,CRunP->tc);
                            put_event(new event(e));
                            run_status = CURRENT_TIME;
                            rm_event(CURRENT_TIME,pre_empt_cpu);
                        }
                    }
                    
                    proc->state_tc = CURRENT_TIME;
                    sche->add_process(proc);
                    CALL_SCHEDULER = true; //conditional on whether something is run
                    break;
                }
                case DONE:{
                    if (verbose){
                    cout << CURRENT_TIME  << " " << proc->pid << " " <<  timeInPrevState <<": Done" << endl;
                    }
                    proc_sum[proc->pid].finishing_time = CURRENT_TIME;
                    proc_sum[proc->pid].turnaround_time = CURRENT_TIME- proc->at;
                    if(CURRENT_TIME >= run_status) CRunP = nullptr;
                    CALL_SCHEDULER = true;
                    break;
                    
                }
                case RUNNING:{
                    process_state from_state = RUNNING;
                    process_state to_state;
                    if (quantum >= proc->rem_cpu_b)
                    {
                    //create event for either preemption or blocking
                    Q = proc->rem_cpu_b;
                    to_state = BLOCK;
                    }
                    else{
                         Q=quantum;
                         to_state = PREEMPTION;
                    }
                    if(proc->rem_cpu_b >= evt->rem_time){
                        proc->rem_cpu_b = evt->rem_time;
                    }
                    if(proc->rem_cpu_b <= quantum)
                    {
                        if(proc->rem_cpu_b >= evt->rem_time){
                            proc->rem_cpu_b = evt->rem_time;
                        if(verbose){
                            cout << CURRENT_TIME << " " << proc->pid << " " << timeInPrevState << ": " << "READY" << " -> " << "RUNNG" << " cb=" << proc->rem_cpu_b <<" rem=" << evt->rem_time << " prio=" << proc->dynamic_prio+1 << endl;
                        }
                        proc_sum[proc->pid].CPU_wait_time = proc_sum[proc->pid].CPU_wait_time + timeInPrevState;
                        pre_empt_cpu = proc->rem_cpu_b;
                        process_state from_state = RUNNING;
                        process_state to_state = DONE;
                        proc->state_tc = CURRENT_TIME;
                        time = evt->rem_time;
                        int rem_time = 0;
                        run_status = CURRENT_TIME + evt->rem_time;
                        event e(CURRENT_TIME + evt->rem_time,proc, proc->state_tc,from_state,to_state,rem_time);
                        put_event(new event(e));
                        if(CURRENT_TIME >= run_status) CRunP = nullptr;
                        break;
                        
                    }
                    }
                    
                    if(verbose){
                        cout << CURRENT_TIME << " " << proc->pid << " " << timeInPrevState << ": " << "READY" << " -> " << "RUNNG" << " cb=" << proc->rem_cpu_b <<" rem=" << evt->rem_time << " prio=" << proc->dynamic_prio+1 << endl;
                    }
                        proc_sum[proc->pid].CPU_wait_time = proc_sum[proc->pid].CPU_wait_time + timeInPrevState;
                        run_status = CURRENT_TIME + Q;
                        pre_empt_cpu = proc->rem_cpu_b;
                        proc->rem_cpu_b = proc->rem_cpu_b - Q ;
                        proc->rem_cpu_b = proc->rem_cpu_b;
                        int rem_time = evt->rem_time - Q;
                        proc->tc = rem_time;
                        time = evt->rem_time;
                        proc->state_tc = CURRENT_TIME;
                        event e(CURRENT_TIME + Q,proc,
                                proc->state_tc,from_state,to_state,rem_time);
                        put_event(new event(e));
                        break;
                }
                case BLOCK:{
                    int io_b = myrandom(proc->ini_IO);
                    proc_sum[proc->pid].IO_time = proc_sum[proc->pid].IO_time + io_b;
                    if(CURRENT_TIME >= F){
                        I_O_time += F-C;
                        F = CURRENT_TIME +io_b;
                        C = CURRENT_TIME;
                    }
                    else{
                        if(CURRENT_TIME+io_b > F) F = CURRENT_TIME +io_b;
                    }
                
                   
                    if(verbose){
                    cout << CURRENT_TIME << " " << proc->pid << " " << timeInPrevState << ": " << "RUNNG" << " -> " << "BLOCK " << " ib=" << io_b <<" rem=" << evt->rem_time << endl;
                    }

                    process_state from_state = BLOCK ;
                    process_state  to_state = READY;
                    proc->state_tc = CURRENT_TIME;
                    //All: When a process returns from I/O its dynamic priority is reset (to (static_priority-1).
                    proc->dynamic_prio = proc->static_prio - 1;
                    event e(CURRENT_TIME + io_b,proc, proc->state_tc,from_state,to_state,evt->rem_time);
                    put_event(new event(e));
                    //if current time is more than time it took to run a process in prev state i.e no process is running
                    if(CURRENT_TIME >= run_status) CRunP = nullptr;
                              
                    CALL_SCHEDULER = true;
                    break;
                }
                case PREEMPTION:{
                    if(verbose){
                    cout << CURRENT_TIME << " " << proc->pid << " " << timeInPrevState << ": " << "RUNNG" << " -> " << "READY " << " cb=" << proc->rem_cpu_b <<" rem=" << evt->rem_time << " prio=" << proc->dynamic_prio+1 << endl;
                    }
                    proc->state_tc = CURRENT_TIME;
                    sche->add_process(proc);
                    if(CURRENT_TIME >= run_status) CRunP = nullptr;
                    CALL_SCHEDULER = true;     //conditional on whether something is run
                    break;
                }
            }// switch ends here
            
            // remove current event object from Memory
            delete evt; evt=NULL;
            
            if(CALL_SCHEDULER){
                if (get_next_event_time() == CURRENT_TIME){
                    continue;   //process next event from
                }
                CALL_SCHEDULER = false; // reset global flag
                
                if (CRunP == nullptr)
                {
                    CRunP = sche->get_next_process();
                    if (CRunP == nullptr) continue;
                    else{
                        // create event to make this process runnable for same time
                        process_state from_state = READY ;
                        process_state  to_state = RUNNING;
                        if(CRunP->rem_cpu_b == 0){
                            CRunP->rem_cpu_b = myrandom(CRunP->ini_cb);
                        }
                        event e(CURRENT_TIME,CRunP, CRunP->state_tc, from_state, to_state,CRunP->tc);
                        put_event(new event(e));
                    }
                    
                }
            }
            
        } // while
        return ;
        
    } // main execution
};



int main(int argc,char * argv[]) {
    
    // Read random file and initialize
    Initialize_random(argv[argc-1]);
    get_options(argc,argv);
    
    // Read input file and create processes
    ifstream infile(argv[argc-2]);
    string line;
    int id = 0;
    int at,tc,cb,io;
    process_sum ps = process_sum();
    
    //Read file and update process queue
    while(infile >> at >> tc >> cb >> io){
        int stat_prio = myrandom(max_prio);
        process p(id,at,tc,cb,io,stat_prio,(stat_prio - 1),at,tc,0);
        proces.push_back(new process(p));
        final_proc.push(new process(p));
        proc_sum.push_back(ps);
        id++;
    }
    
    
    // Read process queue and update event queue
    
    while(!proces.empty()){
        process* p = proces.front();
        int s_time = 0,rem_time = p->tc;
        process_state from_state = CREATED ;
        process_state  to_state = READY;
        event e(p->at, p, s_time, from_state, to_state,rem_time);
        eve.push_back(new event(e));
        proces.pop_front();
    }
    
   
    DESimulation Simulation;
   
    Simulation.main_exe();
    print_summary();
    
    return 0;
}
