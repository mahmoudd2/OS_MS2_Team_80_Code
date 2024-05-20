#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_LINES 1000
#define MAX_LINE_LENGTH 1000
// #define MAX_PROCESS 1000
// #define MAX_QUEUE_SIZE 100
#define MAX_INSTRUCTIONS 8

char *instructions[100];

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

int allocate_memory(PCB *pcb, char *process_id, int size_needed, int *lower_bound, int *upper_bound, char **pragram)
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
      int temp = 0;
      for (int r = 8; r < size_needed; r++)
      {
        Memory[r + i].Name = strdup("Instruction");
        Memory[r + i].Value = strdup(pragram[temp]);
        temp++;
      }
      return 1;
    }
  }
  return 0;
}
void store_variable(int lower_bound, char *name, char *value) {
  int var_bound = lower_bound + 8;  // Start from 8 as instructions start from 8.
  int found = 0;

  // First, check if the variable already exists and update its value.
  for (int i = lower_bound + 5; i < var_bound; i++) {
    if (Memory[i].Name != NULL && strcmp(Memory[i].Name, name) == 0) {
      free(Memory[i].Value);  // Free the old value
      Memory[i].Value = strdup(value);
      found = 1;
      break;
    }
  }

  // If the variable was not found, find an empty slot and store the new variable.
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

  // If no empty slot is found, print an error message.
  if (!found) {
    printf("Memory is full, cannot store %s = %s\n", name, value);
  }
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
    else if (Memory[i].Name != NULL)
    {
      printf("Memory[%d]: %s\n", i, Memory[i].Name);
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

// Function to get a value from memory by name
char* get_value_from_memory(const char* name) {
  for (int i = 0; i < 60; i++) {
    if (Memory[i].Name != NULL && strcmp(Memory[i].Name, name) == 0) {
      return Memory[i].Value;
    }
  }
  return NULL;
}

void printFromTo(char *start_num, char *end_num)
{
  int start = atoi(start_num);
  int end = atoi(end_num);
  printf("Printing From %d To %d\n",start,end);

  for (int i = start; i <= end ; i++)
  {
    printf("%d ",i);
  }
  printf("\n\n");
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

void print (char *var_name)
{
  if (var_name != NULL)
  {
    char *value = get_value_from_memory(var_name);
    printf("%s = %s\n",var_name,value);
  }
  else
  {
    printf("el print msh shgal");
  }

}



void execute_line(MemoryWord *Mem, Interpreter *interpreter, int lower_bound, int PC) {
  if (Mem[PC].Value == NULL) {
    fprintf(stderr, "Error: Null pointer encountered at PC %d\n", PC);
    return;
  }

  // Create a writable copy of the line
  char line[MAX_LINE_LENGTH];
  strncpy(line, Mem[PC].Value, MAX_LINE_LENGTH - 1);
  line[MAX_LINE_LENGTH - 1] = '\0';  // Ensure null-termination

  char *token = strtok(line, " ");
  if (token == NULL) {
    fprintf(stderr, "Error: No token found in line\n");
    return;
  }

  if (strcmp(token, "assign") == 0) {
    char *var_name = strtok(NULL, " ");
    char *value_type = strtok(NULL, " ");

    if (var_name != NULL && value_type != NULL && strcmp(value_type, "input") == 0) {
      char value[MAX_LINE_LENGTH];
      printf("Enter your input for %s: ", var_name);
      fgets(value, MAX_LINE_LENGTH, stdin);
      if (value[strlen(value) - 1] == '\n') {
        value[strlen(value) - 1] = '\0';
      }
      printf("Assigned %s = %s\n", var_name, value);
      store_variable(lower_bound, var_name, value);
    } else {
      fprintf(stderr, "Error: Insufficient arguments for assign command\n");
    }
  } else if (strcmp(token, "printFromTo") == 0) {
    char *start_str = strtok(NULL, " ");
    char *end_str = strtok(NULL, " ");
    if (start_str != NULL && end_str != NULL) {
      char *start_value = get_value_from_memory(start_str);
      char *end_value = get_value_from_memory(end_str);
      if (start_value != NULL && end_value != NULL) {
        printFromTo(start_value, end_value);
      } else {
        fprintf(stderr, "Error: One or both variables not found in memory\n");
      }
    } else {
      fprintf(stderr, "Error: Insufficient arguments for printFromTo command\n");
    }
  } else if (strcmp(token, "print") == 0) {
    char *var_name = strtok(NULL, " ");
    if (var_name != NULL) {
      print(var_name);
    }
  } else {
    printf("mfesh haga\n");
  }
}

void execute_program(MemoryWord *Memory, Interpreter *interpreter, int lower_bound, int upper_bound,int pc)
{
  for (int i = pc; i < upper_bound; i++)
  {
    execute_line(Memory, interpreter, lower_bound,pc);
    pc++;
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
    instructions[num_lines] = program[num_lines];
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

  int lower_bound1 = 0, upper_bound1 = 0;
  int lower_bound2, upper_bound2;
  int lower_bound3, upper_bound3;

  PCB *pcb1 = create_pcb(1, lower_bound1, upper_bound1);

  if (allocate_memory(pcb1, "1", size_needed1, &lower_bound1, &upper_bound1,program1))
  {
    execute_program(Memory, &interpreter, lower_bound1,upper_bound1,8);
    free(pcb1);
  }
  else
  {
    printf("Failed to allocate memory for Program 1\n");
  }

  PCB *pcb2 = create_pcb(2, lower_bound2, upper_bound2);
  if (allocate_memory(pcb2,"2", size_needed2, &lower_bound2, &upper_bound2,program2)) {
   
    execute_program(Memory, &interpreter, lower_bound2,upper_bound2,23);
    free(pcb2);
  }
  else
  {
    printf("Failed to allocate memory for Program 2\n");
  }
  
  PCB *pcb3 = create_pcb(3, lower_bound3, upper_bound3);

  if (allocate_memory(pcb3,"3", size_needed3, &lower_bound3, &upper_bound3,program3)) {
    execute_program(Memory, &interpreter, lower_bound3,upper_bound3,38);
    free(pcb3);
  } 
  else
  {
    printf("Failed to allocate memory for Program 3\n");
  }
  // Memory[0].Name = "PID:";
  // Memory[0].Value = "1";
  // Memory[1].Name = "STATE:";
  // Memory[1].Value = "READY";
  // Memory[2].Name = "PC:";
  // Memory[2].Value = "8";
  // Memory[3].Name = "Lower_bound:";
  // Memory[3].Value = "0";  
  // Memory[4].Name = "Upper_Bound:";
  // Memory[4].Value = "14";
  // Memory[5].Name = "Var1:";
  // Memory[5].Value = NULL;
  // Memory[6].Name = "Var2:";
  // Memory[6].Value = NULL;
  // Memory[7].Name = "Var3:";
  // Memory[7].Value = NULL;
  // Memory[8].Name = "INST1:";
  // Memory[8].Value = "semWait userInput";
  // Memory[9].Name = "INST2:";
  // Memory[9].Value = "assign a input";
  // Memory[10].Name = "INST3:";
  // Memory[10].Value = "assign b input";
  // Memory[11].Name = "INST4:";
  // Memory[11].Value = "semSignal userInput";
  // Memory[12].Name = "INST5:";
  // Memory[12].Value = "semWait userOutput";
  // Memory[13].Name = "INST6:";
  // Memory[13].Value = "printFromTo a b";
  // Memory[14].Name = "INST7:";
  // Memory[14].Value = "semSignal userOutput";
  // print_memory();
  
  execute_program(Memory,&interpreter,lower_bound1,14,8);
  print_memory();
  return 0;
  // store_variable(0,"a","HAMADA");
  // store_variable(0,"b","10");
  // writeFile(Memory[5].Value,Memory[6].Value);
  // printFromTo(Memory[5].Value,Memory[6].Value);
  // print(Memory[5].Name);
  // char *temppp = get_value_from_memory("a",0,14);
  // printf("temp: %s\n",temppp);

  // free_program_lines(program1, num_lines_program1);
  // free_program_lines(program2, num_lines_program2);
  // free_program_lines(program3, num_lines_program3);

  // printf("Instructions Array:\n");
  // for (int i = 0; i < 9; i++) {
  //   if (instructions[i] != NULL) {
  //     printf("Instruction[%d]: %s\n", i, instructions[i]);
  //   } else {
  //     printf("Instruction[%d]: Empty\n", i);
  //   }
  // }
}


//print w printfromto w assign w writefile shagalen fol 
// na2es at2kd mn read file w a3ml semwait w semsignal