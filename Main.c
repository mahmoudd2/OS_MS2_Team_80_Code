#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h> // Include pthread library for mutex

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
} Interpreter;

// Interpreter functions
void assign(char *var_name, char *value, Interpreter *interpreter) {
    // Do something with assigned value
    printf("Assigned %s = %s\n", var_name, value);
}

void print_from_to(int start, int end) {
    for (int num = start; num <= end; num++) {
        printf("%d ", num);
    }
    printf("\n");
}

void execute_line(char *line, Interpreter *interpreter) {
    char *token = strtok(line, " ");
    if (strcmp(token, "assign") == 0) {
        char *var_name = strtok(NULL, " ");
        char *value = strtok(NULL, " ");
        assign(var_name, value, interpreter);
    } else if (strcmp(token, "mutexLock") == 0) { // Change to mutexLock
        char *resource = strtok(NULL, " ");
        if (strcmp(resource, "userInput") == 0) {
            mutex_lock(&interpreter->user_input_mutex);
        } else if (strcmp(resource, "userOutput") == 0) {
            mutex_lock(&interpreter->user_output_mutex);
        }
    } else if (strcmp(token, "mutexUnlock") == 0) { // Change to mutexUnlock
        char *resource = strtok(NULL, " ");
        if (strcmp(resource, "userInput") == 0) {
            mutex_unlock(&interpreter->user_input_mutex);
        } else if (strcmp(resource, "userOutput") == 0) {
            mutex_unlock(&interpreter->user_output_mutex);
        }
    } else if (strcmp(token, "printFromTo") == 0) {
        int start = atoi(strtok(NULL, " "));
        int end = atoi(strtok(NULL, " "));
        print_from_to(start, end);
    }
}

void execute_program(char **program, int num_lines, Interpreter *interpreter) {
    for (int i = 0; i < num_lines; i++) {
        execute_line(program[i], interpreter);
    }
}

int main() {
    // Example program
    char *program[] = {
        "mutexLock userInput", // Change to mutexLock
        "assign a input",
        "assign b input",
        "mutexUnlock userInput", // Change to mutexUnlock
        "mutexLock userOutput", // Change to mutexLock
        "printFromTo 1 5",
        "mutexUnlock userOutput" // Change to mutexUnlock
    };

    // Initialize interpreter
    Interpreter interpreter;
    mutex_init(&interpreter.user_input_mutex);
    mutex_init(&interpreter.user_output_mutex);

    // Execute program
    execute_program(program, sizeof(program) / sizeof(program[0]), &interpreter);

    return 0;
}
