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
#define SEM_NAME "/xjakub41"

sem_t *mutex;
sem_t *zakaznik;
sem_t *urednik;
sem_t *rada1;
sem_t *rada2;
sem_t *rada3;
sem_t *logmafor; // pro logovani

struct shared_data{
    FILE  *outputStream;
    bool closed;
    uint32_t line_number;
    uint32_t rada1_state;
    uint32_t rada2_state;
    uint32_t rada3_state;
};

int shared_memory_id;
struct shared_data *shared_data = NULL;

int init_shared_mem(){
    shared_memory_id = shmget(IPC_PRIVATE, sizeof(struct shared_data), IPC_CREAT | 0666);

    if(shared_memory_id == -1){
        return 1;
    }

    shared_data = shmat(shared_memory_id, NULL, 0);

    if(shared_data == (void *) -1){
        shmctl(shared_memory_id, IPC_RMID, NULL);
        return 1;
    }

    shared_data->line_number = 1;
    shared_data->closed = false;
    shared_data->rada1_state = 0;
    shared_data->rada2_state = 0;
    shared_data->rada3_state = 0;

    return 0;
}

void clear_shared_mem(){
    shmdt(shared_data);
    shmctl(shared_memory_id, IPC_RMID, NULL);
}

void semaphore_init(){
    mutex = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    zakaznik = sem_open("xjakzakaznik", O_CREAT, 0666, 0);
    urednik = sem_open("xjakurednik", O_CREAT, 0666, 0);
    rada1 = sem_open("xjakrada1", O_CREAT, 0666, 0);
    rada2 = sem_open("xjakrada2", O_CREAT, 0666, 0);
    rada3 = sem_open("xjakrada3", O_CREAT, 0666, 0);
    logmafor = sem_open("xjaklogmafor", O_CREAT, 0666, 1);
}

void semaphore_clear(){
    sem_close(mutex);
    sem_close(zakaznik);
    sem_close(urednik);
    sem_close(rada1);
    sem_close(rada2);
    sem_close(rada3);
    sem_close(logmafor);
    sem_unlink(SEM_NAME);
    sem_unlink("xjakzakaznik");
    sem_unlink("xjakurednik");
    sem_unlink("xjakrada1");
    sem_unlink("xjakrada2");
    sem_unlink("xjakrada3");
    sem_unlink("xjaklogmafor");
}

void custom_print(const char* fmt, ...) {
    sem_wait(logmafor);

    va_list arg;
    va_start(arg, fmt);
    fprintf(shared_data->outputStream, "%d: ", shared_data->line_number);
    vfprintf(shared_data->outputStream, fmt, arg);
    va_end(arg);
    fflush(shared_data->outputStream);
    shared_data->line_number++;

    sem_post(logmafor);
}

void amimir(uint32_t millis) {
    uint32_t time = rand() % (millis + 1);
    usleep(time * 1000);
}

// main slouzi jako hlavni proces
int main(int argc, char *argv[]){

    if(argc != 6){
        fprintf(stderr, "Wrong number of arguments.\n");
        return 1;
    }

    if(init_shared_mem() != 0){
        fprintf(stderr, "Error: Unable to initialize shared memory.\n");
        clear_shared_mem();
        return 1;
    }

    shared_data->outputStream = fopen("proj2.out", "w");

    if(!shared_data->outputStream){
        fprintf(stderr, "Error: Unable to open output file.\n");
        fclose(shared_data->outputStream);
        return 1;
    }

    // Promenne pro procesy
    int NZ = strtol(argv[1], NULL, 10); // Pocet zakazniku
    int NU = strtol(argv[2], NULL, 10); // Pocet uredniku
    int TZ = strtol(argv[3], NULL, 10); // Maximalni doba cekani zakaznika v milisekundach
    int TU = strtol(argv[4], NULL, 10); // Maximalni delka prestavky urednika v milisekundach
    int F = strtol(argv[5], NULL, 10); // Maximalni cas v milisekundach, po kterem je uzavrena posta pro nove prichozi

    if (!(TZ >= 0 && TZ <= 10000) || !(TU >= 0 && TU <= 100) || !(F >= 0 && F <= 10000))
    {
        fprintf(stderr, "Error: Range/s of the given argument/s wrong.\n");
        return 1;
    }

    semaphore_init();

    // Vytvor NZ procesu zakazniku

    for (int i = 1; i <= NZ; i++){
        pid_t idZ = fork();

        if (idZ == 0){
            srand(getpid());
            custom_print("A: Z %d: started", i);
            amimir(TZ);

            if(shared_data->closed){
                custom_print("A: Z %d: finished", i);
                exit(0);
            }

            int line = rand() % 3 + 1; // nahodne cislo 1-3

            if(line == 1){
                custom_print("A: Z %d: entering office for a service %d", i, line);
                shared_data->rada1_state++;
                sem_post(rada1);
            }
            else if(line == 2){
                custom_print("A: Z %d: entering office for a service %d", i, line);
                shared_data->rada2_state++;
                sem_post(rada2);                
            }
            else if(line == 3){
                custom_print("A: Z %d: entering office for a service %d", i, line);
                shared_data->rada3_state++;
                sem_post(rada3);            
            }

            sem_wait(urednik);
            custom_print("A: Z %d: called by office worker", i);
            amimir(10);
            custom_print("A: Z %d: going home", i);
            exit(0);
        }
    }

    // Vytvor NU procesu uredniku

    for (int i = 1; i <= NZ; i++){
        pid_t idU = fork();
        if(idU == 0){
            srand(getpid());
            while(!shared_data->closed){

                if(shared_data->rada1_state == 0 && shared_data->rada2_state == 0 && shared_data->rada3_state == 0)
                {
                    custom_print("A: U %d: taking break", i);
                    amimir(TU);
                    custom_print("A: U %d: break finished", i);
                    continue;
                }
                
                int choice = rand() % 3 + 1;

                switch (choice){
                    case 1:
                        if(shared_data->rada1_state == 0) continue;
                        custom_print("A: U %d: serving a service of type %d", i, choice);
                        sem_wait(zakaznik);

                        sem_wait(mutex);
                        shared_data->rada1_state--;
                        sem_post(mutex);
                        
                        amimir(10);
                        custom_print("A: U %d: service finished", i);
                        sem_post(urednik);
                        break;

                    case 2:
                        if(shared_data->rada2_state == 0) continue;
                        custom_print("A: U %d: serving a service of type %d", i, choice);
                        sem_wait(zakaznik);

                        sem_wait(mutex);
                        shared_data->rada2_state--;
                        sem_post(mutex);

                        amimir(10);
                        custom_print("A: U %d: service finished", i);
                        sem_post(urednik);
                        break;

                    case 3:
                        if(shared_data->rada3_state == 0) continue;
                        custom_print("A: U %d: serving a service of type %d", i, choice);
                        sem_wait(zakaznik);

                        sem_wait(mutex);
                        shared_data->rada3_state--;
                        sem_post(mutex);

                        amimir(10);
                        custom_print("A: U %d: service finished", i);
                        sem_post(urednik);
                        break;
                }
            }

            if(shared_data->closed && shared_data->rada1_state == 0 && shared_data->rada2_state == 0 && shared_data->rada3_state == 0){
                custom_print("A: U %d: going home", i);
                exit(0);
            }

        }
        

    }
    // Cekej pomoci volani usleep nahodny cas v intervalu <F/2, F>

    // Vypis A: closing

    // Pockej na ukonceni vsech procesu, ktere aplikace vytvari. Jakmile jsou ukonceny, ukonci sebe s kodem 0.
    while(wait(NULL) > 0);
    semaphore_clear();
    clear_shared_mem();
    exit(0);

}