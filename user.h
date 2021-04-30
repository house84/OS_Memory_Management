/*
 * Author: Nick House
 * Project: Memory Management
 * Course: CS-4760 Operating Systems, Spring 2021
 * File Name: user.h
 */

#ifndef USER_H
#define USER_H

int idx;                          //User Process Index
int mID;                          //Message ID
static void freeSHM();
static void initSysTime();

#endif //OS_MEMORY_MANAGEMENT_USER_H
