#ifndef TOOLS_H
#define TOOLS_H

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DATE_FORMAT "%m-%d-%y_%H-%M-%S"
#define DFL_FILENAME "log.txt"

// Time
char *get_current_time(char* format);

// Files
FILE *create_dump_file();

#endif //TOOLS_H
