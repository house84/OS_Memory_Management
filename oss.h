/*
 * Author: Nick House
 * Project: Memory Management
 * Course: CS-4760 Operating Systems, Spring 2021
 * File Name: oss.h
 */

#ifndef OSS_H
#define OSS_H
#include "sharedFunc.h"
#include "shared.h"

bool debug;
bool spawnFlag;                        //Varialbe to signal forking process
bool sigFlag;                          //Signal Termination Has been init
time_t t;                              //Hold Time
struct itimerval timer;                //Set Timer
int concProc;                          //Number of Concurrent Processes
int totalProc; 					       //Number of total procedures
char logfile[50];                      //Logfile Name
pid_t pidArray[100];                   //Variable for Process PID's
key_t keySem;                          //Shm Key for Sem
key_t keyMsg;                          //Shm Key for Message 1
key_t keyMsg2;                         //Shm key for Message 2
key_t keyMsg3;                         //Shm key for Message 3
size_t memSize;                        //memSize for getshm()
key_t keySysTime;                      //Shm Key
int32_t memory[8];                     //Bit Arr for System Memory 256K

static void signalHandler();
static void openLogfile();
static void createSharedMemory();
static void closeLogfile();
static void help();
static void freeSharedMemory();
static void spawn();


#endif //OS_MEMORY_MANAGEMENT_OSS_H
