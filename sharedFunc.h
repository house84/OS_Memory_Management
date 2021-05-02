/*
 * Author: Nick House
 * Project: Memory Management
 * Course: CS-4760 Operating Systems, Spring 2021
 * File Name: sharedFunc.h
 */

#ifndef SHAREDFUNC_H
#define SHAREDFUNC_H
#include "headers.h"

void setShmid();
void setSemID();
void semWait();
void semSignal();
void incrementSysTime();
int getRand();
void perrorHandler();
const char * getSysTime();
float getTime();
struct system *sys;
struct sembuf sops;



#endif //OS_MEMORY_MANAGEMENT_SHAREDFUNC_H
