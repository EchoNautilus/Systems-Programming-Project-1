#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

//  CITS2002 Project 1 2023
//  Student1:   24187986   Julian Emir Prawira
//  Student2:   STUDENT-NUMBER2   NAME-2

//  myscheduler (v1.0)
//  Compile with:  cc -std=c11 -Wall -Werror -o myscheduler myscheduler.c

//  THESE CONSTANTS DEFINE THE MAXIMUM SIZE OF sysconfig AND command DETAILS
//  THAT YOUR PROGRAM NEEDS TO SUPPORT.  YOU'LL REQUIRE THESE //  CONSTANTS
//  WHEN DEFINING THE MAXIMUM SIZES OF ANY REQUIRED DATA STRUCTURES.

#define MAX_DEVICES                     4
#define MAX_DEVICE_NAME                 20
#define MAX_COMMANDS                    10
#define MAX_COMMAND_NAME                20
#define MAX_SYSCALLS_PER_PROCESS        40
#define MAX_RUNNING_PROCESSES           50

//  NOTE THAT DEVICE DATA-TRANSFER-RATES ARE MEASURED IN BYTES/SECOND,
//  THAT ALL TIMES ARE MEASURED IN MICROSECONDS (usecs),
//  AND THAT THE TOTAL-PROCESS-COMPLETION-TIME WILL NOT EXCEED 2000 SECONDS
//  (SO YOU CAN SAFELY USE 'STANDARD' 32-BIT ints TO STORE TIMES).

#define DEFAULT_TIME_QUANTUM            100

#define TIME_CONTEXT_SWITCH             5
#define TIME_CORE_STATE_TRANSITIONS     10
#define TIME_ACQUIRE_BUS                20

//  ----------------------------------------------------------------------

#define CHAR_COMMENT                    '#'

// Define constants for states

#define READY 0
#define RUNNING 1
#define BLOCKED 2
#define EXIT 3

// Define data structures for processes, devices, queues, etc.

struct processes { // Declare struct for processes
    int state; // Declare state variable
    int time_spent_on_cpu; // Declare time spent on CPU variable
};

// ----------------------------------------------------------------------

struct readyQueue{ // Declare struct for ready queue
    int number_of_processes_in_queue; // Declare variable to store number of processes in queue
    int processes_in_queue[MAX_RUNNING_PROCESSES]; // Use an array to store process IDs
};

// Enqueue function for readyQueue
void enqueueReady(struct readyQueue *queue, int process_id) { // Declare enqueue function with a pointer to readyQueue struct, and process ID
    if (queue->number_of_processes_in_queue < MAX_RUNNING_PROCESSES) { // Check if queue is full
        queue->processes_in_queue[queue->number_of_processes_in_queue] = process_id; // Add process ID to the end of the queue
        queue->number_of_processes_in_queue++; // Increment number of processes in queue
    }
}

// Dequeue function for readyQueue
int dequeueReady(struct readyQueue *queue){ // Declare dequeue function with pointer to readyQueue struct. No need to pass process ID as parameter because it will be removed from the queue
    if (queue->number_of_processes_in_queue > 0) { // Check if there are processes in the queue
        int process_id = queue->processes_in_queue[0]; // Get process ID from the front of the queue
        for (int i = 1; i < queue->number_of_processes_in_queue; i++) { // Shift remaining processes to the left
            queue->processes_in_queue[i - 1] = queue->processes_in_queue[i]; 
        }
        queue->number_of_processes_in_queue--; // Decrement number of processes in queue
        return process_id; // Return process ID
    }
    return -1; // Return -1 if queue is empty
}

// ----------------------------------------------------------------------
struct blockedQueue{ // Declare struct for blocked queue
    int number_of_processes_in_queue; // Declare variable to store number of processes in queue
    int processes_in_queue[MAX_RUNNING_PROCESSES]; // Use an array to store process IDs
};

// Enqueue function for blockedQueue
void enqueueBlocked(struct blockedQueue *queue, int process_id) { // Declare enqueue function with a pointer to blockedQueue struct, and process ID
    if (queue->number_of_processes_in_queue < MAX_RUNNING_PROCESSES) { // Check if queue is full
        queue->processes_in_queue[queue->number_of_processes_in_queue] = process_id; // Add process ID to the end of the queue
        queue->number_of_processes_in_queue++; // Increment number of processes in queue
    }
}

// Dequeue function for blockedQueue
int dequeueBlocked(struct blockedQueue *queue){ // Declare dequeue function with pointer to blockedQueue struct. No need to pass process ID as parameter because it will be removed from the queue
    if (queue->number_of_processes_in_queue > 0) { // Check if there are processes in the queue
        int process_id = queue->processes_in_queue[0]; // Get process ID from the front of the queue
        for (int i = 1; i < queue->number_of_processes_in_queue; i++) { // Shift remaining processes to the left
            queue->processes_in_queue[i - 1] = queue->processes_in_queue[i]; 
        }
        queue->number_of_processes_in_queue--; // Decrement number of processes in queue
        return process_id; // Return process ID
    }
    return -1; // Return -1 if queue is empty
}

// ----------------------------------------------------------------------
struct cpu { // Declare struct for CPU
    int id_of_process_currently_using_the_cpu; // Declare variable to store ID of process currently using the CPU
    int time_spent_on_the_cpu; // Declare variable to store time spent on the CPU
};

// ----------------------------------------------------------------------

void read_sysconfig(char argv0[], char filename[]){ // Declare function to read sysconfig file
    FILE *file = fopen(filename, "r"); // Opening the file.

    if (file == NULL){ // Check if the file exists.
        fprintf(stderr, "Error opening file %s\n", filename); // Print error message if the file does not exist.
        return; // Exit the function if the file does not exist. Because it's void, it doesn't return anything.
    }
    // Read the file line by line.
    char line[100]; // Declare line variable (token size is 100)

    while (fgets(line, sizeof(line), file) != NULL ){ // Read each line
        char *token = strtok(line, "  \t\n"); // Split line into tokens
        while (token != NULL) { // Print each token
            printf("%s\n", token); // Print token
            token = strtok(NULL, " "); // Get next token
        }
    }
    fclose(file); // Close file
    return;
}

void read_commands(char argv0[], char filename[]) { // Declare function to read command file
    FILE *file = fopen(filename, "r"); // Open file
    if (file == NULL) { // Check if file exists
        fprintf(stderr, "Error opening file %s\n", filename);
        return; 
    }
    char line[100]; // Declare line variable (token size is 100)

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
    fclose(file); // Close file
    return;
}

//  ----------------------------------------------------------------------

// Commands to be executed

void read(struct cpu *cpu, struct blockedQueue *blocked_queue, struct processes *processes_array, int process_id, char *device_name, int bytes_to_read) { // Declare read function
    cpu->time_spent_on_the_cpu += TIME_CONTEXT_SWITCH; // Simulate context switch time (time passing, 5 microseconds)

    printf("Process %d is reading %d bytes from device %s\n", process_id, bytes_to_read, device_name); // Perform the read operation (for simulation purposes)

    cpu->time_spent_on_the_cpu += bytes_to_read; // Simulate time to read from device (time passing, 1 microsecond per byte)

    enqueueBlocked(blocked_queue, process_id); // Add the process to the blocked queue
    processes_array[process_id].state = BLOCKED; // Change the state of the process to blocked
}

void write() {
    // Write to a device.
    // Add the process to the blocked queue.
}

void sleep() {
    // Put a process to sleep.
    // Add the process to the blocked queue.
}

void wait() {
    // Wait for a process to finish.
    // Add the process to the blocked queue.
}

void exit() {
    // Exit a process.
    // Remove the process.
}

//  ----------------------------------------------------------------------

void execute_commands(void){
    
}


//  ----------------------------------------------------------------------

int main(int argc, char *argv[])
{
//  ENSURE THAT WE HAVE THE CORRECT NUMBER OF COMMAND-LINE ARGUMENTS
    if(argc != 3) {
        printf("Usage: %s sysconfig-file command-file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

//  READ THE SYSTEM CONFIGURATION FILE
    read_sysconfig(argv[0], argv[1]);

//  READ THE COMMAND FILE
    read_commands(argv[0], argv[2]);

//  EXECUTE COMMANDS, STARTING AT FIRST IN command-file, UNTIL NONE REMAIN
    execute_commands();

//  PRINT THE PROGRAM'S RESULTS
    printf("measurements  %i  %i\n", 0, 0);

    exit(EXIT_SUCCESS);
}
