#include <stdio.h>
#include <stdlib.h>

#define TIME_QUANTUM 3

typedef struct {
    int pid;           // Process ID
    int burst_time;    // Burst Time
    int remaining_time;// Remaining Time
} Process;

// Define a node for the process queue
typedef struct Node {
    Process *process;
    struct Node *next;
} Node;

// Define the process queue
typedef struct {
    Node *front;
    Node *rear;
} Queue;

// Function to initialize the queue
void init_queue(Queue *q) {
    q->front = q->rear = NULL;
}

// Function to enqueue a process
void enqueue(Queue *q, Process *p) {
    Node *new_node = (Node *)malloc(sizeof(Node));
    new_node->process = p;
    new_node->next = NULL;

    if (q->rear == NULL) {
        q->front = q->rear = new_node;
    } else {
        q->rear->next = new_node;
        q->rear = new_node;
    }
}

// Function to dequeue a process
Process* dequeue(Queue *q) {
    if (q->front == NULL) {
        printf("Queue is empty!\n");
        return NULL;
    }

    Node *temp = q->front;
    Process *p = temp->process;
    q->front = q->front->next;

    if (q->front == NULL) {
        q->rear = NULL;
    }

    free(temp);
    return p;
}

// Function to check if the queue is empty
int is_empty(Queue *q) {
    return q->front == NULL;
}

// Function to simulate execution of a process
void execute(Process *process, int exec_time) {
    printf("Executing process %d for %d units.\n", process->pid, exec_time);
}

void round_robin(Process processes[], int num_processes, int time_quantum) {
    Queue q;
    init_queue(&q);

    // Enqueue all processes initially
    for (int i = 0; i < num_processes; i++) {
        enqueue(&q, &processes[i]);
    }

    int time = 0;
    while (!is_empty(&q)) {
        Process *p = dequeue(&q);
        if (p->remaining_time > 0) {
            if (p->remaining_time <= time_quantum) {
                // Process can finish within the time quantum
                int exec_time = p->remaining_time;
                execute(p, exec_time);
                time += exec_time;
                p->remaining_time = 0;
                printf("Process %d completed. Total time: %d\n", p->pid, time);
            } else {
                // Process can't finish within the time quantum
                execute(p, time_quantum);
                time += time_quantum;
                p->remaining_time -= time_quantum;
                printf("Process %d executed for %d units. Remaining time: %d. Total time: %d\n", p->pid, time_quantum, p->remaining_time, time);
                // Re-queue the process since it's not finished
                enqueue(&q, p);
            }
        }
    }
}

int main() {
    // Define processes with their burst times
    Process processes[] = {
        {1, 10, 10},
        {2, 5, 5},
        {3, 8, 8}
    };

    int num_processes = sizeof(processes) / sizeof(processes[0]); // Number of processes
    int time_quantum = TIME_QUANTUM; // Time quantum for Round Robin

    printf("Round Robin Scheduling:\n");
    round_robin(processes, num_processes, time_quantum);

    return 0;
}
