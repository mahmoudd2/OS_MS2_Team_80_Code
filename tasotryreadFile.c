#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMORY_SIZE 60
#define MAX_PROCESSES 10
#define MAX_INSTRUCTIONS 10
#define FILENAME_SIZE 50
#define DATA_SIZE 256

typedef struct {
    int processID;
    char processState[20];
    int currentPriority;
    int programCounter;
    int memoryLowerBound;
    int memoryUpperBound;
} ProcessControlBlock;

typedef struct {
    ProcessControlBlock pcb;
    char instructions[MAX_INSTRUCTIONS][50];
    int instructionCount;
} Process;

typedef struct {
    Process* processes[MAX_PROCESSES];
    int count;
} Queue;

Queue readyQueues[4];
Queue blockedQueue;
int memory[MEMORY_SIZE];
int memoryIndex = 0;

int userInputMutex = 1;
int userOutputMutex = 1;
int fileMutex = 1;

void semWait(int* mutex);
void semSignal(int* mutex);
void readFile(Process* process, char* filename);
void writeFile(Process* process, char* filename, char* data);
void interpretInstruction(Process* process);
void schedule();
void dequeueProcess(Queue* queue, int index);
void enqueueProcess(Queue* queue, Process* process);

void semWait(int* mutex) {
    while (*mutex == 0) {}
    *mutex = 0;
}

void semSignal(int* mutex) {
    *mutex = 1;
}

void readFile(Process* process, char* filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Cannot open file %s\n", filename);
        return;
    }
    
    char line[DATA_SIZE];
    while (fgets(line, sizeof(line), file)) {
        printf("Process %d reading file %s: %s", process->pcb.processID, filename, line);
    }
    
    fclose(file);
}

void writeFile(Process* process, char* filename, char* data) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("Error: Cannot create file %s\n", filename);
        return;
    }
    
    fprintf(file, "%s", data);
    fclose(file);
    printf("Process %d wrote to file %s: %s\n", process->pcb.processID, filename, data);
}

void interpretInstruction(Process* process) {
    if (process->pcb.programCounter >= process->instructionCount) {
        strcpy(process->pcb.processState, "Finished");
        return;
    }

    char* instruction = process->instructions[process->pcb.programCounter];
    char command[20], argument1[50], argument2[50];
    
    sscanf(instruction, "%s %s %s", command, argument1, argument2);
    
    if (strcmp(command, "readFile") == 0) {
        semWait(&fileMutex);
        readFile(process, argument1);
        semSignal(&fileMutex);
    } else if (strcmp(command, "writeFile") == 0) {
        semWait(&fileMutex);
        writeFile(process, argument1, argument2);
        semSignal(&fileMutex);
    }
    
    process->pcb.programCounter++;
}

void dequeueProcess(Queue* queue, int index) {
    for (int i = index; i < queue->count - 1; i++) {
        queue->processes[i] = queue->processes[i + 1];
    }
    queue->count--;
}

void enqueueProcess(Queue* queue, Process* process) {
    queue->processes[queue->count++] = process;
}

void schedule() {
    while (1) {
        int allQueuesEmpty = 1;
        for (int i = 0; i < 4; i++) {
            if (readyQueues[i].count > 0) {
                allQueuesEmpty = 0;
                Process* process = readyQueues[i].processes[0];
                
                printf("Executing process %d: %s\n", process->pcb.processID, process->instructions[process->pcb.programCounter]);
                interpretInstruction(process);
                
                if (strcmp(process->pcb.processState, "Finished") == 0) {
                    printf("Process %d finished\n", process->pcb.processID);
                    dequeueProcess(&readyQueues[i], 0);
                } else {
                    dequeueProcess(&readyQueues[i], 0);
                    enqueueProcess(&readyQueues[i], process);
                }
                
                break;  // Move to the next scheduling event
            }
        }
        
        if (allQueuesEmpty) {
            break;
        }
    }
}

int processIDCounter = 1;

Process* createProcess(char* programFile, int priority) {
    Process* process = (Process*)malloc(sizeof(Process));
    process->pcb.processID = processIDCounter++;
    strcpy(process->pcb.processState, "Ready");
    process->pcb.currentPriority = priority;
    process->pcb.programCounter = 0;
    process->pcb.memoryLowerBound = memoryIndex;
    
    FILE* file = fopen(programFile, "r");
    if (file == NULL) {
        printf("Error: Cannot open program file %s\n", programFile);
        free(process);
        return NULL;
    }
    
    process->instructionCount = 0;
    while (fgets(process->instructions[process->instructionCount], 50, file) != NULL) {
        process->instructionCount++;
    }
    
    fclose(file);
    
    process->pcb.memoryUpperBound = memoryIndex + process->instructionCount;
    memoryIndex += process->instructionCount;
    
    if (memoryIndex >= MEMORY_SIZE) {
        printf("Error: Not enough memory\n");
        free(process);
        return NULL;
    }
    
    return process;
}

int main() {
    for (int i = 0; i < 4; i++) {
        readyQueues[i].count = 0;
    }
    blockedQueue.count = 0;

    Process* process1 = createProcess("program1.txt", 1);
    Process* process2 = createProcess("program2.txt", 2);
    Process* process3 = createProcess("program3.txt", 3);

    if (process1 != NULL) enqueueProcess(&readyQueues[0], process1);
    if (process2 != NULL) enqueueProcess(&readyQueues[1], process2);
    if (process3 != NULL) enqueueProcess(&readyQueues[2], process3);
    
    //schedule();

    return 0;
}
