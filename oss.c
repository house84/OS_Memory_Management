/*
 * Author: Nick House
 * Project: Memory Management
 * Course: CS-4760 Operating Systems, Spring 2021
 * File Name: oss.c
 */

#include "oss.h"
#include "shared.h"
#include "headers.h"
#include "sharedFunc.h"

int main(int argc, char * argv[]){

    //Set Bool
    sigFlag = false;
    spawnFlag = false;
    debug = false;

    //Initialize Signal Handling
    signal(SIGINT, signalHandler);

    //Initialize Logfile
    //Set Initial Parameters
    memset(logfile, '\0', sizeof(logfile));
    strcpy(logfile, "logfile");

    totalProc = 0;
    allocatedFrames = 0;
    frameQ = initQueue();
    faultQ = initCircleQ();

    srand(time(NULL));

    //Parse Input Args
    int c = 0;
    while(( c = getopt(argc, argv, "hd")) != -1){

        switch(c){
            case 'h':	help(argv[0]);
                exit(EXIT_SUCCESS);

            case 'd': 	//Set Time
                debug = true;
                break;

            default:	//Defalut Failure Exit
                fprintf(stderr, "Invalid Argument, see usage [-h]\n");
                exit(EXIT_FAILURE);
        }

    }

    //Open logfile
    openLogfile();
    createSharedMemory();
    sys->debug = debug;
    int mID;

    int index;
    int iterTime;
    concProc = 0;
    sys->run = true;

    time_t now;
    struct tm *tm;
    now = time(0);

    tm = localtime(&now);
    int startSeconds = tm->tm_sec;
    bool go = true;
    bool terminateTimer = false;

    //while(true){
    int i;
    for(i = 0; i < maxConProc; ++i){

        //Check Timer
        if(terminateTimer == false){

            now = time(0);
            tm = localtime(&now);
            int stopSeconds = tm->tm_sec;

            //Set Run Timer
            if((stopSeconds - startSeconds) >= 10){
                terminateTimer = true;
            }
        }

        //critical
        semWait(mutex);
        //Increment Sys Time by 1-500 ms
        incrementSysTime(rand()%500000001 + 1000000);
        semSignal(mutex);

        if( concProc < maxConProc ){

            //do some spawning
            int idx = getUserIdxBit();
            spawn(idx);
            mID = idx+1;
            active[idx] = true;           //Track Active Processes
            ++totalProc;                //Track Total Processes
            ++concProc;                 //Track Concurrent Processes

            //Allow User Process to Initialize
            if(msgrcv(shmidMsgInit, &bufI, sizeof(bufI.mtext), mID, 0) == -1){
                perrorHandler("Master: ERROR: Failed to RCV Message from user msgrcv() ");
            }
        }

        //Handle User Memory Request
        memoryHandler();

        //Check fault Queue
        checkFaultQ();

		//check for Finished Processes
        int status;
        pid_t user_id = waitpid(-1, &status, WNOHANG);

        if(user_id > 0 ){
            --concProc;
        }

        //Check if OSS should terminate
        if(totalProc > maxProc || terminateTimer == false){

            sys->run = false;

            int j;
            //Send Message to Users to Terminate
            for(j = 0; j < maxConProc; ++j ){

                //Iterate through Active Processes
                if( active[j] == true ){

                    //Allow any user waiting on MsgSnd to continue
                    if(msgrcv(shmidMsgInit, &bufR, sizeof(bufR.mtext), mID, IPC_NOWAIT) == -1){
                        perrorHandler("Master: ERROR: Failed to RCV Message from user msgrcv() ");
                    }

                    bufS.mtype = j+1;
                    bufS.action = TERMINATE;
                    strcpy(bufS.mtext, "terminate");
                    if(msgsnd( shmidMsgSend, &bufS, sizeof(bufS.mtext), IPC_NOWAIT) == -1){
                        perrorHandler("Master: ERROR: Failed to Send Message to User ");
                    }
                }
            }

            break;
        }
    }

    if(debug == true){
        fprintf(stderr, "Master: DEBUG: Driver Loop Exited Total Processes: %d - Time: %s\n", totalProc, getSysTime());
    }

    //Allow Processes to finish
    while(wait(NULL) > 0){}

    //Clean up Resources
    signalHandler(3126);

    if(debug == true){
        fprintf(stderr, "Master: DEBUG: Resources Free Exiting Program\n");
    }

    return 0;
}

//page Handler
static void memoryHandler(){

    //Summon the Daemon
    specialDaemon();

    int idx;
    bool invalidAddr = false;

    //ADD LOGIC TO HANDLE USERS MESSAGES TO REQUEST MEMORY READ
     bufR.mtype =-1;
     if(msgrcv( shmidMsgRec, &bufR, sizeof(bufR.mtext), 0, IPC_NOWAIT) == -1){
           perrorHandler("Master: ERROR: Failed to Receive Message msgrcv() ");
     }

     //If User Message Received
     if(bufR.mtype != -1){

         idx = bufR.mtype - 1;
         int pageMinAddr = bufR.page*1024;             //p0 = Addressable from 0
         int pageMaxAddr = ((bufR.page+1)*1024) - 1;   //p0 = Addressable to 1023
         int page = bufR.page;
         bool invalid = false;
         bool terminate = false;

         //+++for Testing To Terminate
         bufR.address = -1;
         //++++++

         //Check if Address is invalid, then terminate
         if( bufR.action == TERMINATE ){

             freeUserResources(idx, page);
             return;
         }
         else if( bufR.address < pageMinAddr || bufR.address > pageMaxAddr ) {

             fprintf(stdout, "Master: Address %d is Invalid, Terminating. Time: %s", bufR.address, getSysTime());

             invalid = true;
             //Invalid Address Terminate User Process
             bufS.mtype = bufR.mtype;
             bufS.action = TERMINATE;
             strcpy(bufS.mtext, "terminate");
             if (msgsnd(shmidMsgSend, &bufS, sizeof(bufS.mtext), 0) == -1) {
                 perrorHandler("Master: ERROR: Failed to Send Message to User ");
             }

             freeUserResources(idx, page);
             return;
         }


         //check if frame has been allocated System memory
         if( sys->pTable[idx].pageT[page].allocated ) {

             //set valid bit = true,
             sys->pTable[idx].pageT[page].validBit = true;

             //refBit or dirtyBit depending on Read/Write
             if(bufR.action == READ){
                 sys->pTable[idx].pageT[page].refByte = true;
             }
             else{
                 sys->pTable[idx].pageT[page].dirtyBit = true;
             }

             bufS.mtype = bufR.mtype;
             bufS.action = VALID;
             strcpy(bufS.mtext, "valid");

             if(msgsnd( shmidMsgSend, &bufS, sizeof(bufS.mtext), 0) == -1){
                 perrorHandler("Master: ERROR: Failed to Send Message to User ");
             }

             return;
         }

         //search memory bit vector for space in OSS
         int memIdx = getMemoryBit();
         if(memIdx >= 0){

             //Allocate Memory
             sys->pTable[idx].pageT[page].frameIdx = memIdx;
             setMemoryBit(memIdx);
         }
          else{

             //Second Chance Find replacement and allocate
             memIdx = fifo();

             //Allocate Memory
             sys->pTable[idx].pageT[page].frameIdx = memIdx;
             setMemoryBit(memIdx);
          }

          ++allocatedFrames;

          //set time
          sys->pTable[idx].pageT[page].time = getTime();

          //Set allocated = true; in User Frame
          sys->pTable[idx].pageT[page].allocated = true;

          //Set ValidBit
          sys->pTable[idx].pageT[page].validBit = true;

          //set faultQRemove to 14ms past current time
          sys->pTable[idx].pageT[page].faultQRemove = getTime() + .014;

          if( bufR.action == READ ){

              sys->pTable[idx].pageT[page].refByte = true;
          }
          else {

              sys->pTable[idx].pageT[page].dirtyBit = true;
          }

          //Add to frameQueue
          enqueue(frameQ, idx, page);

          //Add to faultQ
          circleEnqueue(faultQ, idx, page, sys->pTable[idx].pageT[page].faultQRemove);
     }
}


//Free A User's Resources from system when Terminating
static void freeUserResources(int idx, int page){

    //Look at sys->PCB[userIdx]->
    int i;
    for (i = 0; i < 32; ++i) {

        //If User Has Page Tables Allocated Free Resources back to System
        if (sys->pTable[idx].pageT[i].allocated == true) {

            int frameIdx = sys->pTable[idx].pageT[page].frameIdx;
            fprintf(stderr, "Master: Clearing frame %d\n", frameIdx);
            unsetMemoryBit(frameIdx);
            sys->pTable[idx].pageT[i].allocated = false;
            if(sys->pTable[idx].pageT[page].dirtyBit == true){
                semWait(mutex);
                incrementSysTime(getRand(10000000, 14000000));
                semSignal(mutex);
                fprintf(stderr, "Master: Dirty Bit of frame %d set, time added to system clock\n", frameIdx);
            }
            removeQ(frameQ, idx, page);
        }
    }

    //Remove User from OSS User management
    active[idx] = false;
    unsetUserIdxBit(idx);
}

//Find Available Memory Bit
static int getMemoryBit(){
    int idx;

    //search for Index

    return idx;
}

//Set Bit for Allocated Page in Sys Memory
static void setMemoryBit(int idx){

}

//Unset Bit from Memory Vector
static void unsetMemoryBit(int idx){

}

//Find Available User Idx
static int getUserIdxBit(){

    unsigned int i = 1;
    int idx = 0;

    //Search bitVector R->L until 0 is found
    while(( i & userBitVector) && (idx < maxConProc)){

        i <<= 1;
        ++idx;
    }

    if( idx < maxConProc ){

        //Set Bit Func
        setUserIdxBit(idx);

        return idx;
    }

    else{

        return -1;
    }
}


//Set Bit for User Process Idx
static void setUserIdxBit(int idx) {

    userBitVector |= ( 1 << idx );
}

//Unset Bit from User Vector
static void unsetUserIdxBit(int idx){

    userBitVector &= ~( 1 << idx );
}

//Run Daemon Process to Free Frames
static void specialDaemon(){

    //if Free frames < 10% Run Daemon
    if( frameQ->currSize >  231){

        fprintf(stderr, "Master: Daemon Process Running - Time: %s\n", getSysTime());

        //Set iterations to 5% total Pages Allocated
        int iter = .05 * frameQ->currSize;
        struct p_Node *newNode = frameQ->head;

        int i;
        for(i = 0; i < iter; ++i)
        {
            if(newNode == NULL) { break; }

            //Free Oldest Frames with validBit == false
            if(sys->pTable[newNode->idx].pageT[newNode->page].validBit == false){

                int frameIdx = sys->pTable[newNode->idx].pageT[newNode->page].frameIdx;
                fprintf(stderr, "Master: Daemon Process is clearing Frame %d\n", frameIdx);

                //Free allocated Memory used by User
                unsetMemoryBit(frameIdx);

                //Reset Frames Values to Default
                sys->pTable[newNode->idx].pageT[newNode->page].allocated = false;
                sys->pTable[newNode->idx].pageT[newNode->page].refByte = false;
                sys->pTable[newNode->idx].pageT[newNode->page].validBit = false;
                if(sys->pTable[newNode->idx].pageT[newNode->page].dirtyBit){

                    //Increment System Time for Dirty Bit Opt Cost
                    semWait(mutex);
                    incrementSysTime(getRand(10000000, 14000000));
                    semSignal(mutex);
                    sys->pTable[newNode->idx].pageT[newNode->page].dirtyBit = false;
                    fprintf(stderr, "Master: Dirty Bit of frame %d set, time added to system clock\n", frameIdx);
                }

                //Remove Frame from Queue
                removeQ(frameQ, newNode->idx, newNode->page);
            }
            else{

                sys->pTable[newNode->idx].pageT[newNode->page].validBit = false;
            }

            newNode = newNode->next;
        }
    }
}

static int fifo(){
    int index;

    //do FIFO stuff

    return index;
}

static void checkFaultQ(){

    struct p_Node * newNode;

    //Check if designated 14ms has passed
    while(checkTimerIO(faultQ, getTime())){

        newNode = circleDequeue(faultQ);

        bufS.mtype = newNode->idx+1;
        bufS.action = VALID;
        strcpy(bufS.mtext, "valid");
        int idx = newNode->idx;
        char action[100];
        strcpy(action, sys->pTable[idx].pageT[newNode->page].action);
        int addr = sys->pTable[idx].pageT[newNode->page].frameIdx;

        fprintf(stderr, "Master: Indicating P%d that %s has happened at address %d\n", idx, action, addr);

        if(msgsnd( shmidMsgSend, &bufS, sizeof(bufS.mtext), 0) == -1){
            perrorHandler("Master: ERROR: Failed to Send Message to User ");
        }
    }
}


//Check User Message
static void checkMsg(){

    //Check for Messages by terminated Procesess
    if(msgrcv(shmidMsgRec, &bufR, sizeof(bufR.mtext), 0, 0) == -1){

        perrorHandler("Master: ERROR: Failed to Receive Message from User ");
    }

    if(debug == true ){

        //Print Received Message
        fprintf(stderr, "Master: DEBUG: Message Received: %s \n", bufR.mtext);
    }

    if( bufR.mtext == "terminate"){

        active[bufR.mtype-1] = false;
        fprintf(logPtr, "Master: P%ld Terminating\n", bufR.mtype + 1);
        fprintf(stdout, "Master: P%ld Terminating\n", bufR.mtype + 1);
    }
}


//Signal Handler
static void signalHandler(int sig){

    time(&t);

    //Check for Signal Type
    if( sig == SIGINT ) {

        fprintf(stderr, "\nProgram Terminated by User\n");
        fprintf(logPtr, "\nTime: %sProgram Terminated by User\n", ctime(&t));

    }else if( sig == 3126 ){

        fprintf(stderr, "\n|||==> All Processes Have Completed <===|||\n");
        fprintf(stderr, "|||==>   Freeing System Resources   <===|||\n");
        fprintf(logPtr, "\n|||==> All Processes Have Completed <===|||\n");
        fprintf(logPtr, "\nTime: %s", ctime(&t));

    }else{

        fprintf(stderr, "\nProgram Terminated due to Timer\n");
    }

    //Close Logfile Ptr
    closeLogfile();

    //Allow Potential Creating Processes to add PID to Array
    while(spawnFlag == true){}

    //Free Memory Resources
    freeSharedMemory();

    //Exit Normally
    if( sig == 3126 ) { exit(EXIT_SUCCESS); }


    //Terminate Child Processes
    int i;
    for( i = 0; i < totalProc; ++i){

        if(kill(pidArray[i], SIGKILL ) == -1 && errno != ESRCH ){

            perrorHandler("Master: ERROR: Failed to Kill Processes ");
        }
    }

    //Destroy Potential Zombies
    while( wait(NULL) != -1 || errno == EINTR );

    exit(EXIT_SUCCESS);
}


//Create Shared Memory
static void createSharedMemory(){

    //=== System Time Shared Memory
    if((keySysTime = ftok("Makefile", 'a')) == -1){

        perrorHandler("Master: ERROR: Failed to generate keySysTime ftok() ");
    }

    if((shmidSystem = shmget(keySysTime, sizeof(struct system), IPC_CREAT|S_IRUSR|S_IWUSR)) == -1){

        perrorHandler("Master: ERROR: Failed to get shmidSysTime, shmget() ");
    }


    sys = (struct system *) shmat(shmidSystem, NULL, 0);


    //=== Messaging to Send to User
    if((keyMsg = ftok("oss.c", 'a')) == -1){

        perrorHandler("Master: ERROR: Failed to generate keyMsg, ftok() ");
    }

    if((shmidMsgSend = msgget(keyMsg, IPC_CREAT|S_IRUSR|S_IWUSR)) == -1){

        perrorHandler("Master: ERROR: Failed to generate shmidMsgSend, msgget() ");
    }

    //=== Message to Receive From User
    if((keyMsg2 = ftok("user.c", 'a')) == -1){

        perrorHandler("Master: ERROR: Failed to generate keyMsg2, ftok() ");
    }

    if((shmidMsgRec = msgget(keyMsg2, IPC_CREAT|S_IRUSR|S_IWUSR)) == -1){

        perrorHandler("Master: ERROR: Failed to generate shmidMsgRec, msgget() ");
    }

    //=== Message to Receive Initialized from User
    if((keyMsg3 = ftok("user.h", 'a')) == -1){

        perrorHandler("Master: ERROR: Failed to generate keyMsg3, ftok() ");
    }

    if((shmidMsgInit = msgget(keyMsg3, IPC_CREAT|S_IRUSR|S_IWUSR)) == -1){

        perrorHandler("Master: ERROR: Failed to generate shmidMsgInit, msgget() ");
    }

    //=== Set Mutex Sem
    if((keySem = ftok("sharedFunc.c", 'a')) == -1){

        perrorHandler("Master: ERROR: Failed to generate semKey, ftok() ");
    }

    if((shmidSem = semget(keySem, 1, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR )) == -1){

        perrorHandler("Master: ERROR: Failed to generate shmidSem, semget() ");
    }

    if((semctl(shmidSem, mutex, SETVAL, 1)) == -1){

        perrorHandler("Master: ERROR: Failed to create Mutex Sem semctl() ");
    }
}

//Free Shared Memory
static void freeSharedMemory(){

    //Detach System Pointer
    if(shmdt(sys) == -1){

        perrorHandler("Master: ERROR: Failed to detach shmidSysTime, shmdt() ");
    }

    //Destroy System Time Memory
    if(shmctl(shmidSystem, IPC_RMID, NULL) == -1){

        perrorHandler("Master: ERROR: Failed to Desttory st, shmctl() ");
    }


    //Destroy Message Q
    if(msgctl(shmidMsgSend, IPC_RMID, NULL) == -1){

        perrorHandler("Master: ERROR: Failed to Destroy shmidMsgSend, msgctl() ");
    }

    if(msgctl(shmidMsgRec, IPC_RMID, NULL) == -1){

        perrorHandler("Master: ERROR: Failed to Destroy shmidMsgRec, msgctl() ");
    }

    if(msgctl(shmidMsgInit, IPC_RMID, NULL) == -1){

        perrorHandler("Master: ERROR: Failed to Destroy shmidMsgInit, msgctl() ");
    }

    if((semctl(shmidSem, 0, IPC_RMID)) == -1 ){

        perrorHandler("Master: ERROR: Failed to release sem Memory semctl() ");
    }
}


//Display Usage
static void help(char *program){

    printf("\n//=== %s Usage Page ===//\n", program);
    printf("\n%s [-h][-v]\n", program);
    printf("%s -h      This Usage Page\n", program);
    printf("%s -v      Turn Debug Mode On\n", program);

}

//Open New Logfile
static void openLogfile(){

    logPtr = fopen(logfile, "w");

    time(&t);

    fprintf(logPtr, "\n//========================= Log Opened ========================//\n");
    fprintf(logPtr, "Time: %s", ctime(&t));
    fprintf(logPtr, "//=============================================================//\n");
}


//Close Logfile
static void closeLogfile(){


    fprintf(logPtr, "\n//========================= Log Closed ========================//\n");
    fprintf(logPtr, "Time: %s", ctime(&t));
    fprintf(logPtr, "//=============================================================//\n\n");
    fclose(logPtr);
}

//Spawn Child Process
static void spawn(int idx){

    //Check if prograom is terminating
    if(sigFlag == true) { return; }

    pid_t process_id;

    if((process_id = fork()) < 0){

        perrorHandler("Master: ERROR: Failed to fork process fork() ");
    }

    if(process_id == 0){

        //Temp Block Handler from Terminating
        spawnFlag = true;

        //Add Process to Process Array
        pidArray[idx]  = process_id;

        //Release Block
        spawnFlag = false;

        //Index arg
        char buffer_idx[10];
        sprintf(buffer_idx, "%d", idx);

        //shmidSysTime arg
        char buffer_sysTime[50];
        sprintf(buffer_sysTime, "%d", shmidSystem);

        //shmidMsg arg
        char buffer_msgId[50];
        sprintf(buffer_msgId, "%d", shmidMsgSend);

        //bool run = true;
        //shmidMsgRcv arg
        char buffer_msgId2[50];
        sprintf(buffer_msgId2, "%d", shmidMsgRec);

        //shmidMsg3 arg
        char buffer_msgId3[50];
        sprintf(buffer_msgId3, "%d", shmidMsgInit);

        //shmidSem
        char buffer_shmidSem[50];
        sprintf(buffer_shmidSem, "%d", shmidSem);

        if(sys->debug == true){
            fprintf(stderr, "Master: DEBUG: Process[%d] Spawning - Time: %s\n", idx, getSysTime());
        }

        //Call user file with child process
        if(execl("./user_proc", "user_proc", buffer_idx, buffer_sysTime, buffer_msgId,buffer_msgId2, buffer_msgId3, buffer_shmidSem, (char*) NULL)){

            perrorHandler("Master: ERROR: Failed to execl() child process ");
        }

        exit(EXIT_SUCCESS);
    }
}
