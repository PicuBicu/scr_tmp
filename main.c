#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include "logger.h"

#define LOGGING_STATE_SIGNAL_NUM SIGRTMIN
#define DUMP_FILE_SIGNAL_NUM SIGRTMIN + 1

void *first_thread_routine(void *arg);
void *second_thread_routine(void *arg);
void create_dump_file_function(FILE *file);

int main() {
    srand(time(NULL));
    printf("PID = %d\nENABLE/DISABLE LOGGING TO FILE SIGNAL = %d\nDUMP FILE CREATION SIGNAL = %d\n",
           getpid(),
           LOGGING_STATE_SIGNAL_NUM,
           DUMP_FILE_SIGNAL_NUM);
    if (create_logger(ENABLED,
                      LOGGING_STATE_SIGNAL_NUM,
                      DUMP_FILE_SIGNAL_NUM,
                      DFL_FILENAME) == 0) {
        set_dump_function(create_dump_file_function);
        pthread_t first_tid, second_tid;
        pthread_create(&first_tid, NULL, first_thread_routine, NULL);
        pthread_create(&second_tid, NULL, second_thread_routine, NULL);
        pthread_join(first_tid, NULL);
        pthread_join(second_tid, NULL);
        destroy_logger();
        return 0;
    }
    return EXIT_FAILURE;
}

void create_dump_file_function(FILE *file) {
    fprintf(file, "Register function, Random %d\n", rand());
}

void *first_thread_routine(void *arg) {
    while (1) {
        log_message(MIN, "Watek pierwszy, pierwsza linia");
        log_message(STANDARD, "Watek pierwszy, druga linia");
        log_message(MAX, "Watek pierwszy, trzecia linia");
        sleep(1);
    }
    return NULL;
}

void *second_thread_routine(void *arg) {
    while (1) {
        log_message(MIN, "Watek drugi, pierwsza linia");
        log_message(STANDARD, "Watek drugi, druga linia");
        log_message(MAX, "Watek drugi, trzecia linia");
//        if (rand() % 12 == 0) {
//            log_message(MAX, "Watek drugi, Zmiana logowania");
//            change_logger_priority(MAX);
//        } else {
//            change_logger_priority(MIN);
//        }
        sleep(1);
    }
    return NULL;
}

