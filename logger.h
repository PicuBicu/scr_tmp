#ifndef INIT_H
#define INIT_H

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

// Priority level can be set to MIN, STANDARD or MAX
typedef enum {
    MIN = 1,
    STANDARD = 2,
    MAX = 3
} log_priority;

// Logging state checks if logging is enabled or disabled
typedef enum {
    DISABLED,
    ENABLED
} logging_state;

// Signal num type
typedef int signum;

// Register function that user need to implement
typedef void (*dump_function_ptr)(FILE *f);

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
                  char *filename);
void destroy_logger();

// User function
void log_message(log_priority d, char *message);
int add_dump_file_function(dump_function_ptr fun);

void change_logger_priority(log_priority priority);
char *get_log_type(log_priority detail);

#endif //INIT_H
