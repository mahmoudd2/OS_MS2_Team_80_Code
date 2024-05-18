#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_LINES 1000
#define MAX_LINE_LENGTH 1000
#define MEMORY_SIZE 60
#define MAX_PROCESSES 10
#define MAX_INSTRUCTIONS 10
#define FILENAME_SIZE 50
#define DATA_SIZE 256

typedef struct {
    int processID;
    char processState[20];
    int currentPriority;
    int programCounter;
    int memoryLowerBound;
    int memoryUpperBound;
} ProcessControlBlock;

typedef struct {
    ProcessControlBlock pcb;
    char instructions[MAX_INSTRUCTIONS][50];
    int instructionCount;
} Process;

typedef struct {
    Process* processes[MAX_PROCESSES];
    int count;
} Queue;

typedef struct {
    pthread_mutex_t mutex;
} Mutex;

typedef struct {
    Mutex userInputMutex;
    Mutex userOutputMutex;
    Mutex fileMutex;
} Mutexes;

Mutexes mutexes;

void mutex_init(Mutex *mutex) {
    pthread_mutex_init(&mutex->mutex, NULL);
}

void mutex_lock(Mutex *mutex) {
    pthread_mutex_lock(&mutex->mutex);
}

void mutex_unlock(Mutex *mutex) {
    pthread_mutex_unlock(&mutex->mutex);
}

void semWait(Mutex *mutex) {
    mutex_lock(mutex);
}

void semSignal(Mutex *mutex) {
    mutex_unlock(mutex);
}

void assign(char *x, char *y) {
    char userInput[100];
    printf("Enter your input: ");
    fgets(userInput, sizeof(userInput), stdin);
    if (userInput[strlen(userInput) - 1] == '\n') {
        userInput[strlen(userInput) - 1] = '\0';
    }
    int number;
    y = userInput;
    if (sscanf(userInput, "%d", &number) == 1) {
        printf("You entered an integer: %d\n", number);
    } else {
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

void readFile(Process* process, char* filename) {
    semWait(&mutexes.fileMutex);
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Cannot open file %s\n", filename);
        semSignal(&mutexes.fileMutex);
        return;
    }
    char line[DATA_SIZE];
    while (fgets(line, sizeof(line), file)) {
        printf("Process %d reading file %s: %s", process->pcb.processID, filename, line);
    }
    fclose(file);
    semSignal(&mutexes.fileMutex);
}

void writeFile(Process* process, char* filename, char* data) {
    if (process == NULL) {
        printf("Error: Process is NULL\n");
        return;
    }
    semWait(&mutexes.fileMutex);
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("Error: Cannot create file %s\n", filename);
        semSignal(&mutexes.fileMutex);
        return;
    }
    fprintf(file, "%s", data);
    fclose(file);
    printf("Process %d wrote to file %s: %s\n", process->pcb.processID, filename, data);
    semSignal(&mutexes.fileMutex);
}


void execute_line(char *line) {
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
        char *value = strtok(NULL, " ");
        if (var_name != NULL && value != NULL) {
            assign(var_name, value);
        } else {
            fprintf(stderr, "Error: Insufficient arguments for assign command\n");
        }
    } else if (strcmp(token, "readFile") == 0) {
        char *filename = strtok(NULL, " ");
        if (filename != NULL) {
            readFile(NULL, filename);
        } else {
            fprintf(stderr, "Error: Insufficient arguments for readFile command\n");
        }
    } else if (strcmp(token, "writeFile") == 0) {
        char *filename = strtok(NULL, " ");
        char *data = strtok(NULL, " ");
        if (filename != NULL && data != NULL) {
            writeFile(NULL, filename, data);
        } else {
            fprintf(stderr, "Error: Insufficient arguments for writeFile command\n");
        }
    }
}

void execute_program(char **program, int num_lines) {
    for (int i = 0; i < num_lines; i++) {
        execute_line(program[i]);
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
    char *program1_path = "program1.txt";
    char *program2_path = "program2.txt";
    char *program3_path = "program3.txt";

    char *program1[MAX_LINES];
    char *program2[MAX_LINES];
    char *program3[MAX_LINES];

    int num_lines_program1 = read_program_file(program1_path, program1);
    int num_lines_program2 = read_program_file(program2_path, program2);
    int num_lines_program3 = read_program_file(program3_path, program3);

    mutex_init(&mutexes.fileMutex);

    execute_program(program1, num_lines_program1);
    execute_program(program2, num_lines_program2);
    execute_program(program3, num_lines_program3);

    free_program_lines(program1, num_lines_program1);
    free_program_lines(program2, num_lines_program2);
    free_program_lines(program3, num_lines_program3);

    return 0;
}
