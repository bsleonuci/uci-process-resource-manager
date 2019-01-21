#include <iostream>
#include <fstream>
#include <list>
#include <map>
#include <tuple>
#include <string>

#define MAX_PROCESSES 64
#define INPUT_FILE "F:\\input.txt"
#define OUTPUT_FILE "F:\\76919463.txt"

using namespace std;

typedef class ProcessControlBlock PCB;
typedef class ResourceControlBlock RCB;
typedef class ReadyList RL;
typedef list<PCB*> PCBList;
typedef map<RCB*, int> RCBMap;
typedef tuple <PCB*, int> RequestTuple;
typedef list<RequestTuple> BlockList;

class ResourceControlBlock {
private:
    string rid;
    int total;
    int available;
    BlockList block_list;

public:

    ResourceControlBlock(){
    }

    ResourceControlBlock(string new_rid, int units) :
        rid(new_rid), total(units), available(total){
    }

    string get_rid(){
        return rid;
    }

    int total_units(){
        return total;
    }

    int available_units(){
        return available;
    }

    void subtract_available(int amount){
        available -= amount;
    }

    void add_available(int amount){
        available += amount;
    }

    bool contains_process(PCB &process){
        for(auto it = block_list.begin(); it != block_list.end(); ++it){
            if(get<0>(*it) == &process) return true;
        }
        return false;
    }

    void add_existing(PCB &process, int amount){
        for(auto it = block_list.begin(); it != block_list.end(); ++it){
            if(get<0>(*it) == &process) get<1>(*it) += amount;
        }
    }

    void add_to_blocked(PCB &process, int amount){
        if (contains_process(process)) add_existing(process, amount);
        else block_list.push_back(RequestTuple(&process, amount));
    }

    void remove_request(PCB &process){
        RequestTuple to_remove(nullptr, -1);
        for(auto it = block_list.begin(); it != block_list.end(); ++it){
            if(get<0>(*it) == &process) to_remove = *it;
        }
        if(get<0>(to_remove) != nullptr){
            //cout << "Removing request from releasing process..." << endl;
            block_list.remove(to_remove);
        }
    }

    RequestTuple serve_next_request(){
        RequestTuple to_return(nullptr, -1);
        for(auto it = block_list.begin(); it != block_list.end(); ++it){
            if(get<1>(*it) <= available){
                to_return = *it;
                break;
            }
        }
        if (get<0>(to_return) != nullptr) block_list.remove(to_return);
        return to_return;
    }

    bool block_list_empty(){
        return block_list.empty();
    }

    ~ResourceControlBlock(){
    }
};

class ProcessControlBlock {
private:
    string pid = "null";
    RCBMap other_resources;
    int priority;
    string state = "unused";
    PCB* parent = nullptr;
    PCBList children;
    void* tlist = nullptr;

public:

    ProcessControlBlock(){}

    ProcessControlBlock(string new_pid, int new_priority, RL &ready_list) :
        pid(new_pid), priority(new_priority), state("ready"), tlist(&ready_list){
    }

    ProcessControlBlock(string new_pid, int new_priority, PCB &creator, RL &ready_list) :
        pid(new_pid), priority(new_priority), state("ready"), parent(&creator), tlist(&ready_list){
    }

    string get_state(){
        return state;
    }

    string get_pid(){
        return pid;
    }

    int get_priority(){
        return priority;
    }

    bool holding_resource(RCB &resource){
        for(auto it = other_resources.begin(); it != other_resources.end(); ++it){
            if((it->first == &resource) && (it->second > 0)) return true;
        }
        return false;
    }

    PCBList get_children(){
        return children;
    }

    RCBMap held_resources(){
        return other_resources;
    }

    int is_ancestor(PCB &process){
        if(&process == this) return 1;
        else if(children.empty()) return 0;
        else{
            int current_count = 0;
            for(auto it = children.begin(); it != children.end(); ++it){
                current_count += (*it)->is_ancestor(process);
            }
            return current_count;
        }
    }

    int amount_held(RCB &resource){
        if(holding_resource(resource)){
            return other_resources[&resource];
        }
        return 0;
    }

    void add_resource(RCB &resource, int amount){
        if(holding_resource(resource)) other_resources[&resource] += amount;
        else other_resources[&resource] = amount;
        resource.subtract_available(amount);
    }

    void release_resource(RCB &resource, int amount){
        if(holding_resource(resource)){
                other_resources[&resource] -= amount;
                //cout << "Resource " << pid << " now holding " << other_resources[&resource] << " units of resource " << resource.get_rid() << endl;
                resource.add_available(amount);
        }
        //else cout << "Resource not held by process " << pid << endl;
    }

    void set_tlist(ReadyList &rl){
        tlist = &rl;
    }

    void set_tlist(RCB &resource){
        tlist = &resource;
    }

    RCB* get_blocked_on(){
        return (RCB*)tlist;
    }

    void add_child(PCB &child){
        children.push_back(&child);
    }

    void set_state(string new_state){
        state = new_state;
    }

    ~ProcessControlBlock(){
    }
};

class ReadyList {
private:
    PCBList system;
    PCBList user;
    PCBList init;

public:

    ReadyList(){
    }

    void add(PCB &process){
        switch(process.get_priority()){
            case 0:
                init.push_back(&process);
                break;
            case 1:
                user.push_back(&process);
                break;
            case 2:
                system.push_back(&process);
                break;
        }
    }

    /*void delete_from_list(PCBList &list, PCB &process){

        for(auto it = list.begin(); it != list.end(); ++it){
            if (*it == &process) {
                list.erase(it);
                break;
            }
        }
    }*/
	
	//remove process <process> from the appropriate priority list
    void remove(PCB &process){
        switch(process.get_priority()){
            case 0:
                init.remove(&process);
                break;
            case 1:
                user.remove(&process);
                break;
            case 2:
                system.remove(&process);
                break;
        }
    }

	//return the process that is at the front of the highest non-empty queue
    PCB* find_highest_priority(){
        if (system.empty()){
            if (user.empty()){
                return init.front();
            }
            else{
                return user.front();
            }
        }
        else{
            return system.front();
        }
    }

	//empty all process queues, destroying all processes and freeing all resources
    void clear_all(){
        system.clear();
        user.clear();
        init.clear();
    }

    ~ReadyList(){
    }
};

class Kernel {
private:
    ifstream ifile;
    ofstream ofile;
    string input;
    PCB* current_process;
    RL ready_list;
    RCB R1, R2, R3, R4;
    PCB AvailablePCB[MAX_PROCESSES];

public:
    Kernel(){
        ifile.open(INPUT_FILE);
        ofile.open(OUTPUT_FILE);
        R1 = RCB("R1", 1);
        R2 = RCB("R2", 2);
        R3 = RCB("R3", 3);
        R4 = RCB("R4", 4);
        current_process = &(AvailablePCB[0]);
        *current_process = PCB("init", 0, ready_list);
        ready_list.add(*current_process);
        scheduler();
    }

    ~Kernel(){
    }

    void run(){
        while(!ifile.eof()){
            ifile >> input;
            parse_command(input);
        }
        ifile.close();
        ofile.close();
    }

    void parse_command(string command){

        if(command == "exit") return;

		//initialize kernel
        else if(command == "init") init();

		//create pid
        else if(command == "cr"){
            string new_pid;
            int new_priority;
            ifile >> new_pid;
            ifile >> new_priority;
            if(new_priority == 1 || new_priority == 2)
                create_process(*current_process, new_pid, new_priority);
            else ofile << "error ";
        }

		//destroy pid
        else if(command == "de"){
            string de_pid;
            ifile >> de_pid;
            destroy_process(de_pid);
        }

		//request resource
        else if(command == "req"){
            string req_rid;
            int amount;
            ifile >> req_rid;
            ifile >> amount;
            request(req_rid, amount);
        }

		//release resource
        else if(command == "rel"){
            string rel_rid;
            bool error;
            int amount;
            ifile >> rel_rid;
            ifile >> amount;
            error = release(rel_rid, amount, *current_process);
            if(!error) scheduler();
        }

        else if(command == "to") time_out();

        else return;
    }

    void init(){
        ofile << endl;
        ready_list.clear_all();
        R1 = RCB("R1", 1);
        R2 = RCB("R2", 2);
        R3 = RCB("R3", 3);
        R4 = RCB("R4", 4);
        for(int i = 0; i < MAX_PROCESSES; ++i) AvailablePCB[i] = PCB();
        current_process = &(AvailablePCB[0]);
        *current_process = PCB("init", 0, ready_list);
        ready_list.add(*current_process);
        scheduler();
    }

    PCB* find_PCB(string de_pid){
        PCB *to_return = nullptr;
        for(int i = 0; i < MAX_PROCESSES; ++i){
            if(AvailablePCB[i].get_pid() == de_pid) to_return = &(AvailablePCB[i]);
        }
        return to_return;
    }

    RCB* find_RCB(string req_rid){
        if(req_rid == "R1") return &R1;
        else if(req_rid == "R2") return &R2;
        else if(req_rid == "R3") return &R3;
        else if(req_rid == "R4") return &R4;
        else return nullptr;
    }

	//request <amount> units of resource <req_rid>
    void request(string req_rid, int amount){
        RCB *requested = find_RCB(req_rid);
        if (requested == nullptr){
            ofile << "error ";
            return;
        }
        else{
            if((current_process->amount_held(*requested) + amount) > requested->total_units()){
                    ofile << "error ";
                    return;
            }
            else if (amount <= requested->available_units()){
                current_process->add_resource(*requested, amount);
            }
            else {
                current_process->set_state("blocked");
                current_process->set_tlist(*requested);
                ready_list.remove(*current_process);
                requested->add_to_blocked(*current_process, amount);
            }
            scheduler();
        }
    }

	//release <amount> units of resource <rel_rid> from process <releaser>
    bool release(string rel_rid, int amount, PCB &releaser){
        RCB *released = find_RCB(rel_rid);
        if(amount > releaser.amount_held(*released)){
                ofile << "error ";
                return true;
        }
        else{
            //ofile << "Releasing amount..." << endl;
            releaser.release_resource(*released, amount);
            while(!(released->block_list_empty())){
                //ofile << "Processing pending requests..." << endl;
                RequestTuple next = released->serve_next_request();
                if(get<0>(next) == nullptr) break;
                else{
                    //ofile << "Adding " << (get<0>(next))->get_pid() << " to rl..." << endl;
                    (get<0>(next))->add_resource(*released, get<1>(next));
                    (get<0>(next))->set_state("ready");
                    (get<0>(next))->set_tlist(ready_list);
                    ready_list.add(*(get<0>(next)));
                }
            }
        }
        return false;
    }

	//create a new process with pid <new_pid> and priority <new_priority>, make it a child of <current>
    void create_process(PCB &current, string new_pid, int new_priority){
        if(find_PCB(new_pid) != nullptr){
            ofile << "error ";
            return;
        }
        //ofile << "Creating new process using pid = " << new_pid << " and priority level " << new_priority << endl;
        PCB* new_process = nullptr;
        for(int i = 0; i < MAX_PROCESSES ; ++i){
            if (AvailablePCB[i].get_state() == "unused"){
                new_process = &AvailablePCB[i];
                break;
            }
        }

        if(new_process == nullptr){
            ofile << "error ";
            return;
        }
        *new_process = PCB(new_pid, new_priority, current, ready_list);
        ready_list.add(*new_process);
        current_process->add_child(*new_process);
        scheduler();
    }

	//trace process and free all resources, then do the same recursively for children
    void recursive_destroy(PCB* to_destroy){
        PCBList descendants = to_destroy->get_children();
        RCBMap held = to_destroy->held_resources();
        for(auto it = descendants.begin(); it != descendants.end(); ++it){
            if (!descendants.empty()) recursive_destroy(*it);
        }
        for(auto it = held.begin(); it != held.end(); ++it){
            if(it->second > 0) release(it->first->get_rid(), it->second, *to_destroy);
        }
        if(to_destroy->get_state() == "blocked") to_destroy->get_blocked_on()->remove_request(*to_destroy);
        ready_list.remove(*to_destroy);
        *to_destroy = PCB();
    }

	//delete <de_pid> process and all of its children, as long as <de_pid> is child of current process
    void destroy_process(string de_pid){
        if(de_pid == "init"){
            ofile << "error ";
            return;
        }
        PCB* to_destroy = find_PCB(de_pid);
        if (to_destroy == nullptr){
                ofile << "error ";
                return;
        }
        else{
            if(current_process->is_ancestor(*to_destroy)){
                recursive_destroy(to_destroy);
            }
            else{
                ofile << "error ";
                return;
            }
        }
        scheduler();
    }

	//stop current process and call scheduler to determine next process
    void time_out(){
        ready_list.remove(*current_process);
        current_process->set_state("ready");
        ready_list.add(*current_process);
        scheduler();
    }

	//set new current process
    void preempt(PCB &oldp, PCB &newp){
        newp.set_state("running");
        current_process = &newp;
    }

	//determine which process should run next based on priority
    void scheduler(){
        PCB* p = ready_list.find_highest_priority();
        if(current_process->get_priority() < p->get_priority() || current_process->get_state() != "running" || current_process == nullptr)
            preempt(*current_process, *p);
        ofile << current_process->get_pid() << " ";
    }
};


int main()
{
    Kernel ResourceManager;
    ResourceManager.run();
    return 0;
}
