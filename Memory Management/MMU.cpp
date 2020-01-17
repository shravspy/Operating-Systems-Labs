//
//  main.cpp
//  Assignment_3
//
//  Created by Shreya Pandey on 11/23/19.
//  Copyright © 2019 Operating Systems. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include<cstring>
#include <limits.h>

using namespace std;


// define a class or struct for vm
struct vms {
    int start_vpage;
    int end_vpage;
    int write;
    int file_map;
};

class pstats{
public:
    int pid;
    int unmaps;
    int maps;
    int ins;
    int outs;
    int fins;
    int fouts;
    int zeros;
    int segv;
    int segprot;
    pstats()
    {
        pid = unmaps = maps = ins = outs = fins = fouts = zeros= segv= segprot= 0;
    }
};

class PTE
{
public:
    unsigned int present:1;
    unsigned int modified:1;
    unsigned int referenced:1;
    unsigned int pagedout:1;
    unsigned int write:1;
    unsigned int file_map:1;
    unsigned int frame_no:7;
    //unsigned int indexPhysical:6;
    PTE()
    {
        present=modified=referenced=pagedout=0;
        //frame_no=0;
    }
    
};   // can only be total of 32-bit size !!!!

class FTE {
public:
    
    int process;
    int pte_no;
    int f_no;
    unsigned int age:32;
    unsigned long time_last_use;
    FTE()
    {
        f_no = -1;
        //Map = 0;
        process = -1;
        pte_no = -1;
        age=0;
    }
    
};

int frame_count = 128;  //default
const int page_count = 64;
vector<vector<vms> > N_process;
vector<PTE*> pageV;
vector<vms> vm;
vector<pstats> p_stats;
vector<FTE> frame_table;
queue<int> allocated_frame_table;
string my_file;
ifstream infile;
long long COST = 0;
int m_unmaps = 400 ;
int  page_in_out =3000 ;
int file_in_out = 2500 ;
int zero = 150 ;
int segv = 240 ;
int  segprot = 300;
long inst_cost = 0;
int cnxt_switch = 0;
int p_exit = 0;
int inst_count = 0;
bool O= false; bool P= false; bool F = false; bool S=false;
vector<int> r ;
int ofs = 0;


// Initialize Random File
void Initialize_random(char * Fl){
    
    ifstream rfile(Fl);
    string l;
    getline(rfile,l);
    int len = atoi(l.c_str());
    //r.push_back(num);
    
    int i =0;
    while(i< len)
    {
        getline(rfile,l);
        int num = atoi(l.c_str());
        //cout << num << endl;
        r.push_back(num);
        i++;
    }
    rfile.close();
}
int myrandom(int burst){
    if(ofs==r.size()) ofs = 0;
    int val = (r[ofs] % burst);
    ofs++;
    return val;
    
}

void Frame_table_Output(){
    string FTE_output = "FT: ";
    for(int i = 0; i<frame_count; i++){
        //cout << i << " ";
        FTE frame = frame_table[i];
        if (frame.pte_no == -1) FTE_output += "* ";
        else FTE_output += to_string(frame.process)+":"+to_string(frame.pte_no)+" ";
    }
    //cout << "frame_count " << frame_count << endl;
    cout << FTE_output << endl;
}

void Page_Stats_Output(){
    string Page_Stats;
    //PROC[0]: U=10 M=26 I=0 O=4 FI=0 FO=0 Z=26 SV=0 SP=0
    //for ( int j=0; j<p_stats.size();j++){
      for(int i = 0; i<p_stats.size(); i++){
          //cout << p_stats.size() << endl;
        Page_Stats = "PROC[" + to_string(i) +"]: ";
        pstats ps = p_stats[i];
        Page_Stats += "U=" + to_string(ps.unmaps) + " M=" + to_string(ps.maps) + " I=" + to_string(ps.ins) + " O=" + to_string(ps.outs)  + " FI="+ to_string(ps.fins) + " FO=" + to_string(ps.fouts)  + " Z=" + to_string(ps.zeros) + " SV=" + to_string(ps.segv) + " SP=" + to_string(ps.segprot);
        cout << Page_Stats << endl;
    }
}

void Summary () {
    string summary = "TOTALCOST ";
    //printf("TOTALCOST %lu %lu %lu %llu\n", inst_count, ctx_switches, process_exits, cost);
    for (int i =0; i < p_stats.size(); i++){
        pstats ps = p_stats[i];
        COST += (((ps.unmaps+ps.maps)*m_unmaps) +
                ((ps.fins+ps.fouts)*file_in_out) +
                ((ps.outs+ps.ins)* page_in_out) +
                (ps.segv * segv) +(ps.segprot * segprot) + (ps.zeros* zero));
    }
    COST += inst_cost;
    summary += to_string(inst_count) + " " + to_string(cnxt_switch) + " " + to_string(p_exit) + " " + to_string(COST) ;
    cout << summary << endl;
    
}


void Page_table_Output() {
    for (int i=0; i<pageV.size();i++){
        string PTE_output="PT["+to_string(i)+"]: ";
        for(int j = 0;j <page_count; j++){
            PTE page = pageV[i][j];
            if (page.present==1)
            {
                if(page.referenced==1)PTE_output += to_string(j)+":R";
                else PTE_output += to_string(j)+":-";
                if(page.modified==1) PTE_output += "M";
                else PTE_output += "-";
                if(page.pagedout==1) PTE_output += "S ";
                else PTE_output += "- ";
           }
           else{
                if(page.pagedout==1)PTE_output += "# ";
                else PTE_output += "* ";
        }
    }
        cout << PTE_output << endl;
}
}

//---------------Class Pager ----------------------

class Pager {
    public:
    int handptr = 0;
    virtual FTE* select_victim_frame() = 0;   // virtual base class
};

//-----------------FIFO-------------------------


class FIFO : public Pager
{
   //int handptr = 0;
public:
    FTE* select_victim_frame(){
        if (handptr==frame_count) handptr=0;
        FTE *frame = &frame_table[handptr];
        //cout<< handptr << endl;
        //cout << frame->pte_no << " victim" << endl;
        handptr++;
        return frame;
    }
};

//------------------------Random--------------------

class Random : public Pager
{
public:
    FTE* select_victim_frame(){
        //if (handptr==frame_count) handptr=0;
        handptr = myrandom(frame_count);
        FTE *frame = &frame_table[handptr];
        //cout<< handptr << endl;
        //cout << frame->pte_no << " victim" << endl;
        //handptr++;
        return frame;
    }
};

//--------------------Clock--------------------------

class Clock : public Pager
{
public:
    FTE* select_victim_frame(){
        if (handptr==frame_count) handptr=0;
        FTE *frame = NULL;   // = &frame_table[handptr];
        int cnt = 0;
        while(cnt <= frame_count){
            if (handptr==frame_count) handptr=0;
            frame = &frame_table[handptr];
            int pro_no = frame->process;
            int vp = frame->pte_no;
            PTE *pc = &pageV[pro_no][vp];
            if(pc->referenced == 0){
                handptr++;
                break;
                //return frame;
            }
            else{
                pc->referenced = 0;
            }
            cnt++;
            handptr++;
        }
        return frame;
    }
};

//--------------------Enhanced Second Chance / NRU--------------
int last_reset_time = 0;

class NRU : public Pager
{
public:
    FTE* select_victim_frame(){
        if (handptr==frame_count) handptr=0;
        
        int small_class = 15;
        bool isreset = false;
        if(inst_count - last_reset_time >= 50){
            isreset = true;
            last_reset_time = inst_count;
        }
        int cnt = 0;
        int fr_no = 0 ;
        FTE *frame; // = &frame_table[handptr];
        FTE *f = NULL;
        
        while(cnt < frame_count){
            if (handptr==frame_count) handptr=0;
            frame = &frame_table[handptr];
            int refbit = 0;
            int modbit = 0;
            PTE *p = &pageV[frame->process][frame->pte_no];
            refbit = p->referenced;
            modbit = p->modified;
            int class_index = (2*refbit) + modbit;
            if(class_index < small_class){
                small_class = class_index;
                fr_no = frame->f_no;
                f = frame;
            }
            if(class_index==0){
                if(! isreset ){
                    handptr = fr_no +1;
                    return f;
                }
            }
            cnt++ ;
            handptr++;
        }
        if(isreset){
            for(int i = 0; i < frame_count ; i++){
                FTE *fr = &frame_table[i];
                PTE *pv = &pageV[fr->process][fr->pte_no];
                pv->referenced = 0;
            }
        }
        handptr = fr_no + 1;
        return f;
    }
};

//---------------------Aging--------------------------

class Aging  : public Pager
{
    int small;
public:
    FTE* select_victim_frame(){
        if (handptr==frame_count) handptr=0;
        int cnt = 0;
        int fr_no = 0 ;
        FTE *frame; // = &frame_table[handptr];
        FTE *f = NULL;
        for(int i = 0; i<frame_count;i++){
            frame = &frame_table[i];
            frame->age = frame->age >> 1;
            
            int pro_no = frame->process;
            int vp = frame->pte_no;
            PTE *pc = &pageV[pro_no][vp];
            
            if(pc->referenced == 1){
                frame->age = (frame->age | 0x80000000);
                pc->referenced = 0;
            }
        }
        while(cnt < frame_count){
            if (handptr==frame_count) handptr=0;
            //frame=&frame_table[handptr];
            frame = &frame_table[handptr];
            if(cnt==0) {
                small=frame->age;
                fr_no = frame->f_no;
                f = frame;
            }
            else{
                if(small > frame->age){
                    small = frame->age;
                    fr_no = frame->f_no;
                    f = frame;
                }
            }
            
            cnt++ ;
            handptr++;
    }
        handptr = fr_no +1;
        return f;
    }
};

//--------------------------Working Set-----------------

const int tao = 49;

class Working_Set  : public Pager
{
public:
    FTE* select_victim_frame(){
        long AGE = 0;
        long small_time = LONG_MAX;
        if (handptr==frame_count) handptr=0;
        int cnt = 0;
        int fr_no = 0 ;
        int refbit = 0;
        FTE *frame; // = &frame_table[handptr];
        FTE *f = NULL;
        
        while(cnt < frame_count){
            if (handptr==frame_count) handptr=0;
            frame = &frame_table[handptr];
            PTE *pt = &pageV[frame->process][frame->pte_no];
            AGE = (inst_count - 1) - frame->time_last_use;
            refbit = pt->referenced;
            if(refbit==1){
                frame->time_last_use = inst_count - 1;
                pt->referenced = 0;
            }
            
            else if(refbit==0 && AGE > tao){
                //return this page
                f = frame;
                fr_no = frame->f_no;
                handptr = fr_no +1;
                return f;
            }
            //remember smallest page
            if(frame->time_last_use < small_time){
                small_time = frame->time_last_use;
                fr_no = frame->f_no;
                f = frame;
            }
            cnt++ ;
            handptr++;
        }
        handptr = fr_no +1;
        return f;
    }
    
};

Pager *THE_PAGER ;  //= new FIFO;

FTE*  allocate_frame_from_free_list(){
    if(allocated_frame_table.empty()) return NULL;
    else{
        int frame_index= allocated_frame_table.front();
        allocated_frame_table.pop();
        return &frame_table[frame_index];
        
    }
}

FTE *get_frame() {
    FTE *frame = allocate_frame_from_free_list();
    if (frame == NULL) frame = THE_PAGER->select_victim_frame();
    return frame;
}


bool get_next_instruction(char &c,int &vpage){
    string line;
    while(getline(infile,line)){
    if (line[0]=='#') {
            continue;}
    c = line[0];
    int p = stoi(line.substr(1));
    vpage = p;
    return true;
    }
    return false;
}



//-------------------------Simulation -------------------------
void Simulation(){
    
    int current_process = 0;
    char operation = 'c';
    int vpage = 0;
    //int count = 0;
    //long long COST = 0;
    //each access (read or write) costs 1 cycles and a context switch costs 121 cycles and process exit costs 175 cycles.
    
    
    for(int i = 0; i<frame_count; i++){
        //cout << "IN INITIAL ASSIGNMENT " << i << endl;
        FTE f=FTE();
        f.f_no = i;
        allocated_frame_table.push(i);
        frame_table.push_back(f);
    }
    
    while(get_next_instruction(operation,vpage)) {
        
        int r1=0;
        int r2 =0;
        int w = 0;
        int f_map = 0;
        bool flag = false;
        pstats *p_st = &p_stats[current_process];
        
        // handle special case of “c” and “e” instruction
        if(operation=='c'){
            inst_cost += 121;
            cnxt_switch += 1;
            current_process = vpage;
            if(O) cout << inst_count<<": ==> "<< operation <<" " << vpage <<endl;
            inst_count++;
            continue;
        }
        if(operation=='e'){
            p_exit += 1;
            inst_cost += 175 ;
            current_process = vpage;
            if(O) cout << inst_count<<": ==> "<< operation <<" " << vpage <<endl;
            cout << "EXIT current process "<< current_process <<endl;
            inst_count++;
            // iterate through virtual page of current process and set present to zero and remove reverse mapping from frame
            //print UNMAP and print FOUT f_map is 1 and it is paged out =1
            for (int i = 0 ; i<page_count ; i++){
                PTE *p = &pageV[current_process][i];
                if (p->present == 1){
                    int frame = p->frame_no;
                    FTE *f = &frame_table[frame];
                    allocated_frame_table.push(frame);
                    if (O) cout << " UNMAP "<< f->process << ":" << f->pte_no<< endl;
                    p_st->unmaps += 1;
                    f->pte_no = -1;
                    f->process = -1;
                    if(p->file_map==1 && p->modified==1){
                        if (O) cout<<" FOUT"<<endl;
                        p_st->fouts += 1;
                    }
                }
                    p->present = 0;
                    p->modified = 0;
                    p-> referenced = 0;
                    p->frame_no = 0;
                    p->pagedout = 0;
                }
            }

        // now the real instructions for read and write
        if (operation=='r' || operation=='w'){
            inst_cost += 1;
            if (O) cout << inst_count<<": ==> "<< operation <<" " << vpage <<endl; // 0: ==> c 0
            inst_count++;
            vector<vms> VM = N_process[current_process];
            
            for(int i=0; i<VM.size(); i++){
                vms wf = VM[i];
                //cout << wf.start_vpage << " " << wf.end_vpage << " " << wf.write << " " << wf.file_map <<endl;
                r1 = wf.start_vpage;
                r2 = wf.end_vpage;
                if(r1<= vpage && vpage <= r2){
                    flag=true;
                    w = wf.write;
                    f_map = wf.file_map;
                }
            }
            
            if (flag){
                PTE *pte = &pageV[current_process][vpage];// in reality this is done by hardware
                //cout << "present: " << pte->present << endl;
                FTE *newframe;
                //cout << "PTE" << endl;
                if ( !pte->present) { // Enter when page is not present
                    // this in reality generates the page fault exception and now you execute
                    newframe = get_frame();
                    //cout << "Shreya" << endl;
                    // check if process was mapped
                    if(newframe->pte_no != -1){
                        int v = newframe->pte_no;
                        int p = newframe ->process;
                        pstats *p_st = &p_stats[p];
                        //if mapped then
                        if(O) cout << " UNMAP "<< p << ":" << v <<endl;
                        p_st->unmaps += 1;
                        PTE *pt = &pageV[p][v];
                        
                        if(pt->file_map==1 && pt->modified==1){
                            p_st->fouts += 1;
                            if (O) cout <<" FOUT" << endl;
                        }
                        else if (pt->file_map==0 && pt->modified ==1){
                            if (O) cout << " OUT" << endl;
                            p_st->outs += 1;
                            pt->pagedout=1;
                        }
                        pt->present = 0;
                        
                    }
                    newframe->process = current_process;
                    newframe->pte_no = vpage;
                    pte->frame_no = newframe->f_no;
                    pte->write = w;
                    pte->file_map = f_map;
                    pte->modified = 0;
                    
                    if(pte->file_map == 1){
                       if(O) cout<<" FIN" << endl;
                       p_st->fins += 1;
                    }
                    else if(pte->pagedout == 1){
                        if(O) cout << " IN" << endl;
                        p_st->ins+=1;
                    }
                    else{
                        if(O) cout << " ZERO" << endl;
                        p_st->zeros += 1;
                    }
                    pte->present = 1;
                    if(O) cout << " MAP " << pte->frame_no <<endl;
                    newframe->time_last_use = inst_count - 1;
                    newframe->age = 0;
                    p_st->maps+=1;
                }
                //else{
                if(operation=='r'){
                        pte->referenced = 1;
                        //pte->modified = 0;
                }
                if(operation=='w'){
                    pte->referenced = 1;
                    if(pte->write==1){
                        if(O) cout << " SEGPROT" << endl;
                        p_st->segprot+=1;
                    }
                    else {
                        pte->modified = 1;
                    }
                }
                //cout << pte->modified<< "write" << endl;
                //}
                }
            
            //if page is not present in segmentation
            else{
                if(O) cout << " SEGV" << endl;
                p_st->segv+=1;
            }
            }
        }
        if(P) Page_table_Output();
        if(F) Frame_table_Output();
        if(S){
            Page_Stats_Output();
            Summary ();
        }
    }



void get_input(int argc, char *argv[]){
    int index=0;
    int c=0;
    bool algo_flag=false;
    bool invalid_algo=false;
    opterr = 0;
    while ((c = getopt (argc, argv, "a:o:f:")) != -1){
        switch (c) {
        case 'a':
        {
            algo_flag=true;
            if(optarg[0] == 'e')
                THE_PAGER = new NRU();
            else if(optarg[0] == 'r')
                THE_PAGER = new Random();
            else if(optarg[0] == 'f')
                THE_PAGER = new FIFO();
            else if(optarg[0] == 'c')
                THE_PAGER = new Clock();
            else if(optarg[0] == 'a')
                THE_PAGER = new Aging();
            else if(optarg[0] == 'w')
                THE_PAGER = new Working_Set();
            else
            {
                invalid_algo=true;
            }
            break;
        }
            
            
        case 'o':
        {
            for(int i=0; optarg[i]!='\0'; i++)
            {
                if(optarg[i] == 'O') O = true;
                else if(optarg[i] == 'P') P = true;
                else if(optarg[i] == 'F') F = true;
                else if(optarg[i] == 'S') S = true;
            }
            break;
        }
        case 'f':
        {
            frame_count = atoi(optarg);
            break;
        }
            
        default: cout<<"Invalid Arguments!";
    }
    }
    
    // READING INPUT FILE AND RANDOM FILE
     for (index = optind; index < argc; index++)
    {
        if(index==argc-2){
            my_file = argv[index];
            infile.open(my_file);
            string line;
            while(!infile.eof()){
                getline(infile,line);
                //cout << line << endl;
                if (line[0]=='#') continue;
                else {
                    pstats ps = pstats();
                    int proc = stoi(line);
                    for(int n = 0; n < proc ; n++){
                        getline(infile,line);
                        while(line[0]=='#'){
                            getline(infile,line);
                        }
                        int no_of_seg = stoi(line);
                        for(int i=0; i < no_of_seg; i++){
                            vms vm1;
                            getline(infile,line);
                            char l[line.size()+1];
                            strcpy(l, line.c_str());
                            char *pch;
                            char* num[4];
                            int k =0;
                            pch = strtok (l," ");
                            while (pch != NULL)
                            {
                                num[k]= pch;
                                pch = strtok (NULL, " ");
                                k++;
                            }
                            vm1.start_vpage= stoi(num[0]);
                            vm1.end_vpage = stoi(num[1]);
                            vm1.write = stoi(num[2]);
                            vm1.file_map = stoi(num[3]);
                            vm.push_back(vm1);
                        }
                        p_stats.push_back(ps);
                        N_process.push_back(vm);
                        PTE* page_table = new PTE[page_count];
                        for(int i = 0; i < page_count; i++){
                            page_table[i] = PTE();
                        }
                        pageV.push_back(page_table);
                        // erase the first seg_mength length elements:
                        vm.erase (vm.begin(),vm.begin()+(no_of_seg));
                    }
                    break;
                }
            }
        }
        // For Random File Initialization
        if (index == argc-1) {
            Initialize_random(argv[index]);
        }
    }
}



int main(int argc,char * argv[]) {
    
    // Read file and generate VMS
    get_input(argc,argv);
    
    // Simulation
    Simulation();
}
