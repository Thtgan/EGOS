#if !defined(__WAIT_LIST_H)
#define __WAIT_LIST_H

#include<kit/types.h>
#include<multitask/process.h>
#include<multitask/spinlock.h>
#include<structs/singlyLinkedList.h>

typedef struct {
    int size;
    SinglyLinkedList waitList;
    SinglyLinkedListNode* tail;
} WaitList;

typedef struct {
    SinglyLinkedListNode node;
    Process* process;
} WaitListNode;

void initWaitList(WaitList* list);

void initWaitListNode(WaitListNode* node, Process* process);

void addWaitListTail(WaitList* list, WaitListNode* node);

void removeWaitListHead(WaitList* list);

WaitListNode* getWaitListHead(WaitList* list);

#endif // __WAIT_LIST_H
