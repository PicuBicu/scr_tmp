#include "tools.h"

char *get_current_time(char* format) {
    char timeBuf[30];
    char *current_name = malloc(sizeof(char) * 50);
    if (current_name == NULL) {
        return NULL;
    }
    time_t raw_time = time(0);
    struct tm *local_time = localtime(&raw_time);
    strftime(timeBuf, 30, format, local_time);
    sprintf(current_name, "%s", timeBuf);
    return current_name;
}

FILE *create_dump_file() {
    char *full_date = get_current_time(DATE_FORMAT);
    if (full_date == NULL) {
        return NULL;
    }
    return fopen(strcat(full_date, ".txt"), "w+");
}

