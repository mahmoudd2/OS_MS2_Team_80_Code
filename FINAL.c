#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h> 

#define MAX_LINES 1000
#define MAX_LINE_LENGTH 1000
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
    int priority;
    int PC;
    int memory_lower_bound;
    int memory_upper_bound;
} PCB;

typedef enum {
    INSTRUCTION,
    VARIABLE,
    PCB_DATA
} MemoryType;

typedef struct {
    char var_name[20];
    char value[20];
} Variable;

typedef struct
{
    MemoryType type;
    union {
        char instruction[100];
        Variable variable;
        PCB pcb;
    } data;
}MemoryWord;


typedef struct
{
    MemoryWord Mem[60];
} Memory;

Memory* create_memory() {
    Memory *mem = (Memory *)malloc(sizeof(Memory));
    for (int i = 0; i < 60; i++) {
        mem->Mem[i].type = INSTRUCTION;
        strcpy(mem->Mem[i].data.instruction, "");
    }
    return mem;
}

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

void execute_line(char *line, Interpreter *interpreter) {
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

  if (strcmp(token, "assign") == 0) 
    {
            // Handle assign command
            char *var_name = strtok(NULL, " ");
            char *value = strtok(NULL, " ");
            if (var_name != NULL && value != NULL) {
            assign(var_name, value, interpreter);
        } 
        else 
        {
            fprintf(stderr, "Error: Insufficient arguments for assign command\n");
        }
    } 
    else if (strcmp(token, "mutexLock") == 0) 
    {
        // Handle mutexLock command
        char *resource = strtok(NULL, " ");
        if (resource != NULL) {
            if (strcmp(resource, "userInput") == 0) {
                mutex_lock(&interpreter->user_input_mutex);
            }
        }
    }
    else if (strcmp(token, "printFromTo") == 0) {
        // Handle printFromTo command
        char *start_str = strtok(NULL, " ");
        char *end_str = strtok(NULL, " ");
        if (start_str != NULL && end_str != NULL) {
            int start = atoi(start_str);
            int end = atoi(end_str);
            print_from_to(start, end);
        } else {
            fprintf(stderr, "Error: Insufficient arguments for printFromTo command\n");
        }
    }
}

void execute_program(char **program, int num_lines, Interpreter *interpreter) {
    for (int i = 0; i < num_lines; i++) {
        execute_line(program[i], interpreter);
    }
}

void assign(char *x, char *y, Interpreter *interpreter) {
    
    char userInput[100];

    // Read input from the user
    printf("Enter your input: ");
    fgets(userInput, sizeof(userInput), stdin);
    
    // Remove trailing newline character
    if (userInput[strlen(userInput) - 1] == '\n') {
        userInput[strlen(userInput) - 1] = '\0';
    }
    // Attempt to convert input to an integer
    int number;
    y = userInput;

    if (sscanf(userInput, "%d", &number) == 1) {
        // Input is an integer
        printf("You entered an integer: %d\n", number);
    } else {
        // Input is a string
        // y = userInput;
        printf("You entered a string: %s\n", y);
    }
    
    printf("Assigned %s = %s\n", x, y);

}

void print_from_to(int start, int end) {
  for (int num = start; num <= end; num++) {
    printf("%d ", num);
  }
  printf("\n");
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

int main() {
    Memory *memory = create_memory();
    int lower_bound, upper_bound;

    // Example process with 10 instructions
    int num_instructions = 10;
    if (allocate_memory(memory, 1, num_instructions, &lower_bound, &upper_bound)) {
        printf("Process 1 allocated memory from %d to %d\n", lower_bound, upper_bound);
        PCB *pcb1 = create_pcb(1, 1, lower_bound, upper_bound);
        store_pcb(memory, pcb1);
    } else {
        printf("Failed to allocate memory for Process 1\n");
    }

    print_memory(memory);

    
    
    // File paths for program files
    char *program1_path = "program1.txt";
    char *program2_path = "program2.txt";
    char *program3_path = "program3.txt";

    // Read program files
    char *program1[MAX_LINES];
    // char *program2[MAX_LINES];
    // char *program3[MAX_LINES];
    
    int num_lines_program1 = read_program_file(program1_path, program1);
    // int num_lines_program2 = read_program_file(program2_path, program2);
    // int num_lines_program3 = read_program_file(program3_path, program3);

    // Initialize interpreter
    Interpreter interpreter;
    mutex_init(&interpreter.user_input_mutex);
    mutex_init(&interpreter.user_output_mutex);

    // Execute program 1
    execute_program(program1, num_lines_program1, &interpreter);

    // Execute program 2
    // execute_program(program2, num_lines_program2, &interpreter);

    // Execute program 3
    // execute_program(program3, num_lines_program3, &interpreter);

    return 0;
}