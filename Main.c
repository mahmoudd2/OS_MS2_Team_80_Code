#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMORY_SIZE 60
#define MAX_PROCESSES 10
#define MAX_LEVELS 4
#define MAX_LINE_LENGTH 100
#define MAX_PROGRAM_LINES 20

// Define Process States
typedef enum
{
  NEW,
  READY,
  RUNNING,
  WAITING,
  TERMINATED
} ProcessState;

// Define PCB structure
typedef struct
{
  int pid;
  ProcessState state;
  int priority;
  int program_counter;
  int memory_lower_bound;
  int memory_upper_bound;
  char *instructions[MAX_PROGRAM_LINES];
  int instruction_count;
} PCB;

// Memory structure
typedef struct
{
  char *data[MEMORY_SIZE];
} Memory;

// Process Manager structure
typedef struct
{
  PCB pcb_list[MAX_PROCESSES];
  int process_count;
  int next_pid;
  Memory memory;
} ProcessManager;

void initialize_process_manager(ProcessManager *pm)
{
  pm->process_count = 0;
  pm->next_pid = 1; // Start PID from 1

  for (int i = 0; i < MEMORY_SIZE; i++)
  {
    pm->memory.data[i] = NULL;
  }

  for (int i = 0; i < MAX_PROCESSES; i++)
  {
    pm->pcb_list[i].pid = -1; // Indicates unused PCB slot
  }
}

int create_process(ProcessManager *pm, const char *file_path)
{
  if (pm->process_count >= MAX_PROCESSES)
  {
    printf("Error: Maximum number of processes reached.\n");
    return -1;
  }

  // Allocate memory for the process
  int memory_start = -1;
  for (int i = 0; i <= MEMORY_SIZE - MAX_PROGRAM_LINES; i++)
  {
    int free = 1;
    for (int j = 0; j < MAX_PROGRAM_LINES; j++)
    {
      if (pm->memory.data[i + j] != NULL)
      {
        free = 0;
        break;
      }
    }
    if (free)
    {
      memory_start = i;
      break;
    }
  }

  if (memory_start == -1)
  {
    printf("Error: Not enough memory to create process.\n");
    return -1;
  }

  // Initialize PCB
  PCB *pcb = &pm->pcb_list[pm->process_count];
  pcb->pid = pm->next_pid++;
  pcb->state = NEW;
  pcb->priority = 1; // Default priority
  pcb->program_counter = 0;
  pcb->memory_lower_bound = memory_start;
  pcb->memory_upper_bound = memory_start + MAX_PROGRAM_LINES - 1;
  pcb->instruction_count = 0;

  // Read program file
  FILE *file = fopen(file_path, "r");
  if (file == NULL)
  {
    printf("Error: Unable to open file %s\n", file_path);
    return -1;
  }

  char line[MAX_LINE_LENGTH];
  while (fgets(line, sizeof(line), file) != NULL)
  {
    // Remove newline character
    line[strcspn(line, "\n")] = '\0';

    // Store instruction in PCB
    pcb->instructions[pcb->instruction_count] = strdup(line);
    pcb->instruction_count++;

    if (pcb->instruction_count > MAX_PROGRAM_LINES)
    {
      printf("Error: Program exceeds maximum number of lines.\n");
      fclose(file);
      return -1;
    }
  }
  fclose(file);

  // Mark memory as allocated
  for (int i = memory_start; i < memory_start + pcb->instruction_count; i++)
  {
    pm->memory.data[i] = "ALLOCATED"; // Placeholder for actual data
  }

  pm->process_count++;
  return pcb->pid;
}

void execute_instruction(PCB *pcb)
{
  if (pcb->program_counter < pcb->instruction_count)
  {
    printf("Executing instruction for process %d: %s\n", pcb->pid, pcb->instructions[pcb->program_counter]);
    // Here you would parse and execute the instruction
    // For this example, we'll just print the instruction

    pcb->program_counter++;
  }
  else
  {
    pcb->state = TERMINATED;
    printf("Process %d terminated\n", pcb->pid);
  }
}

void schedule_processes(ProcessManager *pm)
{
  // Define the quantum for each level
  int quantum[MAX_LEVELS] = {1, 2, 4, 8};
  // Initialize the queues
  PCB *ready_queues[MAX_LEVELS][MAX_PROCESSES];
  int queue_counts[MAX_LEVELS] = {0};

  // Populate the ready queues based on process states
  for (int i = 0; i < pm->process_count; i++)
  {
    PCB *pcb = &pm->pcb_list[i];
    if (pcb->state == NEW || pcb->state == READY)
    {
      pcb->state = READY;
      ready_queues[pcb->priority][queue_counts[pcb->priority]++] = pcb;
    }
  }

  // Schedule processes from the highest priority queue to the lowest
  for (int level = 0; level < MAX_LEVELS; level++)
  {
    for (int i = 0; i < queue_counts[level]; i++)
    {
      PCB *pcb = ready_queues[level][i];
      if (pcb->state == READY)
      {
        pcb->state = RUNNING;
        printf("Running process %d at priority level %d\n", pcb->pid, pcb->priority);

        // Simulate execution for the time quantum of this level
        for (int j = 0; j < quantum[level]; j++)
        {
          execute_instruction(pcb);
          if (pcb->state == TERMINATED)
          {
            break;
          }
        }

        if (pcb->state != TERMINATED)
        {
          // If process is not terminated, move to appropriate queue
          if (level < MAX_LEVELS - 1)
          {
            pcb->priority++; // Demote to lower priority
          }
          pcb->state = READY; // Set state back to READY
        }
      }
    }
  }
}

int main()
{
  ProcessManager pm;
  initialize_process_manager(&pm);

  // Example programs with their sizes
  create_process(&pm, "program1.txt");
  create_process(&pm, "program2.txt");
  create_process(&pm, "program3.txt");

  // Schedule processes
  for (int cycle = 0; cycle < 10; cycle++)
  {
    printf("Cycle %d\n", cycle);
    schedule_processes(&pm);
  }

  return 0;
}
