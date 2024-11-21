#include "a2_helper.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>

sem_t sem_start, sem_end;
int active_threads=0;
sem_t limited_area, barrier_sem,barrier_2;
sem_t *sem_P85, *sem_P23;


void *thread_function8(void *arg) {

    int thread_number = *((int *) arg);

    if (thread_number == 1) {
        sem_wait(&sem_start); 
    }

    if (thread_number == 5) { sem_wait(sem_P23); }

    info(BEGIN, 8, thread_number);

    

    if (thread_number == 2) {
        sem_post(&sem_start); 
    }


    if (thread_number == 2) {
        sem_wait(&sem_end); 
    }

    info(END, 8, thread_number);

    if (thread_number == 5) { sem_post(sem_P85); }

    if (thread_number == 1) {
        sem_post(&sem_end); 
    }


    return NULL;
}

void *thread_function7(void *arg) {

    int thread_number = *((int *) arg);
    sem_wait(&limited_area);


    info(BEGIN, 7, thread_number);

    if (thread_number == 12) {
        sem_wait(&barrier_sem);
        active_threads++;
        if (active_threads == 5) {
            sem_post(&barrier_sem);
        }
    }

    info(END, 7, thread_number);

    sem_post(&limited_area);
    return NULL;
}

void *thread_function2(void *arg) {

    int thread_number = *((int *) arg);


    if (thread_number == 2) { sem_wait(sem_P85); }

    info(BEGIN, 2, thread_number);

  
    info(END, 2, thread_number);

    if (thread_number == 3) { sem_post(sem_P23);}


    return NULL;
}


void thread_P8() {

    pthread_t threads[5];
    int thread_numbers[5];

    for (int i = 0; i < 5; ++i) {
        thread_numbers[i] = i + 1;
        pthread_create(&threads[i], NULL, thread_function8, &thread_numbers[i]);
    }

    for (int i = 0; i < 5; ++i) {
        pthread_join(threads[i], NULL);
    }       
}

void thread_P7() {

    pthread_t threads[36];
    int thread_numbers[36];

    for (int i = 0; i < 36; ++i) {
         thread_numbers[i] = i + 1;
        pthread_create(&threads[i], NULL, thread_function7, &thread_numbers[i]);
    }

    for (int i = 0; i < 36; ++i) {
        pthread_join(threads[i], NULL);
    }
}

void process_P1() {
    
    info(BEGIN, 1, 0);

    if (fork() == 0) {
        
        info(BEGIN, 2, 0);
        pthread_t threads[4];
        int thread_numbers[4];

        for (int i = 0; i < 4; ++i) {
            thread_numbers[i] = i + 1;
            pthread_create(&threads[i], NULL, thread_function2, &thread_numbers[i]);
        }

        if (fork() == 0) {
            info(BEGIN, 3, 0);

            if (fork() == 0) {
                info(BEGIN, 5, 0);
                info(END, 5, 0);
                exit(0);
            } 

            if (fork() == 0) {
                info(BEGIN, 6, 0);
                info(END, 6, 0);
                exit(0);
            } 

            if (fork() == 0) {
                info(BEGIN, 7, 0);
                thread_P7();
                info(END, 7, 0);
                exit(0);
            } 

            if (fork() == 0) {
                info(BEGIN, 8, 0);
                thread_P8();
                info(END, 8, 0);
                exit(0);
            } 

            wait(NULL); 
            wait(NULL); 
            wait(NULL); 
            wait(NULL); 
            info(END, 3, 0);
            exit(0);
        } 

        for (int i = 0; i < 4; ++i) {
            pthread_join(threads[i], NULL);
        } 

        wait(NULL); 
        info(END, 2, 0);
        exit(0);
    } else {
        wait(NULL); 
    }

    if (fork() == 0) {
        info(BEGIN, 4, 0);

        if (fork() == 0) {
            info(BEGIN, 9, 0);
            info(END, 9, 0);
            exit(0);
        } else {
            wait(NULL); 
        }

        info(END, 4, 0);
        exit(0);
    } else {
        wait(NULL); 
    }

    info(END, 1, 0);
}


int main() {

    sem_init(&sem_start, 0, 0);
    sem_init(&sem_end, 0, 0);
    sem_init(&limited_area, 0, 5);
    sem_init(&barrier_sem, 0, 1);   
    sem_init(&barrier_2, 0, 0);
    sem_P85 = sem_open("/sem_P85", O_CREAT, 0644, 0); 
    sem_P23 = sem_open("/sem_P23", O_CREAT, 0644, 0); 
    init();
    process_P1();
    sem_destroy(&sem_start);
    sem_destroy(&sem_end);
    sem_destroy(&limited_area);
    sem_destroy(&barrier_sem);
    sem_destroy(&barrier_2);
    sem_close(sem_P85);
    sem_close(sem_P23);
    sem_unlink("/sem_P85");
    sem_unlink("/sem_P23");
    

    return 0;
}
