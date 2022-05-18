#ifndef PROJECT_1_INIT_H
#define PROJECT_1_INIT_H

#include <signal.h>
#include <semaphore.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdarg.h>
#include "tools.h"

// Atomic variables that control signals
extern atomic_int is_log_enabled_flag, dump_log_flag;

// Priority level can be set two three values
typedef enum {
    MIN,
    STANDARD,
    MAX
} log_priority;

// Logging state checks if logging is enabled or disabled
typedef enum {
    DISABLED,
    ENABLED
} logging_state;

// Signal num
typedef int signum;
extern signum logging_state_signal, dump_file_signal;
// Register function that user need to implement
typedef void (*dump_function_ptr)(FILE *f);

//Log priority
extern log_priority current_log_priority;
// Check library initialization state
extern bool is_initialized;

// TODO: need description
extern FILE *file;
extern unsigned int dump_functions_array_index;
extern size_t dump_functions_array_size;
// TODO: end

// Threads in which context signals are handled
extern pthread_t mainTid, logTid;

// Mutexes for controlling critical sections
extern pthread_mutex_t logging_to_file_mutex, register_function_mutex, change_priority_mutex;

// Controlling signals via atomic variables
void handle_logging_state_change_signal(int signo, siginfo_t *info, void *other);
void handle_dump_file_creation_signal(int signo, siginfo_t *info, void *other);

// Handling signals in threads contexts
void *logging_enability_routine(void *arg);
void *dump_file_creation_routine(void *arg);

// Handling logger lifecycle
int create_logger(logging_state state,
                  signum logging_state_signal_num,
                  signum dump_file_signal_num,
                  char *filename,
                  size_t dump_file_array_init_size);
void destroy_logger();

// User function
void log_message(log_priority d, char *message);
int add_dump_file_function(dump_function_ptr fun);

void change_logger_priority(log_priority priority);
char *get_log_type(log_priority detail);

#endif //PROJECT_1_INIT_H
