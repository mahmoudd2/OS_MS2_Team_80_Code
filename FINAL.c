#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h> 

#define MAX_LINES 1000
#define MAX_LINE_LENGTH 1000
#define MAX_PROCESS 1000
#define MAX_QUEUE_SIZE 100
#define MAX_INSTRUCTIONS 8

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
  Mutex file_mutex
} Interpreter;

typedef enum
{
  NEW,
  READY,
  RUNNING,
  WAITING,
  TERMINATED
} ProcessState;

typedef struct {
    int Pid;
    ProcessState State;
    // int priority;
    int PC;
    int memory_lower_bound;
    int memory_upper_bound;
} PCB;

typedef struct
{
    char *Name;
    char *Value;
}MemoryWord; // name w data strings bsss

MemoryWord Mem[60];
// ARRAY

// typedef struct {
//     PCB pcb;
//     char instructions[MAX_INSTRUCTIONS][50];
//     int instructionCount;
// } Process;



// cell by cell el pcb


// Process processes[MAX_PROCESS];

// typedef struct
// {
//   PCB *processes[MAX_QUEUE_SIZE];
//   int front;
//   int rear;
//   int size;
// } Queue;


// typedef enum {
//     INSTRUCTION,
//     VARIABLE,
//     PCB_DATA
// } MemoryType;

// typedef struct {
//     char var_name[20];
//     char value[20];
// } Variable;

int allocate_memory(Memory *mem, int process_id, int num_instructions, int *lower_bound, int *upper_bound) {
    int size_needed = num_instructions + 3 + 1; // Instructions + Variables + PCB
    for (int i = 0; i <= 60 - size_needed; i++) {
        int free_block = 1;
        for (int j = 0; j < size_needed; j++) {
            if (mem->Mem[i + j].type != INSTRUCTION || strcmp(mem->Mem[i + j].data.instruction, "") != 0) {
                free_block = 0;
                break;
            }
        }
        if (free_block) {
            *lower_bound = i;
            *upper_bound = i + size_needed - 1;
            for (int j = 0; j < size_needed; j++) {
                mem->Mem[i + j].type = PCB_DATA;
                mem->Mem[i + j].data.pcb.Pid = process_id;
            }
            return 1;
        }
    }
    return 0;
}

void deallocate_memory(Memory *mem, int process_id) {
    for (int i = 0; i < 60; i++) {
        if (mem->Mem[i].type == PCB_DATA && mem->Mem[i].data.pcb.Pid == process_id) {
            mem->Mem[i].type = INSTRUCTION;
            strcpy(mem->Mem[i].data.instruction, "");
        }
    }
}

void print_memory(Memory *mem) {
    for (int i = 0; i < 60; i++) {
        if (mem->Mem[i].type == INSTRUCTION) {
            printf("Memory[%d]: Instruction - %s\n", i, mem->Mem[i].data.instruction);
        } else if (mem->Mem[i].type == VARIABLE) {
            printf("Memory[%d]: Variable - %s = %s\n", i, mem->Mem[i].data.variable.var_name, mem->Mem[i].data.variable.value);
        } else if (mem->Mem[i].type == PCB_DATA) {
            printf("Memory[%d]: PCB - Process ID: %d\n", i, mem->Mem[i].data.pcb.Pid);
        }
    }
}

PCB* create_pcb(int process_id, int priority, int lower_bound, int upper_bound) {
    PCB *pcb = (PCB *)malloc(sizeof(PCB));
    pcb->Pid = process_id;
    pcb->State = READY;
    pcb->priority = priority;
    pcb->PC = 0;
    pcb->memory_lower_bound = lower_bound;
    pcb->memory_upper_bound = upper_bound;
    return pcb;
}

void store_pcb(Memory *memory, PCB *pcb) {
    for (int i = pcb->memory_lower_bound; i <= pcb->memory_upper_bound; i++) {
        memory->Mem[i].type = PCB_DATA;
        memory->Mem[i].data.pcb = *pcb;
    }
}

// queues:
void init_queue(Queue *q)
{
  q->front = 0;
  q->rear = -1;
  q->size = 0;
}

int is_empty(Queue *q)
{
  return q->size == 0;
}

int is_full(Queue *q)
{
  return q->size == MAX_QUEUE_SIZE;
}

void enqueue(Queue *q, PCB *pcb)
{
  if (!is_full(q))
  {
    q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
    q->processes[q->rear] = pcb;
    q->size++;
  }
}

PCB *dequeue(Queue *q)
{
  if (!is_empty(q))
  {
    PCB *pcb = q->processes[q->front];
    q->front = (q->front + 1) % MAX_QUEUE_SIZE;
    q->size--;
    return pcb;
  }
  return NULL;
}



int read_program_file(const char *file_path, char **program) {
  FILE *file = fopen(file_path, "r");
  if (file == NULL) {
    fprintf(stderr, "Error: Unable to open file %s\n", file_path);
    return -1; // Return -1 to indicate failure
  }

  int num_lines = 0;
  char line[MAX_LINE_LENGTH];

  // Read lines from file and store them in program array
  while (fgets(line, MAX_LINE_LENGTH, file) != NULL && num_lines < MAX_LINES) {
    // Remove newline character if present
    if (line[strlen(line) - 1] == '\n') {
      line[strlen(line) - 1] = '\0';
    }
    // Allocate memory for line and copy it to program array
    program[num_lines] = strdup(line);
    num_lines++;
  }

  fclose(file);
//   printf("%d",num_lines);
  return num_lines; // Return number of lines read
}

void free_program_lines(char **program, int num_lines) {
  // Free memory allocated for program lines
  for (int i = 0; i < num_lines; i++) {
    free(program[i]);
  }
}

void execute_line(char *line, Interpreter *interpreter, Memory *memory) {
    if (line == NULL) {
        // Handle null pointer error
        fprintf(stderr, "Error: Null pointer encountered\n");
        return;
    }

    char *token = strtok(line, " ");
    if (token == NULL) {
        // Handle error if no token found
        fprintf(stderr, "Error: No token found in line\n");
        return;
    }

    if (strcmp(token, "assign") == 0) {
        // Handle assign command
        char *var_name = strtok(NULL, " ");
        char value[MAX_LINE_LENGTH];
        if (var_name != NULL) {
            assign(var_name, value, interpreter, memory);
        } else {
            fprintf(stderr, "Error: Insufficient arguments for assign command\n");
        }
    } else if (strcmp(token, "mutexLock") == 0) {
        // Handle mutexLock command
        char *resource = strtok(NULL, " ");
        if (resource != NULL) {
            if (strcmp(resource, "userInput") == 0) {
                mutex_lock(&interpreter->user_input_mutex);
            }
        }
    } else if (strcmp(token, "printFromTo") == 0) {
        // Handle printFromTo command
        char *start_str = strtok(NULL, " ");
        char *end_str = strtok(NULL, " ");
        if (start_str != NULL && end_str != NULL) {
            int start = atoi(start_str);
            int end = atoi(end_str);
            print_from_to(start, end, memory);
        } else {
            fprintf(stderr, "Error: Insufficient arguments for printFromTo command\n");
        }
    }
}


void execute_program(char **program, int num_lines, Interpreter *interpreter,Memory *memory) {
    for (int i = 0; i < num_lines; i++) {
        execute_line(program[i], interpreter,memory);
    }
}

void assign(char *x, char *y, Interpreter *interpreter, Memory *memory) {
    // Read input from the user
    printf("Enter your input for %s: ", x);
    fgets(y, MAX_LINE_LENGTH, stdin);

    // Remove trailing newline character
    if (y[strlen(y) - 1] == '\n') {
        y[strlen(y) - 1] = '\0';
    }

    printf("Assigned %s = %s\n", x, y);

    // Store the assigned value in memory
    for (int i = 0; i < 60; i++) {
        if (memory->Mem[i].type == VARIABLE && strcmp(memory->Mem[i].data.variable.var_name, x) == 0) {
            // Variable already exists in memory, update its value
            strcpy(memory->Mem[i].data.variable.value, y);
            return; // Exit the function
        }
    }
    // If the variable doesn't exist in memory, find the first available slot and store it
    for (int i = 0; i < 60; i++) {
        if (memory->Mem[i].type == INSTRUCTION) {
            memory->Mem[i].type = VARIABLE;
            strcpy(memory->Mem[i].data.variable.var_name, x);
            strcpy(memory->Mem[i].data.variable.value, y);
            return; // Exit the function
        }
    }
    printf("Memory is full, cannot store %s = %s\n", x, y);
}


void print_from_to(int start, int end, Memory *memory) {
    if (start < 0 || start >= 60 || end < 0 || end >= 60) {
        fprintf(stderr, "Error: Invalid range\n");
        return;
    }

    for (int num = start; num <= end; num++) {
        if (memory->Mem[num].type == VARIABLE) {
            printf("%s = %s\n\n", memory->Mem[num].data.variable.var_name, memory->Mem[num].data.variable.value);
        } else if (memory->Mem[num].type == INSTRUCTION) {
            printf("Instruction at memory index %d: %s\n", num, memory->Mem[num].data.instruction);
        } else if (memory->Mem[num].type == PCB_DATA) {
            printf("PCB data at memory index %d: Process ID - %d\n", num, memory->Mem[num].data.pcb.Pid);
        } else {
            printf("Unknown type at memory index %d\n", num);
        }
    }
}

void writeFile(Interpreter *interpreter, const char *filename, const char *data) {
    mutex_lock(&interpreter->file_mutex);
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        fprintf(stderr, "Error: Unable to create file %s\n", filename);
        mutex_unlock(&interpreter->file_mutex);
        return;
    }
    fprintf(file, "%s", data);
    fclose(file);
    printf("Data written to file %s: %s\n", filename, data);
    mutex_unlock(&interpreter->file_mutex);
}

void readFile(Interpreter *interpreter, const char *filename) {
    mutex_lock(&interpreter->file_mutex);
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Unable to open file %s\n", filename);
        mutex_unlock(&interpreter->file_mutex);
        return;
    }
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file) != NULL) {
        printf("%s", line);
    }
    fclose(file);
    printf("Data read from file %s\n", filename);
    mutex_unlock(&interpreter->file_mutex);
}

Process* create_process(int process_id, int priority, char **instructions, int instruction_count, Memory *memory) {
    int lower_bound, upper_bound;
    int size_needed = instruction_count + 3 + 1; // Instructions + Variables + PCB

    if (!allocate_memory(memory, process_id, instruction_count, &lower_bound, &upper_bound)) {
        printf("Failed to allocate memory for Process %d\n", process_id);
        return NULL;
    }

    Process *process = (Process *)malloc(sizeof(Process));
    process->pcb.Pid = process_id;
    process->pcb.State = NEW;
    process->pcb.priority = priority;
    process->pcb.PC = 0;
    process->pcb.memory_lower_bound = lower_bound;
    process->pcb.memory_upper_bound = upper_bound;

    for (int i = 0; i < instruction_count; i++) {
        strcpy(process->instructions[i], instructions[i]);
    }
    process->instructionCount = instruction_count;

    store_pcb(memory, &process->pcb);

    return process;
}

void allocate_and_create_process(Memory *memory, int process_id, int priority, char **instructions, int instruction_count) {
    int lower_bound, upper_bound;
    
    if (allocate_memory(memory, process_id, instruction_count, &lower_bound, &upper_bound)) {
        printf("Process %d allocated memory from %d to %d\n", process_id, lower_bound, upper_bound);
        PCB *pcb = create_pcb(process_id, priority, lower_bound, upper_bound);
        store_pcb(memory, pcb);
        
        Process *process = create_process(process_id, priority, instructions, instruction_count, memory);
        if (process != NULL) {
            processes[process_id] = *process;
        } else {
            printf("Failed to create Process %d\n", process_id);
        }
    } else {
        printf("Failed to allocate memory for Process %d\n", process_id);
    }
}


int main() {

    // Initialize interpreter
    Interpreter interpreter;
    mutex_init(&interpreter.user_input_mutex);
    mutex_init(&interpreter.user_output_mutex);

    // File paths for program files
    char *program1_path = "program1.txt";
    char *program2_path = "program2.txt";
    char *program3_path = "program3.txt";

    // Read program files
    char *program1[MAX_LINES];
    char *program2[MAX_LINES];
    char *program3[MAX_LINES];
    
    int num_lines_program1 = read_program_file(program1_path, program1);
    int num_lines_program2 = read_program_file(program2_path, program2);
    int num_lines_program3 = read_program_file(program3_path, program3);

    Memory *memory = create_memory();

    // Example process with the read instructions
    // Execute program 1
    execute_program(program1, num_lines_program1, &interpreter, memory);
    allocate_and_create_process(memory, 1, 1, program1, num_lines_program1);

    // Execute program 2
    execute_program(program2, num_lines_program2, &interpreter, memory);
    allocate_and_create_process(memory, 2, 1, program2, num_lines_program2);

    // Execute program 3
    execute_program(program3, num_lines_program3, &interpreter, memory);
    allocate_and_create_process(memory, 3, 1, program3, num_lines_program3);

    print_memory(memory);

    

    free_program_lines(program1, num_lines_program1);
    // free_program_lines(program2, num_lines_program2);
    // free_program_lines(program3, num_lines_program3);

    return 0;
}
