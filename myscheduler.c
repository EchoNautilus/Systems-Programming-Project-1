//  CITS2002 Project 1 2023
//  Student1: 22974799 Samuel Chew
//  Student2: 24187986 Julian Prawira

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>


// Constants to initialise array sizes
#define MAX_COMMANDS                    10
#define MAX_COMMAND_NAME                20
#define MAX_LINE_SIZE                   100
#define MAX_SYSCALLS_PER_PROCESS        40  
#define MAX_RUNNING_PROCESSES           50
#define MAX_DEVICES                     4
#define MAX_DEVICE_NAME                 20

// Other system constants
#define DEFAULT_TIME_QUANTUM            100
#define CHAR_COMMENT                    '#'
#define TIME_ACQUIRE_BUS                20
#define TIME_CORE_STATE_TRANSITIONS     10
#define TIME_CONTEXT_SWITCH             5

// System call codes
#define READ_CALL                       1
#define WRITE_CALL                      2
#define SLEEP_CALL                      3
#define SPAWN_CALL                      4
#define WAIT_CALL                       5  
#define EXIT_CALL                       6

// System states (what the CPU is focusing on currently)
#define CPU_STATE                       0
#define SLEEP_STATE                     1
#define WAIT_STATE                      2
#define UNBLOCK_IO_STATE                3
#define COMMENCE_IO_STATE               4
#define ABSORB_READY_STATE              5

// Destination codes (where the CPU is transferring a process to)
#define SLEEP_DESTINATION               1
#define READY_DESTINATION               2
#define IO_DESTINATION                  3
#define WAIT_DESTINATION                4
#define CPU_DESTINATION                 5


// ------------------------------------------- DATA STRUCTURES----------------------------------------------------


/*
 * Object: Command
 *
 * name:              name of command
 * cpu_time_order:    array of elapsed computation times
 * syscall_order:     array of system call codes
 * param1_order:      array of device or command codes
 * param2_order:      array of numerical values (readwrite speed or sleep time)
 */
struct Command {
    char name[MAX_COMMAND_NAME];
    int cpu_time_order[MAX_SYSCALLS_PER_PROCESS];
    int syscall_order[MAX_SYSCALLS_PER_PROCESS];
    int param1_order[MAX_SYSCALLS_PER_PROCESS];
    int param2_order[MAX_SYSCALLS_PER_PROCESS];
};
struct Command commands[MAX_COMMANDS];


/*
 * Object: Process
 *
 * pid:                     process ID
 * commandID:               index of corresponding Command object in commands array
 * computation_time:        time elapsed on computation
 * current_line:            current iteration of system call
 * sleep_time_left:         allocated sleep time (if sleep call is made)
 * children_alive:          number of child processes that are still in the system
 * parentID:                process ID of parent process
 */
struct Process {
    int pid;
    int commandID;
    int computation_time;
    int current_line;
    int sleep_time_left;
    int children_alive;
    int parentID;              
};
struct Process all_processes[MAX_RUNNING_PROCESSES];


/*
 * Object: Device
 *
 * name:                    name of device
 * readspeed:               read speed in Bytes per second
 * writespeed:              write speed in Bytes per second
 */
struct Device {
    char name[MAX_DEVICE_NAME];
    int readspeed;
    int writespeed;
};
struct Device devices[MAX_DEVICES];


/*
 * Object: Queue
 *
 * num:                     number of items in queue
 * pids[]:                  array of process IDs that are enqueued inside
 */
struct Queue {
    int num;
    int pids[MAX_RUNNING_PROCESSES];
};


/*
 * Object: CPU
 *
 * pid:                               pid that is computing on the CPU
 * transitioningPid:                  pid that is being transported by the OS from some location to another
 * transitioningDestination:          destination code for the transitioning process
 * isTransitioning:                   true: OS is using CPU to transition a process state, false otherwise
 * time_quantum_left:                 remaining time for CPU process before expiring
 * to_ready_time:                     remaining time for transitioning process before reaching ready queue
 * to_running_time:                   remaining time for transitioning process before reaching CPU
 * to_block_time:                     remaining time for transitioning process before reaching sleep/wait/IO queue
 * OS_task:                           current system state
 * computing_time:                    total time spent on computations
 * 
 */
struct CPU {
    int pid;
    int transitioningPid;
    int transitioningDestination;
    bool isTransitioning;
    int time_quantum_left;
    int to_ready_time;
    int to_running_time;
    int to_block_time;
    int OS_task;
    int computing_time;
};


/*
 * Object: IOcontroller
 *
 * pid:                    process that is currently reading/writing
 * time_remaining:         time left before process completes I/O
 * 
 */
struct IOcontroller {
    int pid;
    int time_remaining;
};


//---------------------------------------------------- INITIALISATION OF OBJECTS AND GLOBAL VARIABLES -------------------------------------------
struct Queue sleepQueue;
struct Queue waitQueue;
struct Queue readyQueue;
struct Queue io_queues[MAX_DEVICES];       // array of queues with each queue assigned to a device
struct CPU cpu;
struct IOcontroller iocontroller;
int num_processes = 0;                     // total number of processes in the system
int num_devices = 0;                       // total number of I/O devices
int global_time = 0;                       // global time for the system that increments in steps of 1 usec
int nullStates = 0;                        // count of how many system states CPU has skipped without doing work
int timeQuantum = DEFAULT_TIME_QUANTUM;




//------------------------------------------------- FUNCTIONS ------------------------------------------------------------------------------------


/*
 * Enqueueing functions
 * Brief:  Enqueue either a multi-blocked I/O queue object or a Queue object
 */
void enqueue(struct Queue *queue, int pid){
    queue->pids[queue->num] = pid;
    queue->num += 1;
}
void enqueue_io_queues(int pid, int device){
    enqueue(&io_queues[device], pid);
}


/*
 * Getter functions
 * Input:    id number of object
 *           
 * Output:   A pointer to the corresponding object
 */
struct Process *getProcessPtr(int pid){
    if (pid != -1){
        return &all_processes[pid];
    }
}
struct Command *getCommandPtr(int commandID){
    return &commands[commandID];
}



/*
 * Getter functions
 * Input:    processID
 *           
 * Output:   value within Command object
 */
int get_current_syscall_time(int pid){
    struct Process *process = getProcessPtr(pid);
    struct Command *command = getCommandPtr(process->commandID);
    return command->cpu_time_order[process->current_line];
}
int get_current_syscall(int pid){
    struct Process *process = getProcessPtr(pid);
    struct Command *command = getCommandPtr(process->commandID);
    return command->syscall_order[process->current_line];
}
int get_current_param1(int pid){
    struct Process *process = getProcessPtr(pid);
    struct Command *command = getCommandPtr(process->commandID);
    return command->param1_order[process->current_line];
}
int get_current_param2(int pid){
    struct Process *process = getProcessPtr(pid);
    struct Command *command = getCommandPtr(process->commandID);
    return command->param2_order[process->current_line];
}



/*
 * Function: create_process
 * Brief:    Creates a Process object for first process or next spawn call
 * 
 * Input:    commandID - corresponing command
 *           parentID - parentID of created process. assign -1 for root process
 * Output:   Pointer to created process
 */
struct Process *create_process(int commandID, int parentID){
    all_processes[num_processes].commandID = commandID;
    all_processes[num_processes].computation_time = 0;
    all_processes[num_processes].parentID = parentID;
    all_processes[num_processes].current_line = -1;
    all_processes[num_processes].sleep_time_left = 0;
    all_processes[num_processes].children_alive = 0;
    all_processes[num_processes].pid = num_processes;
    num_processes += 1;
    return &all_processes[num_processes-1];
}



/*
 * Dequeueing functions
 * Brief:  All functions will return a pid. If none can be dequeued, -1 is returned
 */
int dequeue(struct Queue *queue){
    if (queue->num == 0) {
        return -1;
    }
    int pid = queue->pids[0];
    for (int i = 0; i < queue->num; i++) {
        queue->pids[i] = queue->pids[i+1]; 
    }
    queue->num -= 1;
    return pid;
}
int dequeue_sleep_queue(){
    int process = -1;
    int idx;    
    for (int i=0; i<sleepQueue.num; i++){
        if (getProcessPtr(sleepQueue.pids[i])->sleep_time_left <= 0){
            process = sleepQueue.pids[i];
            idx = i;
            break;
        }
    }
    if (process == -1){
        return -1;
    }
    for (int i=idx+1; i<sleepQueue.num; i++){
        sleepQueue.pids[i-1] = sleepQueue.pids[i];
    }
    sleepQueue.num -= 1;
    return process;
}
int dequeue_wait_queue(){
    int process = -1;
    int idx;    
    for (int i=0; i<waitQueue.num; i++){
        if (getProcessPtr(waitQueue.pids[i])->children_alive == 0){
            process = waitQueue.pids[i];
            idx = i;
            break;
        }
    }
    if (process == -1) {
        return -1;
    }
    for (int i=idx+1; i<waitQueue.num; i++){
        waitQueue.pids[i-1] = waitQueue.pids[i];
    }
    waitQueue.num -= 1;
    return process;
}
int dequeue_io_queues(){
    for (int i=0; i<num_devices; i++){
        if (io_queues[i].num > 0){
            return dequeue(&io_queues[i]);
        }
    }
    return -1;
}



/*
 * Function: advance_sleep_queue
 * Brief:  Advance time for processes in sleep queue by 1 usec
 */
void advance_sleep_queue(){
    for (int i=0; i<sleepQueue.num; i++){
        struct Process *process = getProcessPtr(sleepQueue.pids[i]);
        process->sleep_time_left -= 1;
    }
}

/*
 * Function: advance_sleep_queue
 * Brief:  Advance time for the computing processes in CPU by 1 usec
 */
void advance_compute(int pid){
    struct Process *process = getProcessPtr(pid);
    process->computation_time += 1;
    cpu.time_quantum_left -= 1;
    cpu.computing_time += 1;
}


/*
 * Function: pause_sleep_queue
 * Brief:  Prevent time in sleep queue from advancing by 1 usec
 */
void pause_sleep_queue(){
    for (int i=0; i<sleepQueue.num; i++){
        struct Process *process = getProcessPtr(sleepQueue.pids[i]);
        process->sleep_time_left += 1;
    }
}


/*
 * Function: pause_time
 * Brief:  Prevent time in entire system from advancing by 1 usec
 */
void pause_time(){
    global_time -= 1;
    iocontroller.time_remaining += 1;
    pause_sleep_queue();
}


// Strip whitespace, tabspace, return carriages from left and right of a string
void stripString(char *str) {
    int len = strlen(str);
    int start = 0;
    int end = len - 1;
    while (isspace((unsigned char)str[start]) || str[start] == '\n' || str[start] == '\t' || str[start] == '\r') {
        start++;
    }
    while (end >= 0 && (isspace((unsigned char)str[end]) || str[end] == '\n' || str[end] == '\t' || str[end] == '\r')) {
        end--;
    }
    str[end + 1] = '\0';
    if (start > 0) {
        memmove(str, str + start, len - start + 1);
    }
}


/*
 * Getter functions
 * Input:    string name of object
 *           
 * Output:   integer code corresponding to the object
 */
int findDevice(char name[]){
    for (int i=0; i<MAX_DEVICES; i++){
        if (strcmp(name, devices[i].name) == 0){
            return i;
        }
    }
}
int findCommand(char name[]){
    for (int i=0; i<MAX_COMMANDS; i++){
        if (strcmp(name, commands[i].name) == 0){
            return i;
        }
    }
}



/*
 * String parsing functions

 * Brief:  Takes a line of string input and extracts information
 * It then stores them in appropriate data structures
 */
void parse_syscall_string(char str[], int *p1, int *p2, int *p3, int *p4){
    char *token;
    const char *delimiter = " \t";
    token = strtok(str, delimiter);
    sscanf(token, "%i", p1);
    token = strtok(NULL, delimiter);
    if (strcmp(token, "read") == 0) {
        *p2 = READ_CALL;
        token = strtok(NULL, delimiter);
        *p3 = findDevice(token);
        token = strtok(NULL, delimiter);
        sscanf(token, "%i", p4);
    } else if (strcmp(token, "write") == 0) {
        *p2 = WRITE_CALL;
        token = strtok(NULL, delimiter);
        *p3 = findDevice(token);
        token = strtok(NULL, delimiter);
        sscanf(token, "%i", p4);
    } else if (strcmp(token, "sleep") == 0) {
        *p2 = SLEEP_CALL;
        token = strtok(NULL, delimiter);
        sscanf(token, "%i", p4);
    } else if (strcmp(token, "spawn") == 0) {
        *p2 = SPAWN_CALL;
        token = strtok(NULL, delimiter);
        *p3 = findCommand(token);
    } else if (strcmp(token, "wait") == 0) {
        *p2 = WAIT_CALL;
    } else if (strcmp(token, "exit") == 0) {
        *p2 = EXIT_CALL;
    }
}
void parse_device_string(char str[], char *p1, int *p2, int *p3){
    char *token;
    const char *delimiter = " \t";
    token = strtok(str, delimiter);
    token = strtok(NULL, delimiter);
    strcpy(p1, token);
    token = strtok(NULL, delimiter);
    sscanf(token, "%i", p2);
    token = strtok(NULL, delimiter);
    sscanf(token, "%i", p3);
}
void parse_timequantum_string(char str[], int *p1){
    char *token;
    const char *delimiter = " \t";
    token = strtok(str, delimiter);
    token = strtok(NULL, delimiter);
    sscanf(token, "%i", p1);
}


/*
 * Function: read_sysconfig
 * Brief:  Read sysconfig file into data structures
 */
void read_sysconfig(char filename[]){
    FILE *file = fopen(filename, "r");
    if (file == NULL){
        fprintf(stderr, "Error: SysConfig file could not be opened\n");
    } else {
        char line[MAX_LINE_SIZE];
        char tempLine[MAX_LINE_SIZE];
        int i = 0;
        num_devices = 0;
        while (fgets(line, MAX_LINE_SIZE, file) != NULL) {
            if (line[0] == CHAR_COMMENT) {
                continue;
            } else if (strncmp(line, "timequantum", 10) == 0) {
                strcpy(tempLine, line);
                stripString(tempLine);
                parse_timequantum_string(tempLine, &timeQuantum);
            } else  {
                strcpy(tempLine, line);
                stripString(tempLine);
                parse_device_string(tempLine, devices[i].name, &devices[i].readspeed, &devices[i].writespeed);
                i += 1;
                num_devices += 1;
            }
        }
        fclose(file);
    }
}


/*
 * Function: read_commands
 * Brief:  Read commands file into data structures
 */
void read_commands(char filename[]) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Command file could not be opened\n");
    } else {
        char line[MAX_LINE_SIZE];
        char tempLine[MAX_LINE_SIZE];
        int i = -1;
        int j = 0;
        char timeStr[MAX_LINE_SIZE];
        char syscallStr[MAX_LINE_SIZE];
        char param1Str[MAX_LINE_SIZE];
        char param2Str[MAX_LINE_SIZE];
        while (fgets(line, MAX_LINE_SIZE, file) != NULL) {
            if (line[0] == CHAR_COMMENT) {
                continue;
            } else if (line[0] == '\t'){
                strcpy(tempLine, line);
                stripString(tempLine);
                parse_syscall_string(tempLine, commands[i].cpu_time_order + j, commands[i].syscall_order + j, commands[i].param1_order + j, commands[i].param2_order + j);
                j += 1;
            } else {
                strcpy(tempLine, line);
                stripString(tempLine);
                i += 1;
                strcpy(commands[i].name, tempLine);
                j = 0;
            }
        }
        fclose(file);
    }
}


/*
 * Function: compareDevices
 * Brief:  Comparator function for sorting of devices (qsort) on readspeed and therefore priority
 */
int compareDevices(const void *a, const void *b) { 
    const struct Device *deviceA = (const struct Device *)a;
    const struct Device *deviceB = (const struct Device *)b;   
    if (deviceA->readspeed < deviceB->readspeed){
        return 1;
    }
    if (deviceA->readspeed > deviceB->readspeed){
        return -1;
    }
    return 0;
}


/*
 * Function: handle_syscall
 * Brief:  Changes system state and process state depending on type of system call issued
 */
void handle_syscall(int syscall, int pid){
    struct Process *process = getProcessPtr(pid);
    if (syscall==READ_CALL || syscall==WRITE_CALL){

        cpu.isTransitioning = true;
        cpu.transitioningPid = pid;
        cpu.pid = -1;
        cpu.to_block_time = TIME_CORE_STATE_TRANSITIONS;
        cpu.transitioningDestination = IO_DESTINATION;
    } else if (syscall==SLEEP_CALL){

        process->sleep_time_left = get_current_param2(pid) - TIME_CORE_STATE_TRANSITIONS;
        cpu.pid = -1;
        cpu.transitioningPid = pid;
        cpu.to_block_time = TIME_CORE_STATE_TRANSITIONS;
        cpu.transitioningDestination = SLEEP_DESTINATION;
        cpu.isTransitioning = true;
    } else if (syscall==SPAWN_CALL){

        struct Process *child = create_process(get_current_param1(pid), pid);
        enqueue(&readyQueue, child->pid);
        cpu.transitioningPid = pid;
        cpu.pid = -1;
        cpu.to_ready_time = TIME_CORE_STATE_TRANSITIONS;
        cpu.isTransitioning = true;
        cpu.transitioningDestination = READY_DESTINATION;
        process->children_alive += 1;   
    } else if (syscall==WAIT_CALL){

        if (process->children_alive >= 1){
            cpu.pid = -1;
            cpu.transitioningPid = pid;
            cpu.to_block_time = TIME_CORE_STATE_TRANSITIONS;
            cpu.isTransitioning = true;
            cpu.transitioningDestination = WAIT_DESTINATION;
        } else {
            cpu.pid = -1;
            cpu.transitioningPid = pid;
            cpu.to_ready_time = TIME_CORE_STATE_TRANSITIONS;
            cpu.isTransitioning = true;
            cpu.transitioningDestination = READY_DESTINATION;
        }
    } else if (syscall==EXIT_CALL){

        cpu.pid = -1;
        pause_time();
        cpu.OS_task = SLEEP_STATE;
        if (pid == 0){
            num_processes -= 1;
        } else {
            struct Process *parent = getProcessPtr(process->parentID);
            parent->children_alive -= 1;
            num_processes -= 1;
        }
    } 
}


/*
 * Function: handle_exceeded_time_quantum
 * Brief:  Changes system state and process state when time quantum has exceeded
 */
void handle_exceeded_time_quantum(int pid){
    struct Process *process = getProcessPtr(pid);
    cpu.pid = -1;
    process->current_line -= 1;
    cpu.transitioningPid = pid;
    cpu.to_ready_time = TIME_CORE_STATE_TRANSITIONS;
    cpu.isTransitioning = true;
    cpu.transitioningDestination = READY_DESTINATION;
    pause_time();
}


/*
 * Function: transition_process_from_running
 * Brief:  Changes system state and process state depending on where CPU process is transitioning to
 */
void transition_process_from_running(int pid, int destination){
    nullStates = 0;
    if (destination == SLEEP_DESTINATION){
        if (cpu.to_block_time == 0){
            enqueue(&sleepQueue, pid);
            cpu.isTransitioning = false;
            cpu.OS_task = SLEEP_STATE;
            pause_time();
        } else {
            cpu.to_block_time -= 1;
        }
    } else if (destination == READY_DESTINATION){
        if (cpu.to_ready_time == 0){
            enqueue(&readyQueue, pid);
            cpu.isTransitioning = false;
            cpu.OS_task = SLEEP_STATE;
            pause_time();
        } else {
            cpu.to_ready_time -= 1;
        }
    } else if (destination == IO_DESTINATION){
        if (cpu.to_block_time == 0){
            enqueue_io_queues(pid, get_current_param1(pid));
            cpu.isTransitioning = false;
            cpu.OS_task = SLEEP_STATE;
            pause_time();
        } else {
            cpu.to_block_time -= 1;
        }
    } else if (destination == WAIT_DESTINATION){
        if (cpu.to_block_time == 0){
            enqueue(&waitQueue, pid);
            cpu.isTransitioning = false;
            cpu.OS_task = SLEEP_STATE;
            pause_time();
        } else {
            cpu.to_block_time -= 1;
        }
    }
}



/*
 * Function: advance_system

 * Brief:  Advances entire system state by 1 usec
 * If work done was considered instantaneous, global_time will be rewinded back by 1 usec
 */
void advance_system(){
    if (cpu.OS_task == CPU_STATE && cpu.isTransitioning){
        transition_process_from_running(cpu.transitioningPid, cpu.transitioningDestination);
    } else if (cpu.OS_task == CPU_STATE && !cpu.isTransitioning){
        if (cpu.pid == -1){
            nullStates += 1;
            cpu.OS_task = SLEEP_STATE;
            pause_time();
        } else {
            nullStates = 0;
            if (get_current_syscall_time(cpu.pid) == getProcessPtr(cpu.pid)->computation_time){
                int syscall = get_current_syscall(cpu.pid);
                handle_syscall(syscall, cpu.pid);       
            } else if (cpu.time_quantum_left == 0){
                handle_exceeded_time_quantum(cpu.pid);
            } else {
                advance_compute(cpu.pid);
            }
        }
    } else if (cpu.OS_task == SLEEP_STATE && cpu.isTransitioning){
        nullStates = 0;
        if (cpu.to_ready_time == 0){
            enqueue(&readyQueue, cpu.transitioningPid);
            cpu.isTransitioning = false;
            cpu.OS_task = WAIT_STATE;
            pause_time();
        } else {
            cpu.to_ready_time -= 1;
        }
    } else if (cpu.OS_task == SLEEP_STATE && !cpu.isTransitioning){
        int pid = dequeue_sleep_queue();
        pause_time();
        if (pid == -1) {
            cpu.OS_task = WAIT_STATE;
            nullStates += 1;
        } else {
            nullStates = 0;
            cpu.transitioningPid = pid;
            cpu.to_ready_time = TIME_CORE_STATE_TRANSITIONS;
            cpu.isTransitioning = true;
            cpu.transitioningDestination = READY_DESTINATION;
        }
    } else if (cpu.OS_task == WAIT_STATE && cpu.isTransitioning){
        nullStates = 0;
        if (cpu.to_ready_time == 0) {
            enqueue(&readyQueue, cpu.transitioningPid);
            cpu.isTransitioning = false;
            cpu.OS_task = UNBLOCK_IO_STATE;
            pause_time();
        } else {
            cpu.to_ready_time -= 1;
        }
    } else if (cpu.OS_task == WAIT_STATE && !cpu.isTransitioning){
        int pid = dequeue_wait_queue();
        pause_time();
        if (pid == -1){
            cpu.OS_task = UNBLOCK_IO_STATE;
            nullStates += 1;
        } else {
            nullStates = 0;
            cpu.transitioningPid = pid;
            cpu.to_ready_time = TIME_CORE_STATE_TRANSITIONS;
            cpu.isTransitioning = true;
            cpu.transitioningDestination = READY_DESTINATION;
        }
    } else if (cpu.OS_task == UNBLOCK_IO_STATE && cpu.isTransitioning){
        nullStates = 0;
        if (cpu.to_ready_time == 0){
            enqueue(&readyQueue, cpu.transitioningPid);
            cpu.isTransitioning = false;
            cpu.OS_task = COMMENCE_IO_STATE;
            pause_time();
        } else {
            cpu.to_ready_time -= 1;
        }
    } else if (cpu.OS_task == UNBLOCK_IO_STATE && !cpu.isTransitioning){
        if (iocontroller.pid == -1){
            cpu.OS_task = COMMENCE_IO_STATE;
            nullStates += 1;
            pause_time();
        } else {
            nullStates = 0;
            if (iocontroller.time_remaining > 0){
                iocontroller.time_remaining -= 1;
            } else {
                cpu.transitioningPid = iocontroller.pid;
                iocontroller.pid = -1;
                cpu.to_ready_time = TIME_CORE_STATE_TRANSITIONS;
                cpu.isTransitioning = true;
                cpu.transitioningDestination = READY_DESTINATION;
                pause_time();
            }
        }
    } else if (cpu.OS_task == COMMENCE_IO_STATE && !cpu.isTransitioning){
        int pid = dequeue_io_queues();
        pause_time();
        if (pid == -1){
            cpu.OS_task = ABSORB_READY_STATE;
            nullStates += 1;
        } else {
            nullStates = 0;
            iocontroller.pid = pid;
            int device = get_current_param1(pid);
            int speed;
            if (get_current_syscall(pid) == READ_CALL){
                speed = devices[device].readspeed;
            } else if (get_current_syscall(pid) == WRITE_CALL) {
                speed = devices[device].writespeed;
            }
            int size = get_current_param2(pid);
            iocontroller.time_remaining = (size*1000000)/speed + 20;
            cpu.OS_task = ABSORB_READY_STATE;
        }
    } else if (cpu.OS_task == ABSORB_READY_STATE && cpu.isTransitioning){
        nullStates = 0;
        if (cpu.to_running_time == 0) {
            cpu.pid = cpu.transitioningPid;
            struct Process *process = getProcessPtr(cpu.pid);
            process->current_line += 1;
            cpu.time_quantum_left = timeQuantum;
            cpu.isTransitioning = false;
            cpu.OS_task = CPU_STATE;
            pause_time();
        } else {
            cpu.to_running_time -= 1;
        }
    } else if (cpu.OS_task == ABSORB_READY_STATE && !cpu.isTransitioning){
        int pid = dequeue(&readyQueue);
        pause_time();
        if (pid == -1){
            cpu.OS_task = SLEEP_STATE;
            nullStates += 1;
        } else {
            nullStates = 0;
            cpu.transitioningPid = pid;
            cpu.to_running_time = TIME_CONTEXT_SWITCH;
            cpu.isTransitioning = true;
            cpu.transitioningDestination = CPU_DESTINATION;
        }
    }
}


//------------------------------------------------DEBUGGING FUNCTIONS--------------------------------------------------------------------------
void print_system_state(){
    printf("\n\nGlobal time: %i\n", global_time);

    printf("\t cpu.pid:%i\n", cpu.pid);
    printf("\t cpu.transitioningPid:%i\n", cpu.transitioningPid);
    printf("\t cpu.time_quantum_left:%i\n", cpu.time_quantum_left);
    printf("\t cpu.isTransitioning:%i\n", cpu.isTransitioning);
    printf("\t cpu.OS_task:%i\n", cpu.OS_task);
    printf("\t cpu.computing_time:%i\n", cpu.computing_time);
    printf("\t cpu.transitioningDestination:%i\n", cpu.transitioningDestination);
    printf("\t cpu.to_ready_time:%i\n", cpu.to_ready_time);
    printf("\t cpu.to_block_time:%i\n", cpu.to_block_time);
    printf("\t cpu.to_running_time:%i\n", cpu.to_running_time);
    printf("\t num_processes:%i\n", num_processes);    
}
void print_cpu_process_information(){
    if (cpu.pid == -1){
        printf("CPU does not have process currently\n");
    } else {
        struct Process *process = getProcessPtr(cpu.pid);
        printf("\n\n");
        printf("\t process.pid:%i\n", process->pid);
        printf("\t process.commandID:%i\n", process->commandID);
        printf("\t process.computation_time:%i\n", process->computation_time);
        printf("\t process.current_line:%i\n", process->current_line);
        printf("\t process.sleep_time_left:%i\n", process->sleep_time_left);
        printf("\t process.children_alive:%i\n", process->children_alive);
        printf("\t process.parentID:%i\n", process->parentID);
    }
}
void print_iocontroller_process_information(){
    if (iocontroller.pid == -1){
        printf("iocontroller does not have process currently\n");
    } else {
        printf("\n\n");
        printf("\t iocontroller.pid:%i\n", iocontroller.pid);
        printf("\t iocontroller.time_remaining:%i\n", iocontroller.time_remaining);
    }
}
void print_process_information(int pid){
    struct Process *process = getProcessPtr(pid);
    printf("\n\n");
    printf("\t process.pid:%i\n", process->pid);
    printf("\t process.commandID:%i\n", process->commandID);
    printf("\t process.computation_time:%i\n", process->computation_time);
    printf("\t process.current_line:%i\n", process->current_line);
    printf("\t process.sleep_time_left:%i\n", process->sleep_time_left);
    printf("\t process.children_alive:%i\n", process->children_alive);
    printf("\t process.parentID:%i\n", process->parentID);
}





//------------------------------------------------------ MAIN -----------------------------------------------------------------------
int main(int argc, char *argv[]){

    // File handling
    if(argc != 3) {
        printf("Incorrect number of arguments provided. Usage: %s sysconfig_file command_file\n", argv[0]);
        exit(EXIT_FAILURE);
    }    

    // Initialisation of queue variables
    readyQueue.num = 0;                                            
    sleepQueue.num = 0;
    waitQueue.num = 0;
    for (int i=0; i<num_devices; i++){
        io_queues[i].num = 0;
    }

    // Initialisation of devices
    read_sysconfig(argv[1]);
    qsort(devices, num_devices, sizeof(devices[0]), compareDevices); // sort devices within an array on priority (index 0 being highest priority)


    // Initialisation of commands (read_commands called twice because commands may refer to another command ahead which has not been defined)
    read_commands(argv[2]);
    read_commands(argv[2]);


    // Creation of root process
    struct Process *rootProcess = create_process(0, -1);
    enqueue(&readyQueue, rootProcess->pid);


    // Initialisation of system state
    cpu.isTransitioning = false;
    cpu.OS_task = ABSORB_READY_STATE;
    cpu.computing_time = 0;
    cpu.pid = -1;
    iocontroller.pid = -1;
    iocontroller.time_remaining = 0;



    do {
        print_system_state();
        print_cpu_process_information();
        print_iocontroller_process_information();


        advance_system();
        advance_sleep_queue();
        iocontroller.time_remaining -= 1;
        global_time += 1;
        

        // if CPU cycled through all system states and has done no work, increase idle time by 1 usec
        if (nullStates == 6){  
        advance_sleep_queue();
        iocontroller.time_remaining -= 1;
        global_time += 1;
        nullStates = 0;
        }


    } while (num_processes > 0);


    global_time += 1;  // calibration
    printf("\n");

    // output: measurements  total_time  cpu_usage
    printf("measurements  %i  %i\n", global_time, (cpu.computing_time*100)/global_time);  
    exit(EXIT_SUCCESS);
}