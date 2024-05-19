#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_LINES 1000
#define MAX_LINE_LENGTH 1000
// #define MAX_PROCESS 1000
// #define MAX_QUEUE_SIZE 100
#define MAX_INSTRUCTIONS 8

// char *instructions[8];

// Define Mutex structure
typedef struct
{
  pthread_mutex_t mutex;
} Mutex;

// Mutex functions
void mutex_init(Mutex *mutex)
{
  pthread_mutex_init(&mutex->mutex, NULL);
}

void mutex_lock(Mutex *mutex)
{
  pthread_mutex_lock(&mutex->mutex);
}

void mutex_unlock(Mutex *mutex)
{
  pthread_mutex_unlock(&mutex->mutex);
}

// Define Interpreter structure
typedef struct
{
  Mutex user_input_mutex;
  Mutex user_output_mutex;
  Mutex file_mutex;
} Interpreter;

typedef enum
{
  NEW,
  READY,
  RUNNING,
  WAITING,
  TERMINATED
} ProcessState;

typedef struct
{
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
} MemoryWord; // name w data strings bsss

MemoryWord Memory[60];

void initialize_memory()
{
  for (int i = 0; i < 60; i++)
  {
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
const char *process_state_to_string(ProcessState state)
{
  switch (state)
  {
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

int allocate_memory(PCB *pcb, char *process_id, int size_needed, int *lower_bound, int *upper_bound)
{
  for (int i = 0; i <= 60 - size_needed; i++)
  {
    int free_block = 1;
    for (int j = 0; j < size_needed; j++)
    {
      if (Memory[i + j].Name != NULL)
      {
        free_block = 0;
        break;
      }
    }
    if (free_block)
    {
      *lower_bound = i;
      *upper_bound = i + size_needed - 1;

      Memory[i].Name = strdup("Pid");
      Memory[i].Value = strdup(process_id);

      char pc_str[20];
      char lower_bound_str[20];
      char upper_bound_str[20];

      snprintf(lower_bound_str, sizeof(lower_bound_str), "%d", *lower_bound);
      snprintf(upper_bound_str, sizeof(upper_bound_str), "%d", *upper_bound);
      snprintf(pc_str, sizeof(pc_str), "%d", pcb->PC);

      Memory[i + 1].Name = strdup("State");
      Memory[i + 1].Value = strdup(process_state_to_string(pcb->State));

      Memory[i + 2].Name = strdup("PC");
      Memory[i + 2].Value = strdup(pc_str);

      Memory[i + 3].Name = strdup("memory_lower_bound");
      Memory[i + 3].Value = strdup(lower_bound_str);

      Memory[i + 4].Name = strdup("memory_upper_bound");
      Memory[i + 4].Value = strdup(upper_bound_str);

      for (int k = 0; k < 3; k++)
      {
        Memory[i + 5 + k].Name = strdup("Variable");
        Memory[i + 5 + k].Value = strdup("");
      }

      for (int j = 8; j < size_needed; j++)
      {
        Memory[i + j].Name = strdup("Instruction");
        Memory[i + j].Value = strdup(process_id);
      }
      return 1;
    }
  }
  return 0;
}
void store_variable(int lower_bound, char *name, char *value)
{
  int temp = lower_bound + 8;
  for (int i = lower_bound + 5; i < temp; i++)
  {
    if (Memory[i].Value == NULL)
    {
      Memory[i].Name = name;
      Memory[i].Value = value;
      return;
    }
  }
  printf("Memory is full, cannot store %s = %s\n", name, value);
}

void deallocate_memory(int lower_bound, int upper_bound)
{
  for (int i = lower_bound; i <= upper_bound; i++)
  {
    free(Memory[i].Name);
    free(Memory[i].Value);
    Memory[i].Name = NULL;
    Memory[i].Value = NULL;
  }
}

void print_memory()
{
  int instCount = 1;
  int varCount = 1;
  for (int i = 0; i < 60; i++)
  {
    if (Memory[i].Name != NULL && Memory[i].Value != NULL)
    {
      if (strcmp(Memory[i].Name, "Instruction") == 0)
      {
        printf("Memory[%d]: %s %d = %s\n", i, Memory[i].Name, instCount, Memory[i].Value);
        instCount++;
      }
      else if (strcmp(Memory[i].Name, "Variable") == 0)
      {
        printf("Memory[%d]: %s %d = %s\n", i, Memory[i].Name, varCount, Memory[i].Value);
        varCount++;
      }
      else
      {
        printf("Memory[%d]: %s = %s\n", i, Memory[i].Name, Memory[i].Value);
      }
    }
    else
    {
      printf("Memory[%d]: Empty\n", i);
    }
  }
}

PCB *create_pcb(int process_id, int lower_bound, int upper_bound)
{
  // allocates memory for a new PCB
  PCB *pcb = (PCB *)malloc(sizeof(PCB));
  pcb->Pid = process_id;
  pcb->State = READY;
  pcb->PC = 0;
  pcb->memory_lower_bound = lower_bound;
  pcb->memory_upper_bound = upper_bound;
  return pcb;
}

void print_from_to(int start, int end)
{
  // azon mesh el mafrod nekhoush el memoryy
  if (start < 0 || start >= 60 || end < 0 || end >= 60)
  {
    fprintf(stderr, "Error: Invalid range\n");
    return;
  }

  for (int num = start; num <= end; num++)
  {
    if (Memory[num].Name != NULL && Memory[num].Value != NULL)
    {
      printf("%s = %s\n", Memory[num].Name, Memory[num].Value);
    }
    else
    {
      printf("Memory[%d]: Empty\n", num);
    }
  }
}

void execute_line(char *line, Interpreter *interpreter, int lower_bound)
{
  if (line == NULL)
  {
    fprintf(stderr, "Error: Null pointer encountered\n");
    return;
  }

  char *token = strtok(line, " ");
  if (token == NULL)
  {
    fprintf(stderr, "Error: No token found in line\n");
    return;
  }

  if (strcmp(token, "assign") == 0)
  {
    char *var_name = strtok(NULL, " ");
    char value[MAX_LINE_LENGTH];
    if (var_name != NULL)
    {
      printf("Enter your input for %s: ", var_name);
      fgets(value, MAX_LINE_LENGTH, stdin);
      if (value[strlen(value) - 1] == '\n')
      {
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
    if (start_str != NULL && end_str != NULL)
    {
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

void execute_program(char **program, int num_lines, Interpreter *interpreter, int lower_bound)
{
  for (int i = 0; i < num_lines; i++)
  {
    execute_line(program[i], interpreter, lower_bound);
  }
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
    // instructions[num_lines] = program[num_lines];
    num_lines++;
  }

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

int main()
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

  PCB *pcb1 = create_pcb(1, lower_bound1, upper_bound1);

  if (allocate_memory(pcb1, "1", size_needed1, &lower_bound1, &upper_bound1))
  {
    execute_program(program1, num_lines_program1, &interpreter, lower_bound1);
    free(pcb1);
  }
  else
  {
    printf("Failed to allocate memory for Program 1\n");
  }

  PCB *pcb2 = create_pcb(2, lower_bound2, upper_bound2);
  if (allocate_memory(pcb2, "2", size_needed2, &lower_bound2, &upper_bound2))
  {
    execute_program(program2, num_lines_program2, &interpreter, lower_bound2);
    free(pcb2);
  }
  else
  {
    printf("Failed to allocate memory for Program 2\n");
  }

  PCB *pcb3 = create_pcb(3, lower_bound3, upper_bound3);
  if (allocate_memory(pcb3, "3", size_needed3, &lower_bound3, &upper_bound3))
  {
    execute_program(program3, num_lines_program3, &interpreter, lower_bound3);
    free(pcb3);
  }
  else
  {
    printf("Failed to allocate memory for Program 3\n");
  }

  print_memory();

  // for (int i = 0; i < 8; i++)
  // {
  //   printf("%s\n", instructions[i]);
  // }

  free_program_lines(program1, num_lines_program1);
  free_program_lines(program2, num_lines_program2);
  free_program_lines(program3, num_lines_program3);

  return 0;
}
