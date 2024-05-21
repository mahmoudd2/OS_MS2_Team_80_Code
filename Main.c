#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_LINES 1000
#define MAX_LINE_LENGTH 1000
#define MAX_PROCESSES 3
#define MAX_INSTRUCTIONS 8
#define TIME_QUANTUM 2
#define MEMORY_SIZE 60

int clk = 0;
char *instructions[100];
int PC = 0;
int lower_bound;
int upper_bound;

// Define Mutex structure
typedef struct {
    pthread_mutex_t mutex;
} Mutex;

// Mutex functions
void mutex_init(Mutex *mutex) {
    pthread_mutex_init(&mutex->mutex, NULL);
}

void mutex_lock(Mutex *mutex) {
    pthread_mutex_lock(&mutex->mutex);
}

void mutex_unlock(Mutex *mutex) {
    pthread_mutex_unlock(&mutex->mutex);
}

// Define Interpreter structure
typedef struct {
    Mutex user_input_mutex;
    Mutex user_output_mutex;
    Mutex file_mutex;
} Interpreter;

typedef enum {
    NEW,
    READY,
    RUNNING,
    WAITING,
    TERMINATED
} ProcessState;

typedef struct {
    int Pid;
    ProcessState State;
    int PC;
    int memory_lower_bound;
    int memory_upper_bound;
} PCB;

typedef struct {
    char *Name;
    char *Value;
} MemoryWord; // Name w data strings

MemoryWord Memory[MEMORY_SIZE];

void initialize_memory() {
    for (int i = 0; i < MEMORY_SIZE; i++) {
        Memory[i].Name = NULL;
        Memory[i].Value = NULL;
    }
}

typedef struct {
    int pid;
    int burst_time;
    int executed_INST;
    int arrival_time;
    ProcessState state;
} Process;

typedef struct {
    Process *queue[MAX_PROCESSES];
    int front;
    int rear;
    int size;
} Queue;

void enqueue(Queue *q, Process *process) {
    if (q->size == MAX_PROCESSES) {
        printf("Queue is full, cannot enqueue process.\n");
        return;
    }
    q->rear = (q->rear + 1) % MAX_PROCESSES;
    q->queue[q->rear] = process;
    q->size++;
}

Process *dequeue(Queue *q) {
    if (q->size == 0) {
        printf("Queue is empty, cannot dequeue process.\n");
        return NULL;
    }
    Process *process = q->queue[q->front];
    q->front = (q->front + 1) % MAX_PROCESSES;
    q->size--;
    return process;
}

Queue ready_queue;

void initialize_queue(Queue *q) {
    q->front = 0;
    q->rear = -1;
    q->size = 0;
}

Process *create_process(int processID, int arrival_time, int size) {
    Process *process = (Process *)malloc(sizeof(Process));
    process->pid = processID;
    process->burst_time = size; // Assuming each instruction takes one time unit
    process->executed_INST = 0;
    process->state = READY;
    process->arrival_time = arrival_time;
    return process;
}

const char *process_state_to_string(ProcessState state) {
    switch (state) {
        case NEW:
            return "NEW";
        case READY:
            return "READY";
        case RUNNING:
            return "RUNNING";
        case WAITING:
            return "WAITING";
        case TERMINATED:
            return "TERMINATED";
        default:
            return "UNKNOWN";
    }
}

int PC_calc(int lower_bound, int pc) {
    int temp = pc + lower_bound + 8;
    PC = temp;
    return temp;
}

void store_variable(int lower_bound, char *name, char *value) {
    int var_bound = lower_bound + 8;
    int found = 0;

    for (int i = lower_bound + 5; i < var_bound; i++) {
        if (Memory[i].Name != NULL && strcmp(Memory[i].Name, name) == 0) {
            free(Memory[i].Value);
            Memory[i].Value = strdup(value);
            found = 1;
            break;
        }
    }

    if (!found) {
        for (int i = lower_bound + 5; i < var_bound; i++) {
            if (Memory[i].Value == NULL || strcmp(Memory[i].Value, "") == 0) {
                if (Memory[i].Name != NULL) {
                    free(Memory[i].Name);
                }
                Memory[i].Name = strdup(name);
                if (Memory[i].Value != NULL) {
                    free(Memory[i].Value);
                }
                Memory[i].Value = strdup(value);
                return;
            }
        }
    }

    if (!found) {
        printf("Memory is full, cannot store %s = %s\n", name, value);
    }
}

void deallocate_memory(int lower_bound, int upper_bound) {
    for (int i = lower_bound; i <= upper_bound; i++) {
        free(Memory[i].Name);
        free(Memory[i].Value);
        Memory[i].Name = NULL;
        Memory[i].Value = NULL;
    }
}

int is_process_in_queue(Queue *queue, int pid) {
    for (int i = 0; i < queue->size; i++) {
        int index = (queue->front + i) % MAX_PROCESSES;
        if (queue->queue[index]->pid == pid) {
            return 1; // Process found in the queue
        }
    }
    return 0; // Process not found in the queue
}

void print_memory() {
    int instCount = 1;
    int varCount = 1;
    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (Memory[i].Name != NULL && Memory[i].Value != NULL) {
            if (strcmp(Memory[i].Name, "Instruction") == 0) {
                printf("Memory[%d]: %s %d = %s\n", i, Memory[i].Name, instCount, Memory[i].Value);
                instCount++;
            } else if (strcmp(Memory[i].Name, "Variable") == 0) {
                printf("Memory[%d]: %s %d = %s\n", i, Memory[i].Name, varCount, Memory[i].Value);
                varCount++;
            } else {
                printf("Memory[%d]: %s = %s\n", i, Memory[i].Name, Memory[i].Value);
            }
        } else if (Memory[i].Name != NULL) {
            printf("Memory[%d]: %s\n", i, Memory[i].Name);
        } else {
            printf("Memory[%d]: Empty\n", i);
        }
    }
}

int calculate_memory_bounds(int size_needed, int *lower_bound, int *upper_bound) {
    for (int i = 0; i <= MEMORY_SIZE - size_needed; i++) {
        int free_block = 1;
        for (int j = 0; j < size_needed; j++) {
            if (Memory[i + j].Name != NULL) {
                free_block = 0;
                break;
            }
        }
        if (free_block) {
            *lower_bound = i;
            *upper_bound = i + size_needed - 1;
            return 1;
        }
    }
    return 0;
}

void allocate_memory(PCB *pcb, char *process_id, int size_needed, int lower_bound, int upper_bound, char **program) {
    int i = lower_bound;

    Memory[i].Name = strdup("Pid");
    Memory[i].Value = strdup(process_id);

    char pc_str[20];
    char lower_bound_str[20];
    char upper_bound_str[20];

    snprintf(lower_bound_str, sizeof(lower_bound_str), "%d", lower_bound);
    snprintf(upper_bound_str, sizeof(upper_bound_str), "%d", upper_bound);
    snprintf(pc_str, sizeof(pc_str), "%d", pcb->PC);

    Memory[i + 1].Name = strdup("State");
    Memory[i + 1].Value = strdup(process_state_to_string(pcb->State));

    Memory[i + 2].Name = strdup("PC");
    Memory[i + 2].Value = strdup(pc_str);

    Memory[i + 3].Name = strdup("memory_lower_bound");
    Memory[i + 3].Value = strdup(lower_bound_str);

    Memory[i + 4].Name = strdup("memory_upper_bound");
    Memory[i + 4].Value = strdup(upper_bound_str);

    for (int k = 0; k < 3; k++) {
        Memory[i + 5 + k].Name = NULL;
        Memory[i + 5 + k].Value = NULL;
    }

    int temp = 0;
    for (int r = 8; r < size_needed; r++) {
        Memory[r + i].Name = strdup("Instruction");
        Memory[r + i].Value = strdup(program[temp]);
        temp++;
    }
}

PCB *create_pcb(int process_id, int lower_bound, int upper_bound) {
    PCB *pcb = (PCB *)malloc(sizeof(PCB));
    pcb->Pid = process_id;
    pcb->State = NEW;
    pcb->PC = 0; // Starting at the first instruction
    pcb->memory_lower_bound = lower_bound;
    pcb->memory_upper_bound = upper_bound;
    return pcb;
}

void execute_program(Process *processes[], PCB *pcbs[], int number_of_processes) {
    initialize_queue(&ready_queue);
    initialize_memory();

    for (int i = 0; i < number_of_processes; i++) {
        processes[i]->state = READY;
    }

    // Enqueue the first process at clk = 0
    enqueue(&ready_queue, processes[0]);
    printf("Enqueued Process %d at clock = 0\n", processes[0]->pid);

    while (ready_queue.size > 0) {
        Process *process = dequeue(&ready_queue);
        if (process != NULL) {
            process->state = RUNNING;
            printf("Process %d is running\n", process->pid);

            for (int j = 0; j < TIME_QUANTUM && process->executed_INST < process->burst_time; j++) {
                process->executed_INST++;
                clk++;
                // Enqueue any new processes that arrive at the current clock time
                for (int k = 0; k < number_of_processes; k++) {
                    if (processes[k]->arrival_time == clk && !is_process_in_queue(&ready_queue, processes[k]->pid)) {
                        enqueue(&ready_queue, processes[k]);
                        printf("Enqueued Process %d at clock = %d\n", processes[k]->pid, clk);
                    }
                }
            }

            if (process->executed_INST == process->burst_time) {
                process->state = TERMINATED;
                printf("Process %d terminated\n", process->pid);
                deallocate_memory(pcbs[process->pid]->memory_lower_bound, pcbs[process->pid]->memory_upper_bound);
            } else {
                process->state = READY;
                enqueue(&ready_queue, process);
            }
        }
    }

    printf("All processes are terminated. Clock cycles: %d\n", clk);
    print_memory();
}
int read_program_file(const char *file_path, char **program)
{
  FILE *file = fopen(file_path, "r");
  if (file == NULL)
  {
    fprintf(stderr, "Error: Unable to open file %s\n", file_path);
    return -1;
  }

  int num_lines = 0;
  char line[MAX_LINE_LENGTH];
  while (fgets(line, MAX_LINE_LENGTH, file) != NULL && num_lines < MAX_LINES)
  {
    if (line[strlen(line) - 1] == '\n')
    {
      line[strlen(line) - 1] = '\0';
    }
    program[num_lines] = strdup(line);
    instructions[num_lines] = program[num_lines];
    num_lines++;
  } 
  printf(" program is read\n");
  fclose(file);
  return num_lines;
}

void free_program_lines(char **program, int num_lines)
{
  for (int i = 0; i < num_lines; i++)
  {
    free(program[i]);
  }
}


int main() {
    // Define the program instructions for each process
  char *program1_path = "program1.txt";
  char *program2_path = "program2.txt";
  char *program3_path = "program3.txt";

  char *program1[MAX_LINES];
  char *program2[MAX_LINES];
  char *program3[MAX_LINES];
  int num_lines_program1 = read_program_file(program1_path, program1);
  int size_needed1 = num_lines_program1 + 3 + 5; // Lines + 3 variables + 5 PCB attributes
  
  int num_lines_program2 = read_program_file(program2_path, program2);
  int size_needed2 = num_lines_program2 + 3 + 5;

  int num_lines_program3 = read_program_file(program3_path, program3);
  int size_needed3 = num_lines_program3 + 3 + 5;

    int number_of_processes = 3;
    int burst_times[] = {7, 7, 9};
    int arrival_times[] = {0, 2, 4};

    Process *processes[number_of_processes];
    PCB *pcbs[number_of_processes];

    for (int i = 0; i < number_of_processes; i++) {
        processes[i] = create_process(i, arrival_times[i], burst_times[i]);
    }

    for (int i = 0; i < number_of_processes; i++) {
        int lower_bound, upper_bound;
        int size_needed = burst_times[i] + 8; // instructions + metadata
        if (calculate_memory_bounds(size_needed, &lower_bound, &upper_bound)) {
            pcbs[i] = create_pcb(i, lower_bound, upper_bound);
            allocate_memory(pcbs[i], "PID", size_needed, lower_bound, upper_bound, i == 0 ? program1 : (i == 1 ? program2 : program3));
        } else {
            printf("Failed to allocate memory for process %d\n", i);
            return 1;
        }
    }

    execute_program(processes, pcbs, number_of_processes);

    return 0;
}
