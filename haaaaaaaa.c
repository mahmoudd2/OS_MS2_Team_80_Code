#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROCESSES 10
#define TIME_QUANTUM 2

typedef enum {
    READY,
    RUNNING,
    TERMINATED
} ProcessState;

typedef struct {
    int pid;
    int burst_time;
    int remaining_time;
   // int arrival_time;
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

void round_robin_scheduler(Queue *ready_queue) {
    while (ready_queue->size > 0) {
        Process *current_process = dequeue(ready_queue);
        if (current_process->remaining_time > TIME_QUANTUM) {
            current_process->state = RUNNING;
            printf("Process %d is running for %d time units.\n", current_process->pid, TIME_QUANTUM);
            current_process->remaining_time -= TIME_QUANTUM;
            current_process->state = READY;
            enqueue(ready_queue, current_process);
        } else {
            current_process->state = RUNNING;
            printf("Process %d is running for %d time units.\n", current_process->pid, current_process->remaining_time);
            current_process->remaining_time = 0;
            current_process->state = TERMINATED;
            printf("Process %d has terminated.\n", current_process->pid);
            free(current_process);
        }
    }
}

int main() {
    Queue ready_queue;
    ready_queue.front = 0;
    ready_queue.rear = -1;
    ready_queue.size = 0;

    int burst_times[] = {7, 7, 9};  // Example burst times for processes
    int num_processes = sizeof(burst_times) / sizeof(burst_times[0]);

    for (int i = 0; i < num_processes; i++) {
        Process *process = (Process *)malloc(sizeof(Process));
        process->pid = i + 1;
        process->burst_time = burst_times[i];
        process->remaining_time = burst_times[i];
        process->state = READY;
        enqueue(&ready_queue, process);
    }

    round_robin_scheduler(&ready_queue);

    return 0;
}
