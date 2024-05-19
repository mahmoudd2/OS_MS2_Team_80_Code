#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h> 

#define MAX_LINES 1000
#define MAX_LINE_LENGTH 1000
// #define MAX_PROCESS 1000
// #define MAX_QUEUE_SIZE 100
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

MemoryWord Memory[60];

void initialize_memory() {
  for (int i = 0; i < 60; i++) {
      Memory[i].Name = NULL;
      Memory[i].Value = NULL;
  }
}

// typedef struct
// {
//   PCB *processes[MAX_QUEUE_SIZE];
//   int front;
//   int rear;
//   int size;
// } Queue;

int allocate_memory(int process_id, int size_needed, int *lower_bound, int *upper_bound) {
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
        Memory[i + j].Value = strdup("Process");
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
    else
    {
      printf("Memory[%d]: Empty\n", i);
    }
  }
}

void execute_line(char *line, Interpreter *interpreter, int lower_bound) {
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
    else
    {
      fprintf(stderr, "Error: Insufficient arguments for assign command\n");
    }
  } 
  else if (strcmp(token, "printFromTo") == 0)
  {
    char *start_str = strtok(NULL, " ");
    char *end_str = strtok(NULL, " ");
    if (start_str != NULL && end_str != NULL) {
      int start = atoi(start_str);
      int end = atoi(end_str);
      print_from_to(start, end);
    }
    else
    {
      fprintf(stderr, "Error: Insufficient arguments for printFromTo command\n");
    }
  }
}

void execute_program(char **program, int num_lines, Interpreter *interpreter, int lower_bound) {
  for (int i = 0; i < num_lines; i++) {
    execute_line(program[i], interpreter, lower_bound);
  }
}

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

int main() {
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

  if (allocate_memory(1, size_needed1, &lower_bound1, &upper_bound1)) {
    PCB *pcb1 = create_pcb(1, lower_bound1, upper_bound1);
    execute_program(program1, num_lines_program1, &interpreter, lower_bound1);
    free(pcb1);
  } 
  else 
  {
    printf("Failed to allocate memory for Program 1\n");
  }

  if (allocate_memory(2, size_needed2, &lower_bound2, &upper_bound2)) {
    PCB *pcb2 = create_pcb(2, lower_bound2, upper_bound2);
    execute_program(program2, num_lines_program2, &interpreter, lower_bound2);
    free(pcb2);
  }
  else
  {
    printf("Failed to allocate memory for Program 2\n");
  }

  if (allocate_memory(3, size_needed3, &lower_bound3, &upper_bound3)) {
    PCB *pcb3 = create_pcb(3, lower_bound3, upper_bound3);
    execute_program(program3, num_lines_program3, &interpreter, lower_bound3);
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
}
