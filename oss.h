/*
 * Author: Nick House
 * Project: Memory Management
 * Course: CS-4760 Operating Systems, Spring 2021
 * File Name: oss.h
 */

#ifndef OSS_H
#define OSS_H
#include "headers.h"
#include "sharedFunc.h"
#include "shared.h"
#include "Q.h"

//enum actions{READ, WRITE, VALID, TERMINATE}; 

bool debug;
bool spawnFlag;                        //Varialbe to signal forking process
bool sigFlag;                          //Signal Termination Has been init
time_t t;                              //Hold Time
struct itimerval timer;                //Set Timer
struct Queue * frameQ;                 //Que of All Allocated Pages
struct Queue * processQ;               //Que of All Processes
struct CircleQ * faultQ;               //Circle Q to hold faults
struct p_Node *CPU_Node;               //Node to Hold CPU Process
int concProc;                          //Number of Concurrent Processes
int totalProc; 					       //Number of total procedures
int designatedUsers;                   //User Defined Max Concurrent Processes
char logfile[50];                      //Logfile Name
bool active[maxConProc];               //Array holding active Processes by idx
pid_t pidArray[100];                   //Variable for Process PID's
key_t keySem;                          //Shm Key for Sem
key_t keyMsg;                          //Shm Key for Message 1
key_t keyMsg2;                         //Shm key for Message 2
key_t keyMsg3;                         //Shm key for Message 3
size_t memSize;                        //memSize for getshm()
key_t keySysTime;                      //Shm Key
int allocatedFrames;                   //Track current number of allocated Frames
int32_t userBitVector;                 //Integer to track Concurrent Users
int32_t memory[sysMemBitIndex];        //Bit Arr for System Memory 256K

static void signalHandler();
static void allocateCPU();
static void openLogfile();
static void createSharedMemory();
static void closeLogfile();
static void help();
static void freeSharedMemory();
static void freeUserResources();
static void memoryHandler();
static void spawn();
static void checkMsg();
static void faultHandler();
static void checkFaultQ();
static int fifo();
static void specialDaemon();
static int getMemoryBit();
static void setMemoryBit();
static void unsetMemoryBit();
static int getUserIdxBit();
static void setUserIdxBit();
static void unsetUserIdxBit();


#endif //OS_MEMORY_MANAGEMENT_OSS_H
