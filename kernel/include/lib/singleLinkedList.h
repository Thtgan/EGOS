#if !defined(__SINGLE_LINKED_LIST_H)
#define __SINGLE_LINKED_LIST_H

#include<stdbool.h>
#include<stddef.h>

struct __SingleLinkedListNode {
    struct __SingleLinkedListNode* next;
};

#if !defined(hostPointer)
#define hostPointer(nodePtr, type, member)  ((type*)(((void*)(nodePtr)) - offsetof(type, member)))
#endif

typedef struct __SingleLinkedListNode SingleLinkedListNode;
typedef struct __SingleLinkedListNode SingleLinkedList;

void initSingleLinkedList(SingleLinkedList* list);

void initSingleLinkedListNode(SingleLinkedListNode* node);

bool isSingleListEmpty(SingleLinkedList* list);

SingleLinkedListNode* singleLinkedListGetNext(SingleLinkedListNode* node);

void singleLinkedListInsertBack(SingleLinkedList* node, SingleLinkedListNode* newNode);

void singleLinkedListDeleteNext(SingleLinkedList* node);

#endif // __SINGLE_LINKED_LIST_H
