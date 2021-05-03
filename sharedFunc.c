/*
 * Author: Nick House
 * Project: Memory Management
 * Course: CS-4760 Operating Systems, Spring 2021
 * File Name: sharedFunc.c
 */

#include "oss.h"
#include "user.h"
#include "shared.h"
#include "headers.h"
#include "sharedFunc.h"

//Shared Memory ID
void setShmid(struct system * ptr){

    sys = ptr;
}

//Semaphore ID
void setSemID(int semID){

    shmidSem = semID;
}

//Mutex Wait
void semWait(int sem){

    sops.sem_num = sem;
    sops.sem_op = -1;
    sops.sem_flg = 0;

    if( semop(shmidSem, &sops, 1) == -1 ){

        perror("sharedLib: ERROR: semWait() ");
        exit(EXIT_FAILURE);
    }
}

//Mutex Free
void semSignal(int sem){

    sops.sem_num = sem;
    sops.sem_op = 1;
    sops.sem_flg = 0;

    if( semop(shmidSem, &sops, 1) == -1){

        perror("sharedLib: ERROR: semSignal() ");
        exit(EXIT_FAILURE);
    }
}

//Return Random Int between l and u
int getRand(int l, int u){

    if( l == 0 ){

        return rand() % (u+1);
    }

    return ( rand() % u ) + 1;
}

//Handle Perror
void perrorHandler(char * error){

    char errorString[100];
    snprintf(errorString, sizeof(errorString), error);
    perror(errorString);
    exit(EXIT_FAILURE);
}

//Display Stytem Time
const char * getSysTime(){

    char *sTime;
    asprintf(&sTime, "%04d:%09d", sys->seconds, sys->nanoSeconds);

    return sTime;
}

//Get system Time for Calcs xx.xxx
float getTime(){

    float decimal = sys->nanoSeconds;
    decimal = decimal/1000000000;
    float second = sys->seconds;
    float localT = second+decimal;

    return localT;
}

//Increment System Time
void incrementSysTime(int x){

    sys->nanoSeconds = sys->nanoSeconds + x;

    while(sys->nanoSeconds >= 1000000000 ){

        sys->nanoSeconds = sys->nanoSeconds - 1000000000;

        sys->seconds += 1;
    }
}


