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

#define BIT_SET(a, b) ((a) |= (1ULL << (b)))
#define BIT_CLEAR(a, b) ((a) &= ~(1ULL << (b)))
#define BIT_FLIP(a, b) ((a) ^= (1ULL << (b)))
#define BIT_CHECK(a, b) (!!((a) & (1ULL << (b))))

enum sems{mutex};

//Message Buffer
struct{

    long mtype;
    char mtext[bufLength];
} bufI, bufS, bufR;
//Init,  Send,  Receive

//Shared Memory Variables
int shmidMsgInit;
int shmidMsgSend;
int shmidMsgRec;
int shmidSystem;
int shmidSem;

typedef struct{

    int delimeter;
    int frameNum;
    bool refBit;
    bool validBit;
    bool dirtyBit;
} frame;

typedef struct{

    int mID;
    int idx;
    pid_t pid;
    frame pageT[pTableSize];
} pcb;

struct system{

    int seconds;
    int nanoSeconds;
    int fileLength;
    bool debug;
    pcb pTable[maxConProc];
};

struct system *sys;
FILE * logPtr;

#endif //OS_MEMORY_MANAGEMENT_SHARED_H
