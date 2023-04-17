/*
Autor: Milan Jakubec
Login: xjakub41
Datum: 2023-04-16
Projekt: 2. projekt do IOS (semafory)
*/

#include <limits.h>
#include <semaphore.h>
#include <stdarg.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int init_shared_mem(void);
void clear_shared_mem(void);
void semaphore_init(void);
void semaphore_clear(void);
void custom_print(const char* fmt, ...);
