#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdarg.h> // Include pthread library for mutex

#define MAX_LINES 1000
#define MAX_LINE_LENGTH 1000
#define MAX_QUEUE_SIZE 100
#define LEVELS 4
#define MEMORY_SIZE 60
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

typedef struct
{
  int Pid;
  ProcessState State;
  int priority;
  int PC;
  int memory_lower_bound;
  int memory_upper_bound;
} PCB;

typedef enum
{
  INSTRUCTION,
  VARIABLE,
  PCB_DATA
} MemoryType;

typedef struct
{
  char var_name[20];
  char value[20];
} Variable;

typedef struct
{
  MemoryType type;
  union
  {
    char instruction[100];
    Variable variable;
    PCB pcb;
  } data;
} MemoryWord;

typedef struct
{
  MemoryWord Mem[60];
} Memory;

typedef struct
{
  PCB *processes[MAX_QUEUE_SIZE];
  int front;
  int rear;
  int size;
} Queue;
// Interpreter functions
void assign(char *x, char *y, Interpreter *interpreter)
{

  char userInput[100];

  // Read input from the user
  printf("Enter your input: ");
  fgets(userInput, sizeof(userInput), stdin);

  // Remove trailing newline character
  if (userInput[strlen(userInput) - 1] == '\n')
  {
    userInput[strlen(userInput) - 1] = '\0';
  }
  // Attempt to convert input to an integer
  int number;
  y = userInput;

  if (sscanf(userInput, "%d", &number) == 1)
  {
    // Input is an integer
    printf("You entered an integer: %d\n", number);
  }
  else
  {
    // Input is a string
    // y = userInput;
    printf("You entered a string: %s\n", y);
  }

  printf("Assigned %s = %s\n", x, y);
}

void print_from_to(int start, int end)
{
  for (int num = start; num <= end; num++)
  {
    printf("%d ", num);
  }
  printf("\n");
}

char *read_file(char *filename, Interpreter *interpreter)
{
  // Simulate reading file contents (replace with actual file I/O)
  mutex_lock(&interpreter->user_output_mutex);
  printf("** Simulating file read (replace with actual file access) **\n");
  mutex_unlock(&interpreter->user_output_mutex);
  return "This is content from the file"; // Replace with actual file reading logic
}

int read_program_file(const char *file_path, char **program)
{
  FILE *file = fopen(file_path, "r");
  if (file == NULL)
  {
    fprintf(stderr, "Error: Unable to open file %s\n", file_path);
    return -1; // Return -1 to indicate failure
  }

  int num_lines = 0;
  char line[MAX_LINE_LENGTH];

  // Read lines from file and store them in program array
  while (fgets(line, MAX_LINE_LENGTH, file) != NULL && num_lines < MAX_LINES)
  {
    // Remove newline character if present
    if (line[strlen(line) - 1] == '\n')
    {
      line[strlen(line) - 1] = '\0';
    }
    // Allocate memory for line and copy it to program array
    program[num_lines] = strdup(line);
    num_lines++;
  }

  fclose(file);
  return num_lines; // Return number of lines read
}

void free_program_lines(char **program, int num_lines)
{
  // Free memory allocated for program lines
  for (int i = 0; i < num_lines; i++)
  {
    free(program[i]);
  }
}

void execute_line(char *line, Interpreter *interpreter)
{
  if (line == NULL)
  {
    // Handle null pointer error
    fprintf(stderr, "Error: Null pointer encountered\n");
    return;
  }

  char *token = strtok(line, " ");
  if (token == NULL)
  {
    // Handle error if no token found
    fprintf(stderr, "Error: No token found in line\n");
    return;
  }

  if (strcmp(token, "assign") == 0)
  {
    // Handle assign command
    char *var_name = strtok(NULL, " ");
    char *value = strtok(NULL, " ");
    if (var_name != NULL && value != NULL)
    {
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
    if (resource != NULL)
    {
      if (strcmp(resource, "userInput") == 0)
      {
        mutex_lock(&interpreter->user_input_mutex);
      }
    }
  }
}
void execute_program(char **program, int num_lines, Interpreter *interpreter)
{
  for (int i = 0; i < num_lines; i++)
  {
    execute_line(program[i], interpreter);
  }
}

void print(char *file[])
{
  int i = 0;
  while (file[i] != NULL)
  {
    printf("%s\n", file[i]);
    i++;
  }
}

// scheduler
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

Queue ready_queues[LEVELS];
int quantum[LEVELS] = {1, 2, 4, 8}; // Quantum times for each level

void schedule(PCB *processes, int num_processes)
{
  // i think hatetshal 3ashan el ready queue mesh ana el pa set it el mafrod lw el
  for (int i = 0; i < num_processes; i++)
  {
    processes[i].State = READY;
    enqueue(&ready_queues[0], &processes[i]); // Initially put all processes in the highest priority queue
  }

  while (!is_empty(ready_queues))
  {
    // boolean to check empty or not 1=empty
    int all_queues_empty = 1;
    for (int level = 0; level < LEVELS; level++)
    {
      if (!is_empty(&ready_queues[level]))
      {
        all_queues_empty = 0;
        // bigib el process el 3aleha el dor
        PCB *current_process = dequeue(&ready_queues[level]);
        current_process->State = RUNNING;

        // Simulate process execution
        for (int q = 0; q < quantum[level]; q++)
        {
          //+1 3ashan level bad2 men 0
          printf("Executing process %d at priority level %d\n", current_process->Pid, level + 1);
          current_process->PC++; // Simulate executing an instruction
          // Here, you could include checks to handle process completion or blocking

          // For demonstration, let's assume the process needs more execution
          if (q == quantum[level] - 1)
          {
            current_process->State = READY;
            if (level < LEVELS - 1)
            {
              enqueue(&ready_queues[level + 1], current_process); // Move to the next lower priority queue
            }
            else
            {
              enqueue(&ready_queues[level], current_process); // Stay in the lowest priority queue
            }
          }
        }
      }
    }
    if (all_queues_empty)
      break;
  }
}
int main()
{
  PCB processes[100];
  int num_processes = 4;

  // Initialize the processes
  for (int i = 0; i < num_processes; i++)
  {
    processes[i].Pid = i + 1;
    processes[i].State = NEW;
    processes[i].priority = 1; // Start with highest priority
    processes[i].PC = 0;
    processes[i].memory_lower_bound = i * (MEMORY_SIZE / num_processes);
    processes[i].memory_upper_bound = (i + 1) * (MEMORY_SIZE / num_processes) - 1;
  }

  // Initialize the ready queues
  for (int i = 0; i < LEVELS; i++)
  {
    init_queue(&ready_queues[i]);
  }

  // Simulate the scheduling
  schedule(processes, num_processes);

  return 0;
}

// int main()
// {
//   // File paths for program files
//   // char *program1_path = "program1.txt";
//   // char *program2_path = "program2.txt";
//   // char *program3_path = "program3.txt";

//   // // Read program files
//   // char *program1[MAX_LINES];
//   // char *program2[MAX_LINES];
//   // char *program3[MAX_LINES];
//   char *stringList[] = {"hagar", "tasneem", "rubina", "mahmoud", NULL};
//   print(stringList);
//   //   int num_lines_program1 = read_program_file(program1_path, program1);
//   //   int num_lines_program2 = read_program_file(program2_path, program2);
//   //   int num_lines_program3 = read_program_file(program3_path, program3);

//   //   // Initialize interpreter
//   //   Interpreter interpreter;
//   //   mutex_init(&interpreter.user_input_mutex);
//   //   mutex_init(&interpreter.user_output_mutex);

//   //   // Execute program 1
//   //   execute_program(program1, num_lines_program1, &interpreter);

//   //   // Execute program 2
//   //   execute_program(program2, num_lines_program2, &interpreter);

//   //   // Execute program 3
//   //   execute_program(program3, num_lines_program3, &interpreter);

//   //   return 0;
// }
