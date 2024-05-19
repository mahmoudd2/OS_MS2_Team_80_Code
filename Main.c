#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h> 

#define MAX_LINES 1000
#define MAX_LINE_LENGTH 1000
#define MAX_INSTRUCTIONS 8
#define MAX_QUEUE_SIZE 100

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
    PCB *processes[MAX_QUEUE_SIZE];
    int front;
    int rear;
    int size;
} Queue;

typedef struct {
    char *Name;
    char *Value;
} MemoryWord;

// Function prototypes
Queue* create_queue();
void enqueue(Queue *q, PCB *process);
PCB* dequeue(Queue *q);
int is_empty(Queue *q);
//void execute_program_with_round_robin(char **program, int num_lines, Interpreter *interpreter, int lower_bound);
void execute_program_with_round_robin(PCB *process, char **program, int num_lines, Interpreter *interpreter);

// Global variables
Queue *ready_queue;

MemoryWord Memory[60];

void initialize_memory() {
    for (int i = 0; i < 60; i++) {
        Memory[i].Name = NULL;
        Memory[i].Value = NULL;
    }
}

int allocate_memory(char* process_id, int size_needed, int *lower_bound, int *upper_bound) {
    for (int i = 0; i <= 60 - size_needed; i++) {
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
            for (int j = 0; j < size_needed; j++) {
                Memory[i + j].Name = strdup("PCB");
                Memory[i + j].Value = strdup(process_id);
            }
            return 1;
        }
    }
    return 0;
}

void deallocate_memory(int lower_bound, int upper_bound) {
    for (int i = lower_bound; i <= upper_bound; i++) {
        free(Memory[i].Name);
        free(Memory[i].Value);
        Memory[i].Name = NULL;
        Memory[i].Value = NULL;
    }
}

void print_memory() {
    for (int i = 0; i < 60; i++) {
        if (Memory[i].Name != NULL && Memory[i].Value != NULL) {
            printf("Memory[%d]: %s = %s\n", i, Memory[i].Name, Memory[i].Value);
        }
        else {
            printf("Memory[%d]: Empty\n", i);
        }
    }
}

PCB* create_pcb(int process_id, int lower_bound, int upper_bound) {
    PCB *pcb = (PCB *)malloc(sizeof(PCB));
    pcb->Pid = process_id;
    pcb->State = READY;
    pcb->PC = 0;
    pcb->memory_lower_bound = lower_bound;
    pcb->memory_upper_bound = upper_bound;
    return pcb;
}

void store_variable(int lower_bound, const char *name, const char *value) {
    for (int i = lower_bound; i < 60; i++) {
        if (Memory[i].Name == NULL) {
            Memory[i].Name = strdup(name);
            Memory[i].Value = strdup(value);
            return;
        }
    }
    printf("Memory is full, cannot store %s = %s\n", name, value);
}

void print_from_to(int start, int end) {
    if (start < 0 || start >= 60 || end < 0 || end >= 60) {
        fprintf(stderr, "Error: Invalid range\n");
        return;
    }

    for (int num = start; num <= end; num++) {
        if (Memory[num].Name != NULL && Memory[num].Value != NULL) {
            printf("%s = %s\n", Memory[num].Name, Memory[num].Value);
        } else {
            printf("Memory[%d]: Empty\n", num);
        }
    }
}

/*void execute_line(char *line, Interpreter *interpreter, int lower_bound) {
    if (line == NULL) {
        fprintf(stderr, "Error: Null pointer encountered\n");
        return;
    }

    char *token = strtok(line, " ");
    if (token == NULL) {
        fprintf(stderr, "Error: No token found in line\n");
        return;
    }

    if (strcmp(token, "assign") == 0) {
        char *var_name = strtok(NULL, " ");
        char value[MAX_LINE_LENGTH];
        if (var_name != NULL) {
            printf("Enter your input for %s: ", var_name);
            fgets(value, MAX_LINE_LENGTH, stdin);
            if (value[strlen(value) - 1] == '\n') {
                value[strlen(value) - 1] = '\0';
            }
            printf("Assigned %s = %s\n", var_name, value);
            store_variable(lower_bound, var_name, value);
        }
        else {
            fprintf(stderr, "Error: Insufficient arguments for assign command\n");
        }
    } 
    else if (strcmp(token, "printFromTo") == 0) {
        char *start_str = strtok(NULL, " ");
        char *end_str = strtok(NULL, " ");
        if (start_str != NULL && end_str != NULL) {
            int start = atoi(start_str);
            int end = atoi(end_str);
            print_from_to(start, end);
        }
        else {
            fprintf(stderr, "Error: Insufficient arguments for printFromTo command\n");
        }
    }
}*/
void execute_line(char *line, Interpreter *interpreter, int lower_bound) {
    if (line == NULL) {
        fprintf(stderr, "Error: Null pointer encountered\n");
        return;
    }

    printf("Executing line: %s\n", line); // Debug statement

    char *token = strtok(line, " ");
    if (token == NULL) {
        fprintf(stderr, "Error: No token found in line\n");
        return;
    }

    printf("Token: %s\n", token); // Debug statement

    if (strcmp(token, "assign") == 0) {
        char *var_name = strtok(NULL, " ");
        char value[MAX_LINE_LENGTH];
        if (var_name != NULL) {
            printf("Enter your input for %s: ", var_name);
            fgets(value, MAX_LINE_LENGTH, stdin);
            if (value[strlen(value) - 1] == '\n') {
                value[strlen(value) - 1] = '\0';
            }
            printf("Assigned %s = %s\n", var_name, value);
            store_variable(lower_bound, var_name, value);
        }
        else {
            fprintf(stderr, "Error: Insufficient arguments for assign command\n");
        }
    } 
    else if (strcmp(token, "printFromTo") == 0) {
        char *start_str = strtok(NULL, " ");
        char *end_str = strtok(NULL, " ");
        if (start_str != NULL && end_str != NULL) {
            int start = atoi(start_str);
            int end = atoi(end_str);
            print_from_to(start, end);
        }
        else {
            fprintf(stderr, "Error: Insufficient arguments for printFromTo command\n");
        }
    }
}


// void execute_program(char **program, int num_lines, Interpreter *interpreter, int lower_bound) {
//     for (int i = 0; i < num_lines; i++) {
//         execute_line(program[i], interpreter, lower_bound);
//     }
// }

int read_program_file(const char *file_path, char **program) {
        FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Unable to open file %s\n", file_path);
        return -1;
    }
    int num_lines = 0;
    char line[MAX_LINE_LENGTH];
    while (fgets(line, MAX_LINE_LENGTH, file) != NULL && num_lines < MAX_LINES) {
        if (line[strlen(line) - 1] == '\n') {
            line[strlen(line) - 1] = '\0';
        }
        program[num_lines] = strdup(line);
        num_lines++;
    }

    fclose(file);
    return num_lines;
}

void free_program_lines(char **program, int num_lines) {
    for (int i = 0; i < num_lines; i++) {
        free(program[i]);
    }
}

void writeFile(char *name, char *data) {
    // Find the filename in memory
    char *filename = NULL;
    for (int i = 0; i < 60; i++) {
        if (Memory[i].Name != NULL && strcmp(Memory[i].Name, "a") == 0) {
            filename = Memory[i].Value;
            break;
        }
    }

    // If filename is not found, exit
    if (filename == NULL) {
        printf("Error: Filename not found in memory.\n");
        return;
    }

    // Open file for writing ("w" mode)
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("Error opening file\n");
        return;
    }

    // Write data to file
    if (fputs(data, file) == EOF) {
        printf("Error writing to file\n");
        fclose(file);
        return;
    }

    // Close the file
    if (fclose(file) != 0) {
        printf("Error closing file\n");
    }
}
char* readFile(char *str) {
    // Find the filename in memory
    char *filename = NULL;
    for (int i = 0; i < 60; i++) {
        if (Memory[i].Name != NULL && strcmp(Memory[i].Name, str) == 0) {
            filename = Memory[i].Value;
            break;
        }
    }

    // If filename is not found, return NULL
    if (filename == NULL) {
        printf("Error: Filename not found in memory.\n");
        return NULL;
    }

    // Open file for reading ("r" mode)
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening file\n");
        return NULL;
    }

    // Determine the size of the file
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the file contents
    char *file_contents = (char *)malloc(file_size + 1);
    if (file_contents == NULL) {
        printf("Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    // Read the file contents into the allocated memory
    fread(file_contents, 1, file_size, file);
    file_contents[file_size] = '\0';

    // Close the file
    if (fclose(file) != 0) {
        printf("Error closing file\n");
    }

    // Return the file contents
    return file_contents;
}




Queue* create_queue() {
    Queue *q = (Queue *)malloc(sizeof(Queue));
    q->front = 0;
    q->rear = -1;
    q->size = 0;
    return q;
}

void enqueue(Queue *q, PCB *process) {
    if (q->size == MAX_QUEUE_SIZE) {
        printf("Queue is full\n");
        return;
    }
    q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
    q->processes[q->rear] = process;
    q->size++;
}

PCB* dequeue(Queue *q) {
    if (is_empty(q)) {
        printf("Queue is empty\n");
        return NULL;
    }
    PCB *process = q->processes[q->front];
    q->front = (q->front + 1) % MAX_QUEUE_SIZE;
    q->size--;
    return process;
}

int is_empty(Queue *q) {
    return (q->size == 0);
}

/*void execute_program_with_round_robin(char **program, int num_lines, Interpreter *interpreter, int lower_bound) {
    int quantum = 2; // Quantum for round-robin scheduling
    int remaining_lines = num_lines;
    int current_line = 0;
    while (remaining_lines > 0) {
        int lines_to_execute = (remaining_lines >= quantum) ? quantum : remaining_lines;
        for (int i = 0; i < lines_to_execute; i++) {
            execute_line(program[current_line], interpreter, lower_bound);
            current_line++;
            remaining_lines--;
        }
        // Simulate context switch or time slice expiration
        printf("Context switch: executing next process or time slice expiration\n");
    }
}
*/
/*void execute_program_with_round_robin(PCB *process, char **program, int num_lines, Interpreter *interpreter) {
    int quantum = 2; // Quantum for round-robin scheduling
    int lines_to_execute = (num_lines - process->PC >= quantum) ? quantum : (num_lines - process->PC);
      printf("Process PID: %d\n", process->Pid);
      printf("Quantum: %d\n", quantum);
      printf("Lines to execute: %d\n", lines_to_execute);

    for (int i = 0; i < lines_to_execute; i++) {
        printf("Executing line %d/%d\n", process->PC + i + 1, num_lines);
        execute_line(program[process->PC], interpreter, process->memory_lower_bound);
        process->PC++;
        if (process->PC >= num_lines) {
            break;
        }
    }
    // Simulate context switch or time slice expiration
    printf("Context switch: executing next process or time slice expiration\n");
}*/
/*void execute_program_with_round_robin(PCB *process, char **program, int num_lines, Interpreter *interpreter) {
    int quantum = 2; // Quantum for round-robin scheduling
    int remaining_lines = num_lines - process->PC; // Calculate remaining lines to execute
    int lines_to_execute = (remaining_lines >= quantum) ? quantum : remaining_lines; // Determine number of lines to execute

    for (int i = 0; i < lines_to_execute; i++) {
        execute_line(program[process->PC], interpreter, process->memory_lower_bound);
        process->PC++; // Increment program counter
        if (process->PC >= num_lines) { // Check if program counter reached end of program
            break;
        }
    }

    if (process->PC < num_lines) { // If program not finished, update process state to READY
        process->State = READY;
        enqueue(ready_queue, process); // Enqueue the process back into the ready queue
    } else { // If program finished, update process state to TERMINATED
        process->State = TERMINATED;
        printf("Process [PID %d] finished execution\n", process->Pid);
        deallocate_memory(process->memory_lower_bound, process->memory_upper_bound);
        free(process);
    }
    // Simulate context switch or time slice expiration
    printf("Context switch: executing next process or time slice expiration\n");
}
*/
void execute_program_with_round_robin(PCB *process, char **program, int num_lines, Interpreter *interpreter) {
    int quantum = 2; // Quantum for round-robin scheduling
    int remaining_lines = num_lines - process->PC; // Calculate remaining lines to execute
    int lines_to_execute = (remaining_lines >= quantum) ? quantum : remaining_lines; // Determine number of lines to execute

    for (int i = 0; i < lines_to_execute; i++) {
        execute_line(program[process->PC], interpreter, process->memory_lower_bound);
        process->PC++; // Increment program counter
        if (process->PC >= num_lines) { // Check if program counter reached end of program
            break;
        }
    }

    if (process->PC < num_lines) { // If program not finished, update process state to READY and re-enqueue
        process->State = READY;
        enqueue(ready_queue, process); // Enqueue the process back into the ready queue
    } else { // If program finished, update process state to TERMINATED and deallocate memory
        process->State = TERMINATED;
        printf("Process [PID %d] finished execution\n", process->Pid);
        deallocate_memory(process->memory_lower_bound, process->memory_upper_bound);
        free(process);
    }
    // Simulate context switch or time slice expiration
    printf("Context switch: executing next process or time slice expiration\n");
}


/*int main()
{
  Interpreter interpreter;
  mutex_init(&interpreter.user_input_mutex);
  mutex_init(&interpreter.user_output_mutex);
  mutex_init(&interpreter.file_mutex);

  initialize_memory();

  char *program1_path = "program1.txt";
  char *program2_path = "program2.txt";
  char *program3_path = "program3.txt";

  char *program1[MAX_LINES];
  char *program2[MAX_LINES];
  char *program3[MAX_LINES];

  int num_lines_program1 = read_program_file(program1_path, program1);
  int num_lines_program2 = read_program_file(program2_path, program2);
  int num_lines_program3 = read_program_file(program3_path, program3);

  int size_needed1 = num_lines_program1 + 3 + 5; // Lines + 3 variables + 5 PCB attributes
  int size_needed2 = num_lines_program2 + 3 + 5;
  int size_needed3 = num_lines_program3 + 3 + 5;

  int lower_bound1, upper_bound1;
  int lower_bound2, upper_bound2;
  int lower_bound3, upper_bound3;

  if (allocate_memory("1", size_needed1, &lower_bound1, &upper_bound1))
  {
    PCB *pcb1 = create_pcb(1, lower_bound1, upper_bound1);
    execute_program_with_round_robin(program1, num_lines_program1, &interpreter, lower_bound1);
    free(pcb1);
  }
  else
  {
    printf("Failed to allocate memory for Program 1\n");
  }

  if (allocate_memory("2", size_needed2, &lower_bound2, &upper_bound2))
  {
    PCB *pcb2 = create_pcb(2, lower_bound2, upper_bound2);
    execute_program_with_round_robin(program2, num_lines_program2, &interpreter, lower_bound2);
    
    //writeFile(Memory[].value, Memory[].Value);

    free(pcb2);
  }
  else
  {
    printf("Failed to allocate memory for Program 2\n");
  }

  if (allocate_memory("3", size_needed3, &lower_bound3, &upper_bound3))
  {
    PCB *pcb3 = create_pcb(3, lower_bound3, upper_bound3);
    execute_program_with_round_robin(program3, num_lines_program3, &interpreter, lower_bound3);
    free(pcb3);
  }
  else
  {
    printf("Failed to allocate memory for Program 3\n");
  }

  print_memory();

  free_program_lines(program1, num_lines_program1);
  free_program_lines(program2, num_lines_program2);
  free_program_lines(program3, num_lines_program3);

  return 0;
}*/
int main() {
    Interpreter interpreter;
    mutex_init(&interpreter.user_input_mutex);
    mutex_init(&interpreter.user_output_mutex);
    mutex_init(&interpreter.file_mutex);

    initialize_memory();

    ready_queue = create_queue();

    char *program1_path = "program1.txt";
    char *program2_path = "program2.txt";
    char *program3_path = "program3.txt";

    char *program1[MAX_LINES];
    char *program2[MAX_LINES];
    char *program3[MAX_LINES];

    int num_lines_program1 = read_program_file(program1_path, program1);
    int num_lines_program2 = read_program_file(program2_path, program2);
    int num_lines_program3 = read_program_file(program3_path, program3);

    int size_needed1 = num_lines_program1 + 3 + 5; // Lines + 3 variables + 5 PCB attributes
    int size_needed2 = num_lines_program2 + 3 + 5;
    int size_needed3 = num_lines_program3 + 3 + 5;

    int lower_bound1, upper_bound1;
    int lower_bound2, upper_bound2;
    int lower_bound3, upper_bound3;

    if (allocate_memory("1", size_needed1, &lower_bound1, &upper_bound1)) {
        PCB *pcb1 = create_pcb(1, lower_bound1, upper_bound1);
        enqueue(ready_queue, pcb1);
    } else {
        printf("Failed to allocate memory for Program 1\n");
    }

    if (allocate_memory("2", size_needed2, &lower_bound2, &upper_bound2)) {
        PCB *pcb2 = create_pcb(2, lower_bound2, upper_bound2);
        enqueue(ready_queue, pcb2);
    } else {
        printf("Failed to allocate memory for Program 2\n");
    }

    if (allocate_memory("3", size_needed3, &lower_bound3, &upper_bound3)) {
        PCB *pcb3 = create_pcb(3, lower_bound3, upper_bound3);
        enqueue(ready_queue, pcb3);
    } else {
        printf("Failed to allocate memory for Program 3\n");
    }

    while (!is_empty(ready_queue)) {
        PCB *current_process = dequeue(ready_queue);
        if (current_process != NULL) {
            current_process->State = RUNNING;
            printf("Currently executing process [PID %d]\n", current_process->Pid);

            // Determine which program to execute
            char **program;
            int num_lines;
            if (current_process->Pid == 1) {
                program = program1;
                num_lines = num_lines_program1;
            } else if (current_process->Pid == 2) {
                program = program2;
                num_lines = num_lines_program2;
            } else {
                program = program3;
                num_lines = num_lines_program3;
            }

            // Execute the program with round-robin scheduling
            execute_program_with_round_robin(current_process, program, num_lines, &interpreter);

            if (current_process->PC < num_lines) {
                current_process->State = READY;
                enqueue(ready_queue, current_process);
            } else {
                current_process->State = TERMINATED;
                printf("Process [PID %d] finished execution\n", current_process->Pid);
                deallocate_memory(current_process->memory_lower_bound, current_process->memory_upper_bound);
                free(current_process);
            }
        }
    }

    print_memory();

    free_program_lines(program1, num_lines_program1);
    free_program_lines(program2, num_lines_program2);
    free_program_lines(program3, num_lines_program3);

    return 0;
}
