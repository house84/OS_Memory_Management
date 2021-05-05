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
    srand(time(NULL));

    //Initialize Signal Handling
    signal(SIGINT, signalHandler);
	designatedUsers = 18; 



    //Parse Input Args
    int c = 0;
    while(( c = getopt(argc, argv, "hdp:")) != -1){

        switch(c){
            case 'h':	help(argv[0]);
                exit(EXIT_SUCCESS);

            case 'd': 	//Set Time
                debug = true;
                break;

			case 'p':  //Set User Processes
				designatedUsers = atoi(optarg); 
				break;

			case ':': 
				fprintf(stderr, "Argument Required...Exiting\n"); 
				exit(EXIT_FAILURE);
				break; 

            default:	//Defalut Failure Exit
                fprintf(stderr, "Invalid Argument, see usage [-h]\n");
                exit(EXIT_FAILURE);
        }

    }
    
	
	
	if( designatedUsers <= 0 ){

		fprintf(stdout, "\nNumber of Processes Must be greater than 0\nExiting...\n");
		exit(EXIT_SUCCESS); 
	}

	if( designatedUsers > 18 ) { designatedUsers = 18; } 
	
    //Set Initial Parameters
    memset(logfile, '\0', sizeof(logfile));
    strcpy(logfile, "logfile");

    totalProc = 0;
    allocatedFrames = 0;
    processQ = initQueue();
    frameQ = initQueue();
    faultQ = initCircleQ();


    //Initialize CPU Node
    CPU_Node = (struct p_Node*)malloc(sizeof(struct p_Node));

    //Open logfile
    openLogfile();
    createSharedMemory();
    sys->debug = debug;
	sys->stats.memAccess = 0; 
	sys->stats.faults = 0; 
	sys->stats.AvgMAS = 0; 
	sys->stats.segFault = 0; 

	
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

   	while(true){

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

        //Check fault Q for IO Ready
        checkFaultQ();


        if( concProc < designatedUsers){ 

            //do some spawning
            int idx = getUserIdxBit();

            if(idx != -1){

                int mID = idx+1;
                spawn(idx);

                active[idx] = true;           //Track Active Processes
                ++totalProc;                //Track Total Processes
                ++concProc;                 //Track Concurrent Processes

                //Allow User Process to Initialize
                if(msgrcv(shmidMsgInit, &bufI, sizeof(bufI.mtext), mID, 0) == -1){
                    perrorHandler("Master: ERROR: Failed to RCV Message from user msgrcv() ");
                }

                enqueue(processQ, idx, 0);

				if(debug == true){

					fprintf(stderr, "Master: Debug: P%d Has been Added to Run Q\n", idx); 
				}
            }

        }

        allocateCPU();

		//check for Finished Processes
        int status;
        pid_t user_id = waitpid(-1, &status, WNOHANG);

        if(user_id > 0 ){
            --concProc;
        }

        //Check if OSS should terminate
        if((totalProc == maxProc || terminateTimer == true)){ // && concProc == 0 ){

		   sys->run = false;
           

			fprintf(stderr, "\n|||==> OSS Completed : Terminating Users : Total Proc = %d <==||\n", totalProc); 
		   
		   sleep(1); 

		   int i; 
		   for( i == 0 ; i < maxConProc; ++i){

			   if( active[i] == true){
					
					bufS.mtype = i+1; 
					msgsnd(shmidMsgSend, &bufS, sizeof(bufS.mtext), IPC_NOWAIT); 
				}
		   }

            break;
        }
    }

    if(debug == true){
        fprintf(stderr, "Master: DEBUG: Driver Loop Exited Total Processes: %d  Time: %s\n", totalProc, getSysTime());
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

//CPU Handler
static void allocateCPU(){

    
	//Check for runnable Processes
    if(processQ->currSize == 0){ 
			
			fprintf(stderr, "Process Q Empty\n"); 

			return; 
		
		}

    //Dequeue Process
    CPU_Node = dequeue(processQ);

    
	//Check Node
    if(CPU_Node == NULL){

        fprintf(stderr,"Que Head Empty\n");
        return;
    }


    int idx = CPU_Node->idx;
    int mID = CPU_Node->idx+1;
	
	if(debug == true ){

		fprintf(stderr, "Master: DEBUG: P%d Allocate CPU\n", idx); 
	}
	

    bufS.mtype = mID;
    strcpy(bufS.mtext, "Run");


    if((msgsnd(shmidMsgSend, &bufS, sizeof(bufS.mtext), 0)) == -1 ){

        fprintf(stderr, "OSS: FAILED::: mID: %d\n", mID);
        perror("oss: ERROR: Failed to Send Msg to User msgsnd() ");
        exit(EXIT_FAILURE);
    }

    //Wait for message from User to simulate end CPU
    msgrcv(shmidMsgRec, &bufR, sizeof(bufR.mtext), mID, 0);

	if(debug == true ){

		fprintf(stderr, "Master: DEBUG: P%d Message Recieved: %s\n", idx, bufR.mtext); 
	}

    int page = sys->pTable[idx].frameIdx;
    int pageMinAddr = page*1024;             //p0 = Addressable from 0
    int pageMaxAddr = ((page+1)*1024) - 1;   //p0 = Addressable to 1023
    int address = sys->pTable[idx].pageT[page].address; 
	int actionNum = sys->pTable[idx].pageT[page].actionNum; 
	char msg[200]; 
	strcpy(msg, sys->pTable[idx].pageT[page].action);  

    //Handle User Message
    if( strcmp(bufR.mtext, "terminate") == 0){

		fprintf(stderr, "Master: P%d Requested Invalid Address Terminating Time: %s\n", idx, getSysTime()); 
		fprintf(logPtr, "Master: P%d Requested Invalid Address Terminating Time: %s\n", idx, getSysTime()); 
		
		freeUserResources(idx, page); 
        
		unsetUserIdxBit(idx);
        active[idx] = false;

        if(debug == true){

            fprintf(stderr, "Master: DEBUG: P%d Process is Terminating Time: %s\n", idx, getSysTime());
        }

        return;
    }

	if( address < pageMinAddr || address > pageMaxAddr ){

		fprintf(stderr, "Master: P%d Requested Invalid Address Terminating Time: %s\n", idx, getSysTime()); 
		fprintf(logPtr, "Master: P%d Requested Invalid Address Terminating Time: %s\n", idx, getSysTime()); 
		
		freeUserResources(idx, page); 

		bufS.mtype = mID; 
		strcpy(bufS.mtext, "terminate"); 
		msgsnd(shmidMsgSend, &bufS, sizeof(bufS.mtext), 0); 
		wait(NULL); 
		
		--concProc; 
		unsetUserIdxBit(idx); 
		active[idx] = false; 

		
		//Increment SegFault
		++sys->stats.segFault; 
        
		
		if(debug == true){

            fprintf(stderr, "Master: DEBUG: P%d Requested Invalid Address -> Terminate %s\n", idx, getSysTime());
        }

		return; 
	}


	//Increment Memory Access
	++sys->stats.memAccess; 

    if( strcmp(bufR.mtext, "Read") == 0){

		fprintf(stderr, "Master: P%d Requesting %s of Address: %d at Time: %s\n", idx, msg, address, getSysTime()); 
		fprintf(logPtr, "Master: P%d Requesting %s of Address: %d at Time: %s\n", idx, msg, address, getSysTime()); 

        //This is for Testing and will go in Memory Handler
		memoryHandler(idx, page, READ); 

        return;
    }

    if(strcmp(bufR.mtext, "Write") == 0){
		

		fprintf(stderr, "Master: P%d Requesting %s of Address: %d at Time: %s\n", idx, msg, address, getSysTime()); 
		fprintf(logPtr, "Master: P%d Requesting %s of Address: %d at Time: %s\n", idx, msg, address, getSysTime()); 
		 
        //Handle User Memory Request
        memoryHandler(idx, page, WRITE);
		
        return;
    }
}


//page Handler
static void memoryHandler(int idx, int page, int RW){

    //Summon the Daemon
    specialDaemon();
	++sys->pTable[idx].pageReferences; 


    //check if frame has been allocated System memory
    if( sys->pTable[idx].pageT[page].allocated ) {

   		//set valid bit = true,
        sys->pTable[idx].pageT[page].validBit = true;

        //refBit or dirtyBit depending on Read/Write
        if(RW == READ){
            sys->pTable[idx].pageT[page].refByte = true;
        }
        else{
            sys->pTable[idx].pageT[page].dirtyBit = true;
        }
		
		//No Page Fault increment System Time 10 ns
		incrementSysTime(10); 
		enqueue(processQ, idx, 0); 
		sys->stats.AvgMAS += .00000001; 

        return;
    }

    //search memory bit vector for space in OSS
    int memIdx = getMemoryBit();
    if(memIdx >= 0){

        //Allocate Memory 
		sys->pTable[idx].pageT[page].frameIdx = memIdx;
    }
    else{


        //Second Chance Find replacement and allocate
        memIdx = fifo();

        //Allocate Memory
        sys->pTable[idx].pageT[page].frameIdx = memIdx;
     }

     ++allocatedFrames;
	 ++sys->pTable[idx].pageFaults; 
	 ++sys->stats.faults; 
	 sys->stats.AvgMAS += .014; 

     //set time
     sys->pTable[idx].pageT[page].time = getTime();

     //Set allocated = true; in User Frame
     sys->pTable[idx].pageT[page].allocated = true;

     //Set ValidBit
     sys->pTable[idx].pageT[page].validBit = true;

     //set faultQRemove to 14ms past current time
     sys->pTable[idx].pageT[page].faultQRemove = getTime() + .014;

     if( RW == READ ){

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


//Free A User's Resources from system when Terminating
static void freeUserResources(int idx, int page){

    if( debug == true){

		fprintf(stderr, "Master: DEBUG: P%d Freeing Allocate Memory\n", idx); 
	}
	
	int i;
    for (i = 0; i < 32; ++i) {

        //If User Has Page Tables Allocated Free Resources back to System
        if (sys->pTable[idx].pageT[i].allocated == true) {

            int frameIdx = sys->pTable[idx].pageT[i].frameIdx;
            fprintf(stderr, "Master: \tClearing frame %d\n", frameIdx);
            unsetMemoryBit(frameIdx);
            sys->pTable[idx].pageT[i].allocated = false;
            if(sys->pTable[idx].pageT[i].dirtyBit == true){
                semWait(mutex);
                incrementSysTime(getRand(10000000, 14000000));
                semSignal(mutex);
                fprintf(stderr, "Master: \tDirty Bit of frame %d set, time added to system clock\n", frameIdx);
                fprintf(logPtr, "Master: \tDirty Bit of frame %d set, time added to system clock\n", frameIdx);
            }
        }
    }
    
	//Print Effective Memory Access
	if(sys->pTable[idx].pageReferences > 0){
		float EMA; 
		float p = sys->pTable[idx].pageFaults/sys->pTable[idx].pageReferences; 
		EMA = (1 - p) * 10 + p * 14000000; 

		fprintf(stderr, "Master: P%d Effective Memory Access Time: %f ns\n", idx, EMA); 
	}

	removeQ(frameQ, idx, page);

}

//Find Available Memory Bit
static int getMemoryBit(){
    
	unsigned int i = 1; 
	int idx = 0; 
	int index = 0; 

	//Search R->L Until 0 is Found
	while(( i & memory[index]) && (idx < 256)){

		i <<= 1; 
		++idx; 
		
		if(idx % 32 == 0) { 
			
			++index; 
			i = 1; 
		}
	}

	if( idx < 256 ){

		setMemoryBit(idx); 

		return idx; 
	}
	
	else{ return -1; }
}

//Set Bit for Allocated Page in Sys Memory
static void setMemoryBit(int idx){

	int index = idx/32; 
	int offset = idx%32; 

	memory[index] |= ( 1 << offset ); 
}


//Unset Bit from Memory Vector
static void unsetMemoryBit(int idx){

	int index = idx/32;
	int offset = idx%32; 

	memory[index] &= ~( 1 << offset ); 
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
        fprintf(logPtr, "Master: Daemon Process Running - Time: %s\n", getSysTime());

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
                fprintf(logPtr, "Master: Daemon Process is clearing Frame %d\n", frameIdx);

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
                    fprintf(logPtr, "Master: Dirty Bit of frame %d set, time added to system clock\n", frameIdx);
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
	int QSize = frameQ->currSize; 
	bool found = false; 

	struct p_Node *newNode = frameQ->head; 

	int i; 
	for( i = 0; i < QSize; ++i ){ //refByte

		if(newNode == NULL) { break; }

		if(sys->pTable[newNode->idx].pageT[newNode->page].refByte == false){
                
				
			int frameIdx = sys->pTable[newNode->idx].pageT[newNode->page].frameIdx;
            
			fprintf(stderr, "Master: P%d Swapping Frame %d\n", newNode->idx, frameIdx);

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
                fprintf(stderr, "Master: \tDirty Bit of frame %d set, time added to system clock\n", frameIdx);
          	}

            //Remove Frame from Queue
            removeQ(frameQ, newNode->idx, newNode->page);

			return frameIdx; 
			
		}
        
		else{

         	sys->pTable[newNode->idx].pageT[newNode->page].refByte = false;
        }
            
		newNode = newNode->next;
	}

	
	fifo(); 
}


//Check if I/O Timer is up 
static void checkFaultQ(){

  	struct p_Node * newNode;
		
	bool release = checkTimerIO(faultQ, getTime()); 

	if( debug == true ){

		fprintf(stderr, "Master: DEBUG: checkFaultQ %d\n", release); 
	}

    //Check if designated 14ms has passed
    while(release){

        newNode = circleDequeue(faultQ);

        bufS.mtype = newNode->idx+1;
        bufS.action = VALID;
        strcpy(bufS.mtext, "valid");
        int idx = newNode->idx;
        char action[100];
        strcpy(action, sys->pTable[idx].pageT[newNode->page].action);
        int addr = sys->pTable[idx].pageT[newNode->page].frameIdx;
		
		release = checkTimerIO(faultQ, getTime()); 
		
		//Add Process Back into RunQ
		enqueue(processQ, idx, 0); 

        fprintf(stderr, "Master: Indicating P%d that %s has happened at address %d\n", idx, action, addr);
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

	fprintf(stderr, "\n|||==>     Performance Stats     <==||\n");
	fprintf(stderr, "Memory Accesses/Second = %f\n", sys->stats.memAccess/getTime()); 
	fprintf(stderr, "Page Faults/Memory Access = %f\n", sys->stats.faults/sys->stats.memAccess); 
	fprintf(stderr, "Average Memory Access Speed = %f ns\n", sys->stats.AvgMAS/sys->stats.memAccess); 
	fprintf(stderr, "Number of Segfaults per Memory Access = %f\n\n", sys->stats.segFault/sys->stats.memAccess); 

	fprintf(logPtr, "\n|||==>     Performance Stats     <==||\n");
	fprintf(logPtr, "Memory Accesses/Second = %f\n", sys->stats.memAccess/getTime()); 
	fprintf(logPtr, "Page Faults/Memory Access = %f\n", sys->stats.faults/sys->stats.memAccess); 
	fprintf(logPtr, "Average Memory Access Speed = %f ns\n", sys->stats.AvgMAS/sys->stats.memAccess); 
	fprintf(logPtr, "Number of Segfaults per Memory Access = %f\n\n", sys->stats.segFault/sys->stats.memAccess); 
    
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
    printf("%s -d      Turn Debug Mode On\n", program);
	printf("%s -p  n   This Designates n Number of Concurrent Processes\n\n", program); 

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
