#include "init.h"

atomic_int is_log_enabled_flag, dump_log_flag;
state_t logging_state;
bool is_initialized = false;
int level, dump;
FILE *file;
int funSize = 0, funCapacity = 5;
FUN *fun;
pthread_t mainTid, logTid;
pthread_mutex_t logging_to_file_mutex, register_function_mutex;

int create_logger(state_t state, int state_signal, int enabling_logs_signal, char *filename) {
    if (is_initialized) {
        return EXIT_FAILURE;
    }
    atomic_store(&is_log_enabled_flag, 0);
    atomic_store(&dump_log_flag, 0);

    logging_state = state;
    level = state_signal;
    dump = enabling_logs_signal;

    file = fopen(filename, "a+");
    if (file == NULL) {
        return EXIT_FAILURE;
    }

    sigset_t set;
    struct sigaction action;
    sigfillset(&set);
    action.sa_sigaction = handle_state_signal; // parametr
    action.sa_flags = SA_SIGINFO;
    action.sa_mask = set;

    if (sigaction(state_signal, &action, NULL) != 0) {
        fclose(file);
        return EXIT_FAILURE;
    }

    sigfillset(&set);
    action.sa_sigaction = handle_enabling_logs_signal;
    action.sa_flags = SA_SIGINFO; // todo: moze mozna wywalic i zmienic ilosc parametrów na 1 ?
    action.sa_mask = set;

    if (sigaction(enabling_logs_signal, &action, NULL) != 0) {
        fclose(file);
        return EXIT_FAILURE;
    }

    if (pthread_mutex_init(&logging_to_file_mutex, NULL) != 0) {
        fclose(file);
        return EXIT_FAILURE;
    }

    if (pthread_mutex_init(&register_function_mutex, NULL) != 0) {
        fclose(file);
        pthread_mutex_destroy(&register_function_mutex);
        return EXIT_FAILURE;
    }

    fun = (FUN *) calloc(sizeof(FUN), funCapacity);
    if (fun == NULL) {
        fclose(file);
        pthread_mutex_destroy(&register_function_mutex);
        pthread_mutex_destroy(&logging_to_file_mutex);
        return EXIT_FAILURE;
    }

    if (pthread_create(&logTid, NULL, executeLogs, NULL) != 0) {
        fclose(file);
        free(fun);
        pthread_mutex_destroy(&register_function_mutex);
        pthread_mutex_destroy(&logging_to_file_mutex);
        return EXIT_FAILURE;
    }

    if (pthread_create(&mainTid, NULL, executeThread, NULL) != 0) {
        fclose(file);
        free(fun);
        pthread_cancel(logTid);
        pthread_mutex_destroy(&register_function_mutex);
        pthread_mutex_destroy(&logging_to_file_mutex);
        return EXIT_FAILURE;
    }

//    todo moze wywalić?
    if (pthread_detach(mainTid) != 0) {
        free(fun);
        fclose(file);
        pthread_cancel(mainTid);
        pthread_cancel(logTid);
        pthread_mutex_destroy(&register_function_mutex);
        pthread_mutex_destroy(&logging_to_file_mutex);
        return EXIT_FAILURE;
    }

    if (pthread_detach(logTid) != 0) {
        fclose(file);
        free(fun);
        pthread_cancel(mainTid);
        pthread_cancel(logTid);
        pthread_mutex_destroy(&register_function_mutex);
        pthread_mutex_destroy(&logging_to_file_mutex);
        return EXIT_FAILURE;
    }

    is_initialized = true;
    return EXIT_SUCCESS;
}

int registerFunction(FUN f) {
    if (is_initialized == true) {
        pthread_mutex_lock(&register_function_mutex);
        if (funSize >= funCapacity) {
            FUN *functions = (FUN *) realloc(fun, sizeof(FUN) * funCapacity * 2);
            if (functions == NULL) {
                return EXIT_FAILURE;
            }
            fun = functions;
            funCapacity = funCapacity * 2;
        }
        fun[funSize++] = f;
        pthread_mutex_unlock(&register_function_mutex);
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}

char *get_current_time() {
    char timeBuf[30];
    char *current_name = malloc(sizeof(char) * 50);
    if (current_name == NULL) {
        return NULL;
    }
    time_t raw_time = time(0);
    struct tm *local_time = localtime(&raw_time);
    strftime(timeBuf, 30, DATE_FORMAT, local_time);
    sprintf(current_name, "%s", timeBuf);
    return current_name;
}

FILE *create_file() {
    char *full_date = get_current_time();
    if (full_date == NULL) {
        return NULL;
    }
    return fopen(strcat(full_date, ".txt"), "w+");
}

void destroy_logger() {
    if (file != NULL) {
        fclose(file);
    }
    if (fun != NULL) {
        free(fun);
    }
    atomic_store(&dump_log_flag, 0);
    atomic_store(&is_log_enabled_flag, 0);
    pthread_cancel(mainTid);
    pthread_cancel(logTid);
    pthread_mutex_destroy(&register_function_mutex);
    pthread_mutex_destroy(&logging_to_file_mutex);
}

void handle_state_signal(int signo, siginfo_t *info, void *other) {
    atomic_store(&is_log_enabled_flag, 1);
}

void handle_enabling_logs_signal(int signo, siginfo_t *info, void *other) {
    atomic_store(&dump_log_flag, 1);
}

void writeToFile(log_detail d, char *data) {
    if (is_initialized && logging_state == ENABLED) {
        pthread_mutex_lock(&logging_to_file_mutex);
        char* full_date = get_current_time();
        fprintf(file, "| %s | %s | %s |\n", full_date, convertDetail(d), data);
        fflush(file);
        pthread_mutex_unlock(&logging_to_file_mutex);
    }
}

static char *convertDetail(log_detail detail) {
    if (detail == MAX) {
        return "MAX";
    }
    if (detail == MIN) {
        return "MIN";
    }
    return "STANDARD";
}

void *executeThread(void *arg) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, level);
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);
    while (true) {
        if ((int)atomic_load(&is_log_enabled_flag) == 1) {
            logging_state = logging_state == DISABLED
                    ? ENABLED
                    : DISABLED;
            atomic_store(&is_log_enabled_flag, 0);
        }
        sleep(1);
    }
    return NULL;
}

void *executeLogs(void *arg) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, dump);
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);
    while (true) {
        if ((int)atomic_load(&dump_log_flag) == 1) {
            FILE *file = create_file();
            if (file != NULL) {
                for (int i = 0; i < funSize; i++) {
                    fun[i](file);
                }
                fclose(file);
            }
            atomic_store(&dump_log_flag, 0);
        }
        sleep(1);
    }
    return NULL;
}
