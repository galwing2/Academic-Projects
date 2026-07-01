#include <math.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

mtx_t qlock;
atomic_int total_passed = 0;

typedef struct Node { //Linked list for the queue items
    void *value;
    struct Node *next;
} Node;

typedef struct ThreadNode { //Linked list for the waiting threads
    cnd_t *cond;
    void *item;
    struct ThreadNode *next;
} ThreadNode;

Node *item_head = NULL; //First item in item queue
Node *item_tail = NULL; //Last item in item queue


ThreadNode *thread_head; //First item in thread waiters queue
ThreadNode *thread_tail; //Last item in thread waiters queue


void initQueue(void){
    //Empty item queue initialization
    item_head = NULL;
    item_tail = NULL;

    thread_head = NULL;
    thread_tail = NULL;

    total_passed = 0;

    //Initialize mutex
    mtx_init(&qlock,mtx_plain);
}

void enqueue(void* item){

    mtx_lock(&qlock);
    //Add the node to the item queue if no threads are sleeping
    if(thread_head==NULL){
        //Create new node with the input item
        Node *newNode = malloc(sizeof(Node));
        newNode->value = item;
        newNode->next = NULL;
        if(item_head==NULL){ //Item head is NULL if queue is empty so head=tail=newnode
            item_head = newNode;
            item_tail = item_head; 
        }
        else{ //We need to add the new node to the end of the item queue and update head
            item_tail->next = newNode;
            item_tail = newNode;
        }
    }
    else{ //There are threads sleeping so we hand the item directly to the longest waiting one
        ThreadNode* waiting_thrd = thread_head;
        thread_head = thread_head->next;
        if(thread_head==NULL){ //Update thread waiters queue if it is now empty
            thread_tail = NULL;
        }
        waiting_thrd->item = item; //Item handoff for the waiting thread
        cnd_signal(waiting_thrd->cond);
        
    }
    mtx_unlock(&qlock);
}

void* dequeue(void){

    void* item;
    mtx_lock(&qlock);

    if(item_head!=NULL){ //Item waiting in item queue
        item = item_head->value; //Extract removed item
        Node* temp = item_head; //Temporary pointer to head for item queue update and removal
        item_head = item_head->next; //Update item queue head
        if(item_head==NULL){ //If item queue is now empty after removal
            item_tail = NULL;
        }
        free(temp);
    }
    else{ //No item in item queue, wait for item handoff from another thread
        cnd_t curr_cv;
        cnd_init(&curr_cv);
        
        ThreadNode* waiting_thread = malloc(sizeof(ThreadNode)); //Thread to wait to receive handoff
        waiting_thread->cond = &curr_cv;
        waiting_thread->item = NULL;
        waiting_thread->next = NULL;

        if(thread_head==NULL){ //If this thread is the only one waiting for handoff update thread queue accordingly
            thread_head = waiting_thread;
            thread_tail = thread_head;
        }
        else{
            thread_tail->next = waiting_thread; //Simply add the newest waiting thread to the queue
            thread_tail = waiting_thread;
        }
        while(waiting_thread->item == NULL) { //Wait until data handoff actually occurred (although we can assume no spurious wake ups happened)
            cnd_wait(&curr_cv, &qlock);
        }
        item = waiting_thread->item;
        cnd_destroy(&curr_cv);
        free(waiting_thread);

    }

    mtx_unlock(&qlock);
    total_passed++; //Total items through item queue is equal to num of items that were dequeued
    return item;
}

size_t visited(void){
    return total_passed;
}

void destroyQueue(void){
    while(item_head!=NULL){ //Free all memory allocated
        Node* temp = item_head;
        item_head = item_head->next;
        free(temp);
    }
    while(thread_head!=NULL){
        ThreadNode* temp = thread_head;
        thread_head = thread_head->next;
        free(temp);
    }
    item_head = NULL;
    item_tail = NULL;
    thread_head = NULL;
    thread_tail = NULL;
    total_passed = 0;
    //Destroy Mutex
    mtx_destroy(&qlock);

}