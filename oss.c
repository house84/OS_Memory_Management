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

    time_t now;
    struct tm *tm;
    now = time(0);

    tm = localtime(&now);
    int startSeconds = tm->tm_sec;
    bool go = true;
    bool stopSpawn = false;

    //while(true){
    int i;
    for(i = 0; i < maxConProc; ++i){

        //Check Timer
        if(stopSpawn == false){

            now = time(0);
            tm = localtime(&now);
            int stopSeconds = tm->tm_sec;

            //Set Run Timer
            if((stopSeconds - startSeconds) >= 10){
                stopSpawn = true;
            }
        }

        //Increment System Time by 1-500 ms

        //critical
        semWait(mutex);
        incrementSysTime(rand()%500000000 + 1000001);
        semSignal(mutex);

        if( concProc < maxConProc && totalProc < maxProc && stopSpawn == false ){

            //do some spawning
            spawn(i);
            mID = i+1;
            ++totalProc;                //Track Total Processes
            ++concProc;                 //Track Concurrent Processes

            if(msgrcv(shmidMsgInit, &bufI, sizeof(bufI.mtext), mID, 0) == -1){

                perrorHandler("Master: ERROR: Failed to RCV Message from user msgrcv() ");
            }
        }

        bufS.mtype = mID;
        strcpy(bufS.mtext, "Test: Oss -> User");
        if(msgsnd( shmidMsgSend, &bufS, sizeof(bufS.mtext), 0) == -1){

            perrorHandler("Master: ERROR: Failed to Send Message to User ");
        }

        //Check for Messages by terminated Procesess
        if(msgrcv(shmidMsgRec, &bufR, sizeof(bufR.mtext), mID, 0) == -1){

            perrorHandler("Master: ERROR: Failed to Receive Message from User ");
        }
		
		if(debug == true ){

        	//Print Received Message
			fprintf(stderr, "Master: DEGUB: Message Received: %s \n", bufR.mtext);
		}
        
		//check for Finished Processes
        int status;
        pid_t user_id = waitpid(-1, &status, WNOHANG);

        if(user_id > 0 ){
            --concProc;
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
