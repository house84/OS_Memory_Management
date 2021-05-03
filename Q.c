/*
 * Author: Nick House
 * Project: Memory Management
 * Course: CS-4760 Operating Systems, Spring 2021
 * File Name: Q.h
 */

#include "Q.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//struct Queue *GQue;

//int main () {
//
//    GQue = initQueue();
//
//    int arr[] = { 3, 4, 5, 7, 43, 6, 7, 54, 32, 23, 54, 63, 2134 };
//
//    int i;
//    int size = sizeof (arr) / sizeof (int);
//
//    for (i = 0; i < size; ++i)
//    {
//
//        // fprintf(stdout, "%d ", arr[i]);
//        enqueue(GQue, i, arr[i]);
//    }
//
//
//    printQ(GQue);
//    removeQ(GQue, 7, 3);
//
//    return 0;
//}
//
//
//


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
    GQue = tempQ;
    free(tempQ);
   // printQ(GQue);
}
