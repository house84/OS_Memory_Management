/*
 * Author: Nick House
 * Project: Memory Management
 * Course: CS-4760 Operating Systems, Spring 2021
 * File Name: shared.h
 */

#ifndef SHARED_H
#define SHARED_H

#include "headers.h"
#define maxConProc 18
#define maxProc 40
#define bufLength 200
#define maxLines 9500

#define pTableSize 32
#define sysMem 256
#define sysMemBitIndex sysMem/pTableSize

enum sems{mutex};
enum actions{READ, WRITE, VALID, TERMINATE};


//Message Buffer
struct{

    long mtype;
    char mtext[bufLength];
    int action;               //READ(0) WRITE(1) TERMINATE(2)
    int page;                 //Index Within Page Table
    int offset;               //Page Offset
    int address;              //Address For Read/Write

} bufI, bufS, bufR;
//Init,  Send,  Receive

//Shared Memory Variables
int shmidMsgInit;
int shmidMsgSend;
int shmidMsgRec;
int shmidSystem;
int shmidSem;

typedef struct{

    int actionNum;               //READ(0) WRITE(1) TERMINATE(2)
    int page;                 //Index Within Page Table
    int offset;               //Page Offset
    int address;              //Address For Read/Write

    int frameIdx;           //Frame Idx for System Bit Array
    char action[bufLength]; //Hold Requested Action
    pid_t pid;              //Pid to help ID Process
    float time;             //Time Allocated to Memory
    float faultQRemove;     //Time added to faultQ
    bool allocated;         //Frame has been allocated
    bool refByte;           //FIFO Second Look Algo
    bool validBit;          //OSS Checks if Address is Valid
    bool dirtyBit;          //Been Written To not sent swapped
} frame;

typedef struct{

    int mID;
    int idx;
    pid_t pid;
    int delimeter;
    int frameIdx;
    frame pageT[pTableSize];
} pcb;

typedef struct{

    int memAPS;              //Number of memory accesses per second
    float faultsPMA;         //Number of page faults per memory access
    float AvgMAS;            //Average Memory Access Speed
    int segFPMA;             //Number of Seg Faults Per memory Access
} statistics;

struct system{

    unsigned int seconds;
    unsigned int nanoSeconds;
    unsigned int fileLength;
    bool run;
    bool debug;
    statistics stats;
    pcb pTable[maxConProc];
};



struct system *sys;
FILE * logPtr;

#endif //OS_MEMORY_MANAGEMENT_SHARED_H
