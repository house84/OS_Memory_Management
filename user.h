/*
 * Author: Nick House
 * Project: Memory Management
 * Course: CS-4760 Operating Systems, Spring 2021
 * File Name: user.h
 */

#ifndef USER_H
#define USER_H
#define readPct 85               //Percentage to Read rather than write
#include "headers.h"


int idx;                          //User Process Index
int mID;                          //Message ID
int references;                   //Track References
int referenceIter;                //Used to Rebase Random Check
int randRef;                      //Random Variable to Check for termination
bool run;
static void freeSHM();            //Free Memory Resources
static bool checkTermMsg();       //Check msg for Terminate
static bool checkTermPct();       //Check if User Should Terminate
static void initSysTime();        //Initiate SHM for System
static void pageRequest();        //Generate Read/Write Request

#endif //OS_MEMORY_MANAGEMENT_USER_H
