#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include "init.h"
#define LEVEL_SIGNAL SIGRTMIN
#define LOG_SIGNAL SIGRTMIN + 1

unsigned long long int factoryNum;
short int originalNum;
void *factory(void *arg);
void *logThread(void *arg);
unsigned long long int countFactory(short int num);
static char *isPrime(unsigned long long num);
void f1(FILE *f);
void f2(FILE *f);
char *prime;
int evenOdd;

int main()
{
    srand(time(NULL));
    printf("pid = %d, level change signal = %d, dump signal = %d\n", getpid(), LEVEL_SIGNAL, LOG_SIGNAL);
    assert(create_logger(ENABLED
                         , LEVEL_SIGNAL
                         , LOG_SIGNAL
                         , DFL_FILENAME
                         , 2) == 0);
    pthread_t tid, tid2;
    pthread_create(&tid, NULL, factory, (void *)&logging_state_signal);
    pthread_create(&tid2, NULL, logThread, (void *)&dump_file_signal);
    pthread_join(tid, NULL);
    pthread_join(tid2, NULL);
    destroy_logger();
    return 0;
}

void *factory(void *arg)
{
    add_dump_file_function(f1);

    while (1)
    {
        originalNum = rand() % 16;
        factoryNum = countFactory(originalNum);
        log_message(MIN, "Starting looking for a factory number");
        char buffer[100];
        sprintf(buffer, "Found a number: %d and the factory: %llu", originalNum, factoryNum);
        log_message(MIN, buffer);
        sleep(1);
    }
    return NULL;
}
void *logThread(void *arg)
{
    add_dump_file_function(f2);
    while (1)
    {
        char buffer[100];
        sprintf(buffer, "Checking whether number %llu is even or odd", factoryNum);
        log_message(STANDARD, buffer);
        evenOdd = factoryNum % 2;
        sprintf(buffer, "Checking whether number %llu is prime or composite", factoryNum);
        log_message(MAX, buffer);
        prime = isPrime(factoryNum);
        sleep(1);
    }
    return NULL;
}

unsigned long long int countFactory(short int num)
{

    if (num == 0)
    {
        return 1;
    }
    unsigned long long int result = 1;
    for (short int i = 2; i <= num; i++)
    {
        result *= i;
    }
    return result;
}

static char *isPrime(unsigned long long num)
{
    if (num <= 2)
        return "PRIME";
    for (unsigned long long i = 2; i <= num / 2; i++)
    {
        if (num % i == 0)
            return "COMPOSITE";
    }
    return "PRIME";
}

void f1(FILE *f)
{
    fprintf(f, "Drawn number is: %d\n", originalNum);
    fprintf(f, "Factory is: %llu\n", countFactory(originalNum));
}
void f2(FILE *f)
{
    factoryNum % 2 == 0 ? fprintf(f, "Factory number is even\n") : fprintf(f, "Factory number is odd\n");
    char *p = isPrime(factoryNum);
    fprintf(f, "Factory number is %s \n", p);
}
