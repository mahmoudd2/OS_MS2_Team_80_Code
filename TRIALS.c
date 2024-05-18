#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h> // Include pthread library for mutex

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
} Interpreter;

// Interpreter functions
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

void writeFile( char *filename,  char *data) {
    // Open file for writing ("w" mode)
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
         printf("Error opening file");
        return;
    }

    // Write data to file
    if (fputs(data, file) == EOF) {
        printf("Error writing to file ");
    }

    // Close the file
    if (fclose(file) != 0) {
        printf("Error closing file ");
    }
}

void print_from_to(int start, int end) {
  for (int num = start; num <= end; num++) {
    printf("%d ", num);
  }
  printf("\n");
}

char *read_file(char *filename, Interpreter *interpreter) {
  // Simulate reading file contents (replace with actual file I/O)
  mutex_lock(&interpreter->user_output_mutex);
  printf("** Simulating file read (replace with actual file access) **\n");
  mutex_unlock(&interpreter->user_output_mutex);
  return "This is content from the file"; // Replace with actual file reading logic
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

  if (strcmp(token, "assign") == 0) {
    // Handle assign command
    char *var_name = strtok(NULL, " ");
    char *value = strtok(NULL, " ");
    if (var_name != NULL && value != NULL) {
      assign(var_name, value, interpreter);
    } else {
      fprintf(stderr, "Error: Insufficient arguments for assign command\n");
    }
  } else if (strcmp(token, "mutexLock") == 0) {
        // Handle mutexLock command
        char *resource = strtok(NULL, " ");
        if (resource != NULL) {
            if (strcmp(resource, "userInput") == 0) {
                mutex_lock(&interpreter->user_input_mutex);
            }
        }
    }
}
void execute_program(char **program, int num_lines, Interpreter *interpreter) {
    for (int i = 0; i < num_lines; i++) {
        // printf("Program %s\n", program[0]);
        execute_line(program[i], interpreter);
    }
}
int main() {
    // File paths for program files
    // char *program1_path = "program1.txt";
    char *program2_path = "program2.txt";
    // char *program3_path = "program3.txt";

    // Read program files
    // char *program1[MAX_LINES];
    char *program2[MAX_LINES];
    // char *program3[MAX_LINES];
    
    // int num_lines_program1 = read_program_file(program1_path, program1);
    int num_lines_program2 = read_program_file(program2_path, program2);
    // int num_lines_program3 = read_program_file(program3_path, program3);

    // Initialize interpreter
    Interpreter interpreter;
    mutex_init(&interpreter.user_input_mutex);
    mutex_init(&interpreter.user_output_mutex);

    // Execute program 1
    // execute_program(program1, num_lines_program1, &interpreter);

    // Execute program 2
    execute_program(program2, num_lines_program2, &interpreter);

    // Execute program 3
    // execute_program(program3, num_lines_program3, &interpreter);

    return 0;
}
