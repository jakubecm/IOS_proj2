/*
Autor: Milan Jakubec
Login: xjakub41
Datum: 2023-04-16
Projekt: 2. projekt do IOS (semafory)
*/


// Includy
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
#include <sys/types.h>
#include <sys/mman.h>

// Semafory pro program
sem_t *mutex;
sem_t *postMutex;
sem_t *urednik;
sem_t *rada1;
sem_t *rada2;
sem_t *rada3;
sem_t *logmafor;
sem_t *barrier;

// Promenne pro sdilenou pamet
FILE  *outputStream;
bool *closed;
uint32_t *line_number;
uint32_t *rada1_waiting;
uint32_t *rada2_waiting;
uint32_t *rada3_waiting;
uint32_t *processes;

// Funkce pro inicializaci semaforu do sdilene pameti a inicializace semaforu na pocatecni hodnoty
void semaphore_init(void){
    mutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    sem_init(mutex, 1, 1);
    postMutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    sem_init(postMutex, 1, 1);
    urednik = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    sem_init(urednik, 1, 0);
    rada1 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    sem_init(rada1, 1, 0);
    rada2 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    sem_init(rada2, 1, 0);
    rada3 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    sem_init(rada3, 1, 0);
    logmafor = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    sem_init(logmafor, 1, 1);
    barrier = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    sem_init(barrier, 1, 0);
}

// Funkce pro uvolneni semaforu ze sdilene pameti a jejich zruseni
void semaphore_clear(void){
    munmap(mutex, sizeof(sem_t));
    sem_destroy(mutex);
    munmap(postMutex, sizeof(sem_t));
    sem_destroy(postMutex);
    munmap(urednik, sizeof(sem_t));
    sem_destroy(urednik);
    munmap(rada1, sizeof(sem_t));
    sem_destroy(rada1);
    munmap(rada2, sizeof(sem_t));
    sem_destroy(rada2);
    munmap(rada3, sizeof(sem_t));
    sem_destroy(rada3);
    munmap(logmafor, sizeof(sem_t));
    sem_destroy(logmafor);
    munmap(barrier, sizeof(sem_t));
    sem_destroy(barrier);
}

// Funkce pro inicializaci promennych do sdilene pameti
void shared_items_init(){
    outputStream = mmap(NULL, sizeof(FILE), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    outputStream = fopen("proj2.out", "w");
    closed = mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    *closed = false;
    line_number = mmap(NULL, sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    *line_number = 1;
    rada1_waiting = mmap(NULL, sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    *rada1_waiting = 0;
    rada2_waiting = mmap(NULL, sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    *rada2_waiting = 0;
    rada3_waiting = mmap(NULL, sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    *rada3_waiting = 0;
    processes = mmap(NULL, sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    *processes = 0;
}

// Funkce pro uvolneni promennych ze sdilene pameti
void shared_items_clear(){
    fclose(outputStream);
    munmap(outputStream, sizeof(FILE));
    munmap(closed, sizeof(bool));
    munmap(line_number, sizeof(uint32_t));
    munmap(rada1_waiting, sizeof(uint32_t));
    munmap(rada2_waiting, sizeof(uint32_t));
    munmap(rada3_waiting, sizeof(uint32_t));
    munmap(processes, sizeof(uint32_t));
}

// Funkce na tisknuti outputu do proj2.out souboru s mutexem
void custom_print(const char* fmt, ...) {
    sem_wait(logmafor);

    va_list arg;
    va_start(arg, fmt);
    fprintf(outputStream, "%d: ", *line_number);
    vfprintf(outputStream, fmt, arg);
    va_end(arg);
    fflush(outputStream);
    (*line_number)++;

    sem_post(logmafor);
}

// Implementace bariery podle knihy, slouzi k pockani na vsechny procesy pri spousteni
void Barrier(int NU, int NZ){

    sem_wait(mutex);
    (*processes)++;
    sem_post(mutex);

    if ((*processes) == (uint32_t)(NU + NZ + 1)){
        sem_post(barrier);
    }

    sem_wait(barrier);
    sem_post(barrier);
}

// hlavni proces
int main(int argc, char *argv[]){

    if(argc != 6){
        fprintf(stderr, "Wrong number of arguments.\n");
        return 1;
    }

    shared_items_init();
    semaphore_init();

    if(mutex == MAP_FAILED || postMutex == MAP_FAILED || urednik == MAP_FAILED || rada1 == MAP_FAILED
     || rada2 == MAP_FAILED || rada3 == MAP_FAILED || logmafor == MAP_FAILED || barrier == MAP_FAILED)
    {
        fprintf(stderr, "Chyba pri alokaci semaforu\n");
        semaphore_clear();
        shared_items_clear();
        exit(1);
    }

    if(!outputStream){
        fprintf(stderr, "Error: Unable to open output file.\n");
        semaphore_clear();
        shared_items_clear();
        return 1;
    }

    // Promenne pro procesy, parsing argumentu
    int NZ = strtol(argv[1], NULL, 10); // Pocet zakazniku
    int NU = strtol(argv[2], NULL, 10); // Pocet uredniku
    int TZ = strtol(argv[3], NULL, 10); // Maximalni doba cekani zakaznika v milisekundach
    int TU = strtol(argv[4], NULL, 10); // Maximalni delka prestavky urednika v milisekundach
    int F = strtol(argv[5], NULL, 10); // Maximalni cas v milisekundach, po kterem je uzavrena posta pro nove prichozi

    if (!(TZ >= 0 && TZ <= 10000) || !(TU >= 0 && TU <= 100) || !(F >= 0 && F <= 10000))
    {
        fprintf(stderr, "Error: Range/s of the given argument/s wrong.\n");
        semaphore_clear();
        shared_items_clear();
        return 1;
    }

    // Vytvoreni NZ procesu zakazniku
    for (int i = 1; i <= NZ; i++)
    {
        pid_t idZ = fork();

        if (idZ == 0)
        {
            Barrier(NU, NZ);
            srand(time(NULL) + i);

            custom_print("Z %d: started\n", i);
            usleep((rand() % (TZ + 1)) * 1000);

            sem_wait(postMutex);
            // Kontrola jestli je posta otevrena, pokud ne, zakaznik jde domu
            if ((*closed) == true)
            {
                sem_post(postMutex);
                custom_print("Z %d: going home\n", i);
                fclose(outputStream);
                exit(0);
            }
    
            int line = rand() % 3 + 1;
            custom_print("Z %d: entering office for a service %d\n", i, line);

            sem_post(postMutex);
            
            // Zarazeni do fronty po nahodnem vyberu rady
            if (line == 1)
            {
                sem_wait(mutex);
                (*rada1_waiting)++;
                sem_post(mutex);

                sem_post(rada1);
            }
            else if (line == 2)
            {
                sem_wait(mutex);
                (*rada2_waiting)++;
                sem_post(mutex);

                sem_post(rada2);
            }
            else if (line == 3)
            {
                sem_wait(mutex);
                (*rada3_waiting)++;
                sem_post(mutex);

                sem_post(rada3);
            }

            // Cekani na zavolani urednikem, vyrizeni a odchod domu

            sem_wait(urednik);

            custom_print("Z %d: called by office worker\n", i);
            usleep((rand() % 11) * 1000);

            custom_print("Z %d: going home\n", i);
            fclose(outputStream);
            exit(0);
        }
        else if (idZ == -1)
        {
            fprintf(stderr, "Error: Fork failed.\n");
            semaphore_clear();
            shared_items_clear();
            return 1;
        }
    }

    // Vytvoreni NU procesu uredniku
    for (int i = 1; i <= NU; i++)
    {
        pid_t idU = fork();
        if (idU == 0)
        {
            Barrier(NU, NZ);
            srand(time(NULL) + i + 2);

            custom_print("U %d: started\n", i);

            // Dokud posta neni zavrena nebo v nejake rade nekdo je, cyklus bezi 
            while (((*closed) == false) || ((*rada1_waiting) != 0) || ((*rada2_waiting) != 0) || ((*rada3_waiting) != 0))
            {
                int choice = rand() % 3 + 1;
                bool valid_choice = false;

                sem_wait(mutex);
                // Nahodny vyber z nepradne rady
                while (!valid_choice)
                {
                    if (choice == 1 && (*rada1_waiting) > 0)
                    {
                        valid_choice = true;
                        sem_post(mutex);
                    }
                    else if (choice == 2 && (*rada2_waiting) > 0)
                    {
                        valid_choice = true;
                        sem_post(mutex);
                    }
                    else if (choice == 3 && (*rada3_waiting) > 0)
                    {
                        valid_choice = true;
                        sem_post(mutex);
                    }
                    else
                    {
                        choice = rand() % 3 + 1;
                        sem_post(mutex);

                        // Pokud jsou vsechny rady prazdne, ukonci vyber
                        if ((*rada1_waiting) == 0 && (*rada2_waiting) == 0 && (*rada3_waiting) == 0)
                        {
                            choice = 0;
                            break;
                        }
                    }
                }

                // Pokud nikdo neni ve fronte, probehne kontrola, jestli je posta zavrena
                // Pokud ano, break z whilu, pokud ne, jde urednik na prestavku 
                if (choice == 0)
                {
                    sem_wait(postMutex);
                    if ((*closed) == false)
                    {
                        custom_print("U %d: taking break\n", i);
                        sem_post(postMutex);
                        if(TU != 0) // pokud delka pauzy je 0, nema tento usleep smysl
                        {
                            usleep((rand() % (TU + 1)) * 1000);
                        }
                        custom_print("U %d: break finished\n", i);
                    }
                    else
                    {
                        sem_post(postMutex);
                        break;
                    } 
                }

                // Jinak probiha podle vyberu fronty ukon se zakaznikem
                else if (choice == 1)
                {
                    sem_wait(mutex);
                    (*rada1_waiting)--;
                    sem_post(mutex);

                    sem_wait(rada1);
                    sem_post(urednik);

                    custom_print("U %d: serving a service of type %d\n", i, choice);
                    usleep((rand() % 11) * 1000);
                    custom_print("U %d: service finished\n", i);
                }
                else if (choice == 2)
                {
                    sem_wait(mutex);
                    (*rada2_waiting)--;
                    sem_post(mutex);

                    sem_wait(rada2);
                    sem_post(urednik);

                    custom_print("U %d: serving a service of type %d\n", i, choice);
                    usleep((rand() % 11) * 1000);
                    custom_print("U %d: service finished\n", i);
                }
                else if (choice == 3)
                {
                    sem_wait(mutex);
                    (*rada3_waiting)--;
                    sem_post(mutex);

                    sem_wait(rada3);
                    sem_post(urednik);

                    custom_print("U %d: serving a service of type %d\n", i, choice);
                    usleep((rand() % 11) * 1000);
                    custom_print("U %d: service finished\n", i);
                }

            }

            custom_print("U %d: going home\n", i);
            fclose(outputStream);
            exit(0);
        }
        else if (idU == -1)
        {
            fprintf(stderr, "Error: Fork failed.\n");
            semaphore_clear();
            shared_items_clear();
            return 1;
        }
    }
    Barrier(NU, NZ);
    // Cekej pomoci volani usleep nahodny cas v intervalu <F/2, F>
    int range = F - F / 2 + 1;
    usleep((rand() % range + F / 2) * 1000);

    // Vypis A: closing
    sem_wait(postMutex);
    custom_print("closing\n");
    (*closed) = true;
    sem_post(postMutex);
    
    // Pockej na ukonceni vsech procesu, ktere aplikace vytvari. Jakmile jsou ukonceny, ukonci sebe s kodem 0.
    while (wait(NULL) > 0);
    shared_items_clear();
    semaphore_clear();
    exit(0);
}
