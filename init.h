//
// Created by sebastian on 19.03.2022.
//

#ifndef PROJECT_1_INIT_H
#define PROJECT_1_INIT_H

#include <signal.h>
#include <semaphore.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdarg.h>
#define DATE_FORMAT "%m-%d-%y_%H-%M-%S"
#define FILENAME "Logs"

enum log_detail {MIN,STANDARD,MAX};
enum state_t {DISABLED,ENABLED};
typedef enum log_detail log_detail;
typedef enum state_t state_t;
typedef void (*FUN)(FILE* f);
extern sem_t mainSem,logSem;
extern log_detail currentLog;
extern bool is_initialized;
extern int dump,level;
extern FILE* file;
extern FUN* functions;
extern int funSize,funCapacity, logNum;
extern pthread_t mainTid,logTid;
extern pthread_mutex_t logging_to_file_mutex,register_function_mutex;

void handle_state_signal(int signo, siginfo_t *info, void* other);
void handle_enabling_logs_signal(int signo, siginfo_t *info, void* other);
void* executeThread(void *arg);
void* executeLogs(void* arg);
int init_logger(state_t state, int state_signal, int enabling_logs_signal, char* filename);
int registerFunction(FUN fun);
FILE* create_file();
void writeToFile(log_detail d, char* data);
void destroy_logger();
static char * convertDetail(log_detail detail);


#endif //PROJECT_1_INIT_H
