//
//  main.cpp
//  Assignment 1
//
//  Created by Shreya Pandey on 9/26/19.
//  Copyright © 2019 Operating Systems. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include<cstring>

using namespace std;

// Define a Struct DataType for symbol table


string filename;
ifstream myfile;
map<string,string> error;
string gettoken(string *poi,int *offset,int *l);
void pass2(std::map<string, int> *dic);
string loc;
map<string,int> sym_mod;
int OFFSET= 0;
int line_no=0;

// parse error function
void _parsererror (int errcode,string l,string offset){
    static string errstr[] = {
        "NUM_EXPECTED", // Number expect
        "SYM_EXPECTED", //Symbol expected
        "ADDR_EXPECTED", // Addressing Expected which is A/E/I/R
        "SYM_TOO_LONG",  // SYMBOL NAME IS TOO LONG
        "TOO_MANY_DEF_IN_MODULE",  // >16
        "TOO_MANY_USE_IN_MODULE", // >16
        "TOO_MANY_INSTR"  // total num_instr exceeds memory size (512)
        
    };
    //cout << l.substr(0,1) << endl;
    cout << "Parse Error line " << l.substr(0,1) << " offset " << offset << ": " << errstr[errcode]<<endl;
}

string _errormsgs(int error){
    static string errstr[] = {
        " Error: This variable is multiple times defined; first value used",  // rule 2
        " is not defined; zero used",                               // rule 3
        " Error: External address exceeds length of uselist; treated as immediate", //rule 6
        " Error: Absolute address exceeds machine size; zero used",              //rule 8
        " Error: Relative address exceeds module size; zero used",               // rule 9
        " Error: Illegal immediate value; treated as 9999",                      // rule 10
        " Error: Illegal opcode; treated as 9999",                           // rule 11
    };
    return errstr[error];
}

string  location(int s){
    if(s<10) loc= "00" + to_string(s);
    else if (s<100) loc="0"+to_string(s);
    else loc=""+to_string(s);
    return loc;
}

string  valu(int a){
    string v;
    if(a<10) v= "000" + to_string(a);
    else if (a<100) v="00"+to_string(a);
    else if (a<1000) v = "0"+ to_string(a);
    else v=""+to_string(a);
    return v;
}



string token_(string t){
    char cstr[t.size() + 1];
    strcpy(cstr, t.c_str());
    char *token = strtok(cstr, " ");
    return (token);
}

/* lets first write token function
 this will get line number and
 offset as arguments and will start scanning file from that point */

string gettoken(string *poi,int *offset,int *l)
{
    float a ;
    string line,s,token;
    line = *poi;
    a = line.length();
    for (int i=0; i < a ; i++){
        if (!isspace(line[i])){
            s+=line[i];
            //*offset += 1;
        }
        else{
            if (!s.empty()){
                token = s + " " + to_string(*l) + " "+ to_string(*offset);
                *offset += s.length() + 1;
                OFFSET = *offset;
                *poi = line.substr(i+1);
                s.clear();
                return token;
            }
        }
    }
    
    if (!s.empty())
    {
        token = s + " " + to_string(*l) + " "+ to_string(*offset);
        //cout << token << endl;
        line="\0";
        OFFSET = *offset;
        *offset = 1;
        getline(myfile,line);
        line_no = *l;
        *l +=1;
        *poi = line.substr(0);
        return token;
    }
    else{
        getline(myfile,line);
        if (myfile.eof()){
            string flag = "-1 "+to_string(*l) + " "+ to_string(*offset);
            return flag;
        }
        OFFSET = *offset;
        *offset = 1;
        line_no = *l;
        *l += 1;
        *poi = line.substr(0);
        return gettoken(poi,offset,l);
    }
} //gettoken ends here




int main(int argc,char **argv)
{
    filename = argv[1];
    myfile.open(filename);
    string line;
    int offset = 1 ;
    int l = 0;
    int module_no = 1;
    int module_size = 0;
    //char addressmode;
    string li;
    string off;
    //int instc;
    int usecount;
    float b,j=0;
    int def_count;
    int total_inst =0;
    map<string,int> var_val;
    map<string,string> error;
    OFFSET = offset;
    line_no = l;
    
    while (!myfile.eof())
    {
        if (line.empty()){
            getline(myfile,line);
            l+=1;
        }
        
        // call gettoken function to get defcount
        string defc = gettoken(&line,&offset,&l);
        if (defc[0]=='-' && defc[1]=='1'){         // get out of while loop module ends file ends
            continue;
        }
        string def = token_(defc);  // we get first token
        b = def.length();           // we get length of first token here
        
        // Line and offset is calculated here
        li = defc.substr(b+1,2);
        off= defc.substr(b+3);
        while (isdigit(def[j])) j++;
        if(j==b) def_count = stoi(def);  // token is a number
        else{ // Raise Integer Error
            _parsererror(0, li, off);
            exit(EXIT_FAILURE);
        }
        int index=def_count+1;
        string arry_sym[index];  // defined to check Rule 5 Warning
        // Number of Instructions Error
        if (def_count > 16 )
        {
            _parsererror(4,li,off);
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i<def_count; i++)
        {
            string sym = gettoken(&line,&offset,&l);
            string symbol = token_(sym);  // we get first token
            arry_sym[i]=symbol;   //defined to check warning for this symbol
            b = symbol.length();   // we get length of first token here
            // Line and offset is calculated here
            li = sym.substr(b+1,2);
            off= sym.substr(b+3);
            if(b>16){ //Raise Symbol Name too large Error
                _parsererror(3, li, off);
                exit(EXIT_FAILURE);
            }
            // Checks for Symbol
            if(symbol[0]=='-' && symbol[1]=='1'){ //Raise Symbol Error
                _parsererror(1, li, off);
                exit(EXIT_FAILURE);
            }
            // Symbol should be alphanumeric else raise Symbol Exception
            if(!isalpha(symbol[0])) {
                _parsererror(1, li, off);
                exit(EXIT_FAILURE);
            }
            string val1 = gettoken(&line,&offset,&l);
            int val;
            string value = token_(val1);
            b=value.length();
            li = val1.substr(b+1,2);
            off= val1.substr(b+3);
            if(value[0]=='-' && value[1]=='1'){ //Raise Value Error EOL
                _parsererror(0, li, off);
                exit(EXIT_FAILURE);
            }
            //Check if value is a number
            j =0 ;
            while (isdigit(value[j])) j++;
            if(j==b) val = stoi(value);  // token is a number
            else { //Raise Number Exception
                _parsererror(0, li, off);
                exit(EXIT_FAILURE);
            }
            
            //Check if Symbol is defined already
            if (var_val.count(symbol)==0)
            {
                sym_mod[symbol] = (module_no);
                var_val[symbol] =(val + module_size);
                error[symbol] = " ";
            }
            else {
                error[symbol]  = _errormsgs(0) ; //Rule 2
            }
        } //for of Def count ends here
        
        /* -----------------------------------USECOUNT Starts Here ----------------------------------- */
        string u_count = gettoken(&line,&offset,&l);
        string uc = token_(u_count);
        b=uc.length();
        li = u_count.substr(b+1,2);
        off= u_count.substr(b+3);
        if(u_count[0]=='-' && u_count[1]=='1'){ //Raise Use Count Error EOF
            _parsererror(0, li, off);
            exit(EXIT_FAILURE);
        }
        //Check if Use Count is a number
        j = 0 ;
        while (isdigit(uc[j])) j++;
        if(j==b) usecount = stoi(uc);  // token is a number
        else { //Raise Number Exception
            _parsererror(0, li, off);
            exit(EXIT_FAILURE);
        }
        
        // Check if usecount is greater than 16
        if (usecount > 16)
        {
            _parsererror(5,li,off);
            exit(EXIT_FAILURE);
        }
        for(int i = 0; i < usecount;i++)
        {
            string sym = gettoken(&line,&offset,&l);
            // 3 Checks EOL(symbol error),sym is alphnum or alpha,sym.length <16
            string symbol = token_(sym);  // we get first token
            b = symbol.length();   // we get length of first token here
            // Line and offset is calculated here
            li = sym.substr(b+1,2);
            off= sym.substr(b+3);
            if(b>16){ //Raise Symbol Name too large Error
                _parsererror(3, li, off);
                exit(EXIT_FAILURE);
            }
            // Checks for Symbol
            if(symbol[0]=='-' && symbol[1]=='1'){ //Raise Symbol Error
                _parsererror(1, li, off);
                exit(EXIT_FAILURE);
            }
            // Symbol should be alphanumeric else raise Symbol Exception
            if(!isalpha(symbol[0])) {
                _parsererror(1, li, off);
                exit(EXIT_FAILURE);
            }
            
        }
        
        /*-------------------------------------------Instruction Section -------------------------------------*/
        int inst;
        string inst_count = gettoken(&line,&offset,&l);
        string IC = token_(inst_count);
        b=IC.size();
        li = inst_count.substr(b+1,2);
        off= inst_count.substr(b+3);
        if(IC[0]=='-' && IC[1]=='1'){ //Raise Value Error EOL
            _parsererror(0, li, off);
            exit(EXIT_FAILURE);
        }
        //Check if value is a number
        j = 0 ;
        while (isdigit(IC[j])) j++;
        if(j==b) inst = stoi(IC);  // token is a number
        else { //Raise Number Exception
            _parsererror(0, li, off);
            exit(EXIT_FAILURE);
        }
        
        // total_Instruction greater than 512
        total_inst += inst;
        if (total_inst > 512)
        {
            _parsererror(6,li,off);
            exit(EXIT_FAILURE);
        }
        
        for (int i=0; i < inst; i++)
        {
            string addressmode = gettoken(&line,&offset,&l);
            string Add = token_(addressmode); // we get first token
            b = Add.size();
            // Line and offset is calculated here
            li = addressmode.substr(b+1,2);
            off= addressmode.substr(b+3);
            if(Add[0]=='-' && Add[1]=='1'){ // EOL Address expected error
                _parsererror(2, li, off);
                exit(EXIT_FAILURE);
            }
            if(Add[0]!='R' && Add[0]!='A' && Add[0]!='E' && Add[0]!='I'){ //Raise Address Exception
                _parsererror(2, li, off);
                exit(EXIT_FAILURE);
            }
            int instruction;
            string instr = gettoken(&line,&offset,&l);
            string I = token_(instr);
            b=I.size();
            li = instr.substr(b+1,2);
            off= instr.substr(b+3);
            if(I[0]=='-' && I[1]=='1'){ //Raise Value Error EOL
                _parsererror(0, li, off);
                exit(EXIT_FAILURE);
            }
            
            //Check if Instruction is a number
            j = 0 ;
            while (isdigit(I[j])) j++;
            if(j==b) instruction = stoi(I);  // token is a number
            else { //Raise Number Exception
                _parsererror(0, li, off);
                exit(EXIT_FAILURE);
            }
            j=0;
            
        }
        /* ----------------------------- 1 Module Ends Here ------------------------*/
        module_size += inst;  //module size is updated in every module
        
        
        // Check if Value is greater than current module size
        //Warning Rule 5 Will test later
        
        if(def_count>0){
            for (int k=0; k < (index-1);k++){
                if(var_val[arry_sym[k]]>module_size)
                {
                    int val=0;
                    cout << "Warning: Module "<< module_no <<": "<< arry_sym[k] << " too big " << var_val[(arry_sym[k])] << " (max=" << module_size-1 << ") assume zero relative\n";
                    var_val[(arry_sym[k])]=(val);
                }
            }
        }
        module_no += 1;
    } //While loop ends here
    
    // Print Symbol Table
    cout << "Symbol Table" << endl;
    string var;
    map<string, int>::iterator it;
    for ( it = var_val.begin(); it != var_val.end(); it++ )
    {
        string sym = it->first;
        var=sym;
        cout << it->first << "=" << it->second << error[sym] << "\n";
    }
    cout << "\n";
    // Print Memory Table
    cout << "Memory Map" << endl;
    myfile.clear();
    myfile.seekg (0, ios::beg);         //reset file to first line
    pass2(&var_val);
    
}


void pass2(std::map<string, int> *dic){
    
    string line;
    int offset = 1 ;
    int l = 1;
    int module_no = 1;
    int module_size = 0;
    //char addressmode;
    int total_inst = 0;
    int def_count;
    int usecount;
    string j ="00";
    map<int,string> error;
    map<int,int> memory_add;
    map<string,int>varval;
    map<string,string> Def_flag;
    varval=*dic;
    
    //Defination Flag
    
    map<string, int>::iterator itr;
    for ( itr = varval.begin(); itr != varval.end(); itr++ )
    {
        string sym = itr->first;
        Def_flag[sym]="D";
    }
    
    
    while (!myfile.eof()) {
        
        if (line.empty()){
            getline(myfile,line);
        }
        /*---------------------------------------Definition begins------------------------------*/
        string defc = gettoken(&line,&offset,&l);
        if (defc[0]=='-' && defc[1]=='1'){         // get out of while loop module ends file ends
            continue;
        }
        string def = token_(defc);  // we get first token
        def_count = stoi(def);
        
        for (int i = 0; i<def_count; i++)
        {
            string sym = gettoken(&line,&offset,&l);
            string val1 = gettoken(&line,&offset,&l);
            
        }
        
        /*----------------------- USECOUNT BEGINS -----------------------------*/
        map<string,string> flag;
        string u_count = gettoken(&line,&offset,&l);
        string uc = token_(u_count);
        usecount = stoi(uc);
        string use_list[usecount];                        //usecount can never be greater than 16
        for(int i = 0; i<usecount;i++)
        {
            string sym = gettoken(&line,&offset,&l);
            string symbol = token_(sym);
            use_list[i] = symbol;
            flag[symbol]="N";
        }
        
        /*--------------------------------INSTRUCTIONS BEGINS------------------------*/
        // Entering into Important Stuff in Pass2
        
        string inst_count = gettoken(&line,&offset,&l);
        string inst = token_(inst_count);
        int instcount = stoi(inst);
        for (int i=0; i<instcount;i++)
        {
            string add_mode = gettoken(&line,&offset,&l);
            string instr = gettoken(&line,&offset,&l);
            int instruction = stoi(token_(instr));
            int opcode = instruction/1000;
            int operand = instruction % 1000;
            // various checks here
            // Command "R"
            if (add_mode[0] == 'R'){
                // check if instruction is valid
                if ((opcode)>10 || opcode==10){
                    //cout << opcode << endl;
                    memory_add[total_inst] = 9999;
                    error[total_inst] = _errormsgs(6);  //" Error: Illegal opcode; treated as 9999", // rule 11
                    cout << location(total_inst) << ": " << valu(memory_add[total_inst])  << "  " << error[total_inst] <<endl;
                }
                else{
                    if(operand>instcount){
                        //cout << instruction << " " << operand <<"  "<< module_size <<  endl;
                        memory_add[total_inst]=(instruction-operand) + module_size;
                        error[total_inst]=_errormsgs(4);  //4 " Error: Relative address exceeds module size; zero used",  // rule 9
                        cout << location(total_inst) << ": " << valu(memory_add[total_inst])<< error[total_inst] <<endl;
                    }
                    else{
                        
                        memory_add[total_inst] = module_size + instruction;      // store instruction + abs address of current module
                        error[total_inst] = " ";
                        cout << location(total_inst) << ": " << valu(memory_add[total_inst]) << "  " << error[total_inst] <<endl;
                    }
                }
            }
            
            //Command "E"
            // array for warning
            //int Rule_7[usecount];
            if ( add_mode[0] == 'E'){
                // Check the operand and get value of use[operand]
                //int operand = instruction % 1000;
                // check if instruction is valid
                if (opcode>10 || opcode==10){
                    memory_add[total_inst] = 9999;
                    error[total_inst] = _errormsgs(6); //" Error: Illegal opcode; treated as 9999", // rule 11
                    cout << location(total_inst) << ": " << valu(memory_add[total_inst]) << "  " << error[total_inst] <<endl;
                }
                if ( operand > usecount-1)
                {
                    //cout << " " << endl;
                    memory_add[total_inst] = instruction;
                    //cout << memory_add[total_inst]<<endl;
                    error[total_inst] = _errormsgs(2);   //Error: External address exceeds length of uselist; treated as immediate", //rule 6
                    cout << location(total_inst) << ": " << valu(memory_add[total_inst])  << "  " << error[total_inst] <<endl;
                }
                else {
                    string name= use_list[operand];
                    flag[name]="U";
                    Def_flag[name]="S";
                    int value;
                    // checking if key is present in var_val
                    if(varval.count(name)==0){
                        value = 0;
                        int address = (instruction - operand) + value;
                        memory_add[total_inst] = address;
                        error[total_inst] = "Error: " + name + _errormsgs(1);  // " Error:%s is not defined; zero used", // rule 3
                        cout << location(total_inst) << ": " << valu(memory_add[total_inst])<< " " << error[total_inst] <<endl;
                    }
                    else{
                        value = varval[name];
                        int address = (instruction - operand) + value;
                        memory_add[total_inst] = address;
                        error[total_inst] = " ";
                        cout << location(total_inst) << ": " << valu(memory_add[total_inst])  << "  " << error[total_inst] <<endl;
                    }
                }
            }
            
            // Command "I"
            if (add_mode[0] == 'I'){
                // immediate operand is unchanged
                if (opcode>10 || opcode==10){
                    memory_add[total_inst] = 9999;
                    error[total_inst] = _errormsgs(5);
                    cout << location(total_inst) << ": " << valu(memory_add[total_inst])  << "  " << error[total_inst] <<endl;
                }
                else{
                    memory_add[total_inst] = instruction;
                    error[total_inst] = " ";
                    cout << location(total_inst) << ": " << valu(memory_add[total_inst])  << "  " << error[total_inst] <<endl;
                }
            }
            // Command "A"
            if (add_mode[0] == 'A')
            {
                if ((operand)>512){
                    memory_add[total_inst] = (instruction-operand);
                    error[total_inst] = _errormsgs(3);   //"Error: Absolute address exceeds machine size; zero used", //rule 8
                    cout << location(total_inst) << ": " << valu(memory_add[total_inst]) << error[total_inst] <<endl;
                }
                else{
                    memory_add[total_inst] = instruction;
                    error[total_inst] = " ";
                    cout << location(total_inst) << ": " << valu(memory_add[total_inst])  << "  " << error[total_inst] <<endl;
                }
            }
            total_inst+=1;
        } // for loop ended here
        
        // Print Warning here
        map<string, string>::iterator i;
        for ( i = flag.begin(); i != flag.end(); i++ )
        {
            string name1 = i->first;
            if(flag[name1][0]=='N'){
                cout << "Warning: Module "<< module_no<<": " << name1 << " appeared in the uselist but was not actually used\n";}
            else continue;
        }
        module_no +=1;
        module_size += instcount;
        
        
    } // end of while loop
    
    //Warning For Rule 4
    cout << endl;
    map<string, string>::iterator loop;
    for ( loop = Def_flag.begin(); loop != Def_flag.end(); loop++ )
    {
        string name1 = loop->first;
        //cout << name1<< endl;
        if(Def_flag[name1][0]=='D'){
            cout << "Warning: Module "<< sym_mod[name1] <<": " << name1 << " was defined but never used\n";}
        else continue;
    }
}


