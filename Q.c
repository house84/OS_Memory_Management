/*
 * Author: Nick House
 * Project: Memory Management
 * Course: CS-4760 Operating Systems, Spring 2021
 * File Name: Q.h
 */

#include "Q.h"
#include "oss.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Initialize Circle Queue for pageFaults
struct CircleQ * initCircleQ(){

    struct CircleQ * cQue = (struct CircleQ *) malloc (sizeof (struct CircleQ));

    //initialize front and rear
    cQue->front = -1;
    cQue->rear = -1;
    cQue->currSize = 0;
    
}


//Check if Circle Q is Empty
static int checkEmpty(struct CircleQ * Q){

    if(Q->front == -1){
        return 1;
    }
    return 0;
}


//Check if Circle Q is Full
static int checkFull(struct  CircleQ * Q){

    if((Q->front == Q->rear +1) || (Q->front == 0 && Q->rear == capacity -1)){
        return -1;
    }
    return 0;
}

//Enqueue to Circle
void circleEnqueue(struct CircleQ * Q, int idx, int page, float timer){

    if(checkFull(Q)){
     //   fprintf(stderr,"Queue: faultQ is full - time:%s\n", getSysTime());
        return;
    }

    struct p_Node *newNode = (struct p_Node *) malloc (sizeof (struct p_Node));

    if(Q->front == -1) { Q->front = 0; }

    Q->rear = (Q->rear +1) % capacity;
    Q->circleQ[Q->rear] = newNode;
    newNode->idx = idx;
    newNode->page = page;
    newNode->timerIO = timer;
    ++Q->currSize;
}


//Dequeue from Circle Q
struct p_Node * circleDequeue(struct CircleQ * Q){

    struct p_Node *newNode;

    if (checkEmpty(Q)) {
      //  fprintf(stderr, "Queue: faultQ is Empty, can not Dequeue - Time: %s\n", getSysTime());
        return NULL;
    }

    newNode = Q->circleQ[Q->front];
    if (Q->front == Q->rear) {
        Q->front = Q->rear = -1;
    }
    else {
        Q->front = (Q->front + 1) % capacity;
    }

    return newNode;
}


//Print Circle Queue
void printCircleQ(struct CircleQ * Q){

    int i;

    if(checkEmpty(Q)){
    //    fprintf(stderr, "Queue: faultQ is Empty\n");
        return;
    }

    fprintf(stdout,"FaultQ Size: %d\nContents: ", Q->currSize);
    for(i = Q->front; i != Q->rear; i = (i + 1 ) % capacity ){

        fprintf(stdout, " [Idx: %d, Page: %d ]", Q->circleQ[i]->idx, Q->circleQ[i]->page);
    }
    fprintf(stdout, "\n\n");
}


//Check Head of Circle Queue
bool checkTimerIO(struct CircleQ * Q, float time){

    if(checkEmpty(Q)){
     //   fprintf(stderr, "Queue: faultQ is Empty\n");
        return false;
    }

    if(Q->circleQ[Q->front]->timerIO <= time){ return true; }

    return false;
}


//Initialize Queue
struct Queue * initQueue ()
{

    struct Queue *que = (struct Queue *) malloc (sizeof (struct Queue));

    //Initialize Null Front and Rear Nodes
    que->head = NULL;
    que->tail = NULL;

    //que->maxSize = procMax;
    que->maxSize = 18;
    que->currSize = 0;

    return que;

}


//Enqueue Process to Queue
void enqueue (struct Queue * Q, int idx, int page)
{

    struct p_Node *newNode = (struct p_Node *) malloc (sizeof (struct p_Node));

    newNode->fakePID = idx;
    newNode->next = NULL;
    newNode->idx = idx;
    newNode->page = page;
    ++Q->currSize;

    //Check if Empty
    if (Q->head == NULL && Q->tail == NULL)
    {

        Q->head = newNode;
        Q->tail = newNode;

        return;
    }

    //Else add Node to End of Que
    Q->tail->next = newNode;
    Q->tail = newNode;

}


//Dequeue Process from Queue
struct p_Node * dequeue (struct Queue * Q)
{

    //Check if Que is empty
    if (Q->head == NULL)
    {
        return NULL;
    }

    struct p_Node *newNode = Q->head;

    --Q->currSize;

    //Make next node the head
    Q->head = Q->head->next;

    //Check if Que is Now Empty
    if (Q->head == NULL)
    {
        Q->tail = NULL;
    }


    return newNode;

}


//Print The Queue for Testing
void printQ (struct Queue * Q)
{

    //Test
    fprintf (stderr, "Print Q\n");

    struct p_Node *newNode = Q->head;

    fprintf (stderr, "Queue Size: %d -> ", Q->currSize);

    while (newNode != NULL)
    {

        fprintf (stderr, "%d ", newNode->fakePID);
        newNode = newNode->next;
    }

    fprintf (stderr, "\n");

}


//Print The Queue for Testing
void removeQ(struct Queue * Q, int idx, int page)
{

    struct Queue * tempQ = initQueue();

    struct p_Node *newNode = Q->head;

    while (newNode != NULL)
    {

        if(newNode->idx == idx && newNode->page == page){

            newNode = newNode->next;
            if(newNode == NULL) { break; }
        }

        enqueue(tempQ, newNode->idx, newNode->page);
        //fprintf (stderr, "%d ", newNode->fakePID);
        newNode = newNode->next;
    }

   // printQ(tempQ);
    *Q = *tempQ;
    free(tempQ);
   // printQ(Q);
}
