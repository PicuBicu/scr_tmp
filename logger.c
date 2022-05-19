#include "logger.h"

atomic_int is_log_enabled_flag, dump_log_flag;
logging_state current_logging_state;
log_priority current_log_priority;
signum logging_state_signal = 0, dump_file_signal = 0;
FILE *logs_file;
bool is_initialized = false;
unsigned int dump_functions_array_index = 0;
size_t dump_functions_array_size = 5;
dump_function_ptr *dump_function_array;
pthread_t logging_enability_tid, dump_file_creation_tid;
pthread_mutex_t logging_to_file_mutex, register_function_mutex, change_priority_mutex;

int create_logger(logging_state state,
                  signum logging_state_signal_num,
                  signum dump_file_signal_num,
                  char *filename,
                  size_t dump_file_array_init_size) {
    if (is_initialized || filename == NULL) {
        return EXIT_FAILURE;
    }

    current_log_priority = MIN;
    current_logging_state = state;
    dump_functions_array_size = dump_file_array_init_size;
    logging_state_signal = logging_state_signal_num;
    dump_file_signal = dump_file_signal_num;

    atomic_store(&is_log_enabled_flag, 0);
    atomic_store(&dump_log_flag, 0);

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

    dump_function_array = (dump_function_ptr *) calloc(sizeof(dump_function_ptr), dump_functions_array_size);
    if (dump_function_array == NULL) {
        fclose(logs_file);
        pthread_mutex_destroy(&register_function_mutex);
        pthread_mutex_destroy(&logging_to_file_mutex);
        pthread_mutex_destroy(&change_priority_mutex);
        return EXIT_FAILURE;
    }

    if (pthread_create(&dump_file_creation_tid, NULL, dump_file_creation_routine, NULL) != 0) {
        fclose(logs_file);
        free(dump_function_array);
        pthread_mutex_destroy(&register_function_mutex);
        pthread_mutex_destroy(&logging_to_file_mutex);
        pthread_mutex_destroy(&change_priority_mutex);
        return EXIT_FAILURE;
    }

    if (pthread_create(&logging_enability_tid, NULL, logging_enability_routine, NULL) != 0) {
        fclose(logs_file);
        free(dump_function_array);
        pthread_cancel(dump_file_creation_tid);
        pthread_mutex_destroy(&register_function_mutex);
        pthread_mutex_destroy(&logging_to_file_mutex);
        pthread_mutex_destroy(&change_priority_mutex);
        return EXIT_FAILURE;
    }

    if (pthread_detach(logging_enability_tid) != 0) {
        free(dump_function_array);
        fclose(logs_file);
        pthread_cancel(logging_enability_tid);
        pthread_cancel(dump_file_creation_tid);
        pthread_mutex_destroy(&register_function_mutex);
        pthread_mutex_destroy(&logging_to_file_mutex);
        pthread_mutex_destroy(&change_priority_mutex);
        return EXIT_FAILURE;
    }

    if (pthread_detach(dump_file_creation_tid) != 0) {
        fclose(logs_file);
        free(dump_function_array);
        pthread_cancel(logging_enability_tid);
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
    if (is_initialized == true) {
        pthread_mutex_lock(&register_function_mutex);
            if (dump_functions_array_index >= dump_functions_array_size) {
                dump_function_ptr *functions = (dump_function_ptr *) realloc(
                        dump_function_array,
                        sizeof(dump_function_ptr) * dump_functions_array_size * 2);
                if (functions == NULL) {
                    return EXIT_FAILURE;
                }
                dump_function_array = functions;
                dump_functions_array_size = dump_functions_array_size * 2;
            }
            dump_function_array[dump_functions_array_index++] = function;
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
    if (dump_function_array != NULL) {
        free(dump_function_array);
    }
    atomic_store(&dump_log_flag, 0);
    atomic_store(&is_log_enabled_flag, 0);
    pthread_cancel(logging_enability_tid);
    pthread_cancel(dump_file_creation_tid);
    pthread_mutex_destroy(&register_function_mutex);
    pthread_mutex_destroy(&logging_to_file_mutex);
    pthread_mutex_destroy(&change_priority_mutex);
}

void handle_logging_state_change_signal(int signo, siginfo_t *info, void *other) {
    atomic_store(&is_log_enabled_flag, 1);
}

void handle_dump_file_creation_signal(int signo, siginfo_t *info, void *other) {
    atomic_store(&dump_log_flag, 1);
}

void log_message(log_priority priority, char *message) {
    if (is_initialized &&
    current_logging_state == ENABLED &&
    current_log_priority <= priority) {
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
        if ((int) atomic_load(&is_log_enabled_flag) == 1) {
            current_logging_state = current_logging_state == DISABLED
                                    ? ENABLED
                                    : DISABLED;
            atomic_store(&is_log_enabled_flag, 0);
        }
        sleep(1);
    }
    return NULL;
}

void *dump_file_creation_routine(void *arg) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, dump_file_signal);
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);
    while (true) {
        if ((int) atomic_load(&dump_log_flag) == 1) {
            FILE *dump_file = create_dump_file();
            if (dump_file != NULL) {
                for (int i = 0; i < dump_functions_array_size; i++) {
                    dump_function_array[i](dump_file);
                }
                fclose(dump_file);
            }
            atomic_store(&dump_log_flag, 0);
        }
        sleep(1);
    }
    return NULL;
}

void change_logger_priority(log_priority new_log_priority) {
    pthread_mutex_lock(&change_priority_mutex);
        current_log_priority = new_log_priority;
    pthread_mutex_unlock(&change_priority_mutex);
}
