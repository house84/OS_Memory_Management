/*
 * Author: Nick House
 * Project: Memory Management
 * Course: CS-4760 Operating Systems, Spring 2021
 * File Name: Q.h
 */


#ifndef Q_H
#define Q_H

struct Queue{                          //Queue for Ready

    struct p_Node *head;
    struct p_Node *tail;

    int	currSize;
    int maxSize;

};

struct p_Node{

    int fakePID;
    struct p_Node *next;
    int idx;
    int page;

};

void enqueue ();
void printQ ();
struct Queue * initQueue ();
struct p_Node * dequeue ();
void removeQ();
struct Queue *GQue;


#endif //OS_MEMORY_MANAGEMENT_Q_H
