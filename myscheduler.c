#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

//  CITS2002 Project 1 2023
//  Student1: 22974799 Samuel Chew
//  Student2: 24187986 Julian Prawira

#define MAX_COMMANDS                    10
#define MAX_COMMAND_NAME                20
#define MAX_LINE_SIZE                   100
#define CHAR_COMMENT                    '#'
#define MAX_SYSCALLS_PER_PROCESS        40  
#define MAX_RUNNING_PROCESSES           50
#define MAX_DEVICES                     4
#define MAX_DEVICE_NAME                 20
#define DEFAULT_TIME_QUANTUM            100
#define READ_CALL                       1
#define WRITE_CALL                      2
#define SLEEP_CALL                      3
#define SPAWN_CALL                      4
#define WAIT_CALL                       5  
#define EXIT_CALL                       6
#define CPU_STATE                       0
#define SLEEP_STATE                     1
#define WAIT_STATE                      2
#define UNBLOCK_IO_STATE                3
#define COMMENCE_IO_STATE               4
#define ABSORB_READY_STATE              5
#define TIME_ACQUIRE_BUS                20
#define TIME_CORE_STATE_TRANSITIONS     10
#define TIME_CONTEXT_SWITCH             5
#define SLEEP_DESTINATION               1
#define READY_DESTINATION               2
#define IO_DESTINATION                  3
#define WAIT_DESTINATION                4
#define CPU_DESTINATION                 5

struct Command {
    char name[MAX_COMMAND_NAME];
    int cpu_time_order[MAX_SYSCALLS_PER_PROCESS];
    int syscall_order[MAX_SYSCALLS_PER_PROCESS];
    int param1_order[MAX_SYSCALLS_PER_PROCESS];
    int param2_order[MAX_SYSCALLS_PER_PROCESS];
};
struct Command commands[MAX_COMMANDS];
struct Process {
    struct Command *command;
    int computation_time;
    int current_line;
    int sleep_time_left;
    int children_alive;
    struct Process *parent;              
};
struct Process all_processes[MAX_RUNNING_PROCESSES];
struct Queue {
    int num;
    struct Process *processes[MAX_RUNNING_PROCESSES];
};
struct CPU {
    struct Process* process;
    int time_quantum_left;
    int to_ready_time;
    int to_block_time;
    int to_running_time;
    bool isTransitioning;
    int OS_task;
    struct Process* transitioningProcess;
    int usage_time;
    int transitioningDestination;
};
struct IOcontroller {
    struct Process* process;
    int time_remaining;
};
struct Device {
    char name[MAX_DEVICE_NAME];
    int readspeed;
    int writespeed;
};
struct Device devices[MAX_DEVICES];
int num_processes = 0;
int num_devices = 0;
int global_time = 0;
struct Queue readyQueue;
struct Queue sleepQueue;
struct Queue waitQueue;
struct CPU cpu;
struct IOcontroller iocontroller;
int timeQuantum = DEFAULT_TIME_QUANTUM;
struct Queue io_queues[MAX_DEVICES];
void enqueue(struct Queue *queue, struct Process *process){
    queue->processes[queue->num] = process;
    queue->num += 1;
}
struct Process *create_process(int command){
    all_processes[num_processes].command = &commands[command];
    all_processes[num_processes].computation_time = 0;
    all_processes[num_processes].parent = NULL;
    all_processes[num_processes].current_line = -1;
    all_processes[num_processes].sleep_time_left = 0;
    all_processes[num_processes].children_alive = 0;
    num_processes += 1;
    return &all_processes[num_processes-1];
}
int get_current_syscall_time(struct Process *process){
    return process->command->cpu_time_order[process->current_line];
}
int get_current_syscall(struct Process *process){
    return process->command->syscall_order[process->current_line];
}
int get_current_param1(struct Process *process){
    return process->command->param1_order[process->current_line];
}
int get_current_param2(struct Process *process){
    return process->command->param2_order[process->current_line];
}
struct Process *dequeue(struct Queue *queue){
    if (queue->num == 0) {
        return NULL;
    }
    struct Process* process = queue->processes[0];
    for (int i = 1; i < queue->num; i++) {
        queue->processes[i - 1] = queue->processes[i]; 
    }
    queue->num -= 1;
    return process;
}
struct Process *dequeue_sleep_queue(){
    struct Process* process = NULL;
    int idx;    
    for (int i=0; i<sleepQueue.num; i++){
        if (sleepQueue.processes[i]->sleep_time_left <= 0){
            process = sleepQueue.processes[i];
            idx = i;
            break;
        }
    }
    if (process == NULL){
        return NULL;
    }
    for (int i=idx+1; i<sleepQueue.num; i++){
        sleepQueue.processes[i-1] = sleepQueue.processes[i];
    }
    sleepQueue.num -= 1;
    return process;
}
struct Process *dequeue_wait_queue(){
    struct Process* process = NULL;
    int idx;    
    for (int i=0; i<waitQueue.num; i++){
        if (waitQueue.processes[i]->children_alive == 0){
            process = waitQueue.processes[i];
            idx = i;
            break;
        }
    }
    if (process == NULL) {
        return NULL;
    }
    for (int i=idx+1; i<waitQueue.num; i++){
        waitQueue.processes[i-1] = waitQueue.processes[i];
    }
    waitQueue.num -= 1;
    return process;
}
struct Process *dequeue_io_queues(){
    for (int i=0; i<num_devices; i++){
        if (io_queues[i].num > 0){
            return dequeue(&io_queues[i]);
        }
    }
    return NULL;
}
void enqueue_io_queues(struct Process *process, int device){
    enqueue(&io_queues[device], process);
}
void advance_sleep_queue(){
    for (int i=0; i<sleepQueue.num; i++){
        sleepQueue.processes[i]->sleep_time_left -= 1;
    }
}
void advance_io(){
    iocontroller.time_remaining -= 1;
}
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
void handle_syscall(int syscall, struct Process *process){
    if (syscall==READ_CALL || syscall==WRITE_CALL){
        cpu.isTransitioning = true;
        cpu.transitioningProcess = process;
        cpu.process = NULL;
        cpu.to_block_time = TIME_CORE_STATE_TRANSITIONS;
        cpu.transitioningDestination = IO_DESTINATION;
        cpu.usage_time += 1;
    } else if (syscall==SLEEP_CALL){
        cpu.process->sleep_time_left = get_current_param2(cpu.process) - TIME_CORE_STATE_TRANSITIONS;
        cpu.process = NULL;
        cpu.transitioningProcess = cpu.process;
        cpu.to_block_time = TIME_CORE_STATE_TRANSITIONS;
        cpu.usage_time += 1;
        cpu.transitioningDestination = SLEEP_DESTINATION;
        cpu.isTransitioning = true;
    } else if (syscall==SPAWN_CALL){
        struct Process *child = create_process(get_current_param1(process));
        enqueue(&readyQueue, child);
        child->parent = process;
        cpu.transitioningProcess = process;
        cpu.process = NULL;
        cpu.to_ready_time = TIME_CORE_STATE_TRANSITIONS;
        cpu.isTransitioning = true;
        cpu.usage_time += 1;
        cpu.transitioningDestination = READY_DESTINATION;
        process -> children_alive += 1;   
    } else if (syscall==WAIT_CALL){
        if (process->children_alive >= 1){
            cpu.process = NULL;
            cpu.transitioningProcess = process;
            cpu.to_block_time = TIME_CORE_STATE_TRANSITIONS;
            cpu.isTransitioning = true;
            cpu.usage_time += 1;
            cpu.transitioningDestination = WAIT_DESTINATION;
        } else {
            cpu.process = NULL;
            cpu.transitioningProcess = process;
            cpu.to_ready_time = TIME_CORE_STATE_TRANSITIONS;
            cpu.isTransitioning = true;
            cpu.usage_time += 1;
            cpu.transitioningDestination = READY_DESTINATION;
        }
    } else if (syscall==EXIT_CALL){
        cpu.process = NULL;
        cpu.OS_task = SLEEP_STATE;
        cpu.usage_time += 1;
        process -> parent ->children_alive -= 1;
        num_processes -= 1;
    } 
}
void handle_exceeded_time_quantum(struct Process *process){
    cpu.process = NULL;
    process->current_line -= 1;
    cpu.transitioningProcess = process;
    cpu.to_ready_time = TIME_CORE_STATE_TRANSITIONS;
    cpu.isTransitioning = true;
    cpu.usage_time += 1;
    cpu.transitioningDestination = READY_DESTINATION;
}
void advance_compute(struct Process *process){
    process -> computation_time += 1;
    cpu.time_quantum_left -= 1;
    cpu.usage_time += 1;
}
void transition_process_from_running(struct Process *process, int destination){
    if (destination == SLEEP_DESTINATION){
        if (cpu.to_block_time == 0){
            enqueue(&sleepQueue, process);
            cpu.isTransitioning = false;
            cpu.OS_task = SLEEP_STATE;
        } else {
            cpu.to_block_time -= 1;
            cpu.usage_time += 1;
        }
    } else if (destination == READY_DESTINATION){
        if (cpu.to_ready_time == 0){
            enqueue(&readyQueue, process);
            cpu.isTransitioning = false;
            cpu.OS_task = SLEEP_STATE;
        } else {
            cpu.to_ready_time -= 1;
            cpu.usage_time += 1;
        }
    } else if (destination == IO_DESTINATION){
        if (cpu.to_block_time == 0){
            enqueue_io_queues(process, get_current_param1(process));
            cpu.isTransitioning = false;
            cpu.OS_task = SLEEP_STATE;
        } else {
            cpu.to_block_time -= 1;
            cpu.usage_time += 1;
        }
    } else if (destination == WAIT_DESTINATION){
        if (cpu.to_block_time == 0){
            enqueue(&waitQueue, process);
            cpu.isTransitioning = false;
            cpu.OS_task = SLEEP_STATE;
        } else {
            cpu.to_block_time -= 1;
            cpu.usage_time += 1;
        }
    }
}
void advance_system(){
    if (cpu.OS_task == CPU_STATE && cpu.isTransitioning){
        transition_process_from_running(cpu.transitioningProcess, cpu.transitioningDestination);
    } else if (cpu.OS_task == CPU_STATE && !cpu.isTransitioning){
        if (get_current_syscall_time(cpu.process) == cpu.process->computation_time){
            handle_syscall(get_current_syscall(cpu.process), cpu.process);
        } else if (cpu.time_quantum_left == 0){
            handle_exceeded_time_quantum(cpu.process);
        } else {
            advance_compute(cpu.process);
        }
    } else if (cpu.OS_task == SLEEP_STATE && cpu.isTransitioning){
        if (cpu.to_ready_time == 0){
            enqueue(&readyQueue, cpu.transitioningProcess);
            cpu.isTransitioning = false;
            cpu.OS_task = WAIT_STATE;
        } else {
            cpu.to_ready_time -= 1;
            cpu.usage_time += 1;
        }
    } else if (cpu.OS_task == SLEEP_STATE && !cpu.isTransitioning){
        struct Process *process = dequeue_sleep_queue();
        if (process == NULL) {
            cpu.OS_task = WAIT_STATE;
        } else {
            cpu.transitioningProcess = process;
            cpu.to_ready_time = TIME_CORE_STATE_TRANSITIONS;
            cpu.isTransitioning = true;
            cpu.transitioningDestination = READY_DESTINATION;
        }
    } else if (cpu.OS_task == WAIT_STATE && cpu.isTransitioning){
        if (cpu.to_ready_time == 0) {
            enqueue(&readyQueue, cpu.transitioningProcess);
            cpu.isTransitioning = false;
            cpu.OS_task = UNBLOCK_IO_STATE;
        } else {
            cpu.to_ready_time -= 1;
            cpu.usage_time += 1;
        }
    } else if (cpu.OS_task == WAIT_STATE && !cpu.isTransitioning){
        struct Process *process = dequeue_wait_queue();
        if (process == NULL){
            cpu.OS_task = UNBLOCK_IO_STATE;
        } else {
            cpu.transitioningProcess = process;
            cpu.to_ready_time = TIME_CORE_STATE_TRANSITIONS;
            cpu.isTransitioning = true;
            cpu.transitioningDestination = READY_DESTINATION;
        }
    } else if (cpu.OS_task == UNBLOCK_IO_STATE && cpu.isTransitioning){
        if (cpu.to_ready_time == 0){
            enqueue(&readyQueue, cpu.transitioningProcess);
            cpu.isTransitioning = false;
            cpu.OS_task = COMMENCE_IO_STATE;
        } else {
            cpu.to_ready_time -= 1;
            cpu.usage_time += 1;
        }
    } else if (cpu.OS_task == UNBLOCK_IO_STATE && !cpu.isTransitioning){
        if (iocontroller.time_remaining > 0){
            iocontroller.time_remaining -= 1;
        } else {
            cpu.transitioningProcess = iocontroller.process;
            iocontroller.process = NULL;
            cpu.to_ready_time = TIME_CORE_STATE_TRANSITIONS;
            cpu.isTransitioning = true;
            cpu.transitioningDestination = READY_DESTINATION;
        }
    } else if (cpu.OS_task == COMMENCE_IO_STATE && !cpu.isTransitioning){
        struct Process *process = dequeue_io_queues();
        if (process == NULL){
            cpu.OS_task = ABSORB_READY_STATE;
        } else {
            iocontroller.process = process;
            int device = get_current_param1(process);
            int speed;
            if (get_current_syscall(process) == READ_CALL){
                speed = devices[device].readspeed;
            } else if (get_current_syscall(process) == WRITE_CALL) {
                speed = devices[device].writespeed;
            }
            int size = get_current_param2(process);
            iocontroller.time_remaining = (size*1000000)/speed + 20;
            cpu.OS_task = ABSORB_READY_STATE;
        }
    } else if (cpu.OS_task == ABSORB_READY_STATE && cpu.isTransitioning){
        if (cpu.to_running_time == 0) {
            cpu.process = cpu.transitioningProcess;
            cpu.process->current_line += 1;
            cpu.time_quantum_left = timeQuantum;
            cpu.isTransitioning = false;
            cpu.OS_task = CPU_STATE;
        } else {
            cpu.to_ready_time -= 1;
            cpu.usage_time += 1;
        }
    } else if (cpu.OS_task == ABSORB_READY_STATE && !cpu.isTransitioning){
        struct Process *process = dequeue(&readyQueue);
        if (process == NULL){
            cpu.OS_task = SLEEP_STATE;
        } else {
            cpu.transitioningProcess = process;
            cpu.to_running_time = TIME_CONTEXT_SWITCH;
            cpu.isTransitioning = true;
            cpu.transitioningDestination = CPU_DESTINATION;
        }
    }
}

int main(int argc, char *argv[]){
    if(argc != 3) {
        printf("Incorrect number of arguments provided. Usage: %s sysconfig_file command_file\n", argv[0]);
        exit(EXIT_FAILURE);
    } else {
        read_sysconfig(argv[1]);
        for (int i; i<num_devices; i++){
            io_queues[i].num = 0;
        }
        readyQueue.num = 0;
        sleepQueue.num = 0;
        waitQueue.num = 0;
        qsort(devices, num_devices, sizeof(devices[0]), compareDevices);
        read_commands(argv[2]);
        read_commands(argv[2]);
        enqueue(&readyQueue, create_process(0));
        cpu.isTransitioning = false;
        cpu.OS_task = ABSORB_READY_STATE;
        cpu.usage_time = 0;
        do {
            printf("The time is currently %i\n", global_time);
            advance_sleep_queue();
            advance_io();    
            advance_system();
            global_time += 1;      
        } while (num_processes != 0);
        printf("measurements  %i  %i\n", global_time, cpu.usage_time/global_time);
        exit(EXIT_SUCCESS);
    }
}