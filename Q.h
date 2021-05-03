/*
 * Author: Nick House
 * Project: Memory Management
 * Course: CS-4760 Operating Systems, Spring 2021
 * File Name: Q.h
 */

#ifndef Q_H
#define Q_H
#define capacity 256
#include "headers.h"


//Queue to Track allocated Fremes
struct Queue{

    struct p_Node *head;
    struct p_Node *tail;

    int	currSize;
    int maxSize;
};

struct p_Node{

    int fakePID;
    float timerIO;
    struct p_Node *next;
    int idx;
    int page;

};

struct CircleQ{

    struct p_Node * circleQ[capacity];
    int currSize;
    int front;
    int rear;

};

void enqueue ();
void removeQ();
void printQ ();
void printCircleQ();
struct Queue * initQueue ();
struct p_Node * dequeue ();

static int checkEmpty();
static int checkFull();
void circleEnqueue(struct CircleQ*, int, int, float);
struct p_Node * circleDequeue();
struct CircleQ * initCircleQ();
bool checkTimerIO(struct CircleQ*, float);




#endif //OS_MEMORY_MANAGEMENT_Q_H
