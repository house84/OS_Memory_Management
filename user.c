/*
 * Author: Nick House
 * Project: Memory Management
 * Course: CS-4760 Operating Systems, Spring 2021
 * File Name: user.c
 */

#include "oss.h"
#include "user.h"
#include "shared.h"
#include "headers.h"
#include "sharedFunc.h"

int main(int argc, char * argv[]){

    srand(time(NULL) ^ (getpid()<<16));

    //Set Shmids
    idx = atoi(argv[1]);                //Set Index
    shmidSystem = atoi(argv[2]);        //Set System SHM
    shmidMsgRec = atoi(argv[3]);        //Messaging from OSS
    shmidMsgSend = atoi(argv[4]);       //Message to OSS for Operation
    shmidMsgInit = atoi(argv[5]);       //Message OSS User has been initiated
    shmidSem = atoi(argv[6]);           //Id for Mutex Semaphore

    initSysTime();
    setSemID(shmidSem);
    setShmid(sys);
    mID = idx+1;

    if(sys->debug == true){
        fprintf(stderr, "User[%d]: DEBUG: Initializing - Time: %s\n", idx, getSysTime());
    }

    //Initialize Messaging
    bufI.mtype = mID;
    strcpy(bufI.mtext, "");

    if( msgsnd(shmidMsgInit, &bufI, sizeof(bufI.mtext), 0) == -1 ){

        perrorHandler("User: ERROR: Failed to msgsnd() on initialization ");
    }

    if(sys->debug == true){
        fprintf(stderr, "User[%d]: DEBUG: Initialized - Time: %s\n", idx, getSysTime());
    }

    //Validate Receive Message
    if(msgrcv( shmidMsgRec, &bufR, sizeof(bufR.mtext), mID, 0) == -1){

        perrorHandler("User: ERROR: Failed to Receive Message from OSS ");
    }

    if(sys->debug == true){
        fprintf(stderr, "User[%d]: DEBUG: Message Received from OSS Msg: %s", idx, bufR.mtext);
    }

    bufS.mtype = mID;
    strcpy(bufS.mtext, "Test: User -> Oss");
    if( msgsnd(shmidMsgRec, &bufI, sizeof(bufI.mtext), 0) == -1 ){

        perrorHandler("User: ERROR: Failed to msgsnd() on initialization ");
    }

    //Free Memory
    freeSHM();

    exit(EXIT_SUCCESS);

}

//Initialize Shared Memory for System Time
static void initSysTime(){

    sys = (struct system *) shmat(shmidSystem, NULL, 0);
}

//Free Shared Memory PTR
static void freeSHM(){

    if(shmdt(sys) == -1){

        perror("user: ERROR: Failed to free ptr shmdt() ");
        exit(EXIT_FAILURE);
    }
}