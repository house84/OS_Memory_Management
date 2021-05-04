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

static void sighub(); 

int main(int argc, char * argv[]){

    srand(time(NULL) ^ (getpid()<<16));

	signal(SIGQUIT, sighup); 

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

    while(run == true){ // && sys->run == true){

        //MOVE INTO FUNCTION LISTEN FOR TERMINATE
      //  if(checkTermMsg()){ break; }

        //Check if User Should Self Terminate
        if(checkTermPct()){ 
			
			if(sys->debug == true){
				fprintf(stderr, "User: P%d DEBUG: Check Terminate True \n", idx);
			}
			run = false; 
			break; 
		}

        //Request Read/Write
        pageRequest();
    }

    if(sys->debug == true){
        fprintf(stderr, "User: DEBUG: P%d Terminating Time: %s\n", idx, getSysTime());
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
    int page = getRand(0,31); 
    bufS.page = page; 
	//~2.4% Chance for bad memory Reference
    int offset = getRand(-10, 1035); 
	sys->pTable[idx].pageT[page].offset = offset; //getRand(-10, 1045); 
//	bufS.offset = offset; 
    //Request Address to Access, Chance for Invalid Request
    int address = page*1024 + offset; 
//	bufS.address = address;
	sys->pTable[idx].pageT[page].address = address; 
//	bufS.address = -1; 

    sys->pTable[idx].frameIdx = page;

    //Randomly Choose Read/Write Based On Read/Write %
    if(getRand(0,100) < readPct){
       // bufS.action = READ;
        sys->pTable[idx].pageT[page].actionNum = READ; 
		strcpy(sys->pTable[idx].pageT[bufS.page].action, "Read");
        strcpy(bufS.mtext, "Read Access Request");
    }
    else {
        //bufS.action = WRITE;
        sys->pTable[idx].pageT[page].actionNum = WRITE; 
		strcpy(sys->pTable[idx].pageT[bufS.page].action, "Write");
        strcpy(bufS.mtext, "Write Access Request");
    }
    
	bufS.mtype = mID;

    //if( msgsnd(shmidMsgSend, &bufS, sizeof(bufS.mtext), IPC_NOWAIT) == -1 ){
    if( msgsnd(shmidMsgSend, &bufS, sizeof(bufS.mtext), 0) == -1 ){
        perrorHandler("User: ERROR: Failed to msgsnd() on initialization ");
    }

    //Validate Receive Message
    if(msgrcv( shmidMsgRec, &bufR, sizeof(bufR.mtext), mID, 0) == -1){

        perrorHandler("User: ERROR: Failed to Receive Message from OSS ");
    }

	if( strcmp(bufR.mtext, "terminate") == 0 ){ run = false; }

	if(sys->debug == true ){

		fprintf(stderr, "User: DEBUG: Address: %d Run: %d Message Received %s\n",sys->pTable[idx].pageT[page].address, run, sys->pTable[idx].pageT[page].action);
	}

}

//Check if User Should Self Terminate
static bool checkTermPct(){

    ++referenceIter;
	//If random Ref between 900-1100 Ref Check for term
    if(referenceIter % randRef == 0){

        int t = getRand(0,2);
        referenceIter = 0;

        //33% Chance Terminate
        if(t != 0) { return false; }

        else { //If Terminate Inform OSS
            
			if(sys->debug == true){

				fprintf(stderr, "User: DEBUG: P%d CheckTerminate = True\n", idx);
			}

			bufS.action = TERMINATE;
            strcpy(bufS.mtext, "terminate");
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


void sighup(){

	fprintf(stderr, " SIGHUP Terminating P%d \n", idx);
	
	if(shmdt(sys) == -1){
		perrorHandler("User: ERROR: Failed to free ptr shmdt() "); 
	}

	exit(EXIT_SUCCESS); 
}
	
