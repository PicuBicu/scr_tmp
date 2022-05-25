#include "logger.h"


sem_t enable_logging_sem, dump_file_sem;
logging_state current_logging_state;
log_priority current_log_priority;
siginfo_t siginfo;
signum logging_state_signal = 0, dump_file_signal = 0;
FILE *logs_file;
bool is_initialized = false;
dump_function_ptr create_dump_file_function_ptr;
pthread_t logging_enability_tid, dump_file_creation_tid;
pthread_mutex_t logging_to_file_mutex, register_function_mutex, change_priority_mutex;

int create_logger(logging_state state,
                  signum logging_state_signal_num,
                  signum dump_file_signal_num,
                  char *filename) {
    if (is_initialized || filename == NULL) {
        return EXIT_FAILURE;
    }

    logging_state_signal = logging_state_signal_num;
    dump_file_signal = dump_file_signal_num;
    current_log_priority = MIN;
    current_logging_state = state;
    logs_file = fopen(filename, "a+");
    if (logs_file == NULL) {
        return EXIT_FAILURE;
    }

    sigset_t set;
    struct sigaction action;
    sigfillset(&set);
    action.sa_sigaction = handle_logging_state_change_signal;
    action.sa_flags = SA_SIGINFO;
    action.sa_mask = set;

    if (sigaction(logging_state_signal_num, &action, NULL) != 0) {
        fclose(logs_file);
        return EXIT_FAILURE;
    }

    sigfillset(&set);
    action.sa_sigaction = handle_dump_file_creation_signal;
    action.sa_flags = SA_SIGINFO;
    action.sa_mask = set;

    if (sigaction(dump_file_signal_num, &action, NULL) != 0) {
        fclose(logs_file);
        return EXIT_FAILURE;
    }

    if (pthread_mutex_init(&logging_to_file_mutex, NULL) != 0) {
        fclose(logs_file);
        return EXIT_FAILURE;
    }

    if (pthread_mutex_init(&register_function_mutex, NULL) != 0) {
        fclose(logs_file);
        pthread_mutex_destroy(&register_function_mutex);
        return EXIT_FAILURE;
    }

    if (pthread_mutex_init(&change_priority_mutex, NULL) != 0) {
        fclose(logs_file);
        pthread_mutex_destroy(&logging_to_file_mutex);
        pthread_mutex_destroy(&register_function_mutex);
        return EXIT_FAILURE;
    }

    if (sem_init(&enable_logging_sem, 0, 0) != 0) {
        fclose(logs_file);
        pthread_mutex_destroy(&logging_to_file_mutex);
        pthread_mutex_destroy(&register_function_mutex);
        pthread_mutex_destroy(&change_priority_mutex);
    }

    if (sem_init(&dump_file_sem, 0, 0) != 0) {
        fclose(logs_file);
        sem_destroy(&enable_logging_sem);
        pthread_mutex_destroy(&logging_to_file_mutex);
        pthread_mutex_destroy(&register_function_mutex);
        pthread_mutex_destroy(&change_priority_mutex);
    }

    if (pthread_create(&dump_file_creation_tid, NULL, dump_file_creation_routine, NULL) != 0) {
        fclose(logs_file);
        sem_destroy(&enable_logging_sem);
        sem_destroy(&dump_file_sem);
        pthread_mutex_destroy(&register_function_mutex);
        pthread_mutex_destroy(&logging_to_file_mutex);
        pthread_mutex_destroy(&change_priority_mutex);
        return EXIT_FAILURE;
    }

    if (pthread_create(&logging_enability_tid, NULL, logging_enability_routine, NULL) != 0) {
        fclose(logs_file);
        sem_destroy(&enable_logging_sem);
        sem_destroy(&dump_file_sem);
        pthread_cancel(dump_file_creation_tid);
        pthread_mutex_destroy(&register_function_mutex);
        pthread_mutex_destroy(&logging_to_file_mutex);
        pthread_mutex_destroy(&change_priority_mutex);
        return EXIT_FAILURE;
    }

    is_initialized = true;
    return EXIT_SUCCESS;
}

int add_dump_file_function(dump_function_ptr function) {
    if (is_initialized) {
        pthread_mutex_lock(&register_function_mutex);
        create_dump_file_function_ptr = function;
        pthread_mutex_unlock(&register_function_mutex);
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}

void destroy_logger() {
    if (logs_file != NULL) {
        fclose(logs_file);
    }
    sem_destroy(&enable_logging_sem);
    sem_destroy(&dump_file_sem);
    pthread_cancel(logging_enability_tid);
    pthread_cancel(dump_file_creation_tid);
    pthread_mutex_destroy(&register_function_mutex);
    pthread_mutex_destroy(&logging_to_file_mutex);
    pthread_mutex_destroy(&change_priority_mutex);
    is_initialized = false;
}

void handle_logging_state_change_signal(int signo, siginfo_t *info, void *other) {
    siginfo = *info;
    sem_post(&enable_logging_sem);
}

void handle_dump_file_creation_signal(int signo, siginfo_t *info, void *other) {
    sem_post(&dump_file_sem);
}

void log_message(log_priority priority, char *message) {
    if (is_initialized && current_logging_state == ENABLED && current_log_priority <= priority) {
        pthread_mutex_lock(&logging_to_file_mutex);
            char *full_date = get_current_time(DATE_FORMAT);
            char *priority_name = get_log_type(priority);
            fprintf(logs_file, "| %s | %s | %s |\n", full_date, priority_name, message);
            fflush(logs_file);
        pthread_mutex_unlock(&logging_to_file_mutex);
    }
}

char *get_log_type(log_priority detail) {
    if (detail == MAX) {
        return "MAX";
    }
    if (detail == MIN) {
        return "MIN";
    }
    return "STANDARD";
}

void *logging_enability_routine(void *arg) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, logging_state_signal);
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);
    while (true) {
        sem_wait(&enable_logging_sem);
        char buffer[10];
        sprintf(buffer, "DATA %d\n", siginfo.si_int);
        write(STDOUT_FILENO, buffer, 10);
        switch (siginfo.si_int) {
            case MIN:         change_logger_priority(MIN); break;
            case STANDARD:         change_logger_priority(STANDARD);break;
            case MAX:         change_logger_priority(MAX);break;
            default: current_logging_state = (current_logging_state == DISABLED)
                                             ? ENABLED
                                             : DISABLED; break;
        }

    }
    return NULL;
}

void *dump_file_creation_routine(void *arg) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, dump_file_signal);
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);
    while (true) {
        sem_wait(&dump_file_sem);
        FILE *dump_file = create_dump_file();
        if (dump_file != NULL) {
            create_dump_file_function_ptr(dump_file);
            fclose(dump_file);
        }
    }
    return NULL;
}

void change_logger_priority(log_priority new_log_priority) {
    pthread_mutex_lock(&change_priority_mutex);
        current_log_priority = new_log_priority;
    pthread_mutex_unlock(&change_priority_mutex);
}