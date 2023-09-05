#include <stdio.h>
#include <stdlib.h>

//  CITS2002 Project 1 2023
//  Student1: 22974799 Samuel Chew
//  Student2: 24187986 Julian Prawira



// check if all these constants are used                             (TODO)
#define MAX_DEVICES                     4
#define MAX_DEVICE_NAME                 20
#define MAX_COMMANDS                    10
#define MAX_COMMAND_NAME                20
#define MAX_SYSCALLS_PER_PROCESS        40
#define MAX_RUNNING_PROCESSES           50

#define DEFAULT_TIME_QUANTUM            100

#define TIME_CONTEXT_SWITCH             5
#define TIME_CORE_STATE_TRANSITIONS     10
#define TIME_ACQUIRE_BUS                20

#define CHAR_COMMENT                    '#'

// Julian's code to be tested                                    (TODO)

#define READY 0
#define RUNNING 1
#define BLOCKED 2
#define EXIT 3




// to be tested                                                   (TODO)
struct Process {
    int state;
    int time_spent_on_the_cpu;  // ??
};

// to be tested                                                (TODO)
struct Queue {
    int number;
    struct* Process processes[MAX_RUNNING_PROCESSES];  // ??
};

// to be tested                                                (TODO)
struct CPU {
    struct Process* process;
};

// do i need to do something else if queue exceeds max capacity? code to be tested              TODO
void enqueue(struct Queue *queue, struct Process *process) {
    if (queue->number < MAX_RUNNING_PROCESSES) {
        queue->processes[queue->number] = process;
        queue->number++;
    }
}

// should i return process id or pointer? this one assumes the queue is filled with ids. code to be tested              TODO
int dequeue(struct Queue *queue){
    if (queue->number > 0) {
        int process_id = queue->processes[0];
        for (int i = 1; i < queue->number; i++) {
            queue->processes[i - 1] = queue->processes[i]; 
        }
        queue->number--;
        return process_id;
    }
    return -1;
}






// code to be tested                                         TODO
void read_sysconfig(char argv0[], char filename[]){
    FILE *file = fopen(filename, "r");

    if (file == NULL){
        fprintf(stderr, "Error opening file %s\n", filename);
        return;
    }
    char line[100];

    while (fgets(line, sizeof(line), file) != NULL ){
        char *token = strtok(line, "  \t\n");
        while (token != NULL) {
            printf("%s\n", token);
            token = strtok(NULL, " ");
        }
    }
    fclose(file);
    return;
}


// code to be tested and understood                                                 TODO
void read_commands(char argv0[], char filename[]) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file %s\n", filename);
        return; 
    }
    char line[100];

    while (fgets(line, sizeof(line), file) != NULL) { // Read each line
        // Tokenize the line to separate command and system calls
        char *command = strtok(line, " \t\n"); // Split line into tokens
        if (command == NULL) { // Check if line is empty
            continue; // Skip empty lines
        }

        // Process the command and its associated system calls
        printf("Command: %s\n", command); // Print command
        char *syscall = strtok(NULL, " \t\n"); // Get first system call
        while (syscall != NULL) { // Print each system call
            printf("System Call: %s\n", syscall); // Print system call
            // Implement logic to execute the system call
            syscall = strtok(NULL, " \t\n"); // Get next system call
        }
    }
    fclose(file);
    return;
}






void execute_commands(void){
    //                       TODO
}



int main(int argc, char *argv[])
{
    if(argc != 3) {
        printf("Usage: %s sysconfig-file command-file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    read_sysconfig(argv[0], argv[1]);
    read_commands(argv[0], argv[2]);
    execute_commands();


    printf("measurements  %i  %i\n", 0, 0);  //  replace 0 with the total time taken (microseconds) and the CPU-utilisation (percentage)  TODO
    exit(EXIT_SUCCESS);
}