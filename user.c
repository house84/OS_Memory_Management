/*
 * Author: Nick House
 * Project: Memory Management
 * Course: CS-4760 Operating Systems, Spring 2021
 * File Name: user.c
 */

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
    run = true;
    references = 0;
    randRef = getRand(900, 1100);
    //Initialize pTable[Frames]


    if(sys->debug == true){
        fprintf(stderr, "User[%d]: DEBUG: Initializing - Time: %s\n", idx, getSysTime());
    }

    //Initialize Messaging
    bufI.mtype = mID;
    strcpy(bufI.mtext, "Initialized");

    //Signal to OSS that User has Initialized
    if( msgsnd(shmidMsgInit, &bufI, sizeof(bufI.mtext), 0) == -1 ){
        perrorHandler("User: ERROR: Failed to msgsnd() on initialization ");
    }

    if(sys->debug == true){
        fprintf(stderr, "User[%d]: DEBUG: Initialized - Time: %s\n", idx, getSysTime());
    }

    bufS.mtype = mID;
    strcpy(bufS.mtext, "Test: User -> Oss");
    if( msgsnd(shmidMsgSend, &bufS, sizeof(bufS.mtext), 0) == -1 ){

        perrorHandler("User: ERROR: Failed to msgsnd() on initialization ");
    }

    while(run == true || sys->run == true){

        //MOVE INTO FUNCTION LISTEN FOR TERMINATE
        if(checkTermMsg()){ break; }

        //Check if User Should Self Terminate
        if(checkTermPct()){ break; }

        //Request Read/Write
        pageRequest();
    }

    if(sys->debug == true){
        fprintf(stderr, "User: DEBUG: P%d Terminating Time: %s", idx, getSysTime());
    }

    //Free Memory
    freeSHM();

    exit(EXIT_SUCCESS);

}

//Generate Memory Request for Read/Write
static void pageRequest(){

    //Iterate Memory Refs
    ++references;
    ++referenceIter;

    //Choose Random Page from Table to Request
    bufS.page = getRand(0,31);
    //~2.4% Chance for bad memory Reference
    bufS.offset = getRand(-10, 1035);
    //Request Address to Access, Chance for Invalid Request
    bufS.address = bufS.page*1024 + bufS.offset;

    sys->pTable[idx].frameIdx = bufS.page;

    //Randomly Choose Read/Write Based On Read/Write %
    if(getRand(0,100) < readPct){
        bufS.action = READ;
        strcpy(sys->pTable[idx].pageT[bufS.page].action, "Read");
        strcpy(bufS.mtext, "Read Access Request");
    }
    else {
        bufS.action = WRITE;
        strcpy(sys->pTable[idx].pageT[bufS.page].action, "Write");
        strcpy(bufS.mtext, "Write Access Request");
    }
    bufS.mtype = mID;

    if(sys->debug == true){
        fprintf(stderr, "User: DEBUG: P%d -> %s\n", idx, bufS.mtext);
    }

    if( msgsnd(shmidMsgSend, &bufS, sizeof(bufS.mtext), IPC_NOWAIT) == -1 ){
        perrorHandler("User: ERROR: Failed to msgsnd() on initialization ");
    }

    //Validate Receive Message
    if(msgrcv( shmidMsgRec, &bufR, sizeof(bufR.mtext), mID, 0) == -1){

        perrorHandler("User: ERROR: Failed to Receive Message from OSS ");
    }

    if( bufR.action == TERMINATE ){ run = false; }
}

//Check if User Should Self Terminate
static bool checkTermPct(){

    //If random Ref between 900-1100 Ref Check for term
    if(referenceIter % randRef == 0){

        int t = getRand(0,2);
        referenceIter = 0;

        //33% Chance Terminate
        if(t != 0) { return false; }

        else { //If Terminate Inform OSS
            bufS.action = TERMINATE;
            strcpy(bufS.mtext, "Terminate");
            if( msgsnd(shmidMsgSend, &bufS, sizeof(bufS.mtext), 0) == -1 ){
                perrorHandler("User: ERROR: Failed to msgsnd() on initialization ");
            }
            return true;
        }
    }

    return false;
}

//Check message for terminate
static bool checkTermMsg(){

    //Validate Receive Message
    if(msgrcv( shmidMsgRec, &bufR, sizeof(bufR.mtext), mID, IPC_NOWAIT) == -1){

        perrorHandler("User: ERROR: Failed to Receive Message from OSS ");
    }

    if(sys->debug == true){
        fprintf(stderr, "User[%d]: DEBUG: Message Received from OSS Msg: %s", idx, bufR.mtext);
    }

    if( bufR.mtext == "terminate") { return true; }
    else { return false; }
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
